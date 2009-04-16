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

#ifndef INPUT_OUTPUT_WORKER_H
#define INPUT_OUTPUT_WORKER_H 1

#include "palo.h"

#include <deque>

namespace palo {
  class Scheduler;
  class Server;
  class WorkerTask;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief worker
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Worker {
  protected:
    static string executable;
    static vector<string> arguments;  
    static int maxFailures;

  public:
    enum ResultStatus {
      RESULT_OK,
      RESULT_FAILED,
    };

    enum WorkerStatus {
      WORKER_NOT_RUNNING,
      WORKER_FAILED,
      WORKER_RUNNING,
      WORKER_WORKING,
    };

  public:
    static void setExecutable (const string& executable);
    static void setArguments (const vector<string>& arguments);

  public:
    Worker (Scheduler*, const string&);
    virtual ~Worker ();

  public:
    virtual bool start ();

    virtual bool execute (semaphore_t, const string& line);

    virtual void executeFinished (const vector<string>& result);

    virtual void executeFailed ();

    virtual void timeoutReached ();

  public:
    WorkerStatus getStatus ();

    ResultStatus getResultStatus ();

    const vector<string>& getResult ();

  private:
#if defined(_MSC_VER)
    bool startProcess (const string& executable, HANDLE, HANDLE);
#endif

  protected:
    void workDone (const string*);

  protected:
    Scheduler * scheduler;
    semaphore_t semaphore;
    vector <string> result;

    WorkerStatus status;
    WorkerTask * workerTask;

    ResultStatus resultStatus;

    deque<semaphore_t> nextSemaphores;
    deque<string> nextLines;
    
    int numFailures;
    string sessionString;
    string currentLine;
    semaphore_t currentSemaphore;
    
    semaphore_t sessionSemaphore;
    bool waitForSessionDone;
    bool waitForShutdownTimeout;
    
#if defined(_MSC_VER)
    HANDLE processHandle;
#else
    pid_t processPid;
#endif
  };

}

#endif
