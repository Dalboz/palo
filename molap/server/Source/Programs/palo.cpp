////////////////////////////////////////////////////////////////////////////////
/// @brief main program
///
/// @file
///
/// Copyright (C) 2006-2008 Jedox AG
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU General Public License (Version 2) as published
/// by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along with
/// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
/// Place, Suite 330, Boston, MA 02111-1307 USA
/// 
/// You may obtain a copy of the License at
///
/// <a href="http://www.jedox.com/license_palo.txt">
///   http://www.jedox.com/license_palo.txt
/// </a>
///
/// If you are developing and distributing open source applications under the
/// GPL License, then you are free to use Palo under the GPL License.  For OEMs,
/// ISVs, and VARs who distribute Palo with their products, and do not license
/// and distribute their source code under the GPL, Jedox provides a flexible
/// OEM Commercial License.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
/// @author Marek Pikulski, marek@pikulski.net
////////////////////////////////////////////////////////////////////////////////

#include "palo.h"

#include <iostream>
#include <fstream>

#include "Exceptions/FileOpenException.h"
#include "Exceptions/FileFormatException.h"

#include "InputOutput/FileDocumentation.h"
#include "InputOutput/HtmlFormatter.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/SignalTask.h"
#include "InputOutput/ProgressCallback.h"

#include "Options/Options.h"

#include "Olap/Database.h"
#include "Olap/PaloSession.h"
#include "Olap/Server.h"
#include "Olap/CachePage.h"
#include "Olap/RollbackStorage.h"

#include "HttpServer/PaloHttpHandler.h"

#if defined(ENABLE_HTTPS)
#include "HttpsServer/PaloHttpsServerCons.h"
#endif

#include "LegacyServer/LegacyServer.h"

#include "Programs/Palo1Converter.h"
#include "Programs/extension.h"

using namespace std;
using namespace palo;

////////////////////////////////////////////////////////////////////////////////
// --SECTION--                     COMMAND LINE ARGUMENTS OR INIT FILE VARIABLES
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief admin http address and port list (-a)
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTP)
static vector<string> AdminPorts;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief auto add databases found (-D)
////////////////////////////////////////////////////////////////////////////////

static bool AutoAddDB = true;

////////////////////////////////////////////////////////////////////////////////
/// @brief auto commit databases and cubes on shutdown (-B)
////////////////////////////////////////////////////////////////////////////////

static bool AutoCommit = true;

////////////////////////////////////////////////////////////////////////////////
/// @brief auto load databases and cubes on startup (-A)
////////////////////////////////////////////////////////////////////////////////

static bool AutoLoadDB = true;

////////////////////////////////////////////////////////////////////////////////
/// @brief palo barrier for caching element (-b)
////////////////////////////////////////////////////////////////////////////////

static double CacheBarrier = 5000.0;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of cache invalidations for clearing cache (-e)
////////////////////////////////////////////////////////////////////////////////

static uint32_t CacheClearBarrier = 5;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of changed cells for clearing cache (-g)
////////////////////////////////////////////////////////////////////////////////

static double CacheClearBarrierCells = 1000.0;

////////////////////////////////////////////////////////////////////////////////
/// @brief palo maximum cache size (-c)
////////////////////////////////////////////////////////////////////////////////

static size_t CacheSize = 100000000;

////////////////////////////////////////////////////////////////////////////////
/// @brief palo maximum undo memory size (-m)
////////////////////////////////////////////////////////////////////////////////

static size_t UndoMemorySize = 10 * 1024 * 1024;

////////////////////////////////////////////////////////////////////////////////
/// @brief palo maximum undo file size (-u)
////////////////////////////////////////////////////////////////////////////////

static size_t UndoFileSize = 50 * 1024 * 1024;

////////////////////////////////////////////////////////////////////////////////
/// @brief change into data directory (-C)
////////////////////////////////////////////////////////////////////////////////

static bool ChangeDirectory = true;

////////////////////////////////////////////////////////////////////////////////
/// @brief data directory (-d)
////////////////////////////////////////////////////////////////////////////////

static string DataDirectory = "./Data";

////////////////////////////////////////////////////////////////////////////////
/// @brief encryption type (-X)
////////////////////////////////////////////////////////////////////////////////

static Encryption_e EncryptionType = ENC_NONE;

////////////////////////////////////////////////////////////////////////////////
/// @brief goalseek cell limit (-J)
////////////////////////////////////////////////////////////////////////////////

static int GoalseekCellLimit = 1000;

////////////////////////////////////////////////////////////////////////////////
/// @brief goalseek timeout (-j)
////////////////////////////////////////////////////////////////////////////////

static int GoalseekTimeout = 10000;

////////////////////////////////////////////////////////////////////////////////
/// @brief extensions directory (-E)
////////////////////////////////////////////////////////////////////////////////

static string ExtensionsDirectory = "../Modules";

////////////////////////////////////////////////////////////////////////////////
/// @brief user friendly service name (-F)
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
static string FriendlyServiceName = "PALO 2.5 Server Service";
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief http address and port list (-h)
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTP)
static vector<string> HttpPorts;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief http port list (-H)
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTPS)
static vector<string> HttpsPorts; // address is identical to http
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief ignore cell data (-U)
////////////////////////////////////////////////////////////////////////////////

static bool IgnoreCellData = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief init file (-i)
////////////////////////////////////////////////////////////////////////////////

static string InitFile = "palo.ini";

////////////////////////////////////////////////////////////////////////////////
/// @brief password for private certificate (-p)
////////////////////////////////////////////////////////////////////////////////

static string KeyFilePassword = "";

////////////////////////////////////////////////////////////////////////////////
/// @brief key files (-K)
////////////////////////////////////////////////////////////////////////////////

static vector<string> KeyFiles;

////////////////////////////////////////////////////////////////////////////////
/// @brief splash limits (-K)
////////////////////////////////////////////////////////////////////////////////

static vector<double> SplashLimits;

////////////////////////////////////////////////////////////////////////////////
/// @brief legacy address and port list (-l)
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_LEGACY)
static vector<string> LegacyPorts;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief logfile (-o)
////////////////////////////////////////////////////////////////////////////////

static string LogFile = "-";

////////////////////////////////////////////////////////////////////////////////
/// @brief require user login and use user access rights (-R)
////////////////////////////////////////////////////////////////////////////////

static bool RequireUserLogin = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief palo maximum rule cache size (-r)
////////////////////////////////////////////////////////////////////////////////

static size_t RuleCacheSize = 100000000;

////////////////////////////////////////////////////////////////////////////////
/// @brief service name (-S)
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
static string ServiceName = "PALOServerService";
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief start as Windows service (-s)
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
static bool StartAsService = false;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief template directory (-t)
////////////////////////////////////////////////////////////////////////////////

static string TemplateDirectory = "../Api";

////////////////////////////////////////////////////////////////////////////////
/// @brief trace file, no trace if empty (-T)
////////////////////////////////////////////////////////////////////////////////

static string TraceFile = "";

////////////////////////////////////////////////////////////////////////////////
/// @brief start worker for cubes (-Y)
////////////////////////////////////////////////////////////////////////////////

static bool UseCubeWorkers = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief enable drillthrough (-y)
////////////////////////////////////////////////////////////////////////////////

static bool DrillThroughEnabled = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief use a fake session id for debugging (-f)
////////////////////////////////////////////////////////////////////////////////

static bool UseFakeSessionId = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief use init file (-n)
////////////////////////////////////////////////////////////////////////////////

static bool UseInitFile = true;

////////////////////////////////////////////////////////////////////////////////
/// @brief use worker login (-x)
////////////////////////////////////////////////////////////////////////////////

static WorkerLoginType LoginType = WORKER_NONE;

////////////////////////////////////////////////////////////////////////////////
/// @brief worker program (-w)
////////////////////////////////////////////////////////////////////////////////

static string WorkerProgram = "";

////////////////////////////////////////////////////////////////////////////////
/// @brief worker program arguments (-w)
////////////////////////////////////////////////////////////////////////////////

static vector<string> WorkerProgramArguments;

////////////////////////////////////////////////////////////////////////////////
// --SECTION--                                    STATIC CONFIGURATION VARIABLES
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief suffix of the original directory
////////////////////////////////////////////////////////////////////////////////

static const string BackupSuffix = ".backup";

////////////////////////////////////////////////////////////////////////////////
/// @brief import logfile
////////////////////////////////////////////////////////////////////////////////

static const string ImportLogFile = "import_logfile.log";

////////////////////////////////////////////////////////////////////////////////
// --SECTION--                                                  STATIC VARIABLES
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief all admin servers
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTP)
static vector<HttpServer*> AdminServers;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief all http servers
////////////////////////////////////////////////////////////////////////////////

static vector<ServerInfo_t*> HttpExtensions;

////////////////////////////////////////////////////////////////////////////////
/// @brief all http servers
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTP)
static vector<HttpServer*> HttpServers;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief all https servers
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTPS)
static vector<HttpServer*> HttpsServers;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief palo scheduler
////////////////////////////////////////////////////////////////////////////////

static Scheduler* PaloScheduler = Scheduler::getScheduler();

////////////////////////////////////////////////////////////////////////////////
/// @brief palo server
////////////////////////////////////////////////////////////////////////////////

static Server * PaloServer = 0;

////////////////////////////////////////////////////////////////////////////////
// --SECTION--                                                          HANDLERS
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief control c
////////////////////////////////////////////////////////////////////////////////

class ControlCHandler : public SignalTask {
public:
	ControlCHandler(Scheduler* scheduler) {
		this->scheduler = scheduler;
	}

#if defined(_MSC_VER)
	void handleSignal (signal_t signal) {
		if (StartAsService) {
			Logger::trace << "ignoring control-c in service mode" << endl;
		}
		else {
			Logger::error << "control-c pressed" << endl;
			PaloServer->beginShutdown(0);
		}
	}
#else
	void handleSignal (signal_t signal) {
		Logger::error << "control-c pressed" << endl;
		PaloServer->beginShutdown(0);
	}
#endif

private:
	Scheduler* scheduler;
};

////////////////////////////////////////////////////////////////////////////////
// --SECTION--                                          WINDOWS SERVICE ROUTINES
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

////////////////////////////////////////////////////////////////////////////////
/// @brief installs a windows service
////////////////////////////////////////////////////////////////////////////////

static void InstallService (const string& command) {
	Logger::info << "adding service '" << FriendlyServiceName << "' (internal '" << ServiceName << "')" << endl;

	SC_HANDLE schSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);

	if (schSCManager == 0) {
		Logger::error << "OpenSCManager failed with " << GetLastError() << endl;
		exit(1);
	}

	SC_HANDLE schService = CreateServiceA( 
		schSCManager,                // SCManager database
		ServiceName.c_str(),         // name of service
		FriendlyServiceName.c_str(), // service name to display
		SERVICE_ALL_ACCESS,          // desired access
		SERVICE_WIN32_OWN_PROCESS,   // service type
		SERVICE_AUTO_START,          // start type
		SERVICE_ERROR_NORMAL,        // error control type
		command.c_str(),             // path to service's binary
		NULL,                        // no load ordering group
		NULL,                        // no tag identifier
		NULL,                        // no dependencies
		NULL,                        // account (LocalSystem); TODO use LocalService
		NULL);                       // password

	CloseServiceHandle(schSCManager);

	if (schService == 0) {
		Logger::error << "CreateServiceA failed with " << GetLastError() << endl;
		exit(1);
	}

	Logger::info << "added service with command line '" << command << "'" << endl;

	CloseServiceHandle(schService); 
}

////////////////////////////////////////////////////////////////////////////////
/// @brief installs a windows service
////////////////////////////////////////////////////////////////////////////////

static void InstallServiceForData (const string& data) {
	CHAR path[MAX_PATH];

	if(! GetModuleFileNameA(NULL, path, MAX_PATH)) {
		Logger::error << "GetModuleFileNameA failed" << endl;
		exit(1);
	}

	// get working directory in wd
	CHAR wd[MAX_PATH];
	string working;

	// absolute path
	if ((3 <= data.size() && data.substr(1, 2) == ":\\") || (2 < data.size() && data.substr(0, 2) == "\\\\")) {
		working = "";
	}

	// relative
	else {
		if (getcwd(wd, MAX_PATH) == 0) {
			Logger::error << "getcwd failed" << endl;
			exit(1);
		}

		working += wd;
		working += "\\";
	}

	// build command
	string command;

	command += "\"";
	command += path;
	command += "\"";

	command += " --start-service --service-name \"" + ServiceName + "\" --data \"" + working + data + "\"";

	// register service
	InstallService(command);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief deletes a windows service
////////////////////////////////////////////////////////////////////////////////

static void DeleteService () { 
	Logger::info << "removing service '" << FriendlyServiceName << "' (internal '" << ServiceName << "')" << endl;

	SC_HANDLE schSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);

	if (schSCManager==0) {
		Logger::error << "OpenSCManager failed with " << windows_error(GetLastError()) << endl;
		exit(1);
	}

	SC_HANDLE schService = OpenServiceA(
		schSCManager,               // SCManager database
		ServiceName.c_str(),        // name of service
		DELETE);                    // only need DELETE access

	CloseServiceHandle(schSCManager);

	if (schService == 0) { 
		Logger::error << "CreateServiceA failed with " << windows_error(GetLastError()) << endl;
		exit(1);
	}

	if (!DeleteService(schService)) {
		Logger::error << "DeleteService failed with " << windows_error(GetLastError()) << endl;
		exit(1);
	}

	CloseServiceHandle(schService);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief service status handler
////////////////////////////////////////////////////////////////////////////////

static SERVICE_STATUS_HANDLE ServiceStatus;

////////////////////////////////////////////////////////////////////////////////
/// @brief set service status
////////////////////////////////////////////////////////////////////////////////

static void SetServiceStatus (DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwCheckPoint, DWORD dwWaitHint) {

	// disable control requests until the service is started
	SERVICE_STATUS ss; 

	if (dwCurrentState == SERVICE_START_PENDING || dwCurrentState == SERVICE_STOP_PENDING) {
		ss.dwControlsAccepted = 0;
	}
	else {
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	}

	// initialize ss structure
	ss.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	ss.dwServiceSpecificExitCode = 0;
	ss.dwCurrentState            = dwCurrentState;
	ss.dwWin32ExitCode           = dwWin32ExitCode;
	ss.dwCheckPoint              = dwCheckPoint;
	ss.dwWaitHint                = dwWaitHint;

	// Send status of the service to the Service Controller.
	if (!SetServiceStatus(ServiceStatus, &ss)) {

		DWORD result = GetLastError();
		Logger::error << "failed to set service status with " << windows_error(result) << endl;

		if (! PaloScheduler->shutdownInProgress()) {
			ss.dwCurrentState = SERVICE_STOP_PENDING;
			ss.dwControlsAccepted = 0;
			SetServiceStatus(ServiceStatus, &ss);

			PaloServer->beginShutdown(0);

			ss.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(ServiceStatus, &ss);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @brief service checkpoint
////////////////////////////////////////////////////////////////////////////////

static DWORD Checkpoint = 0;

////////////////////////////////////////////////////////////////////////////////
/// @brief service pending timeout in milli seconds
////////////////////////////////////////////////////////////////////////////////

static const DWORD PENDING_TIMEOUT = 10 * 60 * 1000;

////////////////////////////////////////////////////////////////////////////////
/// @brief CheckpointCallback
////////////////////////////////////////////////////////////////////////////////

class CheckpointCallback : public ProgressCallback {
public:
	CheckpointCallback (DWORD dwCtrlCode) : dwCtrlCode(dwCtrlCode) {
	}

public:
	void callback () {
		Checkpoint++;
		Logger::trace << "service checkpoint for " << dwCtrlCode << " count " << Checkpoint << endl;
		SetServiceStatus(dwCtrlCode, NO_ERROR, Checkpoint, PENDING_TIMEOUT);
	}

private:
	DWORD dwCtrlCode;
};


////////////////////////////////////////////////////////////////////////////////
/// @brief set service status
////////////////////////////////////////////////////////////////////////////////

static bool isRunning = false;

static void WINAPI ServiceCtrl (DWORD dwCtrlCode) {
	DWORD dwState = SERVICE_RUNNING;

	Logger::trace << "got service control request " << dwCtrlCode << endl;

	switch (dwCtrlCode) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		dwState = SERVICE_STOP_PENDING;
		break;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

	// set the status of the service.
	Logger::trace << "setting service status to " << dwState << endl;

	// stop service
	if (dwCtrlCode == SERVICE_CONTROL_STOP || dwCtrlCode == SERVICE_CONTROL_SHUTDOWN) {
		Logger::info << "begin shutdown of service" << endl;

		Checkpoint = 0;
		SetServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, Checkpoint, PENDING_TIMEOUT);

		PaloServer->beginShutdown(0);
	}
	else {
		SetServiceStatus(dwState, NO_ERROR, 0, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief main body of service
////////////////////////////////////////////////////////////////////////////////

static void ConstructPaloServer();
static void CommitChanges();

static void WINAPI ServiceMain (DWORD dwArgc, LPSTR *lpszArgv) {
	Logger::trace << "registering control handler" << endl;

	// register the service ctrl handler,  lpszArgv[0] contains service name
	ServiceStatus = RegisterServiceCtrlHandlerA(lpszArgv[0], (LPHANDLER_FUNCTION) ServiceCtrl);

	// set start pending
	Checkpoint = 0;
	SetServiceStatus(SERVICE_START_PENDING, 0, Checkpoint, PENDING_TIMEOUT);

	// create server
	CheckpointCallback callback(SERVICE_START_PENDING);
	PaloServer->registerProgressCallback(&callback, 30); //notify service control manager every 30s about progress made

	char * saveMe = new char[1000000];

	try {
		ConstructPaloServer();
		PaloServer->unregisterProgressCallback();

		// now start the main process
		Logger::info << "starting to listen" << endl;
		SetServiceStatus(SERVICE_RUNNING, 0, 0, 0);
		isRunning = true;
		PaloScheduler->run();

		CheckpointCallback callback(SERVICE_STOP_PENDING);
		PaloServer->registerProgressCallback(&callback, 30); //notify service control manager every 30s about progress

		// commit changes
		if (AutoCommit) {
			CommitChanges();
		}
	}
	catch (const ErrorException& e) {
		Logger::error << "got exception '" << e.getMessage() << "' (" << e.getDetails() << "), giving up!" << endl;
	}
	catch (std::bad_alloc) {
		Logger::error << "running out of memory, please reduce the number of loaded databases or cache sizes" << endl;
		palo::CloseExternalModules();

		delete[] saveMe;

		CheckpointCallback callback(SERVICE_STOP_PENDING);
		PaloServer->registerProgressCallback(&callback, 30); //notify service control manager every 30s about progress

		if (AutoCommit) {
			CommitChanges();
		}
	}
	catch (...) {
	}

	palo::CloseExternalModules();

	isRunning = false;

	// service has stopped
	PaloServer->unregisterProgressCallback();
	SetServiceStatus(SERVICE_STOPPED, NO_ERROR, 0, 0);
	while(true) usleep(100000);//wait for SCM to kill service
}


////////////////////////////////////////////////////////////////////////////////
/// @brief main body of service
////////////////////////////////////////////////////////////////////////////////

static void InitService () {
	SERVICE_TABLE_ENTRY ste[] = {
		{ TEXT(""), (LPSERVICE_MAIN_FUNCTION) ServiceMain }, 
		{ NULL, NULL }
	};

	Logger::trace << "starting service control dispatcher" << endl;

	if (! StartServiceCtrlDispatcher(ste)) {
		Logger::error << "StartServiceCtrlDispatcher has failed with " << windows_error(GetLastError()) << endl;
		exit(1);
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
// --SECTION--                                                  HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief set worker login type
////////////////////////////////////////////////////////////////////////////////

static void setWorkerLoginType (const string& type) {
	string t = StringUtils::tolower(type);

	if (t == "information") {
		LoginType = WORKER_INFORMATION;
	}
	else if (t == "authentication") {
		LoginType = WORKER_AUTHENTICATION;
	}
	else if (t == "authorization") {
		LoginType = WORKER_AUTHORIZATION;
	}
	else {
		Logger::error << "unknown worker login type '" << type << "'" << endl;
		throw "unknown worker type";
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief set encryption type
////////////////////////////////////////////////////////////////////////////////

static void setEncryptionType (const string& type) {
	string t = StringUtils::tolower(type);

	if (t == "none") {
		EncryptionType = ENC_NONE;
	}
	else if (t == "optional") {
		EncryptionType = ENC_OPTIONAL;
	}
	else if (t == "required") {
		EncryptionType = ENC_REQUIRED;
	}
	else {
		Logger::error << "unknown encryption type '" << type << "'" << endl;
		throw "unknown encryption type";
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief show usage AFTER parsing the arguments
////////////////////////////////////////////////////////////////////////////////

static bool ShowUsage = false;

////////////////////////////////////////////////////////////////////////////////
/// @brief prints usage
////////////////////////////////////////////////////////////////////////////////

static void usage (Options& opts) {
	if (ShowUsage) {
		string version = Server::getVersion();

		replace(version.begin(), version.end(), ';', '.');

		cout << "Palo Server Version " << version << " (" << Server::getRevision() << ")\n" << endl;
	}

	cout << opts.usage("") << "\n"
#if defined(ENABLE_HTTPS)
		<< "<encryption-type> can be 'none', 'optional', or 'required'\n"
#endif
		<< "<worker-login-type> can be 'information', 'authentication', or\n"
		<< "'authorization'. Setting <address> to \"\" binds the port to any\n"
		<< "address. By default palo will change into the data directory before\n"
		<< "opening the log file.\n" << endl;

	if (ShowUsage) {
		cout
			<< "data-directory:      " << DataDirectory << "\n"
			<< "chdir on startup:    " << (ChangeDirectory ? "true" : "false") << "\n"
			<< "init-file:           " << InitFile << "\n"
			<< "require user login:  " << (RequireUserLogin ? "true" : "false") << "\n"
			<< "use init-file:       " << (UseInitFile ? "true" : "false") << "\n"
			<< "auto-load databases: " << (AutoLoadDB ? "true" : "false") << "\n"
			<< "auto-add databases:  " << (AutoAddDB ? "true" : "false") << "\n"
			<< "auto-commit on exit: " << (AutoCommit ? "true" : "false") << "\n"
			<< "use cube workers:    " << (UseCubeWorkers ? "true" : "false") << "\n"
			<< "drillthrough enabled:" << (DrillThroughEnabled ? "true" : "false") << "\n"
			<< "\n"
			<< "The server side cache is limited to the <total_cache_size_in_bytes>.\n"
			<< "An aggregation is cached only if the corresponding sub-cube contains\n"
			<< " more than <number_of_base_elements_in_aggregation>. After the\n"
			<< "<number_of_cache_invalidations> or after the change of\n"
			<< "<number_of_cells> the cache will be cleared completly:\n"
			<< "\n"
			<< "total cache size in bytes:              " << CacheSize << " (" << int(CacheSize / 1024 / 1024) << "MB)\n"
			<< "total rule cache size in bytes:         " << RuleCacheSize << " (" << int(RuleCacheSize / 1024 / 1024) << "MB)\n"
			<< "number of base elements in aggregation: " << CacheBarrier << "\n"
			<< "number of cache invalidations:          " << CacheClearBarrier << "\n"
			<< "number of cells:                        " << CacheClearBarrierCells << "\n"
			<< "\n"

			<< "In a locked cube area it is possible to undo changes. Each lock can use\n"
			<< "<undo_memory_size_in_bytes_per_lock> bytes in memory and\n"
			<< "<undo_file_size_in_bytes_per_lock> bytes in files for storing changes:\n"
			<< "\n"
			<< "undo memory size in byte per lock: " << UndoMemorySize << " (" << int(UndoMemorySize / 1024 / 1024) << "MB)\n"
			<< "undo file size in byte per lock:   " << UndoFileSize << " (" << int(UndoFileSize / 1024 / 1024) << "MB)\n"
			<< "\n"
			<< "goalseek cell limit:   " << GoalseekCellLimit<<"\n"
			<< "goalseek timeout:   " << GoalseekTimeout<<"ms\n"
			<< "\n"

#if defined(_MSC_VER)
			<< "Or: " << opts.getProgramName() << " install_srv <data-directory>\n"
			<< "    " << opts.getProgramName() << " delete_srv\n\n"
			<< "to install or delete a Windows XP service. You can change the service\n"
			<< "name with --service-name. The default is '" << ServiceName << "'\n"
#endif
			<< endl;
	}

	exit(1);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief shows version
////////////////////////////////////////////////////////////////////////////////

static void showVersion () {
	string version = Server::getVersion();

	replace(version.begin(), version.end(), ';', '.');

	cout << "Palo Server Version " << version << " (" << Server::getRevision() << ")" << endl;
	exit(0);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parse options from command line or file
////////////////////////////////////////////////////////////////////////////////

static void parseOptions (Options& opts, OptionsIterator& iter) {
	opts.clear();

	int optchar;
	string optarg;
	bool seenWorker = false;

	while ((optchar = opts(iter, optarg)) != Options::OPTIONS_END_OPTIONS) {
		if (optchar == 'w') {
			if (! seenWorker) {
				WorkerProgram = "";
				WorkerProgramArguments.clear();
			}

			seenWorker = true;
		}
		else {
			seenWorker = false;
		}

		switch (optchar) {
	  case 'A':
		  AutoLoadDB = ! AutoLoadDB;
		  break;

#if defined(ENABLE_HTTP)
	  case 'a':
		  AdminPorts.push_back(optarg);
		  break;
#endif

	  case 'B':
		  AutoCommit = ! AutoCommit;
		  break;

	  case 'b':
		  CacheBarrier = StringUtils::stringToDouble(optarg);
		  break;

	  case 'C':
		  ChangeDirectory = ! ChangeDirectory;
		  break;

	  case 'c':
		  CacheSize = StringUtils::stringToInteger(optarg);
		  break;

	  case 'D':
		  AutoAddDB = ! AutoAddDB;
		  break;

	  case 'd':
		  DataDirectory = optarg;
		  break;

	  case 'e':
		  CacheClearBarrier = StringUtils::stringToInteger(optarg);
		  break;

	  case 'E':
		  ExtensionsDirectory = optarg;
		  break;

	  case 'f':
		  UseFakeSessionId = ! UseFakeSessionId;
		  break;

	  case 'g':
		  CacheClearBarrierCells = StringUtils::stringToDouble(optarg);
		  break;

#if defined(ENABLE_HTTPS)
	  case 'H':
		  HttpsPorts.push_back(optarg);
		  break;
#endif

#if defined(ENABLE_HTTP)
	  case 'h':
		  HttpPorts.push_back(optarg);
		  break;
#endif

	  case 'i':
		  InitFile = optarg;
		  break;

	  case 'K':
		  KeyFiles.push_back(optarg);
		  break;

	  case 'L':
		  SplashLimits.push_back(StringUtils::stringToDouble(optarg));
		  break;

#if defined(ENABLE_LEGACY)
	  case 'l':
		  LegacyPorts.push_back(optarg);
		  break;
#endif

	  case 'M':
		  PaloHttpHandler::defaultTtl = StringUtils::stringToInteger(optarg);
		  Logger::debug << "setting default session timeout to " << PaloHttpHandler::defaultTtl << endl;
		  break;

	  case 'm':
		  UndoMemorySize = StringUtils::stringToInteger(optarg);
		  break;

	  case 'n':
		  UseInitFile = ! UseInitFile;
		  break;

	  case 'o':
		  LogFile = optarg;
		  break;

	  case 'p':
		  KeyFilePassword = optarg;
		  break;

	  case 'r':
		  RuleCacheSize = StringUtils::stringToInteger(optarg);
		  break;

	  case 'R':
		  RequireUserLogin = ! RequireUserLogin;
		  break;

	  case 't':
		  TemplateDirectory = optarg;
		  break;

	  case 'T':
		  TraceFile = optarg;
		  break;

	  case 'u':
		  UndoFileSize = StringUtils::stringToInteger(optarg);
		  break;

	  case 'U':
		  IgnoreCellData = ! IgnoreCellData;
		  Cube::setIgnoreCellData(IgnoreCellData);
		  break;

	  case 'V':
		  showVersion();
		  break;

	  case 'v':
		  Logger::setLogLevel(optarg);
		  break;

	  case 'w':
		  if (WorkerProgram == "") {
			  WorkerProgram = optarg;
		  }
		  else {
			  WorkerProgramArguments.push_back(optarg);
		  }
		  break;

	  case 'X':
		  setEncryptionType(optarg);
		  break;

	  case 'x':
		  setWorkerLoginType(optarg);
		  break;

	  case 'Y':
		  UseCubeWorkers = ! UseCubeWorkers;
		  break;

	  case 'y':
		  DrillThroughEnabled = true;
		  break;

	  case 'z':
		  GoalseekTimeout = StringUtils::stringToInteger(optarg);
		  break;

	  case 'Z':
		  GoalseekCellLimit = StringUtils::stringToInteger(optarg);
		  break;

#if defined(_MSC_VER)
	  case 'S':
		  ServiceName = optarg;
		  break;

	  case 'F':
		  FriendlyServiceName = optarg;
		  break;

	  case 's':
		  StartAsService = ! StartAsService;
		  break;		
#endif

	  case '?':
		  ShowUsage = ! ShowUsage;
		  break;

	  case Options::OPTIONS_POSITIONAL:
	  case Options::OPTIONS_MISSING_VALUE:
	  case Options::OPTIONS_EXTRA_VALUE:
	  case Options::OPTIONS_AMBIGUOUS:
	  case Options::OPTIONS_BAD_KEYWORD:
	  case Options::OPTIONS_BAD_CHAR:
		  cout << "\n";
		  usage(opts);
		  break;

	  default:
		  cout << "Oops! Got '" << optchar << "'\n" << endl;
		  usage(opts);
		  break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parse command line options
////////////////////////////////////////////////////////////////////////////////

static void parseOptions (int argc, char * argv []) {

	// build options parser
	const char * optv [] = {
		"?|help",
		"A|auto-load",
		"a+admin                 <address> <port>",
		"B|auto-commit",
		"b:cache-barrier         <number_of_base_elements_in_aggregation>",
		"C|chdir",
		"c:cache-size            <total_cache_size_in_bytes>",
		"D|add-new-databases",
		"d:data-directory        <directory>",
		"e:clear-cache           <number_of_cache_invalidations>",
		"E:extensions            <directory>",
		"-f|fake-session-id",
		"g:clear-cache-cells     <number_of_cells>",
		"H+https                 <port>",
		"h+http                  <address> <port>",
		"i:init-file             <init-file>",
		"K+key-files             <ca> <private> <dh>",
		"L+splash-limit          <error> <warning> <info>",
#if defined(ENABLE_LEGACY)
		"l+legacy                <address> <port>",
#endif
		"-M:session-timeout      <seconds>", // for debugging
		"m:undo-memory-size      <undo_memory_size_in_bytes_per_lock>",
		"n|load-init-file",
		"o:log                   <logfile>",
		"p:password              <private-password>",
		"R|user-login",
		"r:rule-cache-size       <total_cache_size_in_bytes>",
		"t:template-directory    <directory>",
#ifdef ENABLE_TRACE_OPTION
		"-T:trace                <trace-file>",
#endif
		"u:undo-file-size        <undo_file_size_in_bytes_per_lock>",
		"-U|ignore-cell-data",
		"V|version",
		"v:verbose               <level>",
		"w+worker                <worker-executable> <argument1> <argument2> <argumentX>",    
		"X:encryption            <encryption-type>",
		"x:workerlogin           <worker-login-type>",
		"Y|use-cube-worker",
		"y|enable-drillthrough",
		"z:goalseek-timeout		 <miliseconds>",
		"Z:goalseek-limit		 <number_of_cells>",
#if defined(_MSC_VER)
		"F:friendly-service-name <service-name>",
		"S:service-name          <service-name>",
		"s|start-service",	
#endif
		0
	};

	Options opts(argv[0], optv);
	opts.setControls(OptionSpecification::NOGUESSING);

	// parse command line options
	OptionsArgvIterator iter(--argc, ++argv);

	parseOptions(opts, iter);

#if defined(_MSC_VER)
	if (iter.size() == 2) {
		const string command = *(iter());
		const string data    = *(iter());

		if (command == "install_srv") {
			InstallServiceForData(data);
		}
		else {
			usage(opts);
		}

		exit(0);
	}
	else if (iter.size() == 1) {
		const string command = *(iter());

		if (command == "delete_srv") {
			DeleteService();
		}
		else {
			usage(opts);
		}

		exit(0);
	}
	else if (iter.size() != 0) {
		cout << "unknown argument " << *iter.getCurrent() << "\n" << endl;
		usage(opts);
	}
#else
	if (iter.size() != 0) {
		cout << "unknown argument " << *iter.getCurrent() << "\n" << endl;
		usage(opts);
	}
#endif

	// usage request on command line
	if (ShowUsage) {
		usage(opts); // will call exit!
	}

	// change into data directory
	if (ChangeDirectory) {
		if (chdir(DataDirectory.c_str()) == -1) {
			Logger::error << "cannot change into directory '" << DataDirectory << "'" << endl;
			throw "missing directory";
		}

		Logger::info << "chdir into '" << DataDirectory << "'" << endl;

		DataDirectory = ".";
	}

	// parse init file options
	if (UseInitFile) {
		OptionsFileIterator fiter(InitFile);

		parseOptions(opts, fiter);
	}

	// usage request in init file (very strange!)
	if (ShowUsage) {
		usage(opts); // will call exit!
	}

	// set logfile
	Logger::setLogFile(LogFile);

	// check key files
	if (! KeyFiles.empty()) {
		if (KeyFiles.size() % 3 != 0) {
			cout << "expecting three key files, not " << KeyFiles.size() << "\n" << endl;
			usage(opts);
		}

		if (KeyFiles.size() > 3) {
			KeyFiles.erase(KeyFiles.begin(), KeyFiles.end() - 3);
		}
	}

	// splash limits
	if (! SplashLimits.empty()) {
		if (SplashLimits.size() % 3 != 0) {
			cout << "expecting three splash limits, not " << SplashLimits.size() << "\n" << endl;
			usage(opts);
		}

		if (SplashLimits.size() > 3) {
			SplashLimits.erase(SplashLimits.begin(), SplashLimits.end() - 3);
		}

		Cube::splashLimit1 = SplashLimits[0];
		Cube::splashLimit2 = SplashLimits[1];
		Cube::splashLimit3 = SplashLimits[2];
	}

	Logger::debug << "using splash limits " 
		<< Cube::splashLimit1 << ", "
		<< Cube::splashLimit2 << ", "
		<< Cube::splashLimit3 << endl;

	// check legacy host/port pairs
#if defined(ENABLE_LEGACY)
	if (LegacyPorts.size() % 2 == 1) {
		cout << "not enough arguments to --legacy\n" << endl;
		usage(opts);
	}
#endif

	// check http host/port pairs
#if defined(ENABLE_HTTP)
	if (HttpPorts.size() % 2 == 1) {
		cout << "not enough arguments to --http\n" << endl;
		usage(opts);
	}
#endif

	// check https ports
#if defined(ENABLE_HTTPS)
	if (! HttpsPorts.empty() && HttpsPorts.size() != HttpPorts.size() / 2) {
		cout << "got " << HttpsPorts.size() << " https ports, but " << (HttpPorts.size() / 2)
			<< " http addresses\n" << endl;
		usage(opts);
	}

	if (EncryptionType == ENC_REQUIRED) {
		if (! HttpPorts.empty() && HttpsPorts.empty()) {
			cout << "encryption type 'REQUIRED', but no https ports given\n" << endl;
			usage(opts);
		}
	}
	else if (EncryptionType == ENC_NONE && ! HttpsPorts.empty()) {
		cout << "encryption type 'NONE', but https ports given\n" << endl;
		usage(opts);
	}
#endif  

	// check admin host/port pairs
#if defined(ENABLE_HTTP)
	if (AdminPorts.size() % 2 == 1) {
		cout << "not enough arguments to --admin\n" << endl;
		usage(opts);
	}
#endif

	// check legacy host/port pairs
#if defined(ENABLE_LEGACY)
#if defined(ENABLE_HTTP)
	if (LegacyPorts.empty() && HttpPorts.empty() && AdminPorts.empty()) {
		cout << "need at least one --legacy or --http or --admin option\n" << endl;
		usage(opts);
	}
#else
	if (LegacyPorts.empty()) {
		cout << "need at least one --legacy option\n" << endl;
		usage(opts);
	}
#endif
#else
#if defined(ENABLE_HTTP)
	if (HttpPorts.empty() && AdminPorts.empty()) {
		cout << "need at least one --http or --admin option\n" << endl;
		usage(opts);
	}
#else
	Logger::error << "neither legacy nor http interface has been enabled in configuration, good luck" << endl;
#endif
#endif

	// set maximum cache size for consolidated elements
	Cube::setMaximumCacheSize(CacheSize);
	Cube::setMaximumRuleCacheSize(RuleCacheSize);
	Cube::setCacheBarrier(CacheBarrier);
	Cube::setCacheClearBarrier(CacheClearBarrier);
	Cube::setCacheClearBarrierCells(CacheClearBarrierCells);
	Cube::setGoalseekCellLimit(GoalseekCellLimit);
	Cube::setGoalseekTimeout(GoalseekTimeout);

	if (UndoMemorySize < 1024*1024) {
		UndoMemorySize = 1024*1024;
		Logger::error << "undo memory size too small, using a size of 1MB" << endl;
	}
	RollbackStorage::setMaximumMemoryRollbackSize(UndoMemorySize);
	RollbackStorage::setMaximumFileRollbackSize(UndoFileSize);

	// start worker
	if (! WorkerProgram.empty()) { 
		string arg;
		for (vector<string>::iterator i = WorkerProgramArguments.begin(); i != WorkerProgramArguments.end(); i++) {
			arg += " \'" + *i + "'";
		} 
		Logger::info << "worker executable: '" << WorkerProgram << "'" << endl;
		Logger::info << "worker arguments: " << arg << endl;
	}

	// cube worker requires worker paramater
	else if (UseCubeWorkers) {
		Logger::error << "use-cube-worker: missing worker executable" << endl;
		throw "missing worker";
	}

	Worker::setExecutable(WorkerProgram);
	Worker::setArguments(WorkerProgramArguments);
	CubeWorker::setUseCubeWorker(UseCubeWorkers);

}

////////////////////////////////////////////////////////////////////////////////
/// @brief autoload new databases from path
////////////////////////////////////////////////////////////////////////////////

static void AddNewDatabases () {
	vector<string> files = FileUtils::listFiles(DataDirectory);

	for (vector<string>::iterator i = files.begin();  i != files.end();  i++) {
		string dbName = *i;
		string file = DataDirectory + "/" + dbName;

		if (StringUtils::isSuffix(file, BackupSuffix)) {
			continue;
		}

		if (Palo1Converter::isPalo1Directory(file)) {
			Palo1Converter converter(PaloServer, DataDirectory, dbName);

			Logger::info << "converting directory " << dbName << endl;

			if (! converter.convertDatabase(ImportLogFile)) {
				Logger::error << "cannot convert, aborting!" << endl;
				throw "cannot convert database";
			}
		}
		else if (FileUtils::isReadable(FileName(file, "database", "csv"))) {
			if (dbName.find_first_not_of(Server::VALID_DATABASE_CHARACTERS) != string::npos) {
				Logger::warning << "directory '" << dbName << "' of database contains an illegal character, skipping" << endl;
			}
			else if (dbName[0] == '.') {
				Logger::warning << "directory '" << dbName << "' of database begins with a dot character, skipping" << endl;
			}
			else {
				if (PaloServer->lookupDatabaseByName(dbName) == 0) {
					Logger::info << "added new directory as database '" << dbName << "'" << endl;

					try {
						PaloServer->addDatabase(dbName, 0);
					}
					catch (const ErrorException& e) {
						Logger::warning << "cannot load database '" << dbName << "'" << endl;
						Logger::warning << "message 1: " << e.getMessage() << endl;
						Logger::warning << "message 2: " << e.getDetails() << endl;
						throw "cannot load database";
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief autoload new databases from path
////////////////////////////////////////////////////////////////////////////////

static void AutoLoadDatabases () {
	Database* systemDB = PaloServer->getSystemDatabase();
	vector<Database*> databases = PaloServer->getDatabases(0);

	for (vector<Database*>::iterator i = databases.begin();  i != databases.end();  i++) {
		Database * database = *i;

		if (database == systemDB) {
			continue;
		}

		try {
			Logger::info << "auto loading database '" << database->getName() << "'" << endl;
			PaloServer->loadDatabase(database, 0);

			vector<Cube*> cubes = database->getCubes(0);

			for (vector<Cube*>::iterator j = cubes.begin();  j != cubes.end();  j++) {
				Cube * cube = *j;

				database->loadCube(cube, 0);
			}
		}
		catch (std::bad_alloc) {
			Logger::error << "running out of memory, please reduce the number of loaded databases" << endl;
			throw "out of memory";
		}
		catch (const ErrorException& e) {
			Logger::warning << "cannot load database '" << database->getName() << "'" << endl;
			Logger::warning << "message 1: " << e.getMessage() << endl;
			Logger::warning << "message 2: " << e.getDetails() << endl;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief construct http server
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTP)
#if defined(ENABLE_HTTP_MODULE)
static InitHttpInterface_fptr ExternalHttpInterface = 0;
#endif

static void ConstructHttpServers (Scheduler * scheduler, Server * server, bool admin) {
	static int32_t TraceFileCounter = 0;
	set< pair<string,int> > seen;

	// create https port list
	vector<int> httpsPorts;

#if defined(ENABLE_HTTPS)
	if (HttpsPorts.empty()) {
		httpsPorts.resize((admin ? AdminPorts.size() : HttpPorts.size()) / 2, 0);
	}
	else {
		for (vector<string>::iterator i = HttpsPorts.begin();  i != HttpsPorts.end();  i++) {
			httpsPorts.push_back(StringUtils::stringToInteger(*i));
		}
	}
#else
	httpsPorts.resize((admin ? AdminPorts.size() : HttpPorts.size()) / 2, 0);
#endif

	// create a server for each port
	vector<string>::iterator iter = (admin ? AdminPorts.begin() : HttpPorts.begin());
	vector<string>::iterator end  = (admin ? AdminPorts.end()   : HttpPorts.end());
	vector<int>::iterator ports   = httpsPorts.begin();

	for (; iter != end; ) {

		// get listen port and address
		string address = *iter++;
		string portString = *iter++;
		int port = StringUtils::stringToInteger(portString.c_str());
		int httpsPort = *ports++;

		if (address == "-") {
			address.clear();
		}

		pair<string,int> ap(address, port);

		if (seen.find(ap) != seen.end()) {
			Logger::warning << "address/port " << address << "/" << port << " already defined, ignoring duplicate entry" << endl;
			continue;
		}

		seen.insert(ap);

		// construct a new http server
		HttpServer * httpServer = 0;

#if defined(ENABLE_HTTP_MODULE)
		if (ExternalHttpInterface == 0) {
			Logger::error << "http interface not loaded, aborting!" << endl;
			throw "http interface missing";
		}
		else {
			httpServer = (*ExternalHttpInterface)(scheduler,
				server,
				TemplateDirectory,
				address, 
				port,
				admin, 
				RequireUserLogin,
				EncryptionType,
				httpsPort);
		}
#else
		httpServer = ConstructPaloHttpServer(scheduler,
			server,
			TemplateDirectory,
			address,
			port,
			admin,
			RequireUserLogin,
			EncryptionType,
			httpsPort);
#endif

		if (httpServer == 0) {
			throw "cannot construct http server";
		}

		// set trace file
		if (! TraceFile.empty()) {
			httpServer->setTraceFile(TraceFile + "-" + StringUtils::convertToString(TraceFileCounter++));
		}

		// keep a list of servers
		if (admin) {
			AdminServers.push_back(httpServer);
		}
		else {
			HttpServers.push_back(httpServer);
		}

		// add possible extensions
		for (vector<ServerInfo_t*>::iterator iter = HttpExtensions.begin();  iter != HttpExtensions.end();  iter++) {
			ServerInfo_t * info = *iter;

			(*(info->httpExtensions))(PaloScheduler, PaloServer, httpServer, admin, false);
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief construct https server
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_HTTPS)
#if defined(ENABLE_HTTPS_MODULE)
static InitHttpsInterface_fptr ExternalHttpsInterface = 0;
#endif

static int32_t TraceFileCounter = 0;

static void ConstructHttpsServers (Scheduler * scheduler, Server * server) {
	if (HttpsPorts.empty()) {
		return;
	}

	set< pair<string,int> > seen;

	// create a server for each port
	vector<string>::iterator iter = HttpPorts.begin();
	vector<string>::iterator end  = HttpPorts.end();
	vector<string>::iterator portIter = HttpsPorts.begin();

	for (; iter != end; ) {

		// get listen port and address
		string address = *iter++;
		*iter++; // ignore HTTP port

		string portString = *portIter++;
		int port = StringUtils::stringToInteger(portString.c_str());

		if (address == "-") {
			address.clear();
		}

		pair<string,int> ap(address, port);

		if (seen.find(ap) != seen.end()) {
			Logger::warning << "address/port " << address << "/" << port << " already defined, ignoring duplicate entry" << endl;
			continue;
		}

		seen.insert(ap);

		// construct a new http server
		HttpServer * httpsServer = 0;

#if defined(ENABLE_HTTPS_MODULE)
		if (ExternalHttpsInterface == 0) {
			Logger::error << "https interface not loaded, aborting!" << endl;
			throw "https interface missing";
		}
		else {
			if (KeyFiles.size() != 3) {
				Logger::error << "ssl keys are not defined, aborting!" << endl;
				throw "no ssl keys";
			}

			httpsServer = (*ExternalHttpsInterface)(scheduler, server,
				KeyFiles[0], KeyFiles[1], KeyFilePassword, KeyFiles[2],
				TemplateDirectory, address, port, EncryptionType);
		}
#else
		httpsServer = ConstructPaloHttpsServer(scheduler, server,
			KeyFiles[0], KeyFiles[1], KeyFilePassword, KeyFiles[2],
			TemplateDirectory, address, port, EncryptionType);
#endif

		if (httpsServer == 0) {
			throw "cannot construct https server";
		}

		// set trace file
		if (! TraceFile.empty()) {
			httpsServer->setTraceFile(TraceFile + "-" + StringUtils::convertToString(TraceFileCounter++));
		}

		// keep a list of servers
		HttpsServers.push_back(httpsServer);

		// add possible extensions
		for (vector<ServerInfo_t*>::iterator iter = HttpExtensions.begin();  iter != HttpExtensions.end();  iter++) {
			ServerInfo_t * info = *iter;

			(*(info->httpExtensions))(PaloScheduler, PaloServer, httpsServer, false, true);
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief construct legacy server
////////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_LEGACY)
#if defined(ENABLE_LEGACY_MODULE)
static InitLegacyInterface_fptr ExternalLegacyInterface = 0;
#endif

static void ConstructLegacyServers (Scheduler * scheduler, Server * server) {
	set< pair<string,int> > seen;

	// create a server for each port
	for (size_t i = 0;  i < LegacyPorts.size();  i += 2) {

		// get listen port and address
		string address = LegacyPorts[i];
		int port = StringUtils::stringToInteger(LegacyPorts[i + 1].c_str());

		if (address == "-") {
			address.clear();
		}

		pair<string,int> ap(address, port);

		if (seen.find(ap) != seen.end()) {
			Logger::warning << "address/port " << address << "/" << port << " already defined, ignoring duplicate entry" << endl;
			continue;
		}

		seen.insert(ap);

		// construct a new legacy server
		LegacyServer * legacy = 0;

#if defined(ENABLE_LEGACY_MODULE)
		if (ExternalLegacyInterface == 0) {
			Logger::error << "legacy interface not loaded, aborting!" << endl;
			throw "legacy interface missing";
		}
		else {
			legacy = (*ExternalLegacyInterface)(scheduler, server, address, port);
		}
#else
		legacy = ConstructPaloLegacyServer(scheduler, server, address, port);
#endif

		if (legacy == 0) {
			throw "cannot construct legacy server";
		}
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief construct palo server
////////////////////////////////////////////////////////////////////////////////

static void ConstructPaloServer () {
	bool loaded = false;

	// load palo server from disk
	try {
		PaloServer->loadServer(0);
		loaded = true;
	}
	catch (const FileFormatException& e) {
		Logger::error << "file format error: " << e.getMessage() << endl;
		Logger::error << "server file is corrupted, giving up!" << endl;
		throw "cannot load server";
	}
	catch (const FileOpenException& e) {
		loaded = false;
		Logger::warning << "failed to load server '" << e.getMessage() << "' (" << e.getDetails() << ")" << endl;
	}
	catch (const ErrorException& e) {
		Logger::error << "cannot load server, giving up! '" << e.getMessage() << "' (" << e.getDetails() << ")" << endl;
		throw "cannot load server";
	}

	// create a new server
	if (! loaded) {
		try {
			PaloServer->saveServer(0);
		}
		catch (const FileFormatException& e) {
			Logger::error << "file format error: " << e.getMessage() << endl;
			Logger::error << "detail: " << e.getDetails() << endl;
			Logger::error << "server file is corrupted, giving up!" << endl;
			throw "cannot load server";
		}
		catch (const FileOpenException&) {
			Logger::error << "cannot save server file, giving up!" << endl;
			throw "cannot load server";
		}
		catch (const ErrorException& e) {
			Logger::error << "cannot save server file, giving up! '" << e.getMessage() << "' (" << e.getDetails() << ")" << endl;
			throw "cannot load server";
		}
	}

	// load or generate system databases
	try {
		PaloServer->addSystemDatabase();
	}
	catch (const FileFormatException& e) {
		Logger::error << "file format error: " << e.getMessage() << endl;
		Logger::error << "detail: " << e.getDetails() << endl;
		Logger::error << "system database file is corrupted, giving up!" << endl;
		throw "cannot load server";
	}
	catch (const FileOpenException&) {
		Logger::error << "cannot load system database file, giving up!" << endl;
		throw "cannot load server";
	}
	catch (const ErrorException& e) {
		Logger::error << "cannot load system database file, giving up! '" << e.getMessage() << "' (" << e.getDetails() << ")" << endl;
		throw "cannot load server";
	}

	// add any new database found in the path
	if (AutoLoadDB && AutoAddDB) {
		AddNewDatabases();
	}

	// login worker
	if (! WorkerProgram.empty()) {
		PaloSession* session = PaloSession::createSession(PaloServer, 0, true, 0);  
		LoginWorker* worker = new LoginWorker(PaloScheduler, session->getEncodedIdentifier(), LoginType, DrillThroughEnabled);
		PaloScheduler->registerWorker(worker);

		PaloServer->setLoginWorker(worker);
	} 

	// autoload other databases
	if (AutoLoadDB) {
		AutoLoadDatabases();
	}

	// rebuild markers
	PaloServer->rebuildAllMarkers();

	// create a fake session id
	if (UseFakeSessionId) {
		PaloSession::createSession(0, PaloServer, 0, false, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief commit changes
////////////////////////////////////////////////////////////////////////////////

static void CommitChanges () {
	try {
		PaloServer->saveServer(0);

		vector<Database*> databases = PaloServer->getDatabases(0);

		for (vector<Database*>::iterator i = databases.begin();  i != databases.end();  i++) {
			Database * database = *i;

			if (database->getStatus() == Database::CHANGED) {
				Logger::info << "commiting changes to database '" << database->getName() << "'" << endl;
				PaloServer->saveDatabase(database, 0);
			}

			vector<Cube*> cubes = database->getCubes(0);

			for (vector<Cube*>::iterator j = cubes.begin();  j != cubes.end();  j++) {
				Cube * cube = *j;

				if (cube->getStatus() == Cube::CHANGED) {
					Logger::info << "commiting changes to cube '" << cube->getName() << "'" << endl;
					database->saveCube(cube, 0);
				}
			}
		}
	}
	catch (const ErrorException& e) {
		Logger::warning << "cannot commit data file '" << e.getMessage() << "' (" << e.getDetails() << ")" << endl;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief main
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

extern "C" SERVER_FUNC int PaloMain (int argc, char * argv[]) {
	srand((int) time(0));
	_setmaxstdio(2048); // this is a hard limit

#else

int main (int argc, char * argv []) {
	srand((int) time(0));

#endif
	Logger::setLogLevel("error");

	char * saveMe = new char[1000000];

	try {
		parseOptions(argc, argv);

		// log version number
		string v = Server::getVersion();
		size_t n = v.find_first_of(";");

		for (; n != string::npos; n = v.find_first_of(";")) {
			v.at(n) = '.';
		}

		Logger::info << "starting Palo " << v << " (" << Server::getRevision() << ")" << endl;

		// check for external modules
		vector<ServerInfo_t*> extensions = palo::CheckExternalModules(PaloScheduler, PaloServer, ExtensionsDirectory);

		for (vector<ServerInfo_t*>::iterator i = extensions.begin();  i != extensions.end();  i++) {
			ServerInfo_t * info = *i;

#if defined(ENABLE_HTTP_MODULE) 
			if (info->httpInterface != 0) {
				ExternalHttpInterface = info->httpInterface;
			}
#endif    

#if defined(ENABLE_HTTPS_MODULE) 
			if (info->httpsInterface != 0) {
				ExternalHttpsInterface = info->httpsInterface;
			}
#endif    

#if defined(ENABLE_LEGACY_MODULE)
			if (info->legacyInterface != 0) {
				ExternalLegacyInterface = info->legacyInterface;
			}
#endif

			if (info->httpExtensions != 0) {
				HttpExtensions.push_back(info);
			}
		}

		// skeleton, will be filled later in ConstructPaloServer
		PaloServer = new Server(PaloScheduler, FileName(DataDirectory, "palo", "csv"));

		// http server
#if defined(ENABLE_HTTP)
		ConstructHttpServers(PaloScheduler, PaloServer, false);
		ConstructHttpServers(PaloScheduler, PaloServer, true);
#endif

		// https server
#if defined(ENABLE_HTTPS)
		ConstructHttpsServers(PaloScheduler, PaloServer);
#endif

		// add legacy server
#if defined(ENABLE_LEGACY)
		ConstructLegacyServers(PaloScheduler, PaloServer);
#endif

		// signals
		ControlCHandler controlCHandler(PaloScheduler);
		PaloScheduler->addSignalTask(&controlCHandler, SIGINT);
		PaloScheduler->addSignalTask(&controlCHandler, SIGTERM);

		// start serving clients
#if defined(_MSC_VER)
		if (StartAsService) {
			InitService();
		}
		else {
			// create server
			ConstructPaloServer();

			// start scheduler
			Logger::info << "starting to listen" << endl;
			PaloScheduler->run();

			// commit changes
			if (AutoCommit) {
				CommitChanges();
			}

			palo::CloseExternalModules();
		}
#else
		// create server
		ConstructPaloServer();

		Logger::info << "starting to listen" << endl;
		PaloScheduler->run();

		// commit changes
		if (AutoCommit) {
			CommitChanges();
		}

		palo::CloseExternalModules();
#endif
	}
	catch (const ErrorException& e) {
		palo::CloseExternalModules();
		Logger::error << "got exception '" << e.getMessage() << "' (" << e.getDetails() << "), giving up!" << endl;
		exit(1);
	}
	catch (std::bad_alloc) {
		Logger::error << "running out of memory, please reduce the number of loaded databases or cache sizes" << endl;
		palo::CloseExternalModules();

		delete[] saveMe;

		if (AutoCommit) {
			CommitChanges();
		}

		exit(1);
	}
	catch (...) {
		palo::CloseExternalModules();
		exit(1);
	}

	Logger::info << "finished." << endl;

	return 0;
}
