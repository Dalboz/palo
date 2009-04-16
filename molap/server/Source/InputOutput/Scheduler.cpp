////////////////////////////////////////////////////////////////////////////////
/// @brief input-output scheduler
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

#include "InputOutput/Scheduler.h"

#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <iostream>

#include "Exceptions/ErrorException.h"

#if defined(_MSC_VER)
#include "InputOutput/HandleTask.h"
#endif

#include "InputOutput/IoTask.h"
#include "InputOutput/ListenTask.h"
#include "InputOutput/Logger.h"
#include "InputOutput/SemaphoreTask.h"
#include "InputOutput/SignalTask.h"
#include "InputOutput/Worker.h"

#include "Olap/PaloSession.h"
#include "Olap/Lock.h"

namespace palo {
  static Scheduler * SignalHandler;
  Scheduler * Scheduler::globalScheduler = 0;


#if defined(_MSC_VER)
  BOOL WINAPI signalHandler (DWORD dwCtrlType) {
    if (SignalHandler != 0) {
      SignalHandler->raiseSignal(SIGINT);
    }

    return TRUE;
  }
#else
  static void signalHandler (int signal) {
    if (SignalHandler != 0) {
      SignalHandler->raiseSignal(signal);
    }
  }
#endif



  Scheduler::Scheduler () 
    : lastWorkerStart(0) {
#ifdef USE_POLL
    pollFds.initialize();
#else
    selectFds.initialize();
#endif

#if defined(_MSC_VER)
    WSADATA wsaData;

    WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

    SignalHandler = this;
    nextCollectTime = 0;
  }



  Scheduler::~Scheduler () {
    SignalHandler = 0;

#if defined(_MSC_VER)
    WSACleanup();
#endif
  }



  Scheduler* Scheduler::getScheduler () {
    if (!globalScheduler) {
      globalScheduler = new Scheduler();
    }
    return globalScheduler;
  }



  void Scheduler::run () {
    time_t nextLockCheck = time(0) + LOCK_CHECK_PERIODE_SECONDS;
    int nextLockCheckRun = 0;
    initializeSignalHandlers();

    stopping = false;
    listenerOpen = true;
    readWriteOpen = true;

    bool abort = false;

    while (! abort) {

      // start workers
      startWorkers();

      // notify workers after a timeout
      startTimeoutWorkers();
      
      // signal handlers
      notifySignalHandlers();

      // check locks
      if (nextLockCheckRun++ % 1000 == 0) {
        time_t now;
        time(&now);

        if (now > nextLockCheck) {
          checkLocks();
          nextLockCheck = time(0) + LOCK_CHECK_PERIODE_SECONDS;
        }
      }

      // socket handlers
      if (listenerOpen || readWriteOpen) {
        socket_t max;
        bool ok = setupSockets(&max);

        if (ok) {

#ifdef USE_POLL
          int n = ::poll(pollFds.begin(), pollFds.size(), DEFAULT_TIMEOUT_MSEC);
#else
          timeval timeout;
        
          timeout.tv_sec = 0;
          timeout.tv_usec = DEFAULT_TIMEOUT_MSEC * 1000;
          
          int n = (int) select((int) max + 1, &readFds, &writeFds, &exceptFds, &timeout);
#endif
          
          if (0 < n) {
            notifyHandlers(n);
          }
          else if (n < 0) {
            if (errno_socket != EINTR_SOCKET) {
              if (! stopping) {
                Logger::error << "poll/select failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")"
                            << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
                beginShutdown();
              }
            }
          }
        }
      }

      // HANDLE handlers
#if defined(_MSC_VER)
      notifyHandleHandlers();
#else
      collectWorkerExitStatus();
#endif

      // shutdown in progress
      if (stopping) {
        time_t now;
        time(&now);

        if (stopTime + GRACE_SECONDS < now) {
          abort = true;
        }

        if (listenerOpen) {
          shutdownListenTasks();
          Logger::info << "listener sockets closed" << endl;
          listenerOpen = false;
        }
      }
    }

    if (listenerOpen) {
      shutdownListenTasks();
    }

    if (readWriteOpen) {
      Logger::info << "read/write sockets closed" << endl;
      shutdownIoTasks();
    }

    // give sockets a chance to close the connection
    usleep(1000);
  }




  void Scheduler::checkLocks () {
    Logger::debug << "checking locks" << endl;

    vector<Lock*> oldLocks;

    for (set<Lock*>::iterator i = locks.begin();  i != locks.end();  i++) {
      Lock* lock = *i;

      if (! PaloSession::isUserActive(lock->getUserIdentifier())) {
        Logger::debug << "lock " << lock->getIdentifier() << " has no active user " << lock->getUserIdentifier() 
                      << ", initiating rollback" << endl;

        oldLocks.push_back(lock);
      }
    }

    for (vector<Lock*>::iterator i = oldLocks.begin();  i != oldLocks.end();  i++) {
      Lock* lock = *i;

      lock->getCube()->rollbackCube(lock->getIdentifier(), 0, 0);
    }
  }



  void Scheduler::shutdownListenTasks () {
    set<IoTask*> t = listenTasks;

    for (set<IoTask*>::iterator i = t.begin();  i != t.end();  i++) {
      IoTask * task = *i;
      set<Task*>::const_iterator j = tasks.find(task);

      if (j != tasks.end()) {
        socket_t readSocket = task->getReadSocket();

        task->handleShutdown();
        closesocket(readSocket);
      }
    }
  }



  void Scheduler::shutdownIoTasks () {
    set<IoTask*> t = readWriteTasks;

    for (set<IoTask*>::iterator i = t.begin();  i != t.end();  i++) {
      IoTask * task = *i;
      set<Task*>::const_iterator j = tasks.find(task);

      if (j != tasks.end()) {
        socket_t readSocket = task->getReadSocket();
        socket_t writeSocket = task->getWriteSocket();

        task->handleShutdown();

        closesocket(readSocket);

        if (readSocket != writeSocket) {
          closesocket(writeSocket);
        }
      }
    }
  }



  void Scheduler::initializeSignalHandlers () {
#if defined(_MSC_VER)
    if(SetConsoleCtrlHandler(&signalHandler, TRUE) != TRUE) {
      Logger::error << "cannot initialize signal handlers" << endl;
    }
#else

    // interrupt
    struct sigaction action;
    memset(&action, 0, sizeof(action));

    action.sa_handler = signalHandler;
    sigfillset(&action.sa_mask);

    int res = sigaction(SIGINT, &action, 0);

    if (res < 0) {
      Logger::error << "cannot initialize signal handlers for interrupts" << endl;
    }

    // pipe
    struct sigaction paction;
    memset(&paction, 0, sizeof(paction));

    paction.sa_handler = SIG_IGN;
    sigfillset(&paction.sa_mask);

    res = sigaction(SIGPIPE, &paction, 0);

    if (res < 0) {
      Logger::error << "cannot initialize signal handlers for pipe" << endl;
    }
#endif
  }



  void Scheduler::raiseSignal (signal_t signal) {
    switch (signal) {
      case SIGINT:
        raisedSignals.insert(signal);
        return;

      default:
        return;
    }
  }



  void Scheduler::notifySignalHandlers () {
    while (! raisedSignals.empty()) {
      set<signal_t>::iterator i = raisedSignals.begin();
      signal_t signal = *i;
      set<SignalTask*> handlers = signalHandlers[signal];

      raisedSignals.erase(i);

      for (set<SignalTask*>::iterator j = handlers.begin();  j != handlers.end();  j++) {
        (*j)->handleSignal(signal);
      }

      Logger::info << "handled signal " << signal << endl;
    }
  }



  void Scheduler::beginShutdown () {
    if (! stopping) {
      Logger::info << "beginning shutdown sequence" << endl;
      stopping = true;
      time(&stopTime);
    }
  }



  bool Scheduler::setupSockets (socket_t* max) {
    *max = 0;

    if (listenTasks.empty() && readWriteTasks.empty()) {
      return false;
    }

#ifdef USE_POLL
    pollFds.clear();

    for (set<IoTask*>::const_iterator i = listenTasks.begin();  i != listenTasks.end();  i++) {
      IoTask * task = *i;
      set<Task*>::const_iterator j = tasks.find(task);

      if (j != tasks.end()) {
        socket_t fd = task->getReadSocket();
        pollfd p;
        
        p.fd      = fd;
        p.events  = POLLRDNORM | POLLHUP;
        p.revents = 0;
        
        pollFds.push_back(p);
        
        if (*max < fd) {
          *max = fd;
        }
      }
    }

    for (set<IoTask*>::const_iterator i = readWriteTasks.begin();  i != readWriteTasks.end();  i++) {
      IoTask * task = *i;
      set<Task*>::const_iterator j = tasks.find(task);

      if (j != tasks.end()) {
        socket_t readSocket = task->getReadSocket();
        socket_t writeSocket = task->getWriteSocket();
        
        if (readSocket == writeSocket) {
          pollfd p;

          // read and write
          p.fd      = readSocket;
          p.events  = POLLHUP;
          p.revents = 0;
        
          if (task->canHandleRead()) {
            p.events |= POLLRDNORM;
          }
        
          if (task->canHandleWrite()) {
            p.events |= POLLWRNORM;
          }
        
          pollFds.push_back(p);
        
          if (*max < readSocket) {
            *max = readSocket;
          }
        }
        else {
          pollfd p;

          // read
          p.fd      = readSocket;
          p.events  = POLLHUP;
          p.revents = 0;
        
          if (task->canHandleRead()) {
            p.events |= POLLRDNORM;
          }
        
          pollFds.push_back(p);
        
          if (*max < readSocket) {
            *max = readSocket;
          }

          // write
          p.fd      = writeSocket;
          p.events  = POLLHUP;
          p.revents = 0;
        
          if (task->canHandleWrite()) {
            p.events |= POLLWRNORM;
          }
        
          pollFds.push_back(p);
        
          if (*max < writeSocket) {
            *max = writeSocket;
          }
        }
      }
    }
#else
    selectFds.clear();

    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_ZERO(&exceptFds);
    
    for (set<IoTask*>::const_iterator i = listenTasks.begin();  i != listenTasks.end();  i++) {
      IoTask * task = *i;
      set<Task*>::const_iterator j = tasks.find(task);
      
      if (j != tasks.end()) {
        socket_t readSocket = task->getReadSocket();
        
        FD_SET(readSocket, &readFds);
        FD_SET(readSocket, &exceptFds);
        
        if (*max < readSocket) {
          *max = readSocket;
        }

        selectFds.push_back(readSocket);
      }
    }
    
    for (set<IoTask*>::const_iterator i = readWriteTasks.begin();  i != readWriteTasks.end();  i++) {
      IoTask * task = *i;
      set<Task*>::const_iterator j = tasks.find(task);
      
      if (j != tasks.end()) {
        socket_t readSocket = task->getReadSocket();
        socket_t writeSocket = task->getWriteSocket();
        
        FD_SET(readSocket, &exceptFds);

        selectFds.push_back(readSocket);

        if (readSocket != writeSocket) {
          FD_SET(writeSocket, &exceptFds);
          selectFds.push_back(writeSocket);
        }
        
        if (task->canHandleRead()) {
          FD_SET(readSocket, &readFds);
        }
        
        if (task->canHandleWrite()) {
          FD_SET(writeSocket, &writeFds);
        }
        
        if (*max < readSocket) {
          *max = readSocket;
        }

        if (*max < writeSocket) {
          *max = writeSocket;
        }
      }
    }
#endif
    
    removePendingTasks();
    
    return true;
  }



  void Scheduler::notifyHandlers (int n) {
#if USE_POLL
    pollfd * j = pollFds.begin();

    for (size_t len = pollFds.size();  0 < len;  j++, len--) {
      map<socket_t, IoTask*>::iterator iter = sockets.find(j->fd);

      // the socket might no longer be valid
      if (iter == sockets.end()) {
        Logger::debug << "socket invalid" << endl;
        continue;
      }

      IoTask * task = iter->second;
      short p       = j->revents;

      try {
        if ((p & POLLHUP) != 0 || (p & POLLERR) != 0) {
          task->handleHangup(j->fd);
        }
        else if ((p & POLLRDNORM) != 0) {
          if (! task->handleRead()) {
            task->handleHangup(j->fd);
          }
        }
        else if ((p & POLLWRNORM) != 0) {
          if (! task->handleWrite()) {
            task->handleHangup(j->fd);
          }
        }
      }
      catch (const ErrorException& ex) {
        Logger::error << "handler failed with " << ex.getMessage() << " (" << ex.getDetails() << ")" << endl;
      }
    }
#else
    socket_t * j = selectFds.begin();

    for (size_t len = selectFds.size();  0 < len;  j++, len--) {
      socket_t fd = *j;

      map<socket_t, IoTask*>::iterator iter = sockets.find(*j);
      
      // the socket might no longer be valid
      if (iter == sockets.end()) {
        continue;
      }

      IoTask * task = iter->second;
        
      try {
        if (FD_ISSET(fd, &exceptFds)) {
          task->handleHangup(fd);
        }
        else if (FD_ISSET(fd, &readFds)) {
          if (! task->handleRead()) {
            task->handleHangup(fd);
          }
        }
        else if (FD_ISSET(fd, &writeFds)) {
          if (! task->handleWrite()) {
            task->handleHangup(fd);
          }
        }
      }
      catch (const ErrorException& ex) {
        Logger::error << "handler failed with " << ex.getMessage() << " (" << ex.getDetails() << ")" << endl;
      }
    }
#endif
  }



#if defined(_MSC_VER)

  void Scheduler::notifyHandleHandlers () {
    for (set<HandleTask*>::iterator htIter = handleTasks.begin();  htIter != handleTasks.end();  htIter++) {
      HandleTask * ht = *htIter;

      if (tasks.find(ht) == tasks.end()) {
        continue;
      }

      if (ht->canHandleWrite()) {
        if (! ht->handleWrite()) {
          ht->handleHangup(ht->getWriteHandle());
        }
      }
      else {
        size_t read = ht->canHandleRead();

        if (0 < read) {
          if (! ht->handleRead(read)) {
            ht->handleHangup(ht->getReadHandle());
          }
        }
      }
    }
    
    removePendingTasks();
  }

#endif

  void Scheduler::registerWorker (Worker * worker) {
    bool found = false;
    for (deque<Worker*>::iterator i = workerList.begin(); i != workerList.end(); i++) {
      if (*i == worker) {
        found = true;
        break;
      }
    }
    if (!found) {
      workerList.push_back(worker);
    }
  }

  void Scheduler::unregisterWorker (Worker * worker) {
    // erease removed worker from workerList
    if (!workerList.empty()) {
      for (deque<Worker*>::iterator i = workerList.begin(); i != workerList.end(); i++) {
        if ( (*i) == worker ) {
          workerList.erase(i);
          break;
        }
      }
    }    

    // add worker to removed worker
    // (used for timeoutWorkers)
    removedWorker.insert(worker);
  }



  void Scheduler::startWorkers () {
    if (workerList.empty()) {
      return;
    }

    struct timeval tv;
    ::gettimeofday(&tv, 0);

    int64_t msec = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;

    if (lastWorkerStart + START_WORKER_DELAY_MSEC > msec) {
      return;
    }

    lastWorkerStart = msec;

    Worker * worker = workerList.front();
    workerList.pop_front();

    Logger::debug << "starting next cube worker" << endl;

    worker->start();
  }


  void Scheduler::registerTimoutWorker (Worker * worker) {
    struct timeval tv;
    ::gettimeofday(&tv, 0);
    int64_t msec = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL + KILL_WORKER_DELAY_MSEC;
    prioTask p(msec, worker);    
    timeoutWorkers.push(p);
  }



  void Scheduler::startTimeoutWorkers () {
    if (timeoutWorkers.empty()) {
      if (!removedWorker.empty()) {
        removedWorker.clear();
      }
      return;
    }

    struct timeval tv;
    ::gettimeofday(&tv, 0);

    int64_t now = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;

    prioTask pt = timeoutWorkers.top();
    
    if (pt.first > now) {
      return;
    }
    
    Logger::debug << "notify worker after timeout" << endl;
    
    set<Worker*>::iterator x = removedWorker.find(pt.second);
    if (x == removedWorker.end()) {
      pt.second->timeoutReached();
    }
    timeoutWorkers.pop();    

    if (timeoutWorkers.empty() && !removedWorker.empty()) {
      removedWorker.clear();
    }
  }



  void Scheduler::addTask (Task* task) {
    tasks.insert(task);
  }



  void Scheduler::removeTask (Task* task) {
    tasks.erase(task);

    SemaphoreTask * semaTask = dynamic_cast<SemaphoreTask*>(task);

    if (semaTask != 0) {
      removeSemaphoreTask(semaTask);
    }
  }




  bool Scheduler::addListenTask (ListenTask* task, int port) {

    // create a new socket
    socket_t fd  = socket(AF_INET, SOCK_STREAM, 0);
    
    if (fd < 0) {
      Logger::error << "socket failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      return false;
    }
    
    // bind socket to an address
    sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);
    
    int res = bind(fd, (const sockaddr*) &addr, sizeof(addr));
    
    if (res < 0) {
      closesocket(fd);
      
      Logger::error << "bind failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      
      return false;
    }
    
    // listen for new connection
    res = listen(fd, 10);
    
    if (res < 0) {
      closesocket(fd);
      
      Logger::error << "listen failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      
      return false;
    }
    
    // set socket to non-blocking
    bool ok = SetNonBlockingSocket(fd);
    
    if (! ok) {
      closesocket(fd);
      
      Logger::error << "cannot switch to non-blocking in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")"
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;

      return false;
    }
    
    // update listen task
    task->setReadSocket(fd);
    sockets[fd] = task;

    // update maps
    listenTasks.insert(task);

    // add new task
    addTask(task);
    
    return true;
  }
  
  
  
  bool Scheduler::addListenTask (ListenTask* task, const string& address, int port) {
    if (address.empty()) {
      return addListenTask(task, port);
    }

    // create a new socket
    socket_t fd  = socket(AF_INET, SOCK_STREAM, 0);
    
    if (fd < 0) {
      Logger::error << "socket failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      return false;
    }
    
    // resolv name
    struct ::hostent * sheep = ::gethostbyname(address.c_str());

    if (sheep == 0) {
      Logger::error << "cannot resolve hostname in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      return false;
    }


    // bind socket to an address
    sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family      = AF_INET;
    memcpy(&(addr.sin_addr.s_addr), sheep->h_addr, sheep->h_length);
    addr.sin_port        = htons(port);
    
    int res = bind(fd, (const sockaddr*) &addr, sizeof(addr));
    
    if (res < 0) {
      closesocket(fd);
      
      Logger::error << "bind failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      
      return false;
    }
    
    // listen for new connection
    res = listen(fd, 10);
    
    if (res < 0) {
      closesocket(fd);
      
      Logger::error << "listen failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      
      return false;
    }
    
    // set socket to non-blocking
    bool ok = SetNonBlockingSocket(fd);
    
    if (! ok) {
      closesocket(fd);
      
      Logger::error << "cannot switch to non-blocking in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")"
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;

      return false;
    }
    
    // update listen task
    task->setReadSocket(fd);
    sockets[fd] = task;

    // update maps
    listenTasks.insert(task);
    
    // add new task
    addTask(task);

    return true;
  }
  
  
  
  void Scheduler::removeListenTask (ListenTask* task) {
    set<Task*>::iterator i = tasks.find(task);
    
    if (i == tasks.end()) {
      Logger::error << "listen task no found in task list" << endl;
      return;
    }

    sockets.erase(task->getReadSocket());
    removableListenTasks.insert(task);

    removeTask(task);
  }
  
  
  
  bool Scheduler::addIoTask (IoTask* task) {
    
    // get read/write socket
    socket_t readSocket = task->getReadSocket();
    socket_t writeSocket = task->getWriteSocket();
    
    // set socket to non-blocking
    bool res = SetNonBlockingSocket(readSocket);
    
    if (! res) {
      Logger::error << "cannot switch to non-blocking in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")"
                    << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;
      
      return false;
    }

    if (readSocket != writeSocket) {
      res = SetNonBlockingSocket(writeSocket);

      if (! res) {
        Logger::error << "cannot switch to non-blocking in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")"
                      << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;

        return false;
      }
    }
    
    // update maps
    sockets[readSocket] = task;
    sockets[writeSocket] = task;

    readWriteTasks.insert(task);

    addTask(task);
    
    return true;
  }
  
  
  
  void Scheduler::removeIoTask (IoTask* task) {
    set<Task*>::iterator i = tasks.find(task);
    
    if (i == tasks.end()) {
      Logger::error << "read-write task no found in task list" << endl;
      return;
    }
    
    socket_t readSocket = task->getReadSocket();
    socket_t writeSocket = task->getWriteSocket();

    sockets.erase(readSocket);
    sockets.erase(writeSocket);
    
    removableIoTasks.insert(task);

    removeTask(task);
  }



  bool Scheduler::addSignalTask (SignalTask* task, signal_t sid) {
    switch (sid) {
      case SIGINT: 
        signalHandlers[sid].insert(task);
        break;

      default:
        return false;
    }

    addTask(task);

    return true;
  }



#if defined(_MSC_VER)

  bool Scheduler::addHandleTask (HandleTask* task) {
    handleTasks.insert(task);

    addTask(task);
    
    return true;
  }
  
  
  
  void Scheduler::removeHandleTask (HandleTask* task) {
    set<Task*>::iterator i = tasks.find(task);
    
    if (i == tasks.end()) {
      Logger::error << "handle task not found in task list" << endl;
      return;
    }
    
    removableHandleTasks.insert(task);

    removeTask(task);
  }



#endif

  void Scheduler::removeSemaphoreTask (SemaphoreTask* task) {
    map< SemaphoreTask*, set<semaphore_t> >::iterator iter = task2Semaphores.find(task);

    if (iter == task2Semaphores.end()) {
      return;
    }

    for (set<semaphore_t>::iterator semas = iter->second.begin();  semas != iter->second.end();  semas++) {
      removeSemaphoreTask(*semas, task);
    }

    task2Semaphores.erase(iter);
    tasks.erase(task);
  }



  void Scheduler::removeSemaphoreTask (semaphore_t semaphore, SemaphoreTask * task) {
    map< semaphore_t, vector< pair<SemaphoreTask*,IdentifierType> > >::iterator iter = semaphore2Tasks.find(semaphore);

    if (iter == semaphore2Tasks.end()) {
      return;
    }

    vector< pair<SemaphoreTask*,IdentifierType> >& sts = iter->second;
    vector< pair<SemaphoreTask*,IdentifierType> > newSet;

    for (vector< pair<SemaphoreTask*,IdentifierType> >::iterator setIter = sts.begin();  setIter != sts.end();  setIter++) {
      if (setIter->first != task) {
        newSet.push_back(*setIter);
      }
    }

    if (newSet.empty()) {
      semaphore2Tasks.erase(iter);
    }
    else {
      iter->second = newSet;
    }
  }



  semaphore_t Scheduler::createSemaphore () {
    static semaphore_t next = 1;

    return next++;
  }



  void Scheduler::deleteSemaphore (semaphore_t semaphore) {
    map< semaphore_t, vector< pair<SemaphoreTask*,IdentifierType> > >::iterator iter = semaphore2Tasks.find(semaphore);

    if (iter == semaphore2Tasks.end()) {
      return;
    }

    vector< pair<SemaphoreTask*,IdentifierType> > tasks = iter->second;
    iter->second.clear();

    for (vector< pair<SemaphoreTask*,IdentifierType> >::iterator taskIter = tasks.begin();  taskIter != tasks.end();  taskIter++) {
      SemaphoreTask * task = (*taskIter).first;
      IdentifierType data  = (*taskIter).second;

      if (this->tasks.find(task) == this->tasks.end()) {
         continue;
      }

      task->handleSemaphoreDeleted(semaphore, data);
    }
  }



  void Scheduler::waitOnSemaphore (semaphore_t semaphore, SemaphoreTask * task, IdentifierType data) {
    semaphore2Tasks[semaphore].push_back(pair<SemaphoreTask*,IdentifierType>(task, data));
    task2Semaphores[task].insert(semaphore);
  }



  void Scheduler::raiseAllSemaphore (semaphore_t semaphore, const string* msg) {
    map< semaphore_t, vector< pair<SemaphoreTask*,IdentifierType> > >::iterator iter = semaphore2Tasks.find(semaphore);

    if (iter == semaphore2Tasks.end()) {
      return;
    }

    vector< pair<SemaphoreTask*,IdentifierType> > tasks = iter->second;
    iter->second.clear();

    for (vector< pair<SemaphoreTask*,IdentifierType> >::iterator taskIter = tasks.begin();  taskIter != tasks.end();  taskIter++) {
      SemaphoreTask * task = (*taskIter).first;
      IdentifierType data  = (*taskIter).second;

      if (this->tasks.find(task) == this->tasks.end()) {
         continue;
      }

      task->handleSemaphoreRaised(semaphore, data, msg);
    }
  }


  
  void Scheduler::removePendingTasks () {
    if (removableListenTasks.size()>0) {
      for (set<IoTask*>::iterator i = removableListenTasks.begin(); i != removableListenTasks.end(); i++) {
        IoTask * task = *i;
        listenTasks.erase(task);
        // delete task; 
      }
      removableListenTasks.clear();
    }

    if (removableIoTasks.size() > 0) {
      for (set<IoTask*>::iterator i = removableIoTasks.begin(); i != removableIoTasks.end(); i++) {
        IoTask * task = *i;
        readWriteTasks.erase(task);
        delete task;
      }
      removableIoTasks.clear();
    }

#if defined(_MSC_VER)
    if (removableHandleTasks.size() > 0) {
      for (set<HandleTask*>::iterator i = removableHandleTasks.begin(); i != removableHandleTasks.end(); i++) {
        HandleTask * task = *i;
        handleTasks.erase(task);
        delete task;
      }
      removableHandleTasks.clear();
    }
#endif
  }



  void Scheduler::collectWorkerExitStatus () {
#if defined(_MSC_VER)
    // done automatically under windows
#else
    struct timeval tv;
    ::gettimeofday(&tv, 0);

    if ( tv.tv_sec > nextCollectTime) {
      nextCollectTime = tv.tv_sec + 60;
      int childExitStatus;
      pid_t ws = waitpid( 0, &childExitStatus, WNOHANG);
      while (ws > 0) {
        Logger::debug << "got exit status of worker pid = '" << ws << "' in Scheduler::collectWorkerExitStatus" << endl;
        pid_t ws2 = waitpid( 0, &childExitStatus, WNOHANG);
        ws = (ws != ws2) ? ws2 : 0;
      };    
    }
#endif
  }
  
  
}
