////////////////////////////////////////////////////////////////////////////////
/// @brief worker task
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

#include "InputOutput/WorkerTask.h"

#include <iostream>

#include "InputOutput/Logger.h"
#include "InputOutput/Worker.h"

namespace palo {
#if defined(_MSC_VER)

  WorkerTask::WorkerTask (Worker* worker, HANDLE readHandle, HANDLE writeHandle)
    : HandleTask(readHandle, writeHandle),
      worker(worker),
      status(IDLE) {
  }



  WorkerTask::~WorkerTask () {
    CloseHandle(readHandle);
    CloseHandle(writeHandle);
  }



  size_t WorkerTask::canHandleRead () {
    if (status == READING) {
      if (worker->getStatus() == Worker::WORKER_FAILED) { // might delete "this"
        return 0;
      }

      return HandleTask::canHandleRead();
    }
    else {
	  DWORD available;
      BOOL ok = PeekNamedPipe(readHandle,
		                      0,
			                  0,
                              0,
                              &available,
                              0);

	  // if worker died, fail in read
	  return (ok == FALSE) ? 1 : 0;
    }
  }

  void WorkerTask::handleHangup (HANDLE fd) {
    worker->executeFailed();
  }

#else

  WorkerTask::WorkerTask (Worker* worker, socket_t readSocket, socket_t writeSocket)
    : IoTask(readSocket, writeSocket),
      ReadTask(readSocket),
      WriteTask(writeSocket),
      ReadWriteTask(readSocket, writeSocket),
      worker(worker),
      status(IDLE) {
  }



  WorkerTask::~WorkerTask () {
    close(readSocket);
    close(writeSocket);
  }



  bool WorkerTask::canHandleRead () {
    if (status == READING) {
      if (worker->getStatus() == Worker::WORKER_FAILED) { // might delete "this"
        return 0;
      }
       
      return ReadTask::canHandleRead();
    }
    else {
      return 0;
    }
  }



  void WorkerTask::handleHangup (socket_t fd) {
    worker->executeFailed();
  }

#endif



  void WorkerTask::execute (const string& line) {
    StringBuffer * sb = new StringBuffer();
    sb->initialize();
    sb->appendText(line.c_str());
    sb->appendEol();

    setWriteBuffer(sb);
  }



#if defined(_MSC_VER)
  bool WorkerTask::handleRead (size_t available) {
    bool res = fillReadBuffer(available);
#else
  bool WorkerTask::handleRead () {
    bool res = fillReadBuffer();
#endif

    if (! res) {
      // executeFailed() is called in WorkerTask::handleHangup!
      // worker->executeFailed();
      return false;
    }

    const char * ptr = readBuffer.c_str() + readPosition;
    const char * end = readBuffer.end();

    while (ptr < end) {
      for (;  ptr < end;  ptr++) {
        if (*ptr == '\n') {
          break;
        }
      }

      if (ptr < end) {
        readPosition = ptr - readBuffer.c_str();
        
        size_t lineLength = readPosition;
        if (lineLength > 0 && *(ptr-1) == '\r') {        
          lineLength--;
        }
        string line(readBuffer.c_str(), lineLength);

        Logger::trace << "got result '" << line << "' for WorkerTask" << endl;

        readBuffer.erase_front(readPosition + 1);
        readPosition = 0;

        if (line == "DONE") {
          status = IDLE;
          worker->executeFinished(result);
          return true;
        }
        else if (line.substr(0,6) == "ERROR;") {
          Logger::warning << "got error message '" << line << "' for WorkerTask" << endl;
          status = IDLE;
          result.push_back(line);
          worker->executeFinished(result);
          return true;
        }
        else {
          result.push_back(line);
        }

        ptr = readBuffer.c_str();
        end = readBuffer.end();
      }
    }

    return true;
  }



  void WorkerTask::setWriteBuffer (StringBuffer * sb, bool ownBuffer) {
    status = WRITING;

#if defined(_MSC_VER)
    HandleTask::setWriteBuffer(sb, ownBuffer);
#else
    WriteTask::setWriteBuffer(sb, ownBuffer);
#endif
  }



  void WorkerTask::completedWriteBuffer () {
    Logger::trace << "completed writing in WorkerTask" << endl;
    status = READING;
    readPosition = 0;
    readBuffer.clear();
    result.clear();
  }
}
