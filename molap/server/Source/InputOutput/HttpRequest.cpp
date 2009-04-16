////////////////////////////////////////////////////////////////////////////////
/// @brief http request
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

#include "InputOutput/HttpRequest.h"

#include <iostream>

#include "Collections/StringBuffer.h"

#include "InputOutput/Logger.h"

namespace palo {
  HttpRequest::HttpRequest (const string& header) {
    type        = HTTP_REQUEST_ILLEGAL;
    requestPath = "";
    
    // 1. split header into lines at "\r\n" 
    // 2. split lines at " "
    // 3. split GET/POST etc. requests    
    
    //
    // check for '\n' (we check for '\r' later)
    //

    const char CR  = '\r';
    const char NL  = '\n';
    const char SPC = ' ';

    int lineNum   = 0;     
    size_t start  = 0;
    size_t end    = header.find(NL);
    
    while (end != string::npos) {
      size_t len = end - start;
    
      //
      // check for '\r'
      //

      char c = header[end - 1];    

      if (end > start && c == CR) {
        len--;
      }
    
      if (len > 0) {
        string line = header.substr(start, len);
  
        //
        // split line at spaces
        //

        size_t e = line.find(SPC);

        if (e != string::npos) {
          if (e > 1) {
            string key   = line.substr(0, e);
            string value = line.substr(e+1, len-e);
            headerFields[key] = value;
  
            // check for request type (GET/POST in line 0),
            // path and parameters
            if (lineNum == 0) {
              if (key == "POST") {
                type = HTTP_REQUEST_POST;
              }
              else if (key == "GET") {
                type = HTTP_REQUEST_GET;
              }
  
              if (type != HTTP_REQUEST_ILLEGAL) {

                // delete "HTTP*" from request
                e = value.find(SPC);
  
                if (e != string::npos) {
                  value = value.substr(0, e);
                  headerFields[key] = value;
                }
    
                // split requestPath and parameters
                e = value.find('?');
          
                if (e != string::npos) {
                  requestPath = value.substr(0, e);
                  value       = value.substr(e + 1);

                  setKeyValue(value);
                }
                else {
  
                  //
                  // request without parameters
                  //
    
                  requestPath = value;
                }
              }
            }
          }
        }
        else {
          headerFields[line] = "";
        }
      
        start = end + 1;
      }
      //else {
      //  // ignore empty lines in header       
      //  Logger::warning << "HttpRequest: found empty line in header" << endl;
      //}
    
      end = header.find(NL, end + 1);
      lineNum++;
    }
  }



  void HttpRequest::setBody (const string& body) {
    setKeyValue(body);
  }



  bool HttpRequest::hasHeaderField (const string& field) const {
    map<string,string>::const_iterator i = headerFields.find(field);

    if (i != headerFields.end()) {
      return true;
    }
    else {
      return false;
    }
  }



  const string& HttpRequest::getHeaderField (const string& field) const {
    static const string error = "";

    map<string,string>::const_iterator i = headerFields.find(field);

    if (i != headerFields.end()) {
      return i->second;
    }
    else {
      return error;
    }    
  }



  const vector<string>& HttpRequest::getKeys () const {
    return requestKeys;
  }



  const string& HttpRequest::getValue (const string& key) const {
    static const string error = "";

    map<string,string>::const_iterator i = requestFields.find(key);

    if (i != requestFields.end()) {
      return i->second;
    }
    else {
      return error;
    }    
  }
  


  void HttpRequest::setKeyValue (const string& params) {
    StringBuffer key;
    StringBuffer value;

    key.initialize();
    key.reserve(params.size() + 1);

    value.initialize();
    value.reserve(params.size() + 1);

    enum { KEY, VALUE } phase = KEY;
    enum { NORMAL, HEX1, HEX2 } reader = NORMAL;

    int hex = 0;

    const char AMB     = '&';
    const char EQUAL   = '=';
    const char PERCENT = '%';
    const char PLUS    = '+';
    const char SPC     = ' ';

    const char * buffer = params.c_str();
    const char * end = buffer + params.size();

    for (; buffer < end; buffer++) {
      char next = *buffer;

      if (phase == KEY && next == EQUAL) {
        phase = VALUE;
        continue;
      }
      else if (next == AMB) {
        phase = KEY;

        key.appendChar0(0);
        string keyStr(key.c_str(), key.length() - 1);
        key.clear();

        value.appendChar0(0);
        string valueStr(value.c_str(), value.length() - 1);
        value.clear();

        map<string,string>::const_iterator i = requestFields.find(keyStr);

        if (i == requestFields.end()) {
          requestKeys.push_back(keyStr);
        }

        requestFields[keyStr] = valueStr;
      
        key = "";
        value = "";
        continue;
      }
      else if (next == PERCENT) {
        reader = HEX1;
        continue;
      }
      else if (reader == HEX1) {
        hex = hex2int(next) * 16;
        reader = HEX2;
        continue;
      }
      else if (reader == HEX2) {
        hex += hex2int(next);
        reader = NORMAL;
        next = (char) hex;
      }
      else if (next == PLUS) {
        next = SPC;
      }

      /*
      if (next == NL) {
        next = SPC;
      }
      else if (next == CR) {
        next = SPC;
      }
      */
      
      if (phase == KEY) {
        key.appendChar0(next);
      }
      else {
        value.appendChar0(next);
      }
    }

    if (! key.empty()) {
      key.appendChar0(0);
      string keyStr(key.c_str(), key.length() - 1);

      value.appendChar0(0);
      string valueStr(value.c_str(), value.length() - 1);

      map<string,string>::const_iterator i = requestFields.find(keyStr);

      if (i == requestFields.end()) {
         requestKeys.push_back(keyStr);
      }

      requestFields[keyStr] = valueStr;
    }

    key.free();
    value.free();
  }
  


  int HttpRequest::hex2int (char ch) {
    if ('0' <= ch && ch <= '9') {
      return ch - '0';
    }
    else if ('A' <= ch && ch <= 'F') {
      return ch - 'A' + 10;
    }
    else if ('a' <= ch && ch <= 'f') {
      return ch - 'a' + 10;
    }

    Logger::warning << "'" << ch << "' is not a hex number" << endl;

    return 0;
  }
  
  
  
  size_t HttpRequest::getContentLegth () const {
    const string& contentLengthString = getHeaderField("Content-Length:");
    if (contentLengthString != "") {
      char *p;    
      long int result = strtol(contentLengthString.c_str(), &p, 10);    
      if (*p != '\0') {
        return 0;
      }
      return result;
    }
    return 0;    
  }
}
