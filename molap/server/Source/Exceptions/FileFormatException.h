////////////////////////////////////////////////////////////////////////////////
/// @brief palo file format exception
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

#ifndef EXCEPTIONS_FILE_FORMAT_EXCEPTION_H
#define EXCEPTIONS_FILE_FORMAT_EXCEPTION_H 1

#include "palo.h"

#include <string>

#include "Exceptions/ErrorException.h"

#include "InputOutput/FileReader.h"

#include "Collections/StringUtils.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo file format exception
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FileFormatException  : public ErrorException {
  public:
    FileFormatException (const string& message, FileReader* file)
      : ErrorException(ErrorException::ERROR_CORRUPT_FILE, message) {
        
     if (file) {        
        this->details = "file '" + file->getFileName().fullPath() 
	  + "' line number '" + StringUtils::convertToString(file->getLineNumber()) 
	  + "'";
     }
    }
  };

}

#endif
