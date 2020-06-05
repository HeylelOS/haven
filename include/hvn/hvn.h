#ifndef HVN_H
#define HVN_H

#include <string>
#include <filesystem>
#include <unordered_set>

namespace hvn {

struct module final : std::unordered_set<std::string> {
	enum target {
		Executable,
		Library,
		Folder,
	};

	static inline module
	create_from_entry(std::filesystem::directory_entry const &entry, bool filled = true) {
		std::string const &name(entry.path().filename());
		target type(Executable);

		if(name.find("lib", 0) == 0) {
			type = Library;
		}

		if(filled) {
			return module::create_from_path(name, type, entry.path());
		} else {
			return module(name, type);
		}
	}

	static inline module
	create_from_path(std::string const &name, target type, std::filesystem::path const &path) {
		size_t const path_length(path.native().length());
		std::filesystem::recursive_directory_iterator path_iterator(path);

		hvn::module module(name, type);

		transform(std::filesystem::begin(path_iterator),
			std::filesystem::end(path_iterator),
			std::inserter(module, module.begin()),
			[=] (std::filesystem::directory_entry const &entry) {
				return entry.path().native().substr(path_length);
			}
		);

		return module;
	}

	inline
	module(std::string const &name = "a.out", target type = Executable)
		: _name(name), _type(type) {
	}

	template<class InputIterator> inline
	module(std::string const &name, target type, InputIterator start, InputIterator end)
		: std::unordered_set<std::string>(start, end), _name(name), _type(type) {
	}

	inline
	module(module const &module)
		: std::unordered_set<std::string>(module), _name(module._name), _type(module._type) {
	}

	inline
	~module(void) = default;

	inline void
	type(target type) {
		this->_type = type;
	}

	inline target
	type(void) const {
		return this->_type;
	}

	inline std::string const &
	name(void) const {
		return this->_name;
	}

	inline std::string
	macro(void) const {
		std::string macro(this->_name);

		std::transform(macro.begin(), macro.end(), macro.begin(),
			[] (char character) {
				return std::isalpha(character) ? std::toupper(character) : '_';
			}
		);

		return macro + "FLAGS";
	}

private:
	std::string const _name;
	target _type;
};

}

/* HVN_H */
#endif
