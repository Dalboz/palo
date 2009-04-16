////////////////////////////////////////////////////////////////////////////////
/// @brief provides documentation of error codes
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

#include "HttpServer/ErrorCodeBrowserDocumentation.h"

#include <iostream>
#include <sstream>

#include "Exceptions/ErrorException.h"

namespace palo {
  ErrorCodeBrowserDocumentation::ErrorCodeBrowserDocumentation (const string& message)
    : BrowserDocumentation(message) {

    vector<string> errorCode;
    vector<string> errorDescription;

    string defaultDescription = ErrorException::getDescriptionErrorType ((ErrorException::ErrorType)0);

    for (int i=0; i<10000; i++) {
      string description = ErrorException::getDescriptionErrorType ((ErrorException::ErrorType) i);
      
      if (description != defaultDescription) {
        errorCode.push_back( StringUtils::convertToString((uint64_t) i));
        errorDescription.push_back(StringUtils::escapeHtml(description));
      }
    }
          
    values["@error_code"]  = errorCode;
    values["@error_description"] = errorDescription;
  }
}
