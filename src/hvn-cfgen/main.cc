#include <string>
#include <filesystem>
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
	const char *output;
} options = {
	.output = "Makefile.rules",
};

}

int
main(int argc, char **argv) {
	int c;

	while((c = getopt(argc, argv, "o:")) != -1) {
		switch(c) {
		case 'o':
			hvn::options.output = optarg;
			break;
		}
	}

	ofstream makefile(hvn::options.output, ofstream::out);

	return hvn::status;
}

