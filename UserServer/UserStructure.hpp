#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <random>
#include <mutex>
#include <set>
#include <map>

#include "Main.hpp"
#include "LogSystem.hpp"
#include "Uuid.hpp"

using namespace std;


class UserCredentialHandler {
public:

	const bool loadFromFile(const string& filename) {
		mlog << Log::Info << "[Credential] Loading, file: " << filename << dlog;
		AUTOLOCK(dataLock);
		ifstream in(filename);
		string username, passwordHashed;
		userData.clear();
		if (!in) {
			mlog << Log::Error << "[Credential] File loading Failed! Filename: " << filename << dlog;
			return false;
		}
		int count;
		in >> count;
		for (int i = 1; i <= count; i++) {
			in >> username >> passwordHashed;
			userData[username] = passwordHashed;
		}
		mlog << Log::Info << "[Credential] file loaded. " << count << " items in all." << dlog;
		return true;
	}

	const bool saveToFile(const string& filename) {
		mlog << Log::Info << "[Credential] Saving, file: " << filename << dlog;
		AUTOLOCK(dataLock);
		ofstream fout(filename);
		if (!fout) {
			mlog << Log::Error << "[Credential] File saving Failed! Filename: " << filename << dlog;
			return false;
		}
		fout << userData.size() << endl;
		for (pair<string, string> i : userData) {
			fout << i.first << " " << i.second << endl;
		}
		fout.flush();
		fout.close();
		if (fout) {
			mlog << Log::Info << "[Credential] File saved." << dlog;
			return true;
		}
		else {
			mlog << Log::Error << "[Credential] File saving Failed! Filename: " << filename << dlog;
			return false;
		}
	}

	const bool isUserVaild(const string& username, const string& passwordHashed) {
		mlog << Log::Info << "[Credential] User checked: {" << username << ", " << passwordHashed << "}" << dlog;
		map<string, string>::iterator iter = userData.find(username);
		if (iter == userData.end() || iter->second != passwordHashed)
			return false;
		else
			return true;
	}

	//const bool isUserVaild(const User& user) { return isUserVaild(user.username, user.passwordHashed); }

	const bool addUser(const string& username, const string& passwordHashed) {
		mlog << Log::Info << "[Credential] New user created: {" << username << ", " << passwordHashed << "}" << dlog;
		AUTOLOCK(dataLock);
		if (userData.find(username) == userData.end()) {
			userData[username] = passwordHashed;
			return true;
		}
		else
			return false;
	}

	const bool deleteUser(const string& username) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = userData.find(username);
		if (iter == userData.end())
			return false;
		userData.erase(iter);
		mlog << Log::Info << "[Credential] User deleted: {" << username << "}" << dlog;
		return true;
	}

	const bool changePassword(const string& username, const string& oldPasswordH, const string& newPasswordH) {
		AUTOLOCK(dataLock);
		map<string, string>::iterator iter = userData.find(username);
		if (iter == userData.end() || iter->second != oldPasswordH)
			return false;
		iter->second = newPasswordH;
		mlog << Log::Info << "[Credential] User password changed: {" << username << ", " << oldPasswordH << " -> " << newPasswordH << "}" << dlog;
		return true;
	}

	// To change the contents of the mapper, use functions provieded above
	const map<string, string>& getUserPasswordHashedMapper() { return userData; }

private:
	mutex dataLock;
	//<username, passwordHashed>
	map<string, string> userData;
};

class UserSessionHandler {
public:

	const bool loadFromFile(const string& filename) {
		mlog << Log::Info << "[Session] Loading, file: " << filename << dlog;
		AUTOLOCK(dataLock);
		ifstream in(filename);
		string username;
		Uuid session;
		sessionMapper.clear();
		int count;
		if (!in) {
			mlog << Log::Error << "[Session] File loading Failed! Filename: " << filename << dlog;
			return false;
		}
		in >> count;
		for (int i = 1; i <= count; i++) {
			in >> username;
			in >> session.sc1 >> session.sc2 >> session.sc3 >> session.sc4 >> session.sc5_high2 >> session.sc5_low4;
			sessionMapper[username] = session;
		}
		mlog << Log::Info << "[Session] File loaded. " << count << " items in all." << dlog;
		return true;
	}

	const bool saveToFile(const string& filename) {
		mlog << Log::Info << "[Session] Saving, file: " << filename << dlog;
		AUTOLOCK(dataLock);
		ofstream fout(filename);
		if (!fout) {
			mlog << Log::Error << "[Session] File saving Failed! Filename: " << filename << dlog;
			return false;
		}
		fout << sessionMapper.size() << endl;
		for (auto& i : sessionMapper) {
			fout << i.first << " ";
			fout << i.second.sc1 << " " << i.second.sc2 << " " << i.second.sc3 << " " << i.second.sc4 << " "
				<< i.second.sc5_high2 << " " << i.second.sc5_low4;
			fout << "\n";
		}
		fout.flush();
		if (fout) {
			mlog << Log::Info << "[Session] File saved." << dlog;
			return true;
		}
		else {
			mlog << Log::Error << "[Session] File saving Failed! Filename: " << filename << dlog;
			return false;
		}
	}

	Uuid getSession(const string& username) {
		AUTOLOCK(dataLock);
		auto iter = sessionMapper.find(username);
		if (iter != sessionMapper.end())
			return iter->second;
		else
			return sessionMapper[username] = Uuid::get();
	}

	Uuid getSessionNoGenerate(const string& username) {
		AUTOLOCK(dataLock);
		auto iter = sessionMapper.find(username);
		if (iter != sessionMapper.end())
			return iter->second;
		else
			return Uuid::nil();
	}

	const bool dismissSession(const string& username) {
		AUTOLOCK(dataLock);
		auto iter = sessionMapper.find(username);
		if (iter == sessionMapper.end())
			return false;
		else {
			sessionMapper.erase(iter);
			return true;
		}
	}

	// To change the contents of the mapper, use functions provieded above
	const map<string, Uuid>& getUserSessionMapper() { return sessionMapper; }

private:

	mutex dataLock;
	// <username, session>
	map<string, Uuid> sessionMapper;
};


class UserServiceHandler {
public:

	const bool loadFromFile(const string& filename) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] Loading, file: " << filename << dlog;
		return cred.loadFromFile(filename + ".credential") && sess.loadFromFile(filename + ".session");
	}

	const bool saveToFile(const string& filename) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] Saving, file: " << filename << dlog;
		return cred.saveToFile(filename + ".credential") && sess.saveToFile(filename + ".session");
	}

	//Bool: is login successful?
	//Uuid: user session if successful
	const pair<bool, Uuid> userLogin(const string& username, const string& passwordHashed) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] UserLogin attempt by {" << username << "}" << dlog;
		if (!cred.isUserVaild(username, passwordHashed))
			return make_pair(false, Uuid());
		else
			return make_pair(true, sess.getSession(username));
	}

	const bool userLogout(const string& username) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] UserLogout attempt by {" << username << "}" << dlog;
		return sess.dismissSession(username);
	}

	const bool checkUserSession(const string& username, const Uuid& session) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] CheckUserSession attempt by {" << username << ", {" << session.toString() << "}}" << dlog;
		return session == sess.getSessionNoGenerate(username);
	}

	const bool checkUserCredentials(const string& username, const string& passwordHashed) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] UserCheck attempt by {" << username << "}" << dlog;
		return cred.isUserVaild(username, passwordHashed);
	}

	const bool createUser(const string& username, const string& passwordHashed) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] User creating: {" << username << ", " << passwordHashed << "}" << dlog;
		return cred.addUser(username, passwordHashed);
	}

	const bool deleteUser(const string& username) {
		AUTOLOCK(dataLock);
		mlog << Log::Info << "[UserService] User deleting: {" << username << "}" << dlog;
		sess.dismissSession(username);
		return cred.deleteUser(username);
	}

	UserCredentialHandler& getCredenticalHandler() { return cred; }
	UserSessionHandler&    getSessionHandler() { return sess; }

private:
	mutex dataLock;
	UserCredentialHandler  cred;
	UserSessionHandler     sess;
};

