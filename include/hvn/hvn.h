#ifndef HVN_H
#define HVN_H

#include <string>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

namespace hvn {

struct module final {
	static constexpr char libraries_prefix[] = "lib";
	static constexpr char libraries_suffix[] = ".so";
	static constexpr char objects_suffix[]   = ".o";

	module(std::filesystem::path const &sources_path) : _name(sources_path.filename()) {
		size_t const sources_path_length(sources_path.native().length());

		for(auto &source_entry : std::filesystem::recursive_directory_iterator(sources_path)) {
			std::string const &source(source_entry.path().native());
			size_t const extension_position(source.rfind('.'));

			if(extension_position == std::string::npos || source[sources_path_length + 1] == '.') {
				continue;
			}

			this->_extensions.emplace(source, extension_position - sources_path_length);
			this->_objects.emplace(std::string(source, sources_path_length, extension_position - sources_path_length).append(objects_suffix),
				source);
		}
	}

	std::string const &
	name(void) const {
		return this->_name;
	}

	std::unordered_set<std::string> const &
	extensions(void) const {
		return this->_extensions;
	}

	std::unordered_map<std::string, std::string> const &
	objects(void) const {
		return this->_objects;
	}

	std::string
	macro(void) const {
		std::string macro(this->_name);

		std::transform(macro.begin(), macro.end(), macro.begin(),
			[] (char character) -> char {
				if(std::isalpha(character)) {
					return std::toupper(character);
				} else if(std::isdigit(character)) {
					return character;
				} else {
					return '_';
				}
			}
		);

		return macro + "FLAGS";
	}

private:
	std::string const _name;
	std::unordered_set<std::string> _extensions;
	std::unordered_map<std::string, std::string> _objects;
};

}

/* HVN_H */
#endif
