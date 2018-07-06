#include <iostream>
#include <cstdlib>
#include "UserServer.hpp"

using namespace std;

#ifdef SFML_SYSTEM_WINDOWS

//Platform-Depedent: Windows
#include "Windows.h"

BOOL systemExitEventHandler(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_C_EVENT)
		mlog << Log::Error << "[Main/EVENT] Control-C Console Exit" << dlog;
	else if (dwCtrlType == CTRL_BREAK_EVENT)
		mlog << Log::Error << "[Main/EVENT] Control-Break Console Exit" << dlog;
	else if (dwCtrlType == CTRL_CLOSE_EVENT)
		mlog << Log::Error << "[Main/EVENT] Control-Close Console Exit" << dlog;
	else if (dwCtrlType == CTRL_LOGOFF_EVENT)
		mlog << Log::Error << "[Main/EVENT] System-Logoff Exit" << dlog;
	else if (dwCtrlType == CTRL_SHUTDOWN_EVENT)
		mlog << Log::Error << "[Main/EVENT] System-Shutdown Exit" << dlog;
	else
		return false;

	userServer.stop();
	return true;
}

#endif // SFML_SYSTEM_WINDOWS

const string versionString = "1.2";
const string compileTime = string(__DATE__) + " " + string(__TIME__);

ofstream logOut("latest.log");

int main(int argc, char* argv[]) {

#ifdef SFML_SYSTEM_WINDOWS
	//Platform-Depedent: Windows
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)systemExitEventHandler, true);
#endif // SFML_SYSTEM_WINDOWS

	dlog.addOutputStream(clog);
	dlog.addOutputStream(logOut);

	cout << "Arch User Server Version " << versionString << ", Compile Time: " << compileTime << endl;
	cout << "    Copyright (c) 2018 Edgaru089" << endl << endl;

	mlog << Log::Info << "[Main] Server Loading..." << dlog;
	mlog << Log::Info << "[Main] UserServer Internal Release" << dlog;

	if (!userServer.loadConfig("server.config")) {
		mlog << Log::FatalError << "[Main] Config loading failed! Exiting!" << dlog;
		return EXIT_FAILURE;
	}

	mlog << Log::Info << "[Main] Server starting to listen on port " << userServer.getPort() << dlog;
	userServer.startListening();

	Clock autoSaveClock;
	while (userServer.isServerRunning()) {
		if (userServer.getAutosaveTime() != Time::Zero && autoSaveClock.getElapsedTime() > userServer.getAutosaveTime()) { // Auto-Save
			mlog << "[Main] Auto-Saving..." << dlog;
			userServer.saveConfig("server.config");
			autoSaveClock.restart();
		}
		sleep(milliseconds(100));
	}

	mlog << Log::Info << "[Main] Server Stopped. Bye!" << dlog;

	return EXIT_SUCCESS;
}
