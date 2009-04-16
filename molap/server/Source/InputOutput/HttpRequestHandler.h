////////////////////////////////////////////////////////////////////////////////
/// @brief http request handler
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

#ifndef INPUT_OUTPUT_HTTP_REQUEST_HANDLER_H
#define INPUT_OUTPUT_HTTP_REQUEST_HANDLER_H 1

#include "palo.h"

#include "Exceptions/ErrorException.h"

namespace palo {
  class HttpResponse;
  class HttpRequest;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief http request handler
  ///
  /// A http request handler is used to answer the request from client. The
  /// function handleHttpRequest is called to generate a suitable response. This
  /// class is the abstract base class for all http request handlers. A request
  /// handler must provide an implementation for the virtual function
  /// handleHttpRequest.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HttpRequestHandler {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief http request handler
    ///
    /// Constructor for the abstract base class. No parameters are necessary.
    ////////////////////////////////////////////////////////////////////////////////

    HttpRequestHandler ();

    virtual ~HttpRequestHandler () {
    }

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief generates a http response given a http request
    ///
    /// This function is called to generate a response to the client. It is pure
    /// virtual, therefore each handler must provide an implementation.
    ////////////////////////////////////////////////////////////////////////////////

    virtual HttpResponse * handleHttpRequest (const HttpRequest*) = 0;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief generates a http response given a http request
    ///
    /// This function is called to generate a response to the client. It is pure
    /// virtual, therefore each handler must provide an implementation.
    ////////////////////////////////////////////////////////////////////////////////

    virtual HttpResponse * handleSemaphoreRaised (semaphore_t, const HttpRequest*, IdentifierType, const string*) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "virtual handleSemaphoreRaised");
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief generates a http response given a http request
    ///
    /// This function is called to generate a response to the client. It is pure
    /// virtual, therefore each handler must provide an implementation.
    ////////////////////////////////////////////////////////////////////////////////

    virtual HttpResponse * handleSemaphoreDeleted (semaphore_t, const HttpRequest*, IdentifierType) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "virtual handleSemaphoreRaised");
    }

  protected:
    HttpRequestHandler (const HttpRequestHandler&);
    HttpRequestHandler& operator= (const HttpRequestHandler&);
  };

}

#endif
