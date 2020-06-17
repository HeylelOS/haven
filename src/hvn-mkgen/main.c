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
static const char rulecxx[] = "\t$(CXX) $(CXXFLAGS) -c -o $@ $<\n";

static const struct hvn_mkgen_extension_rule {
	const char *extension;
	const char *rule;
} extensionrules[] = {
	{ ".c", rulec },
	{ ".cc", rulecxx },
	{ ".cpp", rulecxx },
};

static const char macrosuffix[] = "FLAGS";

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

		if(equal != NULL && namelen == equal - list && strncmp(name, list, equal - list)) {
			const char *current = equal + 1;
			const char *comma;

			while(*current != '\0' && (comma = strchr(current, ',')) != NULL) {
				size_t dependencylen = comma - current;
				char dependency[dependencylen + 1];
				strncpy(dependency, current, dependencylen);
				dependency[dependencylen] = '\0';

				fputc(' ', output);
				hvn_mkgen_print_module_target(dependency, output);

				current = comma + 1;
			}
		}

		++current;
	}
}

static void
hvn_mkgen_print_rule_extension(const char *extension, FILE *output) {
	const struct hvn_mkgen_extension_rule *current = extensionrules,
		* const end = extensionrules + sizeof(extensionrules) / sizeof(*extensionrules);

	while(current != end && strcmp(extension, current->extension) != 0) {
		++current;
	}

	if(current != end) {
		fputs(current->rule, output);
	} else {
		warnx("No rule for files with extension %s", extension);
	}
}

static void
hvn_mkgen_print_module_rules(const char *name, FILE *output,
	size_t sourceslen, const struct hvn_mkgen_args *args) {
	size_t const namelen = strlen(name);
	char path[sourceslen + namelen + 2];
	char * const paths[] = { path, NULL };

	*stpncpy(path, args->sources, sourceslen) = '/';
	stpncpy(path + sourceslen + 1, name, namelen + 1);

	FTS *ftsp = fts_open(paths, FTS_LOGICAL, NULL);
	if(ftsp != NULL) {
		struct hvn_mkgen_array *objects = NULL;
		FTSENT *ftsentp;

		while(ftsentp = fts_read(ftsp), ftsentp != NULL) {
			if(*ftsentp->fts_name != '.') {
				const char *source = ftsentp->fts_path + sourceslen;
				const char *extension;

				switch(ftsentp->fts_info) {
				case FTS_D:
					if(ftsentp->fts_level == 0) {
						fprintf(output, TARGET_OBJ "%s:\n", source);
					} else {
						fprintf(output, TARGET_OBJ "%s: " TARGET_OBJ "%*s\n",
							source, (int)(ftsentp->fts_pathlen - sourceslen - ftsentp->fts_namelen), source);
					}
					fputs("\t$(MKDIR) $@\n", output);
					break;
				case FTS_F:
					extension = strrchr(source, '.');
					if(extension != NULL && extension[1] != '\0') {
						char *object = strndup(source, extension - source + 2);
						object[extension - source + 1] = 'o';

						fprintf(output, TARGET_OBJ "%s: " TARGET_OBJ "%s " TARGET_OBJ "%.*s\n",
							object, source, (int)(ftsentp->fts_pathlen - sourceslen - ftsentp->fts_namelen), source);
						hvn_mkgen_print_rule_extension(extension, output);
						hvn_mkgen_array_append(&objects, object);
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
				free(*current);
				++current;
			}

			if(args->dependencies != NULL) {
				hvn_mkgen_print_module_dependencies(name, namelen, args->dependencies, output);
			}

			fprintf(output, "\n\t$(LD) $(LDFLAGS) $(%s) -o $@ $^\n", macro);
		} else {
			warnx("No objects for module %s", name);
		}

		fts_close(ftsp);
		free(objects);
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
			warnx("Unknown argument -%c", c);
			hvn_mkgen_usage(*argv);
		}
	}

	if(argc - optind > 1) {
		warnx("Too much output files");
		hvn_mkgen_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_mkgen_args args = hvn_mkgen_parse_args(argc, argv);
	const char *filename = argc == optind ? "Makefile.rules.test" : argv[optind];
	struct dirent *entry;
	DIR *modules;
	FILE *output;

	modules = opendir(args.sources);
	if(modules == NULL) {
		err(EXIT_FAILURE, "opendir %s", filename);
	}

	output = fopen(filename, "w");
	if(output == NULL) {
		err(EXIT_FAILURE, "fopen %s", filename);
	}

	fputs(".PHONY: all clean\nall:\n", output);

	size_t const sourceslen = strlen(args.sources);
	while(entry = readdir(modules), entry != NULL) {
		if(*entry->d_name != '.') {
			hvn_mkgen_print_module_rules(entry->d_name, output, sourceslen, &args);
		}
	}

	fputs("all:", output);

	rewinddir(modules);
	while(entry = readdir(modules), entry != NULL) {
		if(*entry->d_name != '.') {
			fputc(' ', output);
			hvn_mkgen_print_module_target(entry->d_name, output);
		}
	}

	fputs("\nclean:\n\trm -rf " TARGET_BIN "/* " TARGET_LIB "/* " TARGET_OBJ "/*\n", output);

	fclose(output);

	closedir(modules);

	free(args.dependencies);

	return EXIT_SUCCESS;
}
