#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <cstring>
#include <cstdio>
#include "StringParser.hpp"
using namespace std;

class Log {
public:

	const string logLevelName[4] = { "INFO","WARN"," ERR", "FATAL ERROR" };

	enum LogLevel {
		Info,
		Warning,
		Error,
		FatalError
	};

	Log() {}
	Log(ostream& output) :out({ &output }) {}
	Log(ostream* output) :out({ output }) {}

	void log(string content, LogLevel level = Info) {
		time_t curtime = time(NULL);
		char buffer[64];
		strftime(buffer, 63, "[%T", localtime(&curtime));
		string final = string(buffer) + " " + logLevelName[level] + "]: " + content + "\n";
		lock.lock();
		for (ostream* i : out) {
			(*i) << final;
			i->flush();
		}
		lock.unlock();
	}

	template<typename... Args>
	void logf(LogLevel level, string format, Args... args) {
		char buf[2560];
		sprintf_s(buf, 2560, format.c_str(), args...);
		log(string(buf), level);
	}

	void operator() (string content, LogLevel level = Info) {
		log(content, level);
	}

	void addOutputStream(ostream& output) { out.push_back(&output); }
	void addOutputStream(ostream* output) { out.push_back(output); }

private:
	vector<ostream*> out;
	recursive_mutex lock;
};

Log defLog;

class LogMessage {
public:

	LogMessage() :level(Log::Info) {}

	LogMessage& operator <<(bool                data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(char                data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(unsigned char       data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(short               data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(unsigned short      data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(int                 data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(unsigned int        data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(long long           data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(unsigned long long  data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(float               data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(double              data) { buffer += StringParser::toString(data); return *this; }
	LogMessage& operator <<(const char*         data) { buffer += string(data); return *this; }
	LogMessage& operator <<(const std::string&  data) { buffer += data; return *this; }

	LogMessage& operator <<(Log::LogLevel      level) { this->level = level;  return *this; }
	LogMessage& operator <<(Log&                 log) { flush(log);  return *this; }

public:

	void setLevel(Log::LogLevel level) { this->level = level; }
	void flush(Log& log) { logout(log); clear(); }
	void logout(Log& log) { log(buffer, level); }
	void clear() { buffer = ""; }

private:
	string buffer;
	Log::LogLevel level;
};

#define mlog LogMessage()
