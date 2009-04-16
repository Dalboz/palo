////////////////////////////////////////////////////////////////////////////////
/// @brief http client test
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

#include "HttpClient.h"

#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;
using namespace triagens;

#ifdef _MSC_VER
#define usleep(v) Sleep(v/1000)
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief prints a short usage message
////////////////////////////////////////////////////////////////////////////////

void usage (const string& name) {
  cerr << "usage: " << name << " <host> <port>" << endl;
  exit(1);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief reads urls from stdin and requests them
////////////////////////////////////////////////////////////////////////////////

int main (int argc, char * argv []) {
  if (argc != 3) {
    usage(argv[0]);
  }

#ifdef _MSC_VER
    WSADATA wsaData;

    WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

  HttpClient httpClient(argv[1], argv[2]);

  bool ok = httpClient.connect();

  if (! ok) {
    cerr << "cannot connect to " << argv[1] << " port " << argv[2]
         << ", error " << httpClient.getLastErrorMessage() << endl;
    exit(1);
  }

  cout << "connected to " << argv[1] << " on port " << argv[2] << endl;

  while (1) {
    string line;
    getline(cin, line);

    if (cin.eof() || line.empty()) {
      break;
    }

    size_t pos = line.find_first_of("?");

    string path;
    string parameters;

    if (pos == string::npos) {
      path = line;
    }
    else {
      path = line.substr(0, pos);
      parameters = line.substr(pos + 1);
    }

    ok = httpClient.post(path, parameters);

    if (! ok) {
      cerr << "cannot retrieve url '" << line << "'"
           << ", error " << httpClient.getLastErrorMessage() << endl;

      if (httpClient.getLastError() == HttpClient::CONNECTION_CLOSED) {
	usleep(2000);

        ok = httpClient.connect();

        if (! ok) {
          cerr << "cannot reconnect, error " << httpClient.getLastErrorMessage() << endl;
          exit(1);
        }
        else {
          cerr << "reconnect successful" << endl;

	  ok = httpClient.post(path, parameters);

	  if (! ok) {
	    cerr << "cannot retrieve url, giving up" << endl;
	    exit(1);
	  }
        }
      }
      else {
        exit(1);
      }
    }
    else {
#if 1
      cout << "URL '" << line
           << "', result " << httpClient.getResponse() << "\n"
           << httpClient.getBody() << "-------------------------------------"
           << endl;
#endif
    }
  }

#ifdef _MSC_VER
    WSACleanup();
#endif
}
