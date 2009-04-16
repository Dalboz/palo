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

#define _CRT_SECURE_NO_DEPRECATE 1

#include "HttpClient.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>

#ifdef _MSC_VER

////////////////////////////////////////////////////////////////////////////////
// defines for windows
////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>

#define usleep(v) Sleep(v/1000)
#define errno_socket WSAGetLastError()
#define EINTR_SOCKET WSAEINTR
#define EWOULDBLOCK_SOCKET WSAEWOULDBLOCK
#define snprintf _snprintf

static bool SetNonBlockingSocket (socket_t fd) {
  DWORD ul = 1;

  return ioctlsocket(fd, FIONBIO, &ul) == SOCKET_ERROR ? false : true;
}

#else

////////////////////////////////////////////////////////////////////////////////
// defines for unix
////////////////////////////////////////////////////////////////////////////////

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#define errno_socket errno
#define EINTR_SOCKET EINTR
#define EWOULDBLOCK_SOCKET EWOULDBLOCK
#define closesocket close

static bool SetNonBlockingSocket (socket_t fd) {
  int flags = fcntl(fd, F_GETFL, 0);

  if (flags < 0) {
    return false;
  }

  flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  
  if (flags < 0) {
    return false;
  }

  return true;
}

#endif

namespace triagens {
  using namespace std;



  HttpClient::HttpClient (const string& host, unsigned int port) 
    : lastError(HTTP_CLIENT_NO_ERROR), sheep(0), host(host), port(port), connected(false) {
    resolveHostname();
  }



  HttpClient::HttpClient (const string& host, const string& port) 
    : lastError(HTTP_CLIENT_NO_ERROR), sheep(0), host(host), port(atoi(port.c_str())), connected(false) {
    resolveHostname();
  }


  void HttpClient::resolveHostname () {
    sheep = ::gethostbyname(host.c_str());

    if (sheep == 0) {
      lastError = CANNOT_RESOLVE_HOSTNAME;
      setLastErrorMessage();
    }
  }



  bool HttpClient::connect () {
    if (sheep == 0) {
      return false;
    }

    // open a socket
    fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
      lastError = CANNOT_CREATE_SOCKET;
      setLastErrorMessage();
      return false;
    }

    // and try to connect the socket to the given address
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy(&(saddr.sin_addr.s_addr), sheep->h_addr, sheep->h_length);
    saddr.sin_port = htons(port);
  
    int res = ::connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));

    if (res < 0) {
      ::closesocket(fd);
      lastError = CANNOT_CONNECT;
      setLastErrorMessage();
      return false;
    }

    // set socket to non-blocking
    bool ok = SetNonBlockingSocket(fd);

    if (! ok) {
      ::closesocket(fd);
      lastError = NON_BLOCKING_FAILED;
      setLastErrorMessage();
      return false;
    }

    // disable nagle's algorithm
    int n = 1;
    res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&n, sizeof(n));

    if (res != 0 ) {
      ::closesocket(fd);
      lastError = NO_DELAY_FAILED;
      setLastErrorMessage();
      return false;
    }

    // we have a connection
    connected = true;

    return true;
  }



  void HttpClient::setLastErrorMessage () {
#ifdef _MSC_VER
    switch (lastError) {
      case HTTP_CLIENT_NO_ERROR: lastErrorMessage = "no error"; break;
      case CONNECTION_CLOSED: lastErrorMessage = "connection closed"; break;
      case CANNOT_RESOLVE_HOSTNAME: lastErrorMessage = "cannot resolve hostname"; break;
      case CANNOT_CREATE_SOCKET: lastErrorMessage = "cannot create socket"; break;
      case CANNOT_CONNECT: lastErrorMessage = "cannot connect"; break;
      case WRITE_FAILED: lastErrorMessage = "write failed"; break;
      case READ_FAILED: lastErrorMessage = "read failed"; break;
      case NON_BLOCKING_FAILED: lastErrorMessage = "cannot set socket to non-blocking"; break;
      case NO_DELAY_FAILED: lastErrorMessage = "cannot disable nagle's algorithm"; break;
    }
#else
    lastErrorMessage = strerror(errno_socket);
#endif
  }



  bool HttpClient::get (const string& path, const string& parameters) {
    if (! connected) {
      return false;
    }

    bool ok = sendGet(path, parameters);

    if (! ok) {
      return false;
    }

    return receive();
  }



  bool HttpClient::post (const string& path, const string& parameters) {
    if (! connected) {
      return false;
    }

    bool ok = sendPost(path, parameters);

    if (! ok) {
      return false;
    }

    return receive();
  }



  bool HttpClient::sendGet (const string& path, const string& parameters) {
    string message = "GET " + path;

    if (! parameters.empty()) {
      message += "?" + parameters;
    }

    message += " HTTP/1.1\r\nContent-Length: 0\r\nHost: " + host + "\r\n\r\n";
    return send(message);
  }



  bool HttpClient::sendPost (const string& path, const string& parameters) {
    char buffer[1024];

    string message = "POST " + path;

    snprintf(buffer, sizeof(buffer)-1, "%d", (int) parameters.size());

    message += " HTTP/1.1\r\nContent-Length: ";
    message += buffer;
    message += "\r\nHost: " + host + "\r\n\r\n" + parameters;
    return send(message);
  }



  bool HttpClient::send (const string& message) {
    const char * ptr = message.c_str();
    size_t length = message.length();

    while (0 < length) {
      int nr = ::send(fd, ptr, (int) length, 0);
      
      if (nr < 0) {
        if (errno_socket == EINTR_SOCKET) {
        }
        else if (errno_socket == EWOULDBLOCK_SOCKET) {
          // add timeout here, if necessary
          usleep(100);
        }
        else {
          ::closesocket(fd);
          lastError = WRITE_FAILED;
          setLastErrorMessage();
          connected = false;
          return false;
        }
      }
      else {
        ptr    += nr;
        length -= nr;
      }
    }

    return true;
  }



  bool HttpClient::receive () {
    resultString.clear();
    resultHeader.clear();
    resultBody.clear();

    bodyLength = 0;

    readingHeader = true;

    while (1) {
      char buffer [100000];
      int need = sizeof(buffer);

      if (! readingHeader) {
        int missing = (int) (bodyLength - resultBody.size());

        if (missing == 0) {
          break;
        }

        if (missing < need) {
          need = missing;
        }
      }


      int nr = ::recv(fd, buffer, need, 0);

      if (nr < 0) {
        if (errno_socket == EINTR_SOCKET) {
        }
        else if (errno_socket == EWOULDBLOCK_SOCKET) {
          // add timeout here, if necessary
          usleep(100);
        }
        else {
          ::closesocket(fd);
          lastError = READ_FAILED;
          setLastErrorMessage();
          connected = false;
          return false;
        }
      }
      else if (nr == 0) {
        ::closesocket(fd);
        lastError = READ_FAILED;
        setLastErrorMessage();
        connected = false;
        return false;
      }
      else {
        processInput(buffer, nr);
      }
    }

    return true;
  }



  void HttpClient::processInput (const char * buffer, size_t length) {
    if (readingHeader) {
      resultString.append(buffer, length);

      size_t pos  = resultString.find("\r\n");
      size_t last = 0;

      while (pos != string::npos) {
        if (pos == last) {
          readingHeader = false;

          if (0 < last) {
            resultBody.append(resultString.substr(last + 2));
          }

          resultString.clear();

          return;
        }
        else {
          string line = resultString.substr(last, pos - last);

          resultHeader.push_back(line);

          last = pos + 2;
          pos  = resultString.find("\r\n", last);

          if (line.compare(0, 15, "Content-Length:", 15) == 0) {
            size_t numpos = line.find_first_not_of(" \r", 15);

            if (numpos == string::npos) {
              bodyLength = 0;
            }
            else {
              bodyLength = ::atoi(line.substr(numpos).c_str());
            }
          }
        }
      }

      if (0 < last) {
        resultString = resultString.substr(last);
      }
    }
    else {
      resultBody.append(buffer, length);
    }
  }
}
