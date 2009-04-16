////////////////////////////////////////////////////////////////////////////////
/// @brief read/write task of a https server
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

#include "HttpsServer/HttpsServerTask.h"

#include <openssl/err.h>

#include <iostream>

#include "HttpsServer/PaloHttpsServer.h"

namespace palo {

  /////////////////////
  // SSL help functions
  /////////////////////

  static string lastSSLError () {
    char buf[121];

    unsigned long err = ERR_get_error();
    string errstr = ERR_error_string(err, buf);

    return errstr;
  }

  ///////////////////////////////
  // constructors and destructors
  ///////////////////////////////
  
  HttpsServerTask::HttpsServerTask (socket_t fd,
                                    PaloHttpsServer* server,
                                    Scheduler* scheduler,
                                    SSL* sslParam,
                                    BIO* bioParam)
    : IoTask(fd, fd),
      ReadTask(fd),
      WriteTask(fd),
      HttpServerTask(fd, server, scheduler),
      accepted(false),
      readBlocked(false),
      readBlockedOnWrite(false),
      writeBlockedOnRead(false),
      shutdownWait(false),
      server(server),
      scheduler(scheduler),
      ssl(sslParam),
      bio(bioParam) {
  }



  HttpsServerTask::~HttpsServerTask () {
    // SSL_free(ssl);
    // BIO_free_all(bio);
  }

  ////////////////////////
  // communication methods
  ////////////////////////

  bool HttpsServerTask::fillReadBuffer () {

    // is the handshake already done?
    if (! accepted) {
      if (! trySSLAccept()) {
        Logger::info << "failed to established SSL connection" << endl;
        return false;
      }

      return true;
    }

    // check if read is blocked by write
    if (writeBlockedOnRead) {
      return trySSLWrite();
    }

    return trySSLRead();
  }



  bool HttpsServerTask::canHandleWrite () {
    if (readBlockedOnWrite) {
      return true;
    }

    if (writeBuffer == 0) {
      return false;
    }

    if (writeBuffer->length() <= writeLength) {
      return false;
    }

    return true;
  }



  bool HttpsServerTask::handleWrite () {

    // is the handshake already done?
    if (! accepted) {
      if (! trySSLAccept()) {
        Logger::info << "failed to established SSL connection" << endl;
        return false;
      }

      return true;
    }

    // check if write is blocked by read
    if (readBlockedOnWrite) {
      if (! trySSLRead()) {
        return false;
      }

      return processRead();
    }

    return trySSLWrite();
  }

  /////////////////////////
  // private helper methods
  /////////////////////////

  bool HttpsServerTask::trySSLAccept () {
    int res = SSL_accept(ssl);

    // accept successful
    if (res == 1) {
      Logger::info << "established SSL connection" << endl;
      accepted = true;
      return true;
    }

    // shutdown of connection
    else if (res == 0) {
      return false;
    }

    // maybe we need more data
    else {
      int err = SSL_get_error(ssl, res);

      if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        return true;
      }
      else {
        Logger::info << "error in SSL handshake" << endl;
        Logger::info << lastSSLError() << endl;
        return false;
      }
    }
  }


  bool HttpsServerTask::trySSLRead () {
    char buffer[100000];

    readBlocked = false;
    readBlockedOnWrite = false;

    int nr = SSL_read(ssl, buffer, sizeof(buffer));

    switch (SSL_get_error(ssl, nr)) {
      case SSL_ERROR_NONE:
        readBuffer.appendText(buffer, nr);
        break;

      case SSL_ERROR_ZERO_RETURN:
        if (! shutdownWait) {
          SSL_shutdown(ssl);
        }

        return false;
        break;

      case SSL_ERROR_WANT_READ:
        readBlocked = true;
        break;

      case SSL_ERROR_WANT_WRITE:
        readBlockedOnWrite = true;
        break;

      default:
        Logger::info << lastSSLError() << endl;
        return false;
    }

    return true;
  }



  bool HttpsServerTask::trySSLWrite () {

    // if no write buffer is left, return
    if (writeBuffer == 0) {
      return true;
    }

    // write buffer to SSL connection
    size_t len = writeBuffer->length() - writeLength;
    int nr = 0;
    
    if (0 < len) {
      writeBlockedOnRead = false;
      nr = SSL_write(ssl, writeBuffer->begin() + writeLength, (int) len);

      switch (SSL_get_error(ssl, nr)) {
        case SSL_ERROR_NONE:
          len -= nr;
          break;

        case SSL_ERROR_WANT_WRITE:
          return false;

        case SSL_ERROR_WANT_READ:
          writeBlockedOnRead = true;
          return true;

        default:
          Logger::info << lastSSLError() << endl;
          return false;
      }
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
}
