#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <set>
#include <map>

#include "Main.hpp"
#include "LogSystem.hpp"

using namespace std;

struct User {
	User() {}
	User(string userName, string passwordHashed):
		username(userName), passwordHashed(passwordHashed) {}

	const bool operator == (const User& right) const { return username == right.username&&passwordHashed == right.passwordHashed; }
	const bool operator != (const User& right) const { return !((*this) == right); }
	const bool operator < (const User& right) const { return username < right.username; }
	const bool operator > (const User& right) const { return username > right.username; }

	string username, passwordHashed;
};

class UserCredentialHandler {
public:

	const bool loadFromFile(const string& filename) {
		mlog << Log::Info << "[Credential] Loading, file: " << filename << defLog;
		AUTOLOCK(dataLock);
		ifstream in(filename);
		string username, passwordHashed;
		userData.clear();
		if(!in) {
			mlog << Log::Error << "[Credential] File loading Failed! Filename: " << filename << defLog;
			return false;
		}
		int count;
		in >> count;
		for(int i = 1; i <= count; i++) {
			in >> username >> passwordHashed;
			userData[username] = passwordHashed;
		}
		mlog << Log::Info << "[Credential] file loaded. " << count << " items in all." << defLog;
		return true;
	}

	const bool saveToFile(const string& filename) {
		mlog << Log::Info << "[Credential] Saving, file: " << filename << defLog;
		AUTOLOCK(dataLock);
		ofstream fout(filename);
		if(!fout) {
			mlog << Log::Error << "[Credential] File saving Failed! Filename: " << filename << defLog;
			return false;
		}
		fout << userData.size() << endl;
		for(pair<string, string> i : userData) {
			fout << i.first << " " << i.second << endl;
		}
		fout.flush();
		fout.close();
		if(fout) {
			mlog << Log::Info << "[Credential] File saved." << defLog;
			return true;
		}
		else {
			mlog << Log::Error << "[Credential] File saving Failed! Filename: " << filename << defLog;
			return false;
		}
	}

	const bool isUserVaild(const string& username, const string& passwordHashed) {
		mlog << Log::Info << "[Credential] User checked: {" << username << ", " << passwordHashed << "}" << defLog;
		map<string, string>::iterator iter = userData.find(username);
		if(iter == userData.end() || iter->second != passwordHashed)
			return false;
		else
			return true;
	}

	const bool isUserVaild(const User& user) { return isUserVaild(user.username, user.passwordHashed); }

	const bool addUser(const string& username, const string& passwordHashed) {
		mlog << Log::Info << "[Credential] New user created: {" << username << ", " << passwordHashed << "}" << defLog;
		AUTOLOCK(dataLock);
		if(userData.find(username) == userData.end()) {
			userData[username] = passwordHashed;
			return true;
		}
		else
			return false;
	}

	const bool deleteUser(const string& username, const string& passwordHashed) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = userData.find(username);
		if(iter == userData.end())
			return false;
		userData.erase(iter);
		mlog << Log::Info << "[Credential] User deleted: {" << username << "}" << defLog;
		return true;
	}

	const bool changePassword(const string& username, const string& oldPasswordH, const string& newPasswordH) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = userData.find(username);
		if(iter == userData.end() || iter->second != oldPasswordH)
			return false;
		iter->second = newPasswordH;
		mlog << Log::Info << "[Credential] User password changed: {" << username << ", " << oldPasswordH << " -> " << newPasswordH << "}" << defLog;
		return true;
	}

private:
	mutex dataLock;
	//<username, passwordHashed>
	map<string, string> userData;
};

class UserSessionHandler {
public:

	const bool loadFromFile(const string& filename) {
		mlog << Log::Info << "[Session] Loading, file: " << filename << defLog;
		AUTOLOCK(dataLock);
		ifstream in(filename);
		string username, session;
		sessionMapper.clear();
		sessionsInUse.clear();
		int count;
		if(!in) {
			mlog << Log::Error << "[Session] File loading Failed! Filename: " << filename << defLog;
			return false;
		}
		in >> count;
		for(int i = 1; i <= count; i++) {
			in >> username >> session;
			sessionMapper[username] = session;
			sessionsInUse.insert(session);
		}
		mlog << Log::Info << "[Session] File loaded. " << count << " items in all." << defLog;
		return true;
	}

	const bool saveToFile(const string& filename) {
		mlog << Log::Info << "[Session] Saving, file: " << filename << defLog;
		AUTOLOCK(dataLock);
		ofstream fout(filename);
		if(!fout) {
			mlog << Log::Error << "[Session] File saving Failed! Filename: " << filename << defLog;
			return false;
		}
		fout << sessionMapper.size() << endl;
		for(pair<string, string> i : sessionMapper) {
			fout << i.first << " " << i.second << endl;
		}
		fout.flush();
		if(fout) {
			mlog << Log::Info << "[Session] File saved." << defLog;
			return true;
		}
		else {
			mlog << Log::Error << "[Session] File saving Failed! Filename: " << filename << defLog;
			return false;
		}
	}

	const string getSession(const string& username) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = sessionMapper.find(username);
		if(iter != sessionMapper.end())
			return iter->second;
		else
			return sessionMapper[username] = _generateSession();
	}

	const string getSessionNoGenerate(const string& username) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = sessionMapper.find(username);
		if(iter != sessionMapper.end())
			return iter->second;
		else
			return ""s;
	}

	const bool dismissSession(const string& username) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = sessionMapper.find(username);
		if(iter == sessionMapper.end())
			return false;
		else {
			_releaseSession(iter->second);
			sessionMapper.erase(iter);
			return true;
		}
	}

private:

	const string _generateSession() {
		string str;
		while(true) {
			str = "";
			for(int i = 0; i < 12; i++)
				str += '0' + rand() % 10;
			if(sessionsInUse.find(str) == sessionsInUse.end())
				break;
		}
		sessionsInUse.insert(str);
		return str;
	}

	const bool _releaseSession(const string& session) {
		set<string>::iterator iter = sessionsInUse.find(session);
		if(iter == sessionsInUse.end())
			return false;
		else {
			sessionsInUse.erase(iter);
			return true;
		}
	}

	mutex dataLock;
	// <username, session>
	map<string, string> sessionMapper;
	set<string> sessionsInUse;
};


class UserServiceHandler {
public:

	const bool loadFromFile(const string& filename) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] Loading, file: " << filename << defLog;
		return cred.loadFromFile(filename + ".credential") && sess.loadFromFile(filename + ".session");
	}

	const bool saveToFile(const string& filename) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] Saving, file: " << filename << defLog;
		return cred.saveToFile(filename + ".credential") && sess.saveToFile(filename + ".session");
	}

	//Bool:   is login successful?
	//string: user session if successful
	const pair<bool, string> userLogin(const User& user) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] UserLogin attempt by {" << user.username << "}" << defLog;
		if(!cred.isUserVaild(user))
			return pair<bool, string>(false, "");
		else
			return pair<bool, string>(true, sess.getSession(user.username));
	}

	const bool userLogout(const User& user) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] UserLogout attempt by {" << user.username << "}" << defLog;
		return sess.dismissSession(user.username);
	}

	const bool checkUserSession(const string& username, const string& session) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] CheckUserSession attempt by {" << username << ", " << session << "}" << defLog;
		return session == sess.getSessionNoGenerate(username);
	}

	const bool checkUserCredentials(const User& user) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] UserCheck attempt by {" << user.username << "}" << defLog;
		return cred.isUserVaild(user);
	}

	const bool createUser(const string& username, const string& passwordHashed) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] User creating: {" << username << ", " << passwordHashed << "}" << defLog;
		return cred.addUser(username, passwordHashed);
	}

	const bool deleteUser(const User& user) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] User deleting: {" << user.username << "}" << defLog;
		sess.dismissSession(user.username);
		return cred.deleteUser(user.username, user.passwordHashed);
	}

private:
	mutex dataLock;
	UserCredentialHandler  cred;
	UserSessionHandler     sess;
};

