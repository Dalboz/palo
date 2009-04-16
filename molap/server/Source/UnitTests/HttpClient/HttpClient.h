////////////////////////////////////////////////////////////////////////////////
/// @brief http client
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
/// @author Frank Celler
/// @author Copyright 2006, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef HTTP_CLIENT_HTTP_CLIENT_H
#define HTTP_CLIENT_HTTP_CLIENT_H 1

#include <string>
#include <vector>

#ifdef _MSC_VER
#include <Winsock2.h>
#include <Ws2tcpip.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// socket type
////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
typedef SOCKET socket_t;
#else
typedef int socket_t;
#endif

struct hostent;

namespace triagens {
  using namespace std;

  class HttpClient {
  public:
    enum ErrorType {
      HTTP_CLIENT_NO_ERROR,
      CONNECTION_CLOSED,
      CANNOT_RESOLVE_HOSTNAME,
      CANNOT_CREATE_SOCKET,
      CANNOT_CONNECT,
      WRITE_FAILED,
      READ_FAILED,
      NON_BLOCKING_FAILED,
      NO_DELAY_FAILED,
    };

  public:
    HttpClient (const string& host, const string& port);
    HttpClient (const string& host, unsigned int port);

  public:
    bool connect ();
    bool get (const string& path, const string& parameters);
    bool post (const string& path, const string& parameters);

  public:
    const string& getLastErrorMessage () const {
      return lastErrorMessage;
    }

    ErrorType getLastError () const {
      return lastError;
    }

    const string& getBody () const {
      return resultBody;
    }

    const string& getResponse () const {
      static const string empty;

      if (resultHeader.empty()) {
        return empty;
      }
      else {
        return resultHeader[0];
      }
    }

  private:
    void setLastErrorMessage ();
    bool sendGet (const string& path, const string& parameters);
    bool sendPost (const string& path, const string& parameters);
    bool send (const string& message);
    bool receive ();
    void processInput (const char * buffer, size_t length);
    void resolveHostname ();

  private:
    ErrorType lastError;
    string lastErrorMessage;

    struct ::hostent * sheep;

    string host;
    unsigned int port;
    bool connected;

    socket_t fd;

    bool readingHeader;

    string resultString;
    vector<string> resultHeader;
    string resultBody;

    size_t bodyLength;
  };

}

#endif
