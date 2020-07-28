/*
	main.c
	Copyright (c) 2020, Valentin Debon

	This file is part of the heaven repository
	subject the BSD 3-Clause License, see LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

int
main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "usage: %s <subcommand> ...\n", *argv);
		exit(EXIT_FAILURE);
	}

	size_t const commandlen = strlen(*argv);
	size_t const subcommandlen = strlen(argv[1]);
	char path[commandlen + subcommandlen + 2];
	strncpy(stpncpy(path, *argv, commandlen) + 1, argv[1], subcommandlen + 1);
	path[commandlen] = '-';

	argv[1] = path;

	if(execvp(path, argv + 1) != 0) {
		err(EXIT_FAILURE, "Unable to process subcommand %s: %s", path + commandlen + 1, path);
	}

	return EXIT_SUCCESS;
}

