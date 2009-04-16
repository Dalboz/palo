////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for input-output tasks
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

#ifndef INPUT_OUTPUT_IO_TASK_H
#define INPUT_OUTPUT_IO_TASK_H 1

#include "palo.h"

#include "InputOutput/Logger.h"
#include "InputOutput/Task.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief abstract base class for input-output tasks
  ///
  /// This abstract class is the base class for all input/output tasks. It
  /// contains the method canHandleRead and canHandleWrite and the callbacks
  /// handleRead, handleWrite, and handleHangup which should be defined by
  /// subclasses.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS IoTask : virtual public Task {
  public:
    IoTask (socket_t readSocket, socket_t writeSocket)
      : readSocket(readSocket), writeSocket(writeSocket) {
    }

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the read socket or INVALID_SOCKET
    ///
    /// This function is used by the scheduler to get the underlying read socket.
    ////////////////////////////////////////////////////////////////////////////////
    
    virtual socket_t getReadSocket () {return readSocket;}
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to check if the task wants to read
    ///
    /// This function is used by the scheduler to check if the task is ready to
    /// receive new data. The scheduler will only check connection for new data
    /// if this function returns true.
    ////////////////////////////////////////////////////////////////////////////////
      
    virtual bool canHandleRead () {return readSocket != INVALID_SOCKET;}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate available data
    ///
    /// This callback is called by the scheduler if there is new data
    /// available. The task must try to read data from the connection in a
    /// non-blocking way.
    ////////////////////////////////////////////////////////////////////////////////
      
    virtual bool handleRead () {return true;}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the write socket or INVALID_SOCKET
    ///
    /// This function is used by the scheduler to get the underlying write socket.
    ////////////////////////////////////////////////////////////////////////////////

    virtual socket_t getWriteSocket () {return writeSocket;}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to check if task wants to write
    ///
    /// This function is used by the scheduler to check if the task is ready to
    /// send data. The scheduler will only check connection for writability if
    /// this function returns true.
    ////////////////////////////////////////////////////////////////////////////////
        
    virtual bool canHandleWrite () {return writeSocket != INVALID_SOCKET;}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate that data can be written
    ///
    /// This callback is called by the scheduler if the connection can consume
    /// data. The task must try to write data to the connection in a
    /// non-blocking way.
    ////////////////////////////////////////////////////////////////////////////////

    virtual bool handleWrite () {return true;}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate hangup
    ///
    /// This callback is called by the scheduler if the connection has been
    /// closed.
    ////////////////////////////////////////////////////////////////////////////////

    virtual void handleHangup (socket_t fd) {
      Logger::trace << "IoTask::handleHangup called" << endl;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate shutdown in progess
    ///
    /// This callback is called by the scheduler to indicate that the scheduler
    /// is shutting done. The task should release any resources it is holding.
    ////////////////////////////////////////////////////////////////////////////////

    virtual void handleShutdown () {}

  protected:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief read socket
    ////////////////////////////////////////////////////////////////////////////////

    socket_t readSocket;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief write socket
    ////////////////////////////////////////////////////////////////////////////////

    socket_t writeSocket;
  };

}

#endif
