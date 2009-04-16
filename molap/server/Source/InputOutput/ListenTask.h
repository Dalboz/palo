////////////////////////////////////////////////////////////////////////////////
/// @brief tasks used to establish connections
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

#ifndef INPUT_OUTPUT_LISTEN_TASK_H
#define INPUT_OUTPUT_LISTEN_TASK_H 1

#include "palo.h"

#include "InputOutput/IoTask.h"
#include "InputOutput/TimerTask.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief task used to establish connections
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ListenTask : virtual public IoTask, virtual public TimerTask {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief standard constructor
    ////////////////////////////////////////////////////////////////////////////////

    ListenTask () 
      : IoTask(INVALID_SOCKET, INVALID_SOCKET) {
    }

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets listen socket
    ////////////////////////////////////////////////////////////////////////////////

    void setReadSocket (socket_t fd) {
      readSocket = fd;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by scheduler to indicate connection request
    ////////////////////////////////////////////////////////////////////////////////

    bool handleRead ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called by task to indicate connection success
    ////////////////////////////////////////////////////////////////////////////////

    virtual void handleConnected (socket_t fd) = 0 ;
  };

}

#endif
