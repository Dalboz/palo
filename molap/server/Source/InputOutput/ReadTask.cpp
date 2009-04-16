////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for input tasks
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

#include "InputOutput/ReadTask.h"

#include <iostream>

#include "InputOutput/Logger.h"

namespace palo {
  using namespace std;



  ReadTask::ReadTask (socket_t readSocket) 
    : IoTask(readSocket, INVALID_SOCKET) {
    readBuffer.initialize();
  }



  ReadTask::~ReadTask () {
    readBuffer.free();
  }



  bool ReadTask::fillReadBuffer () {
    char buffer [100000];

#if defined(_MSC_VER)
    int nr = recv(readSocket, buffer, sizeof(buffer), 0);
#else
    int nr = read(readSocket, buffer, sizeof(buffer));
#endif

    if (nr > 0) {
      readBuffer.appendText(buffer, nr);
    }
    else if (nr == 0) {
      return false;
    }
    else if (errno_socket == EINTR_SOCKET) {
      return fillReadBuffer();
    }
    else if (errno_socket != EWOULDBLOCK_SOCKET) {
      Logger::debug << "read failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                   << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;

      return false;
    }

    return true;
  }
}
