#pragma once

#include <cstdio>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

class StringParser {
public:

	//将多个字符串以seperator为分隔符合并为一个
	static const string combineString(vector<string>& strings, char seperator) {
		if (strings.size() == 0)
			return "";
		string buffer = "";
		ostringstream out(buffer);
		out << strings[0];
		for (int i = 1; i < strings.size(); i++)
			out << seperator << strings[i];
		out.flush();
		return buffer;
	}

	//将按上述方法合并为一个字符串的多个字符串分开放到result里面
	static const void seperateString(string source, char seperator, vector<string>& result) {
		int get;
		string buffer;
		istringstream in(source);
		result.clear();
		while (true) {
			buffer.clear();
			while ((get = in.get())) {
				if (in.eof() || get == seperator)
					break;
				else
					buffer += char(get);
			}
			result.push_back(buffer);
			if (in.eof())
				break;
		}
	}

	//将各种整数/浮点数转换为字符串
	//或者考虑std::to_string()??? 
	static const string toString(bool                   data) { char buff[48]; sprintf(buff, "%d", data);   return string(buff); }
	static const string toString(short                  data) { char buff[48]; sprintf(buff, "%d", data);   return string(buff); }
	static const string toString(unsigned short         data) { char buff[48]; sprintf(buff, "%d", data);   return string(buff); }
	static const string toString(int                    data) { char buff[48]; sprintf(buff, "%d", data);   return string(buff); }
	static const string toString(unsigned int           data) { char buff[48]; sprintf(buff, "%u", data);   return string(buff); }
	static const string toString(long long              data) { char buff[48]; sprintf(buff, "%lld", data); return string(buff); }
	static const string toString(unsigned long long     data) { char buff[48]; sprintf(buff, "%llu", data); return string(buff); }
	static const string toString(float                  data) { char buff[48]; sprintf(buff, "%f", data);   return string(buff); }
	static const string toString(double                 data) { char buff[48]; sprintf(buff, "%lf", data);  return string(buff); }

	//将字符串转换为各种整数/浮点数
	static const bool      toBool(string&     data) { int x;       sscanf(data.c_str(), "%d", &x);   return x; }
	static const short     toShort(string&    data) { int x;       sscanf(data.c_str(), "%d", &x);   return x; }
	static const int       toInt(string&      data) { int x;       sscanf(data.c_str(), "%d", &x);   return x; }
	static const long long toLongLong(string& data) { long long x; sscanf(data.c_str(), "%lld", &x); return x; }
	static const float     toFloat(string&    data) { float x;     sscanf(data.c_str(), "%f", &x);   return x; }
	static const double    toDouble(string&   data) { double x;    sscanf(data.c_str(), "%lf", &x);  return x; }

};
