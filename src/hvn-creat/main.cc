#include <filesystem>
#include <algorithm>
#include <iostream>

extern "C" {
#include <unistd.h>
}

using namespace std;

namespace hvn {

int status;

struct {
	const char *sources;
} options = {
	.sources = "src"
};

void
creat(const char *argument) {
	string const target(argument);

	if(target.find(filesystem::path::preferred_separator) != string::npos) {
		cerr << "Invalid target name: " << target  << "\n";
		status = 1;
		return;
	}

	filesystem::path p(options.sources);
	p.append(target);
	error_code ec;

	if(!filesystem::create_directory(p, ec)) {
		if((bool) ec) {
			cerr << "Unable to create directory " << p << ": " << ec.message() << "\n";
		} else {
			cerr << p << " already exists\n";
		}
	}
}

}

int
main(int argc, char **argv) {
	int c;

	while((c = getopt(argc, argv, "S:")) != -1) {
		switch(c) {
		case 'S':
			hvn::options.sources = optarg;
			break;
		}
	}

	if(argc != optind) {
		filesystem::create_directories(hvn::options.sources);
		for_each(argv + optind, argv + argc, hvn::creat);
	} else {
		cerr << "usage: " << *argv << " <targets>...\n";
	}

	return hvn::status;
}

