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
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <fts.h>
#include <err.h>

#define ARRAY_DEFAULT_CAPACITY 16
#define ARRAY_BEGIN(array) ((void *)((array)->data))
#define ARRAY_END(array) ((void *)((array)->data + (array)->count))

#define TARGET_BIN "$(BINARIES)"
#define TARGET_LIB "$(LIBRARIES)"
#define TARGET_OBJ "$(OBJECTS)"

#define LIBRARIES_SUFFIX ".so"

#define TRUNCATED_DIRLEN(pathlen, sourceslen, namelen) ((int)((pathlen) - (sourceslen) - (namelen) - 1))

struct hvn_mkgen_args {
	const char *sources;
	struct hvn_mkgen_array *dependencies;
};

struct hvn_mkgen_array {
	size_t capacity;
	size_t count;
	void *data[];
};

static const char rulec[] = "\t$(CC) $(CFLAGS) -c -o $@ $<\n";
static const char ruleasm[] = "\t$(AS) $(ASFLAGS) -c -o $@ $<\n";
static const char rulecxx[] = "\t$(CXX) $(CXXFLAGS) -c -o $@ $<\n";

static const struct hvn_mkgen_extension_rule {
	const char *extension;
	const char *rule;
} extensionrules[] = {
	{ ".c", rulec },
	{ ".s", ruleasm },
	{ ".cc", rulecxx },
	{ ".cpp", rulecxx },
};

static const char macrosuffix[] = "FLAGS";

static void
hvn_mkgen_array_append(struct hvn_mkgen_array **arrayp, void *data) {
	struct hvn_mkgen_array *array = *arrayp;

	if(*arrayp == NULL) {
		array = malloc(sizeof(*array) + sizeof(*array->data) * ARRAY_DEFAULT_CAPACITY);

		if(array != NULL) {
			array->capacity = ARRAY_DEFAULT_CAPACITY;
			array->count = 1;
			*array->data = data;
		} else {
			err(EXIT_FAILURE, "malloc");
		}
	} else {
		if(array->capacity == array->count) {
			array->capacity *= 2;
			array = realloc(array, sizeof(*array) + sizeof(*array->data) * array->capacity);
			if(array == NULL) {
				err(EXIT_FAILURE, "realloc");
			}
		}

		array->data[array->count] = data;
		array->count++;
	}

	*arrayp = array;
}

static void
hvn_mkgen_array_free_all(struct hvn_mkgen_array *array) {
	void **current = ARRAY_BEGIN(array), ** const end = ARRAY_END(array);

	while(current != end) {
		free(*current);
		++current;
	}

	free(array);
}

static void
hvn_mkgen_print_module_target(const char *name, FILE *output) {
	if(strncmp("lib", name, 3) == 0) {
		fprintf(output, TARGET_LIB "/%s" LIBRARIES_SUFFIX, name);
	} else {
		fprintf(output, TARGET_BIN "/%s", name);
	}
}

static void
hvn_mkgen_print_module_dependencies(const char *name, size_t namelen, const struct hvn_mkgen_array *dependencies, FILE *output) {
	const char **current = ARRAY_BEGIN(dependencies), ** const end = ARRAY_END(dependencies);

	while(current != end) {
		const char *list = *current;
		const char *equal = strchr(list, '=');

		if(equal != NULL && namelen == equal - list && strncmp(name, list, namelen) == 0) {
			const char *current = equal + 1;
			const char *comma;

			while(comma = strchr(current, ','), comma != NULL) {
				size_t dependencylen = comma - current;
				char dependency[dependencylen + 1];
				strncpy(dependency, current, dependencylen);
				dependency[dependencylen] = '\0';

				fputc(' ', output);
				hvn_mkgen_print_module_target(dependency, output);

				current = comma + 1;
			}

			if(*current != '\0') {
				fputc(' ', output);
				hvn_mkgen_print_module_target(current, output);
			}
		}

		++current;
	}
}

static const char *
hvn_mkgen_rule_extension(const char *extension) {
	const struct hvn_mkgen_extension_rule *current = extensionrules,
		* const end = extensionrules + sizeof(extensionrules) / sizeof(*extensionrules);

	while(current != end && strcmp(extension, current->extension) != 0) {
		++current;
	}

	return current != end ? current->rule : NULL;
}

static void
hvn_mkgen_macro_fill(char *macro, const char *name, size_t namelen) {
	strncpy(macro, name, namelen);
	while(namelen != 0) {
		char const character = *macro;

		if(isalpha(character)) {
			*macro = character - 'a' + 'A';
		} else if(isdigit(character)) {
			*macro = character;
		} else {
			*macro = '_';
		}

		++macro;
		--namelen;
	}
	stpncpy(macro, macrosuffix, sizeof(macrosuffix));
}

static int
hvn_mkgen_fts_compar(const FTSENT **lhs, const FTSENT **rhs) {
	return strcmp((*lhs)->fts_name, (*rhs)->fts_name);
}

static void
hvn_mkgen_print_module_rules(const char *name, FILE *output,
	size_t sourceslen, const struct hvn_mkgen_args *args) {
	size_t const namelen = strlen(name);
	char path[sourceslen + namelen + 2];
	char * const paths[] = { path, NULL };

	*stpncpy(path, args->sources, sourceslen) = '/';
	stpncpy(path + sourceslen + 1, name, namelen + 1);

	FTS *ftsp = fts_open(paths, FTS_LOGICAL, hvn_mkgen_fts_compar);
	if(ftsp != NULL) {
		struct hvn_mkgen_array *objects = NULL;
		FTSENT *ftsentp;

		while(ftsentp = fts_read(ftsp), ftsentp != NULL) {
			if(*ftsentp->fts_name != '.') {
				const char *truncated = ftsentp->fts_path + sourceslen;
				const char *extension;

				switch(ftsentp->fts_info) {
				case FTS_D:
					if(ftsentp->fts_level == 0) {
						fprintf(output, TARGET_OBJ "%s:\n", truncated);
					} else {
						fprintf(output, TARGET_OBJ "%s: " TARGET_OBJ "%.*s\n",
							truncated, TRUNCATED_DIRLEN(ftsentp->fts_pathlen, sourceslen, ftsentp->fts_namelen), truncated);
					}
					fputs("\t$(MKDIR) -p $@\n", output);
					break;
				case FTS_F:
					extension = strrchr(truncated, '.');
					if(extension != NULL && extension[1] != '\0') {
						char *object = strndup(truncated, extension - truncated + 2);
						const char *rule = hvn_mkgen_rule_extension(extension);
						object[extension - truncated + 1] = 'o';

						if(rule != NULL) {
							fprintf(output, TARGET_OBJ "%s: %s " TARGET_OBJ "%.*s\n%s",
								object, ftsentp->fts_path,
								TRUNCATED_DIRLEN(ftsentp->fts_pathlen, sourceslen, ftsentp->fts_namelen), truncated,
								rule);
							hvn_mkgen_array_append(&objects, object);
						}
					}
					break;
				case FTS_DNR:
				case FTS_ERR:
				case FTS_NS:
					errno = ftsentp->fts_errno;
					warn("fts_read %s", ftsentp->fts_path);
					break;
				}
			}
		}

		if(objects != NULL) {
			char **current = ARRAY_BEGIN(objects), ** const end = ARRAY_END(objects);
			char macro[namelen + sizeof(macrosuffix)];

			hvn_mkgen_macro_fill(macro, name, namelen);

			hvn_mkgen_print_module_target(name, output);
			fputc(':', output);

			while(current != end) {
				fprintf(output, " " TARGET_OBJ "%s", *current);
				++current;
			}

			if(args->dependencies != NULL) {
				hvn_mkgen_print_module_dependencies(name, namelen, args->dependencies, output);
			}

			fprintf(output, "\n\t$(LD) $(LDFLAGS) $(%s) -o $@ $^\n", macro);

			hvn_mkgen_array_free_all(objects);
		} else {
			warnx("No objects for module %s", name);
		}

		fts_close(ftsp);
	} else {
		warn("fts_open %s", path);
	}
}

static void
hvn_mkgen_usage(const char *hvnmkgenname) {
	fprintf(stderr, "usage: %s [-S sources] [-d dependencies] [filename]\n", hvnmkgenname);
	exit(EXIT_FAILURE);
}

static const struct hvn_mkgen_args
hvn_mkgen_parse_args(int argc, char **argv) {
	struct hvn_mkgen_args args = {
		.sources = "src",
		.dependencies = NULL,
	};
	int c;

	while((c = getopt(argc, argv, ":S:d:")) != -1) {
		switch(c) {
		case 'S':
			args.sources = optarg;
			break;
		case 'd':
			hvn_mkgen_array_append(&args.dependencies, optarg);
			break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_mkgen_usage(*argv);
		default:
			warnx("Unknown argument -%c", optopt);
			hvn_mkgen_usage(*argv);
		}
	}

	if(argc - optind > 1) {
		warnx("Too many output files");
		hvn_mkgen_usage(*argv);
	}

	return args;
}

static int
hvn_mkgen_string_compar(const void *lhs, const void *rhs) {
	char * const *strp1 = lhs, * const *strp2 = rhs;

	return strcmp(*strp1, *strp2);
}

static struct hvn_mkgen_array *
hvn_mkgen_modules_array(const char *sources) {
	struct hvn_mkgen_array *array = NULL;
	DIR *modules = opendir(sources);
	struct dirent *entry;

	if(modules == NULL) {
		err(EXIT_FAILURE, "opendir %s", sources);
	}

	while(entry = readdir(modules), entry != NULL) {
		if(*entry->d_name != '.') {
			char *name = strdup(entry->d_name);

			if(name == NULL) {
				err(EXIT_FAILURE, "strdup %s", entry->d_name);
			}

			hvn_mkgen_array_append(&array, name);
		}
	}

	closedir(modules);

	if(array != NULL) {
		qsort(ARRAY_BEGIN(array), array->count, sizeof(const char *), hvn_mkgen_string_compar);
	} else {
		errx(EXIT_FAILURE, "No modules available");
	}

	return array;
}

int
main(int argc, char **argv) {
	const struct hvn_mkgen_args args = hvn_mkgen_parse_args(argc, argv);
	struct hvn_mkgen_array *modules = hvn_mkgen_modules_array(args.sources);
	const char **current = ARRAY_BEGIN(modules), ** const end = ARRAY_END(modules);
	const char *filename = argc == optind ? "Makefile.rules" : argv[optind];
	size_t const sourceslen = strlen(args.sources);
	FILE *output;

	output = fopen(filename, "w");
	if(output == NULL) {
		err(EXIT_FAILURE, "fopen %s", filename);
	}

	fputs(".PHONY: all clean\nall:\n", output);

	while(current != end) {
		hvn_mkgen_print_module_rules(*current, output, sourceslen, &args);
		++current;
	}

	fputs("all:", output);

	current = ARRAY_BEGIN(modules);
	while(current != end) {
		fputc(' ', output);
		hvn_mkgen_print_module_target(*current, output);
		++current;
	}

	fputs("\nclean:\n\trm -rf " TARGET_BIN "/* " TARGET_LIB "/* " TARGET_OBJ "/*\n", output);

	fclose(output);

	hvn_mkgen_array_free_all(modules);

	free(args.dependencies);

	return EXIT_SUCCESS;
}
