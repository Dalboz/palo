////////////////////////////////////////////////////////////////////////////////
/// @brief worker
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
////////////////////////////////////////////////////////////////////////////////

#include "InputOutput/Worker.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/WorkerTask.h"

namespace palo {
  string Worker::executable = "";
  vector<string> Worker::arguments;  
  int Worker::maxFailures = 5;


  Worker::Worker (Scheduler* scheduler, const string& sessionString) 
    : scheduler(scheduler), status(WORKER_NOT_RUNNING), sessionString(sessionString) {
    numFailures = 0;
    sessionSemaphore = scheduler->createSemaphore();
    waitForSessionDone = false;
    waitForShutdownTimeout = false;
    workerTask = 0;
  }


  Worker::~Worker () {
    scheduler->deleteSemaphore(sessionSemaphore);
    scheduler->unregisterWorker(this);

    if (workerTask) {
#if defined(_MSC_VER)
      scheduler->removeHandleTask(workerTask);
  
      UINT uExitCode = 0;
      
      // kill worker process
      if (TerminateProcess(processHandle, uExitCode)) {
        Logger::trace << "kill of worker process succeeded" << endl;
        CloseHandle(processHandle);
      }
      else {
        Logger::warning << "kill of worker process failed" << endl;
      }
#else
      scheduler->removeIoTask(workerTask);
  
      // kill worker process
      int val = kill(processPid , SIGKILL);
      if (val) {
        Logger::warning << "kill returned '" << val << "'" << endl;
      }
      else {
        Logger::trace << "kill of worker process succeeded" << endl;
      }
      int childExitStatus;
      pid_t ws = waitpid( processPid, &childExitStatus, WNOHANG);
      if ( WIFEXITED(childExitStatus) ) {
        Logger::warning << "kill of worker process failed" << endl;
      }
      if (ws == processPid) {
        Logger::debug << "got exit status of worker pid = '" << processPid << "' in Worker::~Worker" << endl;
      };
#endif
      workerTask = 0;
    }
    else {
#if defined(_MSC_VER)
#else
      int childExitStatus;
      pid_t ws = waitpid( processPid, &childExitStatus, WNOHANG);
      if (ws == processPid) {
        Logger::debug << "got exit status of worker pid = '" << processPid << "' in Worker::~Worker" << endl;
      };
#endif
    }
  }


  void Worker::setExecutable (const string& executable) {
    Worker::executable = executable;
  }

  void Worker::setArguments (const vector<string>& arguments) {
    Worker::arguments = arguments;
  }


#if defined(_MSC_VER)
  bool Worker::startProcess (const string& executable, HANDLE rd, HANDLE wr) {
    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    BOOL bFuncRetn = FALSE; 
 
    // set up members of the PROCESS_INFORMATION structure
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
 
    // set up members of the STARTUPINFO structure
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO); 

    siStartInfo.dwFlags = STARTF_USESTDHANDLES;
    siStartInfo.hStdInput = rd;
    siStartInfo.hStdOutput = wr;
    siStartInfo.hStdError = wr;

    // create the child process
    bFuncRetn = CreateProcess(NULL, 
                              (char*) executable.c_str(),  // command line 
                              NULL,                     // process security attributes 
                              NULL,                     // primary thread security attributes 
                              TRUE,                     // handles are inherited 
                              CREATE_NEW_PROCESS_GROUP, // creation flags 
                              NULL,                     // use parent's environment 
                              NULL,                     // use parent's current directory 
                              &siStartInfo,             // STARTUPINFO pointer 
                              &piProcInfo);             // receives PROCESS_INFORMATION 
   
    if (bFuncRetn == FALSE) {
      Logger::error << "execute of '" << executable << "' failed" << endl;
      return false;
    }
    else {
      processHandle = piProcInfo.hProcess;
      CloseHandle(piProcInfo.hThread);
      return true;
    }
  }
#endif

  bool Worker::start () {
    if (waitForShutdownTimeout) {
      return false;
    }
    
    if (status == WORKER_RUNNING) {
      return true;
    }

    if (status != WORKER_NOT_RUNNING) {
      return false;
    }

#if defined(_MSC_VER)
    // set the bInheritHandle flag so pipe handles are inherited
    SECURITY_ATTRIBUTES saAttr; 

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
 
    // create a pipe for the child process's STDOUT
    HANDLE hChildStdoutRd, hChildStdoutWr;

    if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
      status = WORKER_FAILED;

      Logger::error << "stdout pipe creation failed" << endl;

      return false;
    }
 
    // create a pipe for the child process's STDIN
    HANDLE hChildStdinRd, hChildStdinWr;

    if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
      status = WORKER_FAILED;

      Logger::error << "stdin pipe creation failed" << endl;

      return false;
    }
 
    string execStr = executable; 
    for(vector<string>::const_iterator i = arguments.begin(); i != arguments.end(); i++) {
      execStr += " " + *i;
    }

    // now create the child process.
    bool fSuccess = startProcess(execStr, hChildStdinRd, hChildStdoutWr);

    if (! fSuccess) {
      status = WORKER_FAILED;

      CloseHandle(hChildStdoutRd);
      CloseHandle(hChildStdoutWr);
      CloseHandle(hChildStdinRd);
      CloseHandle(hChildStdinWr);
      CloseHandle(processHandle);

      return false;
    }
 
    CloseHandle(hChildStdinRd);
    CloseHandle(hChildStdoutWr);

    workerTask = new WorkerTask(this, hChildStdoutRd, hChildStdinWr);

    scheduler->addHandleTask(workerTask);
#else
    int pipe_server_to_child[2];
    int pipe_child_to_server[2];
   
    if (pipe(pipe_server_to_child) == -1) {
      status = WORKER_FAILED;
      Logger::error << "cannot create pipe" << endl;

      return false;
    }

    if (pipe(pipe_child_to_server) == -1) {
      status = WORKER_FAILED;
      Logger::error << "cannot create pipe" << endl;

      close(pipe_server_to_child[0]);
      close(pipe_server_to_child[1]);

      return false;
    }
 
    processPid = fork();
   
    if (processPid == 0) {

      // child process
      Logger::trace << "fork succeeded" << endl;
     
      // set stdin and stdout of child process 
      dup2(pipe_server_to_child[0], 0);
      dup2(pipe_child_to_server[1], 1);
 
      fcntl(0, F_SETFD, 0);
      fcntl(1, F_SETFD, 0);
 
      // close pipes
      close(pipe_server_to_child[0]);
      close(pipe_server_to_child[1]);
      close(pipe_child_to_server[0]);
      close(pipe_child_to_server[1]);
     
#ifdef __SUNPRO_CC
      char ** args = new (char**)[arguments.size() + 2];
#else
      char * args[arguments.size() + 2];
#endif // __SUNPRO_CC

      args[0] = new char[executable.size() + 1];
      strcpy(args[0], executable.c_str());
      int count = 1;

      string argString;

      for(vector<string>::const_iterator i = arguments.begin(); i != arguments.end(); i++) {
        args[count] = new char[(*i).size() + 1];
        strcpy(args[count], (*i).c_str());
        count++;
        argString += " \'" + *i + "'";
      }

      args[count] = 0;

      signal(SIGINT,SIG_IGN);
      
      execv(executable.c_str(), args);

      cerr << "ERROR: execution of '" << executable << "' failed" << endl;

      exit(1);
    }
   
    // parent
    if (processPid == -1) {
      status = WORKER_FAILED;
      Logger::error << "fork failed " << endl;

      close(pipe_server_to_child[0]);
      close(pipe_server_to_child[1]);
      close(pipe_child_to_server[0]);
      close(pipe_child_to_server[1]);

      return false;
    }
 
    Logger::debug << "worker '" << executable << "' started with pid " << processPid << endl;

    close(pipe_server_to_child[0]);
    close(pipe_child_to_server[1]);
 
    socket_t writeSocket = pipe_server_to_child[1];
    socket_t readSocket = pipe_child_to_server[0];
   
    workerTask = new WorkerTask(this, readSocket, writeSocket);

    scheduler->addIoTask(workerTask);
#endif

    status = WORKER_RUNNING;

    // first send a session identifier to the worker process        
    this->semaphore = sessionSemaphore;
    currentSemaphore = sessionSemaphore;
    workerTask->execute("SESSION;" + sessionString);
    waitForSessionDone = true;
    status = WORKER_WORKING;
    
    
    return true;
  }



  void Worker::workDone (const string* msg) {
    scheduler->raiseAllSemaphore(semaphore, msg);
    
    if (waitForSessionDone) {
      waitForSessionDone = false;
    }

    if (! nextSemaphores.empty()) {
      semaphore_t nextSemaphore = nextSemaphores[0];
      string nextLine = nextLines[0];

      nextSemaphores.pop_front();
      nextLines.pop_front();

      execute(nextSemaphore, nextLine);
    }
  }



  bool Worker::execute (semaphore_t semaphore, const string& line) {
    if (status == WORKER_WORKING) {
      Logger::trace << "storing request '" << line << "'" << endl;

      nextSemaphores.push_back(semaphore);
      nextLines.push_back(line);

      return true;
    }

    result.clear();
    resultStatus = RESULT_FAILED;
    
    currentLine = line;

    this->semaphore = semaphore;

    if (status != WORKER_RUNNING) {
      Logger::error << "worker not running" << endl;

      workDone(0);

      return false;
    }

    Logger::trace << "executing worker with '" << line << "'" << endl;

    currentSemaphore = semaphore;
    workerTask->execute(line);

    status = WORKER_WORKING;

    return true;
  }



  void Worker::executeFinished (const vector<string>& result) {
    if (status != WORKER_WORKING) {
      Logger::error << "Worker::executeFinished called, but worker not working" << endl;
      status = WORKER_NOT_RUNNING;
      return;
    }

    this->result = result;
    resultStatus = RESULT_OK;

    status = WORKER_RUNNING;

    workDone(0);
  }



  void Worker::executeFailed () {
    if (status != WORKER_RUNNING && status != WORKER_WORKING) {
      Logger::error << "Worker::executeFailed called, but worker not running or working" << endl;
    }
    else {
#if _MSC_VER
      scheduler->removeHandleTask(workerTask);
#else
      scheduler->removeIoTask(workerTask);
#endif      
      workerTask = 0;
    }

#if defined(_MSC_VER)
    UINT uExitCode = 0;      
    // kill worker process
    TerminateProcess(processHandle, uExitCode);
    CloseHandle(processHandle);
#else
    int childExitStatus;
    pid_t ws = waitpid( processPid, &childExitStatus, WNOHANG);
    if (ws == processPid) {
      Logger::debug << "got exit status of worker pid = '" << processPid << "' in Worker::executeFailed" << endl;
    };
    
#endif

    if (numFailures < maxFailures && ! scheduler->shutdownInProgress() && !waitForShutdownTimeout) {
      bool redoLast = (currentSemaphore != sessionSemaphore && resultStatus == RESULT_FAILED);    
      semaphore_t tempSema = currentSemaphore;
      string      tempLine = currentLine;
      
      numFailures++;
      result.clear();
      status = WORKER_NOT_RUNNING;
      start();
      
      if (redoLast) {
        // redo last command      
        Logger::info << "Worker::executeFailed redo last command '" << tempLine << "'" << endl;
        execute(tempSema, tempLine);
      }

      return;        
    }
    
    
    result.clear();
    resultStatus = RESULT_FAILED;

    status = WORKER_FAILED;

    workDone(0);
    
    if (numFailures == maxFailures) {
      Logger::error << "max worker failures ('" << maxFailures<< "') reached" << endl;
      scheduler->beginShutdown();
    }
  }



  Worker::WorkerStatus Worker::getStatus () {
#if defined(_MSC_VER)
    if (status == WORKER_RUNNING || status == WORKER_WORKING) {
      DWORD exitCode;
      BOOL ok = GetExitCodeProcess(processHandle, &exitCode);

      if (! ok) {
        Logger::warning << "process died, cannot get exit code" << endl;

        TerminateProcess(processHandle, 1);
        executeFailed(); // this will start a new worker task
        return WORKER_FAILED; // but the old worker task is gone
      }
      else if (exitCode != STILL_ACTIVE) {
        Logger::warning << "process died, exit code '" << exitCode << "'" << endl;

        executeFailed(); // this will start a new worker task
        return WORKER_FAILED; // but the old worker task is gone
      }
    }
#else
    int stat;

    if (waitpid(processPid, &stat, WNOHANG) == processPid) {
      if (WIFEXITED(stat) || WIFSIGNALED(stat)) {
        Logger::warning << "process died, exit code '" << WEXITSTATUS(stat) << "'" << endl;

        executeFailed(); // this will start a new worker task
        return WORKER_FAILED; // but the old worker task is gone
      }
    }
#endif

    return status;
  }


  Worker::ResultStatus Worker::getResultStatus () {
    return resultStatus;
  }



  const vector<string>& Worker::getResult () {
    return result;
  }
  
  
  
  void Worker::timeoutReached () {
  }
}
