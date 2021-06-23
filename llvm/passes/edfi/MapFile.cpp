#include "MapFile.h"

#define MAPFILE_STRING		1 /* follwed by string */
#define MAPFILE_FUNCTION	2 /* followed by two string refs */
#define MAPFILE_BASIC_BLOCK	3
#define MAPFILE_INSTRUCTION	4
#define MAPFILE_DINSTRUCTION	5 /* followed by string ref and line num */
#define MAPFILE_FAULT_CANDIDATE	6 /* followed by string ref */
#define MAPFILE_FAULT_INJECTED	7 /* followed by string ref */
#define MAPFILE_MODULE_NAME	8 /* followed by string ref */

MapFile::MapFile(const std::string &path, const std::string &moduleName)
{
	stringRefNext = 0;
	file.open(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	writeModuleName(moduleName);
}

MapFile::~MapFile(void)
{
	file.close();
}

mapfile_stringref_t MapFile::getStringRef(const std::string &s)
{
	std::map<std::string, mapfile_stringref_t>::const_iterator entry = strings.find(s);
	if (entry != strings.end()) return entry->second;

	file.put(MAPFILE_STRING);
	file.write(s.c_str(), s.length() + 1);
	strings.insert(std::pair<std::string, mapfile_stringref_t>(s, stringRefNext));
	return stringRefNext++;
}

void MapFile::writeInt(unsigned long value)
{
	unsigned char b;

	for (;;) {
		b = value & 0x7f;
		value >>= 7;
		if (!value) break;
		file.put(b | 0x80);
	}
	file.put(b);
}

void MapFile::writeModuleName(const std::string &name)
{
	mapfile_stringref_t stringRef = getStringRef(name);
	file.put(MAPFILE_MODULE_NAME);
	writeStringRef(stringRef);
}

void MapFile::writeStringRef(mapfile_stringref_t stringRef)
{
	writeInt(stringRef);
}

void MapFile::writeBasicBlock(void)
{
	file.put(MAPFILE_BASIC_BLOCK);
}

void MapFile::writeDInstruction(const std::string &path, mapfile_lineno_t line)
{
	mapfile_stringref_t stringRef = getStringRef(path);
	file.put(MAPFILE_DINSTRUCTION);
	writeStringRef(stringRef);
	writeInt(line);
}

void MapFile::writeFaultCandidate(const std::string &name)
{
	mapfile_stringref_t stringRef = getStringRef(name);
	file.put(MAPFILE_FAULT_CANDIDATE);
	writeStringRef(stringRef);
}

void MapFile::writeFaultInjected(const std::string &name)
{
	mapfile_stringref_t stringRef = getStringRef(name);
	file.put(MAPFILE_FAULT_INJECTED);
	writeStringRef(stringRef);
}

void MapFile::writeFunction(const std::string &name, const std::string &relPath)
{
	mapfile_stringref_t stringRefName = getStringRef(name);
	mapfile_stringref_t stringRefRelPath = getStringRef(relPath);
	file.put(MAPFILE_FUNCTION);
	writeStringRef(stringRefName);
	writeStringRef(stringRefRelPath);
}

void MapFile::writeInstruction(void)
{
	file.put(MAPFILE_INSTRUCTION);
}
