#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <hvn/hvn.h>

extern "C" {
#include <unistd.h>
}

using namespace std;

namespace hvn {

int status;

struct {
	const char *language_extension;
	const char *sources;
} options = {
	.language_extension = ".c",
	.sources = "src",
};

void
init(const char *argument) {
	string const target(argument);

	if(target.find(filesystem::path::preferred_separator) != string::npos) {
		cerr << "Invalid target name: " << target  << "\n";
		status = 1;
		return;
	}

	filesystem::path path(options.sources);
	path.append(target);
	error_code ec;

	if(filesystem::create_directory(path, ec)) {
		if(path.native().find(module::libraries_prefix) != 0) {
			filesystem::path main_path(path);
			main_path.append("main");
			main_path.replace_extension(options.language_extension);

			fstream main(main_path.native(), ios_base::out | ios_base::trunc);
			switch(language_with_extension(main_path.extension().native())) {
			case C:
				main << "#include <stdio.h>\n\nint\nmain(int argc, char **argv) {\n\n\tputs(\"Hello, World!\");\n\n\treturn 0;\n}\n\n";
				break;
			case CPP:
				main << "#include <iostream>\n\nint\nmain(int argc, char **argv) {\n\n\tstd::cout << \"Hello, World!\\n\";\n\n\treturn 0;\n}\n\n";
				break;
			default:
				break;
			}
		}
	} else if((bool) ec) {
		cerr << "Unable to create directory " << path << ": " << ec.message() << "\n";
	} else {
		cerr << path << " already exists\n";
	}
}

}

int
main(int argc, char **argv) {
	int c;

	while((c = getopt(argc, argv, "S:e:")) != -1) {
		switch(c) {
		case 'S':
			hvn::options.sources = optarg;
			break;
		case 'e':
			hvn::options.language_extension = optarg;
			break;
		}
	}

	if(argc != optind) {
		filesystem::create_directories(hvn::options.sources);
		for_each(argv + optind, argv + argc, hvn::init);
	} else {
		cerr << "usage: " << *argv << " <targets>...\n";
	}

	return hvn::status;
}

