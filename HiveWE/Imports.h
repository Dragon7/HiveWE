#pragma once
#include <vector>

struct Import {
	bool custom;
	std::string path;
	fs::path file_path;
	
	Import(bool c, std::string p, fs::path f) : custom(c), path(p), file_path(f) {};
};

class Imports {
	int version = 1;
public:
	std::vector<Import> imports;
	std::map<std::string,std::vector<std::string>> dirEntries;

	void load(BinaryReader &reader);
	void save();

	void loadDirectoryFile(BinaryReader &reader);
	void saveDirectoryFile();

	void save_imports();

};
