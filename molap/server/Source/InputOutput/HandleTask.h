////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for tasks using Windows HANDLE
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

#ifndef INPUT_OUTPUT_HANDLE_TASK_H
#define INPUT_OUTPUT_HANDLE_TASK_H 1

#include "palo.h"

#include "Collections/StringBuffer.h"

#include "InputOutput/Task.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief abstract base class for tasks using Windows HANDLE
  ///
  /// This abstract class is the base class for tasks using Windows
  /// HANDLE. Under Windows there is a difference between file descriptors and
  /// sockets. Therefore the standard IO tasks cannot be used to communicate
  /// with a pipe.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HandleTask : virtual public Task {
  public:
    HandleTask (HANDLE readHandle, HANDLE writeHandle);

        ~HandleTask ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the read handle or INVALID_HANDLE
    ///
    /// This function is used by the scheduler to get the underlying read handle.
    ////////////////////////////////////////////////////////////////////////////////

    virtual HANDLE getReadHandle () {return readHandle;}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to check if the task wants to read
    ///
    /// This function is used by the scheduler to check if the task is ready to
    /// receive new data. The scheduler will only check connection for new data
    /// if this function returns true.
    ////////////////////////////////////////////////////////////////////////////////

    virtual size_t canHandleRead ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate available data
    ///
    /// This callback is called by the scheduler if there is new data
    /// available. The task must try to read data from the connection in a
    /// non-blocking way.
    ////////////////////////////////////////////////////////////////////////////////

    virtual bool handleRead (size_t) = 0;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the write handle or INVALID_HANDLE
    ///
    /// This function is used by the scheduler to get the underlying write handle.
    ////////////////////////////////////////////////////////////////////////////////

    virtual HANDLE getWriteHandle () {
      return writeHandle;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to check if task wants to write
    ///
    /// This function is used by the scheduler to check if the task is ready to
    /// send data. The scheduler will only check connection for writability if
    /// this function returns true.
    ////////////////////////////////////////////////////////////////////////////////

    virtual bool canHandleWrite () {
      return writeBuffer != 0;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate that data can be written
    ///
    /// This callback is called by the scheduler if the connection can consume
    /// data. The task must try to write data to the connection in a
    /// non-blocking way.
    ////////////////////////////////////////////////////////////////////////////////

    virtual bool handleWrite ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate hangup
    ///
    /// This callback is called by the scheduler if the connection has been
    /// closed.
    ////////////////////////////////////////////////////////////////////////////////

    virtual void handleHangup (HANDLE fd) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate shutdown in progess
    ///
    /// This callback is called by the scheduler to indicate that the scheduler
    /// is shutting done. The task should release any resources it is holding.
    ////////////////////////////////////////////////////////////////////////////////

    virtual void handleShutdown () {}

  protected:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief fills the read buffer
    ///
    /// The function should be called by input task if the scheduler has
    /// indicated that new data is available. It will return true, if data could
    /// be read and false if the connection has been closed.
    ////////////////////////////////////////////////////////////////////////////////

    bool fillReadBuffer (size_t);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks for presence of an active write buffer
    ////////////////////////////////////////////////////////////////////////////////

    bool hasWriteBuffer () const;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets an active write buffer
    ////////////////////////////////////////////////////////////////////////////////

    void setWriteBuffer (StringBuffer*, bool ownBuffer = true);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called if write buffer has been sent
    ///
    /// This called is called if the current write buffer has been sent
    /// completely to the client.
    ////////////////////////////////////////////////////////////////////////////////

    virtual void completedWriteBuffer () = 0;

  protected:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief read handle
    ////////////////////////////////////////////////////////////////////////////////

    HANDLE readHandle;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief write handle
    ////////////////////////////////////////////////////////////////////////////////

    HANDLE writeHandle;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief read buffer
    ///
    /// The function fillReadBuffer stores the data in this buffer.
    ////////////////////////////////////////////////////////////////////////////////

    StringBuffer readBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief the current write buffer
    ////////////////////////////////////////////////////////////////////////////////

    StringBuffer * writeBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief if true, the resource writeBuffer is owned by the write task
    ///
    /// If true, the writeBuffer is deleted as soon as it has been sent to the
    /// client. If false, the writeBuffer is keep alive.
    ////////////////////////////////////////////////////////////////////////////////

    bool ownBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief number of bytes already written
    ////////////////////////////////////////////////////////////////////////////////

    size_t writeLength;
  };

}

#endif
