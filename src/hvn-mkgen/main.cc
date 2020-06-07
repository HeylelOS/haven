#include <fstream>
#include <unordered_map>

#include <hvn/hvn.h>

extern "C" {
#include <unistd.h>
#include <string.h>
}

using namespace std;

namespace hvn {

int status;

struct {
	unordered_map<string, unordered_set<string>> dependencies;
	const char *sources;
	const char *output;
} options = {
	.sources = "src",
	.output = "Makefile.rules",
};

struct mkgen final : ofstream {
	static constexpr char binaries[]  = "$(BINARIES)";
	static constexpr char libraries[] = "$(LIBRARIES)";
	static constexpr char objects[]   = "$(OBJECTS)";

	mkgen(const char *filename) : ofstream(filename) {
		*this << ".PHONY: first all clean\nfirst: all\n";
	}

	~mkgen(void) {
		*this << "all:";

		for(auto &name: this->_modules) {
			*this << " ";
			this->print_module(name);
		}

		*this << "\nclean:\n\trm -rf " << binaries << "/* " << libraries << "/* " << objects << "/*";
	}

	mkgen &
	operator<<(module const &module) {
		unordered_set<string> object_directories { module.name() };

		for(auto &object : module.objects()) {
			this->print_target(module.name());
			*this << object.first << ": " << object.second << " ";

			// Write object parent directory if exists
			size_t const parent(object.first.rfind('/'));
			if(parent != 0) {
				*this << *object_directories.insert(module.name() + string(object.first, 0, parent)).first;
			} else {
				this->print_target(module.name());
			}

			*this << "\n";

			this->print_source_rule(object.second);
		}

		// Make directories for each parent directory
		for(auto &directory : object_directories) {
			this->print_target(directory);

			size_t const parent(directory.rfind('/'));
			if(parent != string::npos) {
				*this << ": ";
				this->print_target(directory.substr(0, parent));
				*this << "\n";
			} else {
				*this << ":\n";
			}

			*this << "\t$(MKDIR) $@\n";
		}

		// Make linking rule
		this->print_module(module.name());
		*this << ":";

		// Linking all objects
		for(auto &object : module.objects()) {
			*this << " ";
			this->print_target(module.name());
			*this << object.first;
		}

		// Adding dependencies
		auto dependency_list_iterator = hvn::options.dependencies.find(module.name());
		if(dependency_list_iterator != hvn::options.dependencies.end()) {
			for(auto &dependency : dependency_list_iterator->second) {
				*this << " ";
				this->print_module(dependency);
			}
		}
		*this << "\n\t$(LD) $(LDFLAGS) $(" << module.macro() << ") -o $@ $^\n";

		this->_modules.insert(module.name());

		return *this;
	}

private:
	unordered_set<string> _modules;

	void
	print_module(string const &name) {
		if(name.find(module::libraries_prefix) == 0) {
			*this << libraries << "/" << name << module::libraries_suffix;
		} else {
			*this << binaries << "/" << name;
		}
	}

	void
	print_target(string const &target) {
		*this << objects << "/" << target;
	}

	void
	print_source_rule(string const &source) {
		string const extension(source, source.rfind('.'));
		// Every source file has an extension, no possible string::npos here

		if(extension == ".c") {
			*this << "\t$(CC) $(CFLAGS) -c -o $@ $<\n";
		} else if(extension == ".cc" || extension == ".cpp") {
			*this << "\t$(CXX) $(CXXFLAGS) -c -o $@ $<\n";
		}
	}
};

}

int
main(int argc, char **argv) {
	int c;

	while((c = getopt(argc, argv, "S:d:o:")) != -1) {
		switch(c) {
		case 'S':
			hvn::options.sources = optarg;
			break;
		case 'd': {
				const char *equal = strchr(optarg, '=');
				if(equal != NULL) {
					unordered_set<string> dependency_list;
					const char *begin = equal + 1, *end;

					while(end = strchr(begin, ','), end != NULL) {
						if(*begin != '\0') {
							dependency_list.emplace(begin, 0, end - begin);
						}

						begin = end + 1;
					}

					if(*begin != '\0') {
						dependency_list.emplace(begin);
					}

					hvn::options.dependencies.insert_or_assign(string(optarg, equal - optarg), dependency_list);
				}
			} break;
		case 'o':
			hvn::options.output = optarg;
			break;
		}
	}

	hvn::mkgen makefile(hvn::options.output);

	for(auto &entry : filesystem::directory_iterator(hvn::options.sources)) {
		makefile << hvn::module(entry.path());
	}

	return hvn::status;
}

