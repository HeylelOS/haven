/*
	main.c
	Copyright (c) 2020, Valentin Debon

	This file is part of the heaven repository
	subject the BSD 3-Clause License, see LICENSE
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <err.h>

struct hvn_pkgen_args {
	const char *log;
	const char *app;
	const char *data;
	const char *toolchain;
};

struct hvn_pkgen_mountpoint {
	const char *origin;
	const char *path;
	int flags;
};

static int
hvn_pkgen_log(const char *path) {
	int const fd = open(path, O_WRONLY | O_CREAT | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	int retval = 0;

	if(fd >= 0) {
		if(dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
			retval = -1;
		}
		close(fd);
	} else {
		retval = -1;
	}

	return retval;
}

static int
hvn_pkgen_deny(const char *path) {
	int const fd = open(path, O_WRONLY);
	int retval = 0;

	if(fd >= 0) {
		static const char deny[] = "deny";
		size_t const size = sizeof(deny) - 1;

		if(write(fd, deny, size) != size) {
			retval = -1;
		}

		close(fd);
	} else {
		retval = -1;
	}

	return retval;
}

static int
hvn_pkgen_id_map_zero(const char *path, id_t id) {
	int const fd = open(path, O_WRONLY);
	int retval = 0;

	if(fd >= 0) {
		static const char format[] = "0 %d 1";
		int const length = snprintf(NULL, 0, format, id) + 1;
		char buffer[length];

		assert(snprintf(buffer, length, format, id) < length);
		if(write(fd, buffer, length) != length) {
			retval = -1;
		}

		close(fd);
	} else {
		retval = -1;
	}

	return retval;
}

static int
hvn_pkgen_mkroot(const char *path, const struct hvn_pkgen_args *args) {
	/* Trying to unmount path if it was previously mounted. */
	if(umount(path) != 0 && errno != EINVAL) {
		warn("umount %s", path);
		return -1;
	}

	/* Create a tmpfs to make our hierarchy */
	if(mount("", path, "tmpfs", 0, NULL) != 0) {
		warn("mount %s", path);
		return -1;
	}

	const struct hvn_pkgen_mountpoint mountpoints[] = {
		{ .origin = "/dev", .path = "/dev", .flags = MS_BIND | MS_REC },
		{ .origin = "/sys", .path = "/sys", .flags = MS_BIND | MS_REC },
		{ .origin = "/proc", .path = "/proc", .flags = MS_BIND | MS_REC },
		{ .origin = args->app, .path = "/app", .flags = MS_BIND | MS_RDONLY },
		{ .origin = args->data, .path = "/data", .flags = MS_BIND },
		{ .origin = args->toolchain, .path = "/usr", .flags = MS_BIND | MS_RDONLY },
	};
	const struct hvn_pkgen_mountpoint *current = mountpoints,
		*end = mountpoints + sizeof(mountpoints) / sizeof(*mountpoints);
	size_t const pathlen = strlen(path);

	while(current != end) {
		char target[pathlen + strlen(current->path) + 1];
		stpncpy(stpncpy(target, path, pathlen), current->path, sizeof(target) - pathlen);

		if(mkdir(target, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
			warn("mkdir %s", target);
			return -1;
		}

		if(mount(current->origin, target, "", current->flags, NULL) != 0) {
			warn("mount %s %s", current->origin, target);
			return -1;
		}

		++current;
	}

	if(chroot(path) != 0) {
		warn("chroot %s", path);
		return -1;
	}

	if(symlink("usr/bin", "/bin") != 0) {
		warn("symlink /bin -> usr/bin");
		return -1;
	}

	if(symlink("usr/lib", "/lib") != 0) {
		warn("symlink /lib -> usr/lib");
		return -1;
	}

	if(chdir("/data") != 0) {
		warn("chdir %s", path);
		return -1;
	}

	return 0;
}

static int
hvn_pkgen_environment(const char *path) {
	FILE *stream = fopen(path, "r");
	int retval = 0;

	if(stream != NULL) {
		char *line = NULL;
		size_t length = 0;
		ssize_t nread;

		while(nread = getline(&line, &length, stream), nread != -1) {
			if(*line != '#' && *line != '\0') {
				if(line[nread - 1] == '\0') {
					line[nread - 1] = '\0';
				}

				char *equal = strchr(line, '=');
				if(equal != NULL) {
					const char *value = equal + 1;
					*equal = '\0';

					if(setenv(line, value, 1) != 0) {
						warn("setenv %s=%s", line, value);
						retval = -1;
						break;
					}
				}
			}
		}

		free(line);
		fclose(stream);
	} else {
		warn("fopen %s", path);
		retval = -1;
	}

	return retval;
}

static void
hvn_pkgen_usage(const char *hvnpkgenname) {
	fprintf(stderr, "usage: %s [-l log] -a app -d data -t toolchain recipes...\n", hvnpkgenname);
	exit(EXIT_FAILURE);
}

static const struct hvn_pkgen_args
hvn_pkgen_parse_args(int argc, char **argv) {
	struct hvn_pkgen_args args = {
		.log = NULL,
		.app = NULL,
		.data = NULL,
		.toolchain = NULL,
	};
	int c;

	while((c = getopt(argc, argv, ":l:a:d:t:")) != -1) {
		switch(c) {
		case 'l':
			args.log = optarg;
			break;
		case 'a':
			args.app = optarg;
			break;
		case 'd':
			args.data = optarg;
			break;
		case 't':
			args.toolchain = optarg;
			break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_pkgen_usage(*argv);
		default:
			warnx("Unknown argument -%c", optopt);
			hvn_pkgen_usage(*argv);
		}
	}

	if(args.app == NULL) {
		warnx("Missing application recipes directory");
		hvn_pkgen_usage(*argv);
	}

	if(args.data == NULL) {
		warnx("Missing data directory");
		hvn_pkgen_usage(*argv);
	}

	if(args.toolchain == NULL) {
		warnx("Missing toolchain");
		hvn_pkgen_usage(*argv);
	}

	if(argc - optind < 1) {
		warnx("Missing app recipes");
		hvn_pkgen_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_pkgen_args args = hvn_pkgen_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;
	/* We must keep ids now, because after unshare(2) unmapped ids
	   are resolved to the overflow user/group id */
	uid_t const uid = getuid();
	gid_t const gid = getgid();

	/* Let's keep the same environment in any build */
	umask(S_IWGRP | S_IWOTH);
	if(clearenv() != 0) {
		errx(EXIT_FAILURE, "clearenv"); // nb: errx, man doesn't says it sets errno
	}

	/* Modify the output of the process if specified. */
	if(args.log != NULL && hvn_pkgen_log(args.log) != 0) {
		err(EXIT_FAILURE, "hvn_pkgen_log: %s", args.log);
	}

	/* Create new user namespace */
	if(unshare(CLONE_NEWUSER | CLONE_NEWNS) != 0) {
		err(EXIT_FAILURE, "unshare");
	}

	/* Map our user id to root for convenience in package creation */
	if(hvn_pkgen_id_map_zero("/proc/self/uid_map", uid) != 0) {
		err(EXIT_FAILURE, "Unable to map user id %d to zero", uid);
	}

	/* Same for groups, don't forget to deny setgroups or the mapping fails */
	if(hvn_pkgen_deny("/proc/self/setgroups") != 0
		|| hvn_pkgen_id_map_zero("/proc/self/gid_map", gid) != 0) {
		err(EXIT_FAILURE, "Unable to map group id %d to zero", uid);
	}

	/* Create our new root mountpoint. /mnt should be available in any system. */
	if(hvn_pkgen_mkroot("/mnt", &args) != 0) {
		errx(EXIT_FAILURE, "Unable to create new root mountpoint");
	}

	/* Starting from here, we have our fake root fully operational */
	if(hvn_pkgen_environment("/app/environ") != 0) {
		errx(EXIT_FAILURE, "Unable to load app environment");
	}

	while(argpos != argend) {
		const char *recipe = *argpos;

		if(strchr(recipe, '/') == NULL && *recipe != '.') {
			size_t const recipelen = strlen(recipe);
			char path[recipelen + 6];
			int wstatus;

			stpncpy(stpncpy(path, "/app/", sizeof(path)), recipe, sizeof(path) - 5);

			printf("Starting recipe %s at %s\n", recipe, path);

			pid_t const pid = fork();
			switch(pid) {
			case 0:
				execl(path, recipe, NULL);
				err(EXIT_FAILURE, "execv %s", path);
				break;
			case -1:
				err(EXIT_FAILURE, "fork");
			default:
				if(waitpid(pid, &wstatus, 0) == -1) {
					err(EXIT_FAILURE, "waitpid");
				}

				if(WIFEXITED(wstatus)) {
					int const status = WEXITSTATUS(wstatus);
					if(status == 0) {
						printf("Recipe %s completed successfully\n", recipe);
					} else {
						errx(EXIT_FAILURE, "Recipe %s failed with status %d", recipe, status);
					}
				} else if(WIFSIGNALED(wstatus)) {
					errx(EXIT_FAILURE, "Recipe %s terminated by signal %d", recipe, WTERMSIG(wstatus));
				} else {
					errx(EXIT_FAILURE, "Unknown condition at termination of recipe %s", recipe);
				}
				break;
			}

		} else {
			errx(EXIT_FAILURE, "Invalid recipe name %s", recipe);
		}

		++argpos;
	}

	return EXIT_SUCCESS;
}

