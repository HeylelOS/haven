/*
	main.c
	Copyright (c) 2019, Valentin Debon

	This file is part of the heaven repository
	subject the BSD 3-Clause License, see LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#include <cmark.h>

struct hvn_mando_args {
	long pagesize;
	int options;
};

static void
hvn_mando_parse_file(const char *file, cmark_parser *parser, const struct hvn_mando_args *args) {
	char buffer[args->pagesize];
	int fd = open(file, O_RDONLY);
	ssize_t readval;

	if(fd < 0) {
		err(EXIT_FAILURE, "open %s", file);
	}

	while((readval = read(fd, buffer, args->pagesize)) > 0) {
		cmark_parser_feed(parser, buffer, readval);
	}

	if(readval < 0) {
		warn("read %s", file);
	}

	close(fd);
}

static void
hvn_mando_print_document(const char *filename, cmark_node *document, const struct hvn_mando_args *args) {
	cmark_iter *iterator = cmark_iter_new(document);
	FILE *output = fopen(filename, "w");
	cmark_event_type event;

	if(output == NULL) {
		err(EXIT_FAILURE, "fopen %s", filename);
	}

	while((event = cmark_iter_next(iterator)) != CMARK_EVENT_DONE) {
		cmark_node *current = cmark_iter_get_node(iterator);

		switch(cmark_node_get_type(current)) {
		case CMARK_NODE_HEADING:
			if(event == CMARK_EVENT_ENTER) {
				fputs(cmark_node_get_heading_level(current) == 1 ? ".TH " : ".SH ", output);
			} else { /* CMARK_EVENT_EXIT */
				fputc('\n', output);
			}
			break;
		case CMARK_NODE_PARAGRAPH:
			if(event == CMARK_EVENT_ENTER) {
				fputs(".PP\n", output);
			} else { /* CMARK_EVENT_EXIT */
				fputc('\n', output);
			}
			break;
		case CMARK_NODE_TEXT:
			fputs(cmark_node_get_literal(current), output);
			break;
		default:
			break;
		}
	}

	cmark_iter_free(iterator);
	fclose(output);
}

static void
hvn_mando_usage(const char *hvnmandoname) {
	fprintf(stderr, "usage: %s output input...\n", hvnmandoname);
	exit(EXIT_FAILURE);
}

static const struct hvn_mando_args
hvn_mando_parse_args(int argc, char **argv) {
	struct hvn_mando_args args = {
		.options = CMARK_OPT_DEFAULT,
	};
	int c;

	while((c = getopt(argc, argv, ":")) != -1) {
		switch(c) {
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_mando_usage(*argv);
		default:
			warnx("Unknown argument -%c", optopt);
			hvn_mando_usage(*argv);
		}
	}

	args.pagesize = sysconf(_SC_PAGESIZE);
	if(args.pagesize < 0) {
		errx(EXIT_FAILURE, "sysconf(_SC_PAGESIZE)");
	}

	if(argc == optind) {
		warnx("Output file not specified");
		hvn_mando_usage(*argv);
	}

	if(argc - optind < 2) {
		warnx("Missing input file(s)");
		hvn_mando_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_mando_args args = hvn_mando_parse_args(argc, argv);
	char **current = argv + optind, ** const argend = argv + argc;
	cmark_parser *parser = cmark_parser_new(args.options);
	const char *filename = *current;
	cmark_node *document;

	while(++current != argend) {
		hvn_mando_parse_file(*current, parser, &args);
	}

	document = cmark_parser_finish(parser);
	cmark_parser_free(parser);

	hvn_mando_print_document(filename, document, &args);

	cmark_node_free(document);

	return EXIT_SUCCESS;
}

