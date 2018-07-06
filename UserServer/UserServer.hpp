#pragma once

#include <thread>
#include <list>
#include <SFML/System.hpp>
#include <SFML/Network.hpp>

#include "Main.hpp"
#include "UserStructure.hpp"
#include "OptionFile.hpp"
#include "LogSystem.hpp"

using namespace std;
using namespace sf;

class ServerNetworkHandler {
public:
	ServerNetworkHandler() :commandLoop(&ServerNetworkHandler::processCommandLoop, this) {}

	const bool listen(int port, UserServiceHandler& service, string adminPassHashed) {
		this->adminPassHashed = adminPassHashed;
		isAdmin = false;

		listening = true;
		TcpListener listener;

		mainSocket.disconnect();
		listener.setBlocking(false);
		listener.listen(port);

		while (listener.accept(mainSocket) != Socket::Done && listening) { sleep(milliseconds(50)); }

		if (listening) {
			listening = false;
			connected = true;
			this->service = &service;
			commandLoop.launch();
			return true;
		}
		else {
			connected = false;
			return false;
		}
	}

#define BEGIN_IF_COMMAND(str) if(command == str)
#define IF_COMMAND(str) else if(command == str)
	void processCommandLoop() {
		Packet pack;
		while (connected&&mainSocket.receive(pack) != Socket::Disconnected) {
			string command;
			pack >> command;
			BEGIN_IF_COMMAND("HELLO") {
				Packet out;
				out << "HELLO";
				mainSocket.send(out);
			}
			IF_COMMAND("LOGIN") {
				string username, passwordHashed;
				pair<bool, string> status;
				pack >> username >> passwordHashed;
				if ((status = service->userLogin(username, passwordHashed)).first) {
					Packet out;
					out << "SESSIONPASS" << true << status.second;
					mainSocket.send(out);
				}
				else {
					Packet out;
					out << "SESSIONPASS" << false << string("");
					mainSocket.send(out);
				}
			}
			IF_COMMAND("LOGOUT") {
				string username, passwordHashed;
				pack >> username >> passwordHashed;
				Packet pack;
				if (service->checkUserCredentials(username, passwordHashed) && service->userLogout(username))
					pack << "LOGOUTSUCCESS";
				else
					pack << "LOGOUTFAILURE";
				mainSocket.send(pack);
			}
			IF_COMMAND("CHECKSESSION") {
				string username, session;
				pack >> username, session;
				Packet out;
				if (service->checkUserSession(username, session))
					out << "SESSIONVAILD";
				else
					out << "SESSIONINVAILD";
				mainSocket.send(out);
			}
			IF_COMMAND("CHECKUSER") {
				string username, passwordHashed;
				pack >> username, passwordHashed;
				Packet out;
				if (service->checkUserCredentials(username, passwordHashed))
					out << "USERVAILD";
				else
					out << "USERINVAILD";
				mainSocket.send(out);
			}
			IF_COMMAND("CREATE") {
				string username, passwordHashed;
				pack >> username >> passwordHashed;
				Packet pack;
				if (service->createUser(username, passwordHashed))
					pack << "CREATESUCCESS";
				else
					pack << "CREATEFAILURE";
				mainSocket.send(pack);
			}
			IF_COMMAND("DELETE") {
				string username, passwordHashed;
				pack >> username >> passwordHashed;
				Packet pack;
				if (service->checkUserCredentials(username, passwordHashed) && service->deleteUser(username))
					pack << "DELETESUCCESS";
				else
					pack << "DELETEFAILURE";
				mainSocket.send(pack);
			}

			/* ---------- Administrative Commands ---------- */
			IF_COMMAND("ADMINLOGIN") {
				string pass;
				Packet ret;
				pack >> pass;
				if (pass == adminPassHashed) {
					isAdmin = true;
					ret << "ADMINOK";
					mlog << "[NetworkHandler] Administrative user logged in from " <<
						mainSocket.getRemoteAddress().toString() << ":" << mainSocket.getRemotePort() << dlog;
				}
				else {
					ret << "ADMINFAILED";
					mlog << "[NetworkHandler] Failed Administrative login attempt from " <<
						mainSocket.getRemoteAddress().toString() << ":" << mainSocket.getRemotePort();
					mlog << "                 with hashed password" << pass << dlog;
				}
				mainSocket.send(ret);
			}
			IF_COMMAND("A_LISTUSERS") {
				Packet ret;
				if (isAdmin) {
					ret << "A_USERS";
					auto& m = service->getCredenticalHandler().getUserPasswordHashedMapper();
					ret << m.size();
					for (auto& i : m)
						ret << i.first << i.second << service->getSessionHandler().getSessionNoGenerate(i.first);
					//mlog << "[NetworkHandler] Users listed by administrative session" << dlog;
				}
				else
					ret << "ADMINFAILED";
				mainSocket.send(ret);
			}
			//IF_COMMAND("A_LISTSESSIONS") {
			//	Packet ret;
			//	if (isAdmin) {
			//		ret << "A_SESSIONS";
			//		auto& m = service->getSessionHandler().getUserSessionMapper();
			//		ret << m.size();
			//		for (auto& i : m)
			//			ret << i.first << i.second;
			//		mlog << "[NetworkHandler] User sessions listed by administrative session" << dlog;
			//	}
			//	else
			//		ret << "ADMINFAILED";
			//	mainSocket.send(ret);
			//}
			IF_COMMAND("A_DELETEUSER") {
				Packet ret;
				if (isAdmin) {
					string name;
					pack >> name;
					service->deleteUser(name);
					ret << "A_USERDELETED";
					mlog << "[NetworkHandler] User \"" << name << "\" forcibly deleted by administrative session" << dlog;
				}
				else
					ret << "ADMINFAILED";
				mainSocket.send(ret);
			}
			IF_COMMAND("A_ADDUSER") {
				Packet ret;
				if (isAdmin) {
					string name, newpass;
					pack >> name >> newpass;
					service->createUser(name, newpass);
					ret << "A_USERADDED";
					mlog << "[NetworkHandler] User \"" << name << "\" forcibly added by administrative session" << dlog;
				}
				else
					ret << "ADMINFAILED";
				mainSocket.send(ret);
			}
			IF_COMMAND("A_CHANGEUSER") {
				Packet ret;
				if (isAdmin) {
					string name, newpass;
					pack >> name >> newpass;
					service->deleteUser(name);
					service->createUser(name, newpass);
					ret << "A_USERCHANGED";
					mlog << "[NetworkHandler] User \"" << name << "\" forcibly changed by administrative session" << dlog;
				}
				else
					ret << "ADMINFAILED";
				mainSocket.send(ret);
			}
			IF_COMMAND("A_REMOVESESS") {
				Packet ret;
				if (isAdmin) {
					string name;
					pack >> name;
					service->userLogout(name);
					ret << "A_SESSREMOVED";
					mlog << "[NetworkHandler] User session of \"" << name << "\" forcibly removed by administrative session" << dlog;
				}
				else
					ret << "ADMINFAILED";
				mainSocket.send(ret);
			}
			IF_COMMAND("A_ADDSESS") {
				Packet ret;
				if (isAdmin) {
					string name;
					pack >> name;
					service->userLogout(name);
					ret << "A_SESSADDED" << service->getSessionHandler().getSession(name);
					mlog << "[NetworkHandler] User session of \"" << name << "\" forcibly added by administrative session" << dlog;
				}
				else
					ret << "ADMINFAILED";
				mainSocket.send(ret);
			}
		}
		connected = false;
		if (isAdmin) {
			mlog << "[NetworkHandler] Administrative user from " <<
				mainSocket.getRemoteAddress().toString() << ":" << mainSocket.getRemotePort() <<
				" passively disconnected." << dlog;
		}
	}

	void disconnect() {
		connected = false;
		mainSocket.disconnect();
	}

	void stopListening() {
		listening = false;
	}

	const bool isConnected() { return connected; }

private:
	Thread commandLoop;
	UserServiceHandler* service;
	TcpSocket mainSocket;
	bool connected, listening;

	string adminPassHashed;
	bool isAdmin;
};

/*
Server:
	LOGIN  username passwordHashed
	LOGOUT username passwordHashed
	CREATE username passwordHashed
	DELETE username passwordHashed
	CHECKSESSION username session
	CHECKUSER username passwordHashed
Client:
	SESSIONPASS bool(vaild) session
	LOGOUTSUCCESS
	LOGOUTFAILURE
	CERATESUCCESS
	CREATEFAILURE
	DELETESUCCESS
	DELETEFAILURE
	SESSIONVAILD
	SESSIONINVAILD
	USERVAILD
	USERINVAILD
*/

class UserServer {
public:

	UserServer() :listen(&UserServer::listenLoop, this), seekAndRemove(&UserServer::seekAndRemoveLoop, this) {}

	const bool loadConfig(string filename) {
		configFilename = filename;
		mlog << Log::Info << "[Server] Config Loading: " << filename << dlog;
		if (service.loadFromFile(filename) && option.loadFromFile(filename + ".option")) {
			processConfig();
			return true;
		}
		else {
			mlog << Log::Error << "[Server] Loading failed! Filename: " << filename << dlog;
			return false;
		}
	}

	void processConfig() {
		port = StringParser::toInt(option.getContent("port"));
		if (option.getContent("autosave-duration") != "0")
			autosaveTime = seconds(StringParser::toFloat(option.getContent("autosave-duration")));
		else
			autosaveTime = Time::Zero;
		adminPassHashed = option.getContent("admin-pass-hashed");
	}

	const bool saveConfig(string filename) {
		if (service.saveToFile(filename))
			return true;
		else {
			mlog << Log::Error << "[Server] Saving failed! Filename: " << filename << dlog;
			return false;
		}
	}

	void startListening() {
		isRunning = true;
		listen.launch();
		seekAndRemove.launch();
	}

	void listenLoop() {
		isListening = true;
		while (isListening) {
			listeningHandler = new ServerNetworkHandler();
			if (!listeningHandler->listen(port, service, adminPassHashed)) {
				delete listeningHandler;
				listeningHandler = NULL;
				continue;
			}
			handlerLock.lock();
			handlers.push_back(listeningHandler);
			handlerLock.unlock();
		}
	}

	void seekAndRemoveLoop() {
		while (isRunning) {
			handlerLock.lock();
			for (list<ServerNetworkHandler*>::iterator i = handlers.begin(); i != handlers.end();) {
				if (!(*i)->isConnected()) {
					delete (*i);
					i = handlers.erase(i);
				}
				else
					i++;
			}
			handlerLock.unlock();
			sleep(milliseconds(500));
		}
	}

	bool isServerRunning() { return isRunning; }

	void stop() {
		isListening = false;

		if (listeningHandler != NULL)
			listeningHandler->stopListening();

		mlog << Log::Info << "[Server] Stopping server..." << dlog;
		mlog << Log::Info << "[Server] Disconnecting clients..." << dlog;

		for (list<ServerNetworkHandler*>::iterator i = handlers.begin(); i != handlers.end();) {
			(*i)->disconnect();
			delete (*i);
			i = handlers.erase(i);
		}
		sleep(milliseconds(50));

		mlog << Log::Info << "[Server] Saving config file..." << dlog;
		service.saveToFile(configFilename);
		sleep(milliseconds(50));

		mlog << Log::Info << "[Server] Server stopped." << dlog;

		isRunning = false;
	}

	const unsigned short int getPort() { return port; }
	const Time getAutosaveTime() { return autosaveTime; }

private:
	string configFilename;
	UserServiceHandler service;
	OptionFile option;

	Thread listen, seekAndRemove;

	Time autosaveTime;
	string adminPassHashed;

	bool isRunning, isListening;
	Uint16 port;
	ServerNetworkHandler* listeningHandler;
	list<ServerNetworkHandler*> handlers;
	mutex handlerLock;
};

UserServer userServer;
