/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <err.h>

#include <cmark.h>

#define SECTION_MAX 8

struct hvn_man_args {
	char *input, *output;
	char *pagename;
	int section;
	int width;
};

struct hvn_man_render_option {
	const char *name;
	int value;
};

static const struct hvn_man_render_option renderoptions[] = {
	{ "sourcepos", CMARK_OPT_SOURCEPOS },
	{ "hardbreaks", CMARK_OPT_HARDBREAKS },
	{ "nobreaks", CMARK_OPT_NOBREAKS },
	{ "smart", CMARK_OPT_SMART },
	{ "safe", CMARK_OPT_SAFE },
	{ "unsafe", CMARK_OPT_UNSAFE },
	{ "validate-utf8", CMARK_OPT_VALIDATE_UTF8 },
};

static void
hvn_man_parse(int fd, cmark_parser *parser) {
	const int pagesize = getpagesize();
	char buffer[pagesize];
	ssize_t readval;

	while(readval = read(fd, buffer, sizeof(buffer)), readval > 0) {
		cmark_parser_feed(parser, buffer, readval);
	}

	if(readval < 0) {
		perror("read");
	}
}

static void
hvn_man_render(FILE *output, cmark_node *document, int options, const struct hvn_man_args *args) {
	char *result = cmark_render_man(document, options, args->width);

	fprintf(output, ".TH %s %d\n", args->pagename, args->section);

	fputs(result, output);

	free(result);
}

static void
hvn_man_usage(const char *progname) {
	fprintf(stderr, "usage: %s [-i <input>] [-o <output>] [-p <pagename>] [-s <section>] [-w <width>] [<render options>...]\n", progname);
	exit(EXIT_FAILURE);
}

static const struct hvn_man_args
hvn_man_parse_args(int argc, char **argv) {
	struct hvn_man_args args = {
		.input = NULL, .output = NULL,
		.pagename = NULL,
		.section = 0,
		.width = 0,
	};
	int c;

	while(c = getopt(argc, argv, ":i:o:p:s:w:"), c != -1) {
		switch(c) {
		case 'i':
			free(args.input);
			args.input = strdup(optarg);
			break;
		case 'o':
			free(args.output);
			args.output = strdup(optarg);
			break;
		case 'p':
			free(args.pagename);
			args.pagename = strdup(optarg);
			break;
		case 's': {
			char *endptr;
			const unsigned long value = strtoul(optarg, NULL, 10);

			if(value == 0 || value > SECTION_MAX || *endptr != '\0') {
				warnx("Invalid section number '%s'", optarg);
				hvn_man_usage(*argv);
			}

			args.section = value;
		} break;
		case 'w': {
			char *endptr;
			const unsigned long value = strtoul(optarg, NULL, 0);

			if(value == 0 || value > INT_MAX || *endptr != '\0') {
				warnx("Invalid wrap width '%s'", optarg);
				hvn_man_usage(*argv);
			}

			args.width = value;
		} break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_man_usage(*argv);
		default:
			warnx("Unknown argument -%c", optopt);
			hvn_man_usage(*argv);
		}
	}

	/* Set output name from input's if available */
	if(args.input != NULL && args.output == NULL) {
		const char *filename = basename(args.input);
		const char *extension = strrchr(filename, '.');
		const size_t length = extension != NULL ?
			extension - filename : strlen(filename);

		args.output = strndup(filename, length);
	}

	/* Set default missing parameters if output is set */
	if(args.output != NULL) {
		const char *filename = basename(args.output);
		const char *extension = strchr(filename, '.');
		const size_t length = extension != NULL ?
			extension - filename : strlen(filename);

		/* Create pagename from output name */
		if(args.pagename == NULL) {
			args.pagename = strndup(filename, length);
		}

		/* Extract section number from extension if possible */
		if(args.section == 0) {
			const char *section = extension + 1;
			char *endptr;

			const unsigned long value = strtoul(section, &endptr, 10);
			if(value != 0 && value <= SECTION_MAX && *endptr == '\0') {
				args.section = value;
			}
		}
	}

	/* Last parameters, if we're in a command pipeline and nothing was specified */

	if(args.pagename == NULL) {
		args.pagename = strdup("?");
	}

	if(args.section == 0) {
		args.section = 1;
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_man_args args = hvn_man_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;
	int options = CMARK_OPT_DEFAULT;

	/* Parse render options */
	while(argpos != argend) {
		const struct hvn_man_render_option *current = renderoptions,
			* const end = renderoptions + sizeof(renderoptions) / sizeof(*renderoptions);
		const char *name = *argpos;

		while(current != end && strcmp(current->name, name) != 0) {
			current++;
		}

		if(current != end) {
			options |= current->value;
		}

		argpos++;
	}

	/* Parse input file */
	cmark_parser *parser = cmark_parser_new(options);
	if(args.input != NULL) {
		int fd = open(args.input, O_RDONLY);

		if(fd < 0) {
			err(EXIT_FAILURE, "open %s", args.input);
		}

		hvn_man_parse(fd, parser);

		close(fd);
	} else {
		hvn_man_parse(STDIN_FILENO, parser);
	}

	/* Print output file */
	cmark_node *document = cmark_parser_finish(parser);
	cmark_parser_free(parser);

	if(args.output != NULL) {
		FILE *output = fopen(args.output, "w");

		if(output == NULL) {
			err(EXIT_FAILURE, "fopen %s", args.output);
		}

		hvn_man_render(output, document, options, &args);

		fclose(output);
	} else {
		hvn_man_render(stdout, document, options, &args);
	}

	cmark_node_free(document);
	free(args.pagename);
	free(args.output);
	free(args.input);

	return EXIT_SUCCESS;
}

