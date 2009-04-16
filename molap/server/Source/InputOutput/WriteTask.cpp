////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for output tasks
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

#include "InputOutput/WriteTask.h"

#include <iostream>

#include "Collections/StringBuffer.h"
#include "InputOutput/Logger.h"

namespace palo {
  using namespace std;



  WriteTask::WriteTask (socket_t writeSocket) 
    : IoTask(INVALID_SOCKET, writeSocket), writeBuffer(0), ownBuffer(true), writeLength(0) {
  }



  WriteTask::~WriteTask () {
    if (writeBuffer != 0) {
      if (ownBuffer) {
        writeBuffer->free();
        delete writeBuffer;
      }
    }
  }



  bool WriteTask::canHandleWrite () {
    if (writeBuffer == 0) {
      return false;
    }

    if (writeBuffer->length() <= writeLength) {
      return false;
    }

    return true;
  }



  bool WriteTask::handleWrite () {
    if (writeBuffer == 0) {
      return true;
    }

    size_t len = writeBuffer->length() - writeLength;
    int nr = 0;

    if (0 < len) {
#if defined(_MSC_VER)
      nr = send(writeSocket, writeBuffer->begin() + writeLength, (int) len, 0);
#else
      nr = write(writeSocket, writeBuffer->begin() + writeLength, (int) len);
#endif

      if (nr < 0) {
        if (errno_socket == EINTR_SOCKET) {
          return handleWrite();
        }
        else if (errno_socket != EWOULDBLOCK_SOCKET) {
          Logger::debug << "write failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                       << " with " << errno_socket << " (" << strerror_socket(errno_socket) << ")" << endl;

          return false;
        }
        else {
          nr = 0;
        }
      }

      len -= nr;
    }

    if (len == 0) {
      if (ownBuffer) {
        writeBuffer->free();
        delete writeBuffer;
      }

      writeBuffer = 0;
      completedWriteBuffer();
    }
    else {
      writeLength += nr;
    }

    return true;
  }



  bool WriteTask::hasWriteBuffer () const {
    return writeBuffer != 0;
  }



  void WriteTask::setWriteBuffer (StringBuffer* buffer, bool ownBuffer) {
    writeLength = 0;

    if (buffer->empty()) {
      if (ownBuffer) {
        buffer->free();
        delete buffer;
      }

      writeBuffer = 0;
      completedWriteBuffer();
    }
    else {
      if (writeBuffer != 0) {
        if (this->ownBuffer) {
          writeBuffer->free();
          delete writeBuffer;
        }
      }

      writeBuffer     = buffer;
      this->ownBuffer = ownBuffer;
    }
  }
}
