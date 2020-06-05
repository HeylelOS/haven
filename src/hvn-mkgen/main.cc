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
	const char *sources;
	const char *objects;
	const char *binaries;
	const char *libraries;
	const char *libraries_extension;
	const char *output;
} options = {
	.sources = "src",
	.objects = "$(OBJECTS)",
	.binaries = "$(BINARIES)",
	.libraries = "$(LIBRARIES)",
	.libraries_extension = ".so",
	.output = "Makefile.rules",
};

inline void
write_target_rule(ofstream &makefile, filesystem::directory_entry const &entry) {
	if(entry.is_directory()) {
		makefile << "\t$(MKDIR) $@\n\n";
	} else {
		string const &extension(entry.path().extension());

		if(extension == ".c") {
			makefile << "\t$(CC) $(CFLAGS) -c -o $@ $<\n\n";
		} else if(extension == ".cc" || extension == ".cpp") {
			makefile << "\t$(CXX) $(CXXFLAGS) -c -o $@ $<\n\n";
		}
	}
}

}

int
main(int argc, char **argv) {
	int c;

	while((c = getopt(argc, argv, "S:O:B:L:l:o:")) != -1) {
		switch(c) {
		case 'S':
			hvn::options.sources = optarg;
			break;
		case 'O':
			hvn::options.objects = optarg;
			break;
		case 'B':
			hvn::options.binaries = optarg;
			break;
		case 'L':
			hvn::options.libraries = optarg;
			break;
		case 'l':
			hvn::options.libraries_extension = optarg;
			break;
		case 'o':
			hvn::options.output = optarg;
			break;
		}
	}

	ofstream makefile(hvn::options.output, ofstream::out);
	hvn::module binaries("bin", hvn::module::Folder);
	hvn::module libraries("lib", hvn::module::Folder);

	makefile << ".PHONY: _all _clean all clean\n\nall: _all\nclean: _clean\n\n";

	for(auto &module_entry : filesystem::directory_iterator(hvn::options.sources)) {
		size_t const module_entry_path_length(module_entry.path().native().length());
		hvn::module targets(hvn::module::create_from_entry(module_entry, false));

		makefile << hvn::options.objects << "/" << targets.name() << ":\n";
		hvn::write_target_rule(makefile, module_entry);

		for(auto &source_entry : filesystem::recursive_directory_iterator(module_entry.path())) {
			filesystem::path target(source_entry.path().native().substr(module_entry_path_length));

			// Ignore hidden files
			if(source_entry.path().filename().native().front() == '.') {
				continue;
			}

			// Target and first dependency if a file
			makefile << hvn::options.objects << "/" << targets.name();
			if(!source_entry.is_directory()) {
				makefile << target.replace_extension(".o").native() << ": " << source_entry.path().native();
			} else {
				makefile << target.native() << ":";
			}

			// Add target to targets list
			targets.insert(target.native());

			// Add parent directory of target
			if(!target.empty()) {
				makefile << " " << hvn::options.objects << "/" << targets.name()
					<< target.remove_filename().native();
			}

			makefile << "\n";
			hvn::write_target_rule(makefile, source_entry);
		}

		// Write target linking
		if(!targets.empty()) {
			// Executable or library target
			switch(targets.type()) {
			case hvn::module::Executable:
				binaries.insert(targets.name());
				makefile << hvn::options.binaries << "/" << targets.name();
				break;
			case hvn::module::Library:
				libraries.insert(targets.name());
				makefile << hvn::options.libraries << "/" << targets.name() << hvn::options.libraries_extension;
				break;
			default:
				cerr << targets.name() << ": Unexpected module type\n";
				continue;
			}

			// Per Module linking rule
			makefile << ": ";
			for(auto &target : targets) {
				makefile << hvn::options.objects << "/" << targets.name() << target;
			}
			makefile << "\n\t$(LD) $(LDFLAGS) $(" << targets.macro() << ") -o $@ $^\n\n";
		}
	}

	// _all rule
	makefile << "_all:";
	for(auto &binary : binaries) { makefile << " " << hvn::options.binaries << "/" << binary; }
	for(auto &library : libraries) { makefile << " " << hvn::options.libraries << "/" << library << hvn::options.libraries_extension; }

	// _clean rule
	makefile << "\n_clean:\n\trm -rf $(wildcard $(BINARIES)/*) $(wildcard $(LIBRARIES)/*) $(wildcard $(OBJECTS)/*)\n\n";

	return hvn::status;
}

