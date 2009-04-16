////////////////////////////////////////////////////////////////////////////////
/// @brief http file request handler
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

#ifndef INPUT_OUTPUT_HTTP_FILE_REQUEST_HANDLER_H
#define INPUT_OUTPUT_HTTP_FILE_REQUEST_HANDLER_H 1

#include "palo.h"

#include <string>

#include "InputOutput/HttpRequest.h"
#include "InputOutput/HttpResponse.h"
#include "InputOutput/HttpRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief http file request handler
  ///
  /// A http file request handler delivers a file to the client. The file can be a
  /// binary file, it is copied to the client without modifications. The purpose of
  /// this handler is to deliver static content, like icons and stylesheets,
  /// residing in the file-system. The content of the file is delivered in one
  /// go, chunked delivery is not supported.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HttpFileRequestHandler : public HttpRequestHandler {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new handler
    ///
    /// Constructs a new file request handler given a filename and a content type.
    ////////////////////////////////////////////////////////////////////////////////

    HttpFileRequestHandler (const string& file, const string& contentType);

    virtual ~HttpFileRequestHandler () {
    }

  public:
    virtual HttpResponse * handleHttpRequest (const HttpRequest*);
    
  private:
    const string filename;
    string contentType;
  };

}

#endif
