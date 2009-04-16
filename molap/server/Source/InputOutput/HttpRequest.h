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

#ifndef INPUT_OUTPUT_HTTP_REQUEST_H
#define INPUT_OUTPUT_HTTP_REQUEST_H 1

#include "palo.h"

#include <string>
#include <vector>
#include <map>

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief http request
  ///
  /// The http server reads the request string from the client and converts it
  /// into an instance of this class. An http request object provides methods to
  /// inspect the header and parameter fields.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HttpRequest {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief http request type
    ////////////////////////////////////////////////////////////////////////////////

    enum HttpRequestType {
      HTTP_REQUEST_GET,
      HTTP_REQUEST_POST,
      HTTP_REQUEST_ILLEGAL
    };

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief http request
    ///
    /// Constructs a http request given the header string. A client request
    /// consists of two parts: the header and the body. For a GET request the
    /// body is always empty and all information about the request is delivered
    /// in the header. For a POST request some information is also delivered in
    /// the body. However, it is necessary to parse the header information,
    /// before the body can be read.
    ////////////////////////////////////////////////////////////////////////////////

    HttpRequest (const string& header);

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the http request type
    ///
    /// A request is either a GET or a POST request.
    ////////////////////////////////////////////////////////////////////////////////

    HttpRequestType getRequestType() const {
      return type;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the path of the request
    ///
    /// The path consists of the URL without the host and without any parameters.
    ////////////////////////////////////////////////////////////////////////////////

    const string& getRequestPath () const { 
      return requestPath; 
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets the body
    ///
    /// In a POST request the body contains additional key/value pairs. The
    /// function parses the body and adds the corresponding pairs.
    ////////////////////////////////////////////////////////////////////////////////

    void setBody (const string& body);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks for a field
    ///
    /// Returns true if the requests contains a header field with given name.
    ////////////////////////////////////////////////////////////////////////////////

    bool hasHeaderField (const string& field) const;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a field
    ///
    /// Returns the value of a header field with given name. If no header field
    /// with the given name was specified by the client, the empty string is
    /// returned.
    ////////////////////////////////////////////////////////////////////////////////

    const string& getHeaderField (const string& field) const;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns all keys
    ///
    /// Returns all keys of the request. The value of key can be extracted using
    /// getValue.
    ////////////////////////////////////////////////////////////////////////////////

    const vector<string>& getKeys () const;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the value of a key
    ///
    /// Returns the value of a key. The empty string is returned if key was not
    /// specified by the client. You can use getKeys to check which keys the
    /// client specified.
    ////////////////////////////////////////////////////////////////////////////////

    const string& getValue (const string& key) const ;
    
    size_t getContentLegth () const;
    
  protected:
    // use default copy constructor HttpRequest (const HttpRequest&);
    HttpRequest& operator= (const HttpRequest&);

  private:
    void setKeyValue (const string& params);
    
    int hex2int (char ch);

  private:
    HttpRequestType type;

    string requestPath;

    map<string,string> headerFields;
    map<string,string> requestFields;
    vector<string>     requestKeys;
  };

}

#endif
