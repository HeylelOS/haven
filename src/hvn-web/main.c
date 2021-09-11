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

struct hvn_web_args {
	char *input, *output;
	char *head, *tail;
};

struct hvn_web_render_option {
	const char *name;
	int value;
};

static const struct hvn_web_render_option renderoptions[] = {
	{ "sourcepos", CMARK_OPT_SOURCEPOS },
	{ "hardbreaks", CMARK_OPT_HARDBREAKS },
	{ "nobreaks", CMARK_OPT_NOBREAKS },
	{ "smart", CMARK_OPT_SMART },
	{ "safe", CMARK_OPT_SAFE },
	{ "unsafe", CMARK_OPT_UNSAFE },
	{ "validate-utf8", CMARK_OPT_VALIDATE_UTF8 },
};

static void
hvn_web_parse(int fd, cmark_parser *parser) {
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
hvn_web_dump(const char *filename, FILE *output, const struct hvn_web_args *args) {
	if(filename != NULL) {
		int fd = open(filename, O_RDONLY);

		if(fd >= 0) {
			const int pagesize = getpagesize();
			char buffer[pagesize];
			ssize_t readval;

			while(readval = read(fd, buffer, sizeof(buffer)), readval > 0) {
				fwrite(buffer, sizeof(*buffer), readval, output);
			}

			close(fd);
		} else {
			warn("open %s", filename);
		}
	}
}

static void
hvn_web_render(FILE *output, cmark_node *document, int options, const struct hvn_web_args *args) {
	hvn_web_dump(args->head, output, args);

	char *result = cmark_render_html(document, options);

	fputs(result, output);

	free(result);

	hvn_web_dump(args->tail, output, args);
}

static void
hvn_web_usage(const char *progname) {
	fprintf(stderr, "usage: %s [-i <input>] [-o <output>] [-H <head>] [-T <tail>] [<render options>...]\n", progname);
	exit(EXIT_FAILURE);
}

static const struct hvn_web_args
hvn_web_parse_args(int argc, char **argv) {
	struct hvn_web_args args = {
		.input = NULL, .output = NULL,
		.head = NULL, .tail = NULL,
	};
	int c;

	while(c = getopt(argc, argv, ":i:o:H:T:"), c != -1) {
		switch(c) {
		case 'i':
			free(args.input);
			args.input = strdup(optarg);
			break;
		case 'o':
			free(args.output);
			args.output = strdup(optarg);
			break;
		case 'H':
			free(args.head);
			args.head = strdup(optarg);
			break;
		case 'T':
			free(args.tail);
			args.tail = strdup(optarg);
			break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_web_usage(*argv);
		default:
			warnx("Unknown argument -%c", optopt);
			hvn_web_usage(*argv);
		}
	}

	/* Set output name from input's if available */
	if(args.input != NULL && args.output == NULL) {
		static const char htmlext[] = ".html";
		const char *filename = basename(args.input);
		const char *extension = strrchr(filename, '.');
		const size_t length = extension != NULL ?
			extension - filename : strlen(filename);
		char buffer[length + sizeof(htmlext)];

		strncpy(buffer, filename, length);
		strncpy(buffer + length, htmlext, sizeof(htmlext));

		args.output = strdup(buffer);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_web_args args = hvn_web_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;
	int options = CMARK_OPT_DEFAULT;

	/* Parse render options */
	while(argpos != argend) {
		const struct hvn_web_render_option *current = renderoptions,
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

		hvn_web_parse(fd, parser);

		close(fd);
	} else {
		hvn_web_parse(STDIN_FILENO, parser);
	}

	/* Print output file */
	cmark_node *document = cmark_parser_finish(parser);
	cmark_parser_free(parser);

	if(args.output != NULL) {
		FILE *output = fopen(args.output, "w");

		if(output == NULL) {
			err(EXIT_FAILURE, "fopen %s", args.output);
		}

		hvn_web_render(output, document, options, &args);

		fclose(output);
	} else {
		hvn_web_render(stdout, document, options, &args);
	}

	cmark_node_free(document);
	free(args.tail);
	free(args.head);
	free(args.output);
	free(args.input);

	return EXIT_SUCCESS;
}

