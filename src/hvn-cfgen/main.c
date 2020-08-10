/*
	main.c
	Copyright (c) 2020, Valentin Debon

	This file is part of the heaven repository
	subject the BSD 3-Clause License, see LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <err.h>

#define SEARCH_MKDIR "mkdir"
#define SEARCH_LD    "ld gold"
#define SEARCH_CC    "clang gcc tcc cc"
#define SEARCH_AS    "as gas "SEARCH_CC
#define SEARCH_CXX   "clang++ g++"

#define FLAGS_D_CC "-g -Wall -fPIC"
#define FLAGS_R_CC "-O -Wall -fPIC -DNDEBUG"

#define FLAGS_D_AS ""
#define FLAGS_R_AS ""

#define FLAGS_D_CXX "-g -Wall -fPIC -std=c++11"
#define FLAGS_R_CXX "-O -Wall -fPIC -std=c++11 -DNDEBUG"

#define INTRO_BEGIN \
	"#!/bin/sh\n" \
	"\n" \
	"while getopts hrd opt\n" \
	"do\n" \
	"	case $opt in\n" \
	"	h) cat <<EOF\n" \
	"\\`configure' configures this package to adapt to many kinds of systems.\n" \
	"\n" \
	"Usage: ./configure [-h] [-r|-d] [VAR=VALUE]...\n" \
	"\n" \
	"To assign environment variables (e.g., CC, CFLAGS...), specify them as\n" \
	"VAR=VALUE.  See below for descriptions of some of the useful variables.\n" \
	"\n" \
	"Defaults for the options are specified in brackets.\n" \
	"\n" \
	"Configuration:\n" \
	"  -h               display this help and exit\n" \
	"\n" \
	"Optional Features:\n" \
	"  -d               configure a debug build (default)\n" \
	"  -r               configure a release build\n" \
	"\n" \
	"Some influential environment variables:\n" \
	"  BINARIES         where binary executables are built.\n" \
	"  LIBRARIES        where libraries are built.\n" \
	"  OBJECTS          where intermediate object files are built.\n" \
	"  MKDIR            tool used to create objects subdirectories.\n" \
	"  LD               linker to use.\n" \
	"  LDFLAGS          arguments to pass to the linker.\n"

#define INTRO_HAS_C \
	"  CC               C compiler to use, default [" SEARCH_CC "].\n" \
	"  CFLAGS           C compiler flags [" FLAGS_R_CC "] when -r specified, [" FLAGS_D_CC "] else.\n"

#define INTRO_HAS_ASM \
	"  AS               Assembly compiler to use, default [" SEARCH_AS "].\n" \
	"  ASFLAGS          Assembly compiler flags [" FLAGS_R_AS "] when -r specified, [" FLAGS_D_AS "] else.\n"

#define INTRO_HAS_CXX \
	"  CXX              C compiler to use, default [" SEARCH_CXX "].\n" \
	"  CXXFLAGS         C compiler flags [" FLAGS_R_CXX "] when -r specified, [" FLAGS_D_CXX "] else.\n"

#define INTRO_END \
	"\n" \
	"Use these variables to override the choices made by \\`configure' or to help\n" \
	"it to find libraries and programs with nonstandard names/locations.\n" \
	"\n" \
	"Report bugs to the package provider.\n" \
	"EOF\n" \
	"		exit 1 ;;\n" \
	"	r) NDEBUG=\"${opt}\" ;;\n" \
	"	d) unset NDEBUG ;;\n" \
	"	?) echo \"Unknown option: ${opt}\" ;;\n" \
	"	esac\n" \
	"done\n" \
	"\n" \
	"shift $((OPTIND - 1))\n" \
	"[ ! -z \"$*\" ] && export -- \"$@\"\n" \
	"\n" \
	"case \"`uname -s`\" in\n" \
	"Darwin) [ -z \"${LDFLAGS}\" ] && LDFLAGS='-lSystem' ;;\n" \
	"Linux) [ -z \"${LDFLAGS}\" ] && LDFLAGS='-lc' ;;\n" \
	"*) printf 'Unsupported operating system\\n' && exit 1 ;;\n" \
	"esac\n"

#define SEARCH_TOOL(toolname, tool, searchtool) \
	"\n" \
	"if [ -z \"${" tool "}\" ]\n" \
	"then\n" \
	"	for " tool " in " searchtool "\n" \
	"	do command -v \"${" tool "}\" && break\n" \
	"	done\n" \
	"fi\n" \
	"\n" \
	"if [ ! -z \"${" tool "}\" ]\n" \
	"then printf \"Using " toolname " '\%s'\\n\" \"${" tool "}\"\n" \
	"else printf 'Unable to find " toolname "\\n' ; exit 1\n" \
	"fi\n"

#define SEARCH_FLAGS(toolname, toolflags, flagsdtool, flagsrtool) \
	"\n" \
	"if [ -z \"${" toolflags "}\" ]\n" \
	"then\n" \
	"	if [ -z \"${NDEBUG}\" ]\n" \
	"	then " toolflags "='" flagsdtool "'\n" \
	"	else " toolflags "='" flagsrtool "'\n" \
	"	fi\n" \
	"fi\n" \
	"printf \"Using " toolname " flags '\%s'\\n\" \"${" toolflags "}\"\n"

#define SEARCH_TOOL_AND_FLAGS(toolname, tool, toolflags, searchtool, flagsdtool, flagsrtool) \
	SEARCH_TOOL(toolname, tool, searchtool) \
	SEARCH_FLAGS(toolname, toolflags, flagsdtool, flagsrtool)

#define OUTRO_BEGIN \
	"\n" \
	"[ -z \"${BINARIES}\" ] && BINARIES=\"build/bin\"\n" \
	"[ -z \"${LIBRARIES}\" ] && LIBRARIES=\"build/lib\"\n" \
	"[ -z \"${OBJECTS}\" ] && OBJECTS=\"build/objects\"\n" \
	"mkdir -p \"${BINARIES}\" \"${LIBRARIES}\" \"${OBJECTS}\"\n" \
	"\n" \
	"cat - Makefile.rules <<EOF > Makefile\n" \
	"BINARIES=${BINARIES}\n" \
	"LIBRARIES=${LIBRARIES}\n" \
	"OBJECTS=${OBJECTS}\n" \
	"\n" \
	"MKDIR=${MKDIR}\n" \
	"LD=${LD}\n" \
	"LDFLAGS=${LDFLAGS}\n"

#define OUTRO_MACRO(macro) \
	macro "=${" macro "}\n"

#define OUTRO_END \
	"\nEOF"

struct hvn_cfgen_args {
	const char *sources;
	unsigned hasC : 1;
	unsigned hasAsm : 1;
	unsigned hasCXX : 1;
};

static void
hvn_cfgen_intro_print_macro(const char *macro, FILE *output) {
	fprintf(output, "  %s\n", macro);
}

static void
hvn_cfgen_search_print_macro(const char *macro, FILE *output) {
	fprintf(output, "[ -z \"${%s}\" ] && %s=\"\"\n", macro, macro);
}

static void
hvn_cfgen_outro_print_macro(const char *macro, FILE *output) {
	fprintf(output, "%s=${%s}\n", macro, macro);
}

static void
hvn_cfgen_print_macros(DIR *modules, FILE *output, void (*print)(const char *, FILE *)) {
	static const char macrosuffix[] = "FLAGS";
	struct dirent *entry;

	while(entry = readdir(modules), entry != NULL) {
		if(*entry->d_name != '.') {
			size_t length = strlen(entry->d_name);
			char macro[length + sizeof(macrosuffix)];
			char *uppercase = macro;

			/* Generate a 'macro' associated to each module in sources, basically uppercased
			   with non alnum's characters replaced by underscores, with FLAGS appended to it */
			strncpy(uppercase, entry->d_name, length);
			while(length != 0) {
				char const character = *uppercase;

				if(isalpha(character)) {
					*uppercase = character - 'a' + 'A';
				} else if(isdigit(character)) {
					*uppercase = character;
				} else {
					*uppercase = '_';
				}

				++uppercase;
				--length;
			}
			stpncpy(uppercase, macrosuffix, sizeof(macrosuffix));

			print(macro, output);
		}
	}
}

static void
hvn_cfgen_usage(const char *hvncfgenname) {
	fprintf(stderr, "usage: %s [-S sources] [-l languages] [filename]\n", hvncfgenname);
	exit(EXIT_FAILURE);
}

static const struct hvn_cfgen_args
hvn_cfgen_parse_args(int argc, char **argv) {
	struct hvn_cfgen_args args = {
		.sources = "src",
		.hasC = 0,
		.hasAsm = 0,
		.hasCXX = 0,
	};
	char *next;
	int c;

	while((c = getopt(argc, argv, ":S:l:")) != -1) {
		switch(c) {
		case 'S':
			args.sources = optarg;
			break;
		case 'l':
			next = strtok(optarg, ",");

			while(next != NULL) {
				const char *language = next;
				next = strtok(NULL, ",");

				if(strcasecmp("C", language) == 0) {
					args.hasC = 1;
					continue;
				}

				if(strcasecmp("ASM", language) == 0) {
					args.hasAsm = 1;
					continue;
				}

				if(strcasecmp("C++", language) == 0) {
					args.hasCXX = 1;
					continue;
				}

				warnx("-%c %s: Unsupported language", optopt, language);
			}
			break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			hvn_cfgen_usage(*argv);
		default:
			warnx("Unknown argument -%c", optopt);
			hvn_cfgen_usage(*argv);
		}
	}

	if(argc - optind > 1) {
		warnx("Too many output files");
		hvn_cfgen_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct hvn_cfgen_args args = hvn_cfgen_parse_args(argc, argv);
	const char *filename = argc == optind ? "configure" : argv[optind];
	DIR *modules;
	FILE *output;

	/* Begin, open up and stuff */
	modules = opendir(args.sources);
	if(modules == NULL) {
		err(EXIT_FAILURE, "opendir %s", filename);
	}

	output = fopen(filename, "w");
	if(output == NULL) {
		err(EXIT_FAILURE, "fopen %s", filename);
	}

	/* Beginning of the script, handle command line arguments, and show supported flags */
	fputs(INTRO_BEGIN, output);

	if(args.hasC == 1) {
		fputs(INTRO_HAS_C, output);
	}

	if(args.hasAsm == 1) {
		fputs(INTRO_HAS_ASM, output);
	}

	if(args.hasCXX == 1) {
		fputs(INTRO_HAS_CXX, output);
	}

	hvn_cfgen_print_macros(modules, output, hvn_cfgen_intro_print_macro);

	fputs(INTRO_END, output);

	/* Configuration part, we look up for tools in the system */
	if(args.hasC == 1) {
		fputs(SEARCH_TOOL_AND_FLAGS("C compiler", "CC", "CFLAGS", SEARCH_CC, FLAGS_D_CC, FLAGS_R_CC), output);
	}

	if(args.hasAsm == 1) {
		fputs(SEARCH_TOOL_AND_FLAGS("Assembly compiler", "AS", "ASFLAGS", SEARCH_AS, FLAGS_D_AS, FLAGS_R_AS), output);
	}

	if(args.hasCXX == 1) {
		fputs(SEARCH_TOOL_AND_FLAGS("C++ compiler", "CXX", "CXXFLAGS", SEARCH_CXX, FLAGS_D_CXX, FLAGS_R_CXX), output);
	}

	fputs(SEARCH_TOOL("mkdir", "MKDIR", SEARCH_MKDIR), output);

	fputs(SEARCH_TOOL("linker", "LD", SEARCH_LD), output);

	fputc('\n', output);

	rewinddir(modules);
	hvn_cfgen_print_macros(modules, output, hvn_cfgen_search_print_macro);

	/* Last part, we redact the head of the configure-generated Makefile */
	fputs(OUTRO_BEGIN, output);

	if(args.hasC == 1) {
		fputs(OUTRO_MACRO("CC") OUTRO_MACRO("CFLAGS"), output);
	}

	if(args.hasAsm == 1) {
		fputs(OUTRO_MACRO("AS") OUTRO_MACRO("ASFLAGS"), output);
	}

	if(args.hasCXX == 1) {
		fputs(OUTRO_MACRO("CXX") OUTRO_MACRO("CXXFLAGS"), output);
	}

	fputc('\n', output);

	rewinddir(modules);
	hvn_cfgen_print_macros(modules, output, hvn_cfgen_outro_print_macro);

	fputs(OUTRO_END, output);

	/* End, clean up and stuff */
	fclose(output);

	closedir(modules);

	if(chmod(filename, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
		err(EXIT_FAILURE, "chmod %s", filename);
	}

	return EXIT_SUCCESS;
}
