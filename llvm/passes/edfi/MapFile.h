#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <stdint.h>

typedef unsigned mapfile_lineno_t;
typedef unsigned mapfile_stringref_t;

class MapFile {
	private:
		std::ofstream file;
		std::map<std::string, mapfile_stringref_t> strings;
		mapfile_stringref_t stringRefNext;
		mapfile_stringref_t getStringRef(const std::string &s);
		void writeInt(unsigned long value);
		void writeModuleName(const std::string &name);
		void writeStringRef(mapfile_stringref_t stringRef);
	public:
		MapFile(const std::string &path, const std::string &moduleName);
		~MapFile(void);
		void writeBasicBlock(void);
		void writeDInstruction(const std::string &path, mapfile_lineno_t line);
		void writeFaultCandidate(const std::string &name);
		void writeFaultInjected(const std::string &name);
		void writeFunction(const std::string &name, const std::string &relPath);
		void writeInstruction(void);
};

