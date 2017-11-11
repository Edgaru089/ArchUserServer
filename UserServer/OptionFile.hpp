#pragma once

#include <string>
#include <fstream>
#include <map>

using namespace std;

class OptionFile
{
public:

	bool loadFromFile(string filename)
	{
		string line, mark, cont;
		this->data.clear();
		ifstream file(filename);
		if (file.fail())
			return false;
		while (!file.eof()) {
			getline(file, line);
			if (mark[0] == '#')
				continue;
			size_t pos = line.find('=');
			if (pos == string::npos)
				continue;
			mark = line.substr(0, pos);
			cont = line.substr(pos + 1, line.length() - pos - 1);
			this->data[mark] = cont;
		}
		return true;
	}

	string getContent(string key) {
		map<string, string>::iterator i;
		if ((i = this->data.find(key)) != this->data.end()) {
			return i->second;
		}
		else
			return "";
	}

private:
	map<string, string> data;
};

