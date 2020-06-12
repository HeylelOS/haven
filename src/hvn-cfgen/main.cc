#include <fstream>

#include <hvn/hvn.h>

extern "C" {
#include <unistd.h>
}

using namespace std;

namespace hvn {

int status;

struct {
	const char *sources;
	const char *output;
} options = {
	.sources = "src",
	.output = "configure",
};

struct cfgen final : ofstream {

	cfgen(const char *filename) : ofstream(filename, ios_base::out | ios_base::trunc) {
	}

	~cfgen(void) {
		bool const hasC = this->_languages.find(C) != this->_languages.end(),
			hasCPP = this->_languages.find(CPP) != this->_languages.end();

		*this << intro_begin;

		if(hasC) {
			*this << 
				"  CC               C compiler to use, default [" << search_cc << "].\n"
				"  CFLAGS           C compiler flags [" << flags_r_cc << "] when -r specified, [" << flags_d_cc << "] else.\n";
		}

		if(hasCPP) {
			*this << 
				"  CXX              C++ compiler to use, default [" << search_cxx << "].\n"
				"  CXXFLAGS         C++ compiler flags [" << flags_r_cxx << "] when -r specified, [" << flags_d_cxx << "] else.\n";
		}

		for(auto &macro : this->_modules_macros) {
			*this << "  " << macro << "\n";
		}

		*this << intro_end;

		if(hasC) {
			*this <<
				"\n"
				"if [ -z \"${CC}\" ]\n"
				"then\n"
				"	for CC in " << search_cc << "\n"
				"	do command -v \"${CC}\" && break\n"
				"	done\n"
				"fi\n"
				"\n"
				"if [ ! -z \"${CC}\" ]\n"
				"then printf \"Using C compiler '\%s'\\n\" \"${CC}\"\n"
				"else printf 'Unable to find C compiler\\n' ; exit 1\n"
				"fi\n"
				"\n"
				"if [ -z \"${CFLAGS}\" ]\n"
				"then\n"
				"	if [ -z \"${NDEBUG}\" ]\n"
				"	then CFLAGS='" << flags_d_cc << "'\n"
				"	else CFLAGS='" << flags_r_cc << "'\n"
				"	fi\n"
				"fi\n"
				"printf \"Using C flags '\%s'\\n\" \"${CFLAGS}\"\n";
		}

		if(hasCPP) {
			*this <<
				"\n"
				"if [ -z \"${CXX}\" ]\n"
				"then\n"
				"	for CXX in " << search_cxx << "\n"
				"	do command -v \"${CXX}\" && break\n"
				"	done\n"
				"fi\n"
				"\n"
				"if [ ! -z \"${CXX}\" ]\n"
				"then printf \"Using C++ compiler '\%s'\\n\" \"${CXX}\"\n"
				"else printf 'Unable to find C++ compiler\\n' ; exit 1\n"
				"fi\n"
				"\n"
				"if [ -z \"${CXXFLAGS}\" ]\n"
				"then\n"
				"	if [ -z \"${NDEBUG}\" ]\n"
				"	then CXXFLAGS='" << flags_d_cxx << "'\n"
				"	else CXXFLAGS='" << flags_r_cxx << "'\n"
				"	fi\n"
				"fi\n"
				"printf \"Using C++ flags '\%s'\\n\" \"${CXXFLAGS}\"\n";
		}

		*this <<
				"\n"
				"if [ -z \"${MKDIR}\" ]\n"
				"then\n"
				"	for MKDIR in " << search_mkdir << "\n"
				"	do command -v \"${MKDIR}\" && break\n"
				"	done\n"
				"fi\n"
				"\n"
				"if [ -z \"${LD}\" ]\n"
				"then\n"
				"	for LD in " << search_ld << "\n"
				"	do command -v \"${LD}\" && break\n"
				"	done\n"
				"fi\n"
				"printf 'Using linker flags '\%s'\\n' \"${LDFLAGS}\"\n"
				"\n";

		for(auto &macro : this->_modules_macros) {
			*this << "[ -z \"${" << macro << "}\" ] && " << macro << "=\"\"\n";
		}

		*this <<
				"\n"
				"[ -z \"${BINARIES}\" ] && BINARIES=\"build/bin\"\n"
				"[ -z \"${LIBRARIES}\" ] && LIBRARIES=\"build/lib\"\n"
				"[ -z \"${OBJECTS}\" ] && OBJECTS=\"build/objects\"\n"
				"mkdir -p \"${BINARIES}\" \"${LIBRARIES}\" \"${OBJECTS}\"\n"
				"\n"
				"cat - Makefile.rules <<EOF > Makefile\n"
				"BINARIES=${BINARIES}\n"
				"LIBRARIES=${LIBRARIES}\n"
				"OBJECTS=${OBJECTS}\n"
				"\n"
				"MKDIR=${MKDIR}\n"
				"LD=${LD}\n"
				"LDFLAGS=${LDFLAGS}\n";

		if(hasC) {
			*this <<
				"CC=${CC}\n"
				"CFLAGS=${CFLAGS}\n";
		}

		if(hasCPP) {
			*this <<
				"CXX=${CXX}\n"
				"CXXFLAGS=${CXXFLAGS}\n";
		}

		*this << "\n";

		for(auto &macro : this->_modules_macros) {
			*this << macro << "=${" << macro << "}\n";
		}

		*this << "\nEOF";
	}

	cfgen &
	operator<<(module const &module) {

		for(auto &extension : module.extensions()) {
			this->_languages.insert(language_with_extension(extension));
		}

		this->_modules_macros.insert(module.macro());

		return *this;
	}

private:
	static constexpr char search_mkdir[] = "mkdir";
	static constexpr char search_ld[]    = "ld gold";
	static constexpr char search_cc[]    = "clang gcc tcc cc";
	static constexpr char search_cxx[]   = "clang++ g++";

	static constexpr char flags_r_cc[] = "-O -Wall -fPIC";
	static constexpr char flags_d_cc[] = "-g -Wall -fPIC";

	static constexpr char flags_r_cxx[] = "-O -Wall -fPIC -std=c++11";
	static constexpr char flags_d_cxx[] = "-g -Wall -fPIC -std=c++11";

	static constexpr char intro_begin[] =
	"#!/bin/sh\n"
	"\n"
	"while getopts hrd opt\n"
	"do\n"
	"	case $opt in\n"
	"	h) cat <<EOF\n"
	"\\`configure' configures this package to adapt to many kinds of systems.\n"
	"\n"
	"Usage: ./configure [-h] [-r|-d] [VAR=VALUE]...\n"
	"\n"
	"To assign environment variables (e.g., CC, CFLAGS...), specify them as\n"
	"VAR=VALUE.  See below for descriptions of some of the useful variables.\n"
	"\n"
	"Defaults for the options are specified in brackets.\n"
	"\n"
	"Configuration:\n"
	"  -h               display this help and exit\n"
	"\n"
	"Optional Features:\n"
	"  -d               configure a debug build (default)\n"
	"  -r               configure a release build\n"
	"\n"
	"Some influential environment variables:\n"
	"  BINARIES         where binary executables are built.\n"
	"  LIBRARIES        where libraries are built.\n"
	"  OBJECTS          where intermediate object files are built.\n"
	"  MKDIR            tool used to create objects subdirectories.\n"
	"  LD               linker to use.\n"
	"  LDFLAGS          arguments to pass to the linker.\n";

	static constexpr char intro_end[] =
	"\n"
	"Use these variables to override the choices made by \\`configure' or to help\n"
	"it to find libraries and programs with nonstandard names/locations.\n"
	"\n"
	"Report bugs to the package provider.\n"
	"EOF\n"
	"		exit 1 ;;\n"
	"	r) NDEBUG=\"${opt}\" ;;\n"
	"	d) unset NDEBUG ;;\n"
	"	?) echo \"Unknown option: ${opt}\" ;;\n"
	"	esac\n"
	"done\n"
	"\n"
	"shift $((OPTIND - 1))\n"
	"[ ! -z \"$*\" ] && export -- \"$@\"\n"
	"\n"
	"case \"`uname -s`\" in\n"
	"Darwin) [ -z \"${LDFLAGS}\" ] && LDFLAGS='-lSystem' ;;\n"
	"Linux) [ -z \"${LDFLAGS}\" ] && LDFLAGS='-lc' ;;\n"
	"*) printf 'Unsupported operating system\\n' && exit 1 ;;\n"
	"esac\n";

	unordered_set<language> _languages;
	unordered_set<string> _modules_macros;
};

}

int
main(int argc, char **argv) {
	int c;

	while((c = getopt(argc, argv, "S:o:")) != -1) {
		switch(c) {
		case 'S':
			hvn::options.sources = optarg;
			break;
		case 'o':
			hvn::options.output = optarg;
			break;
		}
	}

	hvn::cfgen configure(hvn::options.output);

	for(auto &entry : filesystem::directory_iterator(hvn::options.sources)) {
		configure << hvn::module(entry.path());
	}

	return hvn::status;
}

