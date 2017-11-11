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
	ServerNetworkHandler():commandLoop(&ServerNetworkHandler::processCommandLoop, this) {}

	const bool listen(int port, UserServiceHandler& service) {
		listening = true;
		TcpListener listener;

		mainSocket.disconnect();
		listener.setBlocking(false);
		listener.listen(port);

		while(listener.accept(mainSocket) != Socket::Done && listening) { sleep(milliseconds(50)); }

		if(listening) {
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

	void processCommandLoop() {
		Packet pack;
		while(connected&&mainSocket.receive(pack) != Socket::Disconnected) {
			string command;
			pack >> command;
			if(command == "HELLO") {
				Packet out;
				out << "HELLO";
				mainSocket.send(out);
			}
			else if(command == "LOGIN") {
				string username, passwordHashed;
				pair<bool, string> status;
				pack >> username >> passwordHashed;
				if((status = service->userLogin(User(username, passwordHashed))).first) {
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
			else if(command == "LOGOUT") {
				string username, passwordHashed;
				pack >> username >> passwordHashed;
				Packet pack;
				if(service->userLogout(User(username, passwordHashed)))
					pack << "LOGOUTSUCCESS";
				else
					pack << "LOGOUTFAILURE";
				mainSocket.send(pack);
			}
			else if(command == "CHECKSESSION") {
				string username, session;
				pack >> username, session;
				Packet out;
				if(service->checkUserSession(username, session))
					out << "SESSIONVAILD";
				else
					out << "SESSIONINVAILD";
				mainSocket.send(out);
			}
			else if(command == "CHECKUSER") {
				string username, passwordHashed;
				pack >> username, passwordHashed;
				Packet out;
				if(service->checkUserSession(username, passwordHashed))
					out << "USERVAILD";
				else
					out << "USERINVAILD";
				mainSocket.send(out);
			}
			else if(command == "CREATE") {
				string username, passwordHashed;
				pack >> username >> passwordHashed;
				Packet pack;
				if(service->createUser(username, passwordHashed))
					pack << "CREATESUCCESS";
				else
					pack << "CREATEFAILURE";
				mainSocket.send(pack);
			}
			else if(command == "DELETE") {
				string username, passwordHashed;
				pack >> username >> passwordHashed;
				Packet pack;
				if(service->deleteUser(User(username, passwordHashed)))
					pack << "DELETESUCCESS";
				else
					pack << "DELETEFAILURE";
				mainSocket.send(pack);
			}
		}
		connected = false;
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

	UserServer():listen(&UserServer::listenLoop, this), seekAndRemove(&UserServer::seekAndRemoveLoop, this) {}

	const bool loadConfig(string filename) {
		configFilename = filename;
		mlog << Log::Info << "[Server] Config Loading: " << filename << defLog;
		if(service.loadFromFile(filename) && option.loadFromFile(filename + ".option")) {
			processConfig();
			return true;
		}
		else {
			mlog << Log::Error << "[Server] Loading failed! Filename: " << filename << defLog;
			return false;
		}
	}

	void processConfig() {
		port = StringParser::toInt(option.getContent("port"));
	}

	const bool saveConfig(string filename) {
		if(service.saveToFile(filename))
			return true;
		else {
			mlog << Log::Error << "[Server] Saving failed! Filename: " << filename << defLog;
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
		while(isListening) {
			listeningHandler = new ServerNetworkHandler();
			if(!listeningHandler->listen(port, service)) {
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
		while(isRunning) {
			handlerLock.lock();
			for(list<ServerNetworkHandler*>::iterator i = handlers.begin(); i != handlers.end();) {
				if(!(*i)->isConnected()) {
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

		if(listeningHandler != NULL)
			listeningHandler->stopListening();

		mlog << Log::Info << "[Server] Stopping server..." << defLog;
		mlog << Log::Info << "[Server] Disconnecting clients..." << defLog;

		for(list<ServerNetworkHandler*>::iterator i = handlers.begin(); i != handlers.end();) {
			(*i)->disconnect();
			delete (*i);
			i = handlers.erase(i);
		}
		sleep(milliseconds(50));

		mlog << Log::Info << "[Server] Saving config file..." << defLog;
		service.saveToFile(configFilename);
		sleep(milliseconds(50));

		mlog << Log::Info << "[Server] Server stopped." << defLog;

		isRunning = false;
	}

	const unsigned short int getPort() { return port; }

private:
	string configFilename;
	UserServiceHandler service;
	OptionFile option;

	Thread listen, seekAndRemove;

	bool isRunning, isListening;
	Uint16 port;
	ServerNetworkHandler* listeningHandler;
	list<ServerNetworkHandler*> handlers;
	mutex handlerLock;
};

UserServer userServer;
