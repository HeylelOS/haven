/*
	main.c
	Copyright (c) 2020, Valentin Debon

	This file is part of the heaven repository
	subject the BSD 3-Clause License, see LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <err.h>

#ifndef HVN_INIT_DIRECTORY_MODE
#define HVN_INIT_DIRECTORY_MODE (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

struct hvn_init_args {
	const char *sources;
	const char *extension;
};

static const char hello_world_C[] = "#include <stdio.h>\n\nint\nmain(int argc, char **argv) {\n\n\tputs(\"Hello, World!\");\n\n\treturn 0;\n}\n\n";
static const char hello_world_CXX[] = "#include <iostream>\n\nint\nmain(int argc, char **argv) {\n\n\tstd::cout << \"Hello, World!\\n\";\n\n\treturn 0;\n}\n\n";

static const struct hvn_hello_world {
	const char *extension;
	const char *source;
} hello_worlds[] = {
	{ ".c", hello_world_C },
	{ ".cc", hello_world_CXX },
	{ ".cpp", hello_world_CXX },
};

static const char *
hvn_init_hello_world_extension(const char *extension) {
	const struct hvn_hello_world *current = hello_worlds,
		* const end = hello_worlds + sizeof(hello_worlds) / sizeof(*hello_worlds);

	while(current != end && strcmp(current->extension, extension) != 0) {
		++current;
	}

	return current != end ? current->source : NULL;
}

static void
hvn_init_module(const char *modulename, const char *extension) {
	if(strchr(modulename, '/') != NULL) {
		warnx("Invalid module name %s", modulename);
		return;
	}

	if(mkdir(modulename, HVN_INIT_DIRECTORY_MODE) != 0) {
		warn("Unable to create module source directory %s", modulename);
		return;
	}

	const char *helloworldsource;
	if(strncmp("lib", modulename, 3) != 0 && (helloworldsource = hvn_init_hello_world_extension(extension)) != NULL) {
		size_t const modulenamelength = strlen(modulename), extensionlength = strlen(extension);
		char mainpath[modulenamelength + extensionlength + 6];

		*stpncpy(mainpath, modulename, modulenamelength) = '/';
		*stpncpy(stpncpy(mainpath + modulenamelength + 1, "main", 4), extension, extensionlength) = '\0';

		FILE *main = fopen(mainpath, "w");
		if(main != NULL) {
			fputs(helloworldsource, main);
			fclose(main);
		} else {
			warn("Unable to create hello world source file %s", mainpath);
		}
	}
}

static void
hvn_init_usage(const char *hvninitname) {
	fprintf(stderr, "usage: %s [-S sources] [-e extension] targets...\n", hvninitname);
	exit(EXIT_FAILURE);
}

static const struct hvn_init_args
hvn_init_parse_args(int argc, char **argv) {
	struct hvn_init_args args = {
		.sources = "src",
		.extension = ".c",
	};
	int c;

	while((c = getopt(argc, argv, ":S:e:")) != -1) {
		switch(c) {
		case 'S':
			args.sources = optarg;
			break;
		case 'e':
			args.extension = optarg;
			break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_init_usage(*argv);
		default:
			warnx("Unknown argument -%c", c);
			hvn_init_usage(*argv);
		}
	}

	if(argc == optind) {
		warnx("Missing input file(s)");
		hvn_init_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_init_args args = hvn_init_parse_args(argc, argv);
	char **current = argv + optind, ** const argend = argv + argc;

	if(mkdir(args.sources, HVN_INIT_DIRECTORY_MODE) != 0 && errno != EEXIST) {
		err(EXIT_FAILURE, "Unable to create sources directory %s", args.sources);
	}

	if(chdir(args.sources) != 0) {
		err(EXIT_FAILURE, "Unable to change current directory to sources directory %s", args.sources);
	}

	while(current != argend) {
		hvn_init_module(*current, args.extension);
		current++;
	}

	return EXIT_SUCCESS;
}

