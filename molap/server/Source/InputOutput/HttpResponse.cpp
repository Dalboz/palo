////////////////////////////////////////////////////////////////////////////////
/// @brief http response
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

#include "InputOutput/HttpResponse.h"

extern "C" {
#include <time.h>
}



namespace palo {
  HttpResponse::HttpResponse (HttpResponseCode code)
    : code(code), contentType("text/plain;charset=utf-8"), deferred(false) {
    header.initialize();
    body.initialize();
  }



  HttpResponse::HttpResponse (semaphore_t semaphore, IdentifierType clientData)
    : code(NOT_IMPLEMENTED),
      contentType("text/plain;charset=utf-8"), 
      deferred(true),
      semaphore(semaphore),
      clientData(clientData)
  {
    header.initialize();
    body.initialize();
  }



  HttpResponse::~HttpResponse () {
    header.free();
    body.free();
  }



  string HttpResponse::getResponseString (HttpResponseCode code) {
    switch (code) {

      //  Success 2xx
      case OK:              return "200 OK";
      case CREATED:         return "201 Created";
      case ACCEPTED:        return "202 Accepted";
      case PARTIAL:         return "203 Partial Information";
      case NO_RESPONSE:     return "204 No Response";
        
      //  Redirection 3xx
      case MOVED:           return "301 Moved";
      case FOUND:           return "302 Found";
      case METHOD:          return "303 Method";
      case NOT_MODIFIED:    return "304 Not Modified";
        
      //  Error 4xx, 5xx
      case BAD:             return "400 Bad Request";
      case UNAUTHORIZED:    return "401 Unauthorized";
      case PAYMENT:         return "402 Payment Required";
      case FORBIDDEN:       return "403 Forbidden";
      case NOT_FOUND:       return "404 Not Found";
      case ERROR_CODE:      return "500 Internal Error";
      case NOT_IMPLEMENTED: return "501 Not implemented";
        
      // default  
      default:              return "404 Not Found";
    }
  }



  const StringBuffer& HttpResponse::getHeader () {
    static string CRNL = "\r\n";
    
    header.empty();
    header.appendText("HTTP/1.1 ");
    header.appendText(getResponseString(code));
    header.appendText(CRNL);
    
#ifdef USE_DATE_HTTP_HEADER
    // TODO use english locale for strftime
    time_t tt = time(0);
    struct tm* t = gmtime(&tt);
    int max = 32;
    char * s = new char[max];    
    strftime(s,  max, "%a, %d %b %Y %H:%M:%S GMT", t);    
    header.appendText("Date: ");    
    header.appendText(s);
    header.appendText(CRNL);
    delete[] s;
#endif

    header.appendText("Server: ");
    header.appendText("Palo");
    header.appendText(CRNL);
    
    header.appendText("Content-Type: ");
    header.appendText(contentType);
    header.appendText(CRNL);

    header.appendText("Content-Length: ");
    header.appendInteger((uint32_t) body.length());
    header.appendText(CRNL);
    
    header.appendText("Connection: Keep-Alive");
    header.appendText(CRNL);

    if (! tokenName.empty()) {
      header.appendText(tokenName);
      header.appendText(": ");
      header.appendInteger((uint32_t) tokenValue);
      header.appendText(CRNL);
    }

    if (! secondTokenName.empty()) {
      header.appendText(secondTokenName);
      header.appendText(": ");
      header.appendInteger((uint32_t) secondTokenValue);
      header.appendText(CRNL);
    }

    header.appendText(CRNL);

    return header;
  }



  StringBuffer& HttpResponse::getBody () {
    return body;
  }
}
