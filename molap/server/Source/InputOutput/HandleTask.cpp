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

#include "InputOutput/HandleTask.h"

#include "InputOutput/Logger.h"

namespace palo {
  HandleTask::HandleTask (HANDLE readHandle, HANDLE writeHandle)
    : readHandle(readHandle), writeHandle(writeHandle) {
    readBuffer.initialize();
    writeBuffer = 0;
  }



  HandleTask::~HandleTask () {
    readBuffer.free();

    if (writeBuffer != 0) {
      if (ownBuffer) {
        writeBuffer->free();
        delete writeBuffer;
      }
    }
  }



  size_t HandleTask::canHandleRead () {
    DWORD available;
    BOOL ok = PeekNamedPipe(readHandle,
                            0,
                            0,
                            0,
                            &available,
                            0);

    if (ok == FALSE) {
      return 0;
    }

    return (size_t) available;
  }



  bool HandleTask::fillReadBuffer (size_t size) {
    if (size == 0) {
      return false;
    }

    char buffer[1024];
    DWORD nr = 0;

    if (sizeof(buffer) < size) {
      size = sizeof(buffer);
    }

    if (! ReadFile(readHandle, buffer, (DWORD) size, &nr, 0)) {
      Logger::debug << "ReadFile failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                   << " with " << errno_socket << " (" << strerror_socket(GetLastError()) << ")" << endl;

      return false;
    }

    readBuffer.appendText(buffer, nr);

    return true;
  }



  bool HandleTask::handleWrite () {
    if (writeBuffer == 0) {
      return true;
    }

    size_t len = writeBuffer->length() - writeLength;
    int nr = 0;

    if (0 < len) {
      DWORD result;
      BOOL ok = WriteFile(writeHandle, writeBuffer->begin() + writeLength, (int) len, &result, 0);

      if (ok == FALSE) {
        Logger::debug << "WriteFile failed in " << __FUNCTION__ << "(" << __FILE__ << "@" << __LINE__ << ")" 
                     << " with " << errno_socket << " (" << strerror_socket(GetLastError()) << ")" << endl;

        return false;
      }

      len -= result;
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



  bool HandleTask::hasWriteBuffer () const {
    return writeBuffer != 0;
  }



  void HandleTask::setWriteBuffer (StringBuffer* buffer, bool ownBuffer) {
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
