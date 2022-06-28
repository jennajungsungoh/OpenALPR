#pragma once
/*
*		Config Class
*/

#include <string>
#include <map>

class CConfigParser {
public:
	CConfigParser(const std::string& path);
	bool IsSuccess() { return m_table.size() != 0; }
	bool Contain(const std::string& name);
	bool GetBool(const std::string& name);
	std::string GetString(const std::string& name);
	float GetFloat(const std::string& name);
	int GetInt(const std::string& name);

private:
	std::map<std::string, std::string> m_table;
};
