////////////////////////////////////////////////////////////////////////////////
/// @brief base class for error exceptions
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

#ifndef EXCEPTIONS_ERROR_EXCEPTION_H
#define EXCEPTIONS_ERROR_EXCEPTION_H 1

#include "palo.h"

#include <string>

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief base class for error exceptions
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ErrorException {
  public:
    enum ErrorType {
      ERROR_UNKNOWN                    =    1,
      ERROR_INTERNAL                   =    2,
      
      // 100-999 is used by libpalo_ng

      // general error messages
      ERROR_ID_NOT_FOUND               = 1000,
      ERROR_INVALID_FILENAME           = 1001,
      ERROR_MKDIR_FAILED               = 1002,
      ERROR_RENAME_FAILED              = 1003,
      ERROR_AUTHORIZATION_FAILED       = 1004,
      ERROR_INVALID_TYPE               = 1005,
      ERROR_INVALID_COORDINATES        = 1006,
      ERROR_CONVERSION_FAILED          = 1007,
      ERROR_FILE_NOT_FOUND_ERROR       = 1008,
      ERROR_NOT_AUTHORIZED             = 1009,
      ERROR_CORRUPT_FILE               = 1010,
      ERROR_WITHIN_EVENT               = 1011,
      ERROR_NOT_WITHIN_EVENT           = 1012,
      ERROR_INVALID_PERMISSION         = 1013,
      ERROR_INVALID_SERVER_PATH        = 1014,
      ERROR_INVALID_SESSION            = 1015,
      ERROR_PARAMETER_MISSING          = 1016,
      ERROR_SERVER_TOKEN_OUTDATED      = 1017,
      ERROR_INVALID_SPLASH_MODE        = 1018,
      ERROR_WORKER_AUTHORIZATION_FAILED= 1019,
      ERROR_WORKER_MESSAGE             = 1020,
      ERROR_API_CALL_NOT_IMPLEMENTED   = 1021,
      ERROR_HTTP_DISABLED              = 1022,
      ERROR_OUT_OF_MEMORY              = 1023, 

      // database
      ERROR_INVALID_DATABASE_NAME      = 2000,
      ERROR_DATABASE_NOT_FOUND         = 2001,
      ERROR_DATABASE_NOT_LOADED        = 2002,
      ERROR_DATABASE_UNSAVED           = 2003,
      ERROR_DATABASE_STILL_LOADED      = 2004,
      ERROR_DATABASE_NAME_IN_USE       = 2005,
      ERROR_DATABASE_UNDELETABLE       = 2006,
      ERROR_DATABASE_UNRENAMABLE       = 2007,
      ERROR_DATABASE_TOKEN_OUTDATED    = 2008,

      // dimension related errors
      ERROR_DIMENSION_EMPTY            = 3000,
      ERROR_DIMENSION_EXISTS           = 3001,
      ERROR_DIMENSION_NOT_FOUND        = 3002,
      ERROR_INVALID_DIMENSION_NAME     = 3003,
      ERROR_DIMENSION_UNCHANGABLE      = 3004,
      ERROR_DIMENSION_NAME_IN_USE      = 3005,
      ERROR_DIMENSION_IN_USE           = 3006,
      ERROR_DIMENSION_UNDELETABLE      = 3007,
      ERROR_DIMENSION_UNRENAMABLE      = 3008,
      ERROR_DIMENSION_TOKEN_OUTDATED   = 3009,
      ERROR_DIMENSION_LOCKED           = 3010,

      // dimension element related errors
      ERROR_ELEMENT_EXISTS             = 4000,
      ERROR_ELEMENT_CIRCULAR_REFERENCE = 4001,
      ERROR_ELEMENT_NAME_IN_USE        = 4002,
      ERROR_ELEMENT_NAME_NOT_UNIQUE    = 4003,
      ERROR_ELEMENT_NOT_FOUND          = 4004,
      ERROR_ELEMENT_NO_CHILD_OF        = 4005,
      ERROR_INVALID_ELEMENT_NAME       = 4006,
      ERROR_INVALID_OFFSET             = 4007,
      ERROR_INVALID_ELEMENT_TYPE       = 4008,
      ERROR_INVALID_POSITION           = 4009,
      ERROR_ELEMENT_NOT_DELETABLE      = 4010,
      ERROR_ELEMENT_NOT_RENAMABLE      = 4011,
      ERROR_ELEMENT_NOT_CHANGABLE      = 4012,
	  ERROR_INVALID_MODE	           = 4013,
      
      // cube related errors
      ERROR_CUBE_NOT_FOUND             = 5000,
      ERROR_INVALID_CUBE_NAME          = 5001,
      ERROR_CUBE_NOT_LOADED            = 5002,
      ERROR_CUBE_EMPTY                 = 5003,
      ERROR_CUBE_UNSAVED               = 5004,
      ERROR_SPLASH_DISABLED            = 5005,
      ERROR_COPY_PATH_NOT_NUMERIC      = 5006,
      ERROR_INVALID_COPY_VALUE         = 5007,
      ERROR_CUBE_NAME_IN_USE           = 5008,
      ERROR_CUBE_UNDELETABLE           = 5009,
      ERROR_CUBE_UNRENAMABLE           = 5010,
      ERROR_CUBE_TOKEN_OUTDATED        = 5011,
      ERROR_SPLASH_NOT_POSSIBLE        = 5012,
      ERROR_CUBE_LOCK_NOT_FOUND        = 5013,
      ERROR_CUBE_WRONG_USER            = 5014,
      ERROR_CUBE_WRONG_LOCK            = 5015,
      ERROR_CUBE_BLOCKED_BY_LOCK       = 5016,
      ERROR_CUBE_LOCK_NO_CAPACITY      = 5017,
      ERROR_GOALSEEK                   = 5018,
      ERROR_CUBE_IS_SYSTEM_CUBE        = 5019,
  
      // legacy interface
      ERROR_NET_ARG                    = 6000,
      ERROR_INV_CMD                    = 6001,
      ERROR_INV_CMD_CTL                = 6002,
      ERROR_NET_SEND                   = 6003,
      ERROR_NET_CONN_TERM              = 6004,
      ERROR_NET_RECV                   = 6005,
      ERROR_NET_HS_HALLO               = 6006,
      ERROR_NET_HS_PROTO               = 6007,
      ERROR_INV_ARG_COUNT              = 6008,
      ERROR_INV_ARG_TYPE               = 6009,
      ERROR_CLIENT_INV_NET_REPLY       = 6010,

      // worker
      ERROR_INVALID_WORKER_REPLY       = 7000,

      // rules
      ERROR_PARSING_RULE               = 8001,
      ERROR_RULE_NOT_FOUND             = 8002,
      ERROR_RULE_HAS_CIRCULAR_REF      = 8003,
    };

  public:
    static string getDescriptionErrorType (ErrorType type) {
      switch (type) {
        case ERROR_UNKNOWN:
          return "unknown";

        case ERROR_INTERNAL:
          return "internal error";

        case ERROR_ID_NOT_FOUND:
          return "identifier not found";

        case ERROR_INVALID_FILENAME:
          return "invalid filename";

        case ERROR_MKDIR_FAILED:
          return "cannot create directory";

        case ERROR_RENAME_FAILED:
          return "cannot rename file";

        case ERROR_AUTHORIZATION_FAILED:
          return "authorization failed";

        case ERROR_WORKER_AUTHORIZATION_FAILED:
          return "worker authorization failed";

        case ERROR_WORKER_MESSAGE:
          return "worker error";        
        
        case ERROR_INVALID_TYPE:
          return "invalid type";

        case ERROR_INVALID_COORDINATES:
          return "invalid coordinates";

        case ERROR_CONVERSION_FAILED:
          return "conversion failed";

        case ERROR_FILE_NOT_FOUND_ERROR:
          return "file not found";

        case ERROR_NOT_AUTHORIZED:
          return "not authorized for operation";

        case ERROR_CORRUPT_FILE:
          return "corrupt file";

        case ERROR_API_CALL_NOT_IMPLEMENTED:
          return "api call not implemented";

        case ERROR_HTTP_DISABLED:
          return "insecure communication disabled";
          
        case ERROR_OUT_OF_MEMORY:
          return "not enough memory";

        case ERROR_WITHIN_EVENT:
          return "already within event";

        case ERROR_NOT_WITHIN_EVENT:
          return "not within event";

        case ERROR_INVALID_PERMISSION:
          return "invalid permission entry";

        case ERROR_INVALID_SERVER_PATH:
          return "invalid server path";

        case ERROR_INVALID_SESSION:
          return "invalid session";

        case ERROR_PARAMETER_MISSING:
          return "missing parameter";
          
        case ERROR_PARSING_RULE:
          return "parse error in rule";

        case ERROR_RULE_NOT_FOUND:
          return "rule not found";
        
        case ERROR_RULE_HAS_CIRCULAR_REF:
          return "rule has circular reference";
        
        case ERROR_INVALID_SPLASH_MODE:
          return "invalid splash mode";

        case ERROR_SERVER_TOKEN_OUTDATED:
          return "server token outdated";

        case ERROR_INVALID_DATABASE_NAME:
          return "invalid database name";

        case ERROR_DATABASE_NOT_FOUND:
          return "database not found";

        case ERROR_DATABASE_NOT_LOADED:
          return "database not loaded";

        case ERROR_DATABASE_UNSAVED:
          return "database not saved";

        case ERROR_DATABASE_STILL_LOADED:
          return "database still loaded";

        case ERROR_DATABASE_NAME_IN_USE:
          return "database name in use";

        case ERROR_DATABASE_UNDELETABLE:
          return "databae is not deletable";

        case ERROR_DATABASE_UNRENAMABLE:
          return "database in not renamable";

        case ERROR_DATABASE_TOKEN_OUTDATED:
          return "database token outdated";

        case ERROR_DIMENSION_EMPTY:
          return "dimension empty";

        case ERROR_DIMENSION_EXISTS:
          return "dimension already exists";

        case ERROR_DIMENSION_NOT_FOUND:
          return "dimension not found";

        case ERROR_INVALID_DIMENSION_NAME:
          return "invalid dimension name";

        case ERROR_DIMENSION_UNCHANGABLE:
          return "dimension is not changable";

        case ERROR_DIMENSION_NAME_IN_USE:
          return "dimension name in use";

        case ERROR_DIMENSION_IN_USE:
          return "dimension in use";

        case ERROR_DIMENSION_UNDELETABLE:
          return "dimension not deletable";

        case ERROR_DIMENSION_UNRENAMABLE:
          return "dimension not renamable";

        case ERROR_DIMENSION_TOKEN_OUTDATED:
          return "dimension token outdated";
        
        case ERROR_DIMENSION_LOCKED:
          return "dimension is locked";

        case ERROR_ELEMENT_EXISTS:
          return "element already exists";

        case ERROR_ELEMENT_CIRCULAR_REFERENCE:
          return "cirular reference";

        case ERROR_ELEMENT_NAME_IN_USE:
          return "element name in use";

        case ERROR_ELEMENT_NAME_NOT_UNIQUE:
          return "element name not unique";

        case ERROR_ELEMENT_NOT_FOUND:
          return "element not found";

        case ERROR_ELEMENT_NO_CHILD_OF:
          return "element is no child";

        case ERROR_INVALID_ELEMENT_NAME:
          return "invalid element name";

        case ERROR_INVALID_OFFSET:
          return "invalid element offset";

        case ERROR_INVALID_ELEMENT_TYPE:
          return "invalid element type";

        case ERROR_INVALID_POSITION:
          return "invalid element position";

		case ERROR_INVALID_MODE:
		  return "invalid mode";

        case ERROR_ELEMENT_NOT_DELETABLE:
          return "element not deletable";

        case ERROR_ELEMENT_NOT_RENAMABLE:
          return "element not renamable";

        case ERROR_ELEMENT_NOT_CHANGABLE:
          return "element not changable";

        case ERROR_CUBE_NOT_FOUND:
          return "cube not found";

        case ERROR_INVALID_CUBE_NAME:
          return "invalid cube name";

        case ERROR_CUBE_NOT_LOADED:
          return "cube not loaded";

        case ERROR_CUBE_EMPTY:
          return "cube empty";

        case ERROR_CUBE_UNSAVED:
          return "cube not saved";

        case ERROR_CUBE_LOCK_NOT_FOUND:
          return "cube lock not found";

        case ERROR_CUBE_IS_SYSTEM_CUBE:
	  return "cube is system cube";

        case ERROR_CUBE_WRONG_USER:
          return "wrong user for locked area";

        case ERROR_CUBE_WRONG_LOCK:
          return "could not create lock";

        case ERROR_CUBE_BLOCKED_BY_LOCK:
          return "is blocked because of a locked area";

        case ERROR_CUBE_LOCK_NO_CAPACITY:
          return "not enough rollback capacity";

		case ERROR_GOALSEEK:
			return "goalseek error";
        
        case ERROR_SPLASH_DISABLED:
          return "splash disabled";

        case ERROR_COPY_PATH_NOT_NUMERIC:
          return "copy path must be numeric";

        case ERROR_INVALID_COPY_VALUE:
          return "invalid copy value";

        case ERROR_CUBE_NAME_IN_USE:
          return "cube name in use";

        case ERROR_CUBE_UNDELETABLE:
          return "cube is not deletable";

        case ERROR_CUBE_UNRENAMABLE:
          return "cube is not renamable";

        case ERROR_CUBE_TOKEN_OUTDATED:
          return "cube token outdated";

        case ERROR_SPLASH_NOT_POSSIBLE:
          return "splashing is not possible";

        case ERROR_NET_ARG:
          return "legacy error";

        case ERROR_INV_CMD:
          return "legacy error";

        case ERROR_INV_CMD_CTL:
          return "legacy error";

        case ERROR_NET_SEND:
          return "legacy error";

        case ERROR_NET_CONN_TERM:
          return "legacy error";

        case ERROR_NET_RECV:
          return "legacy error";

        case ERROR_NET_HS_HALLO:
          return "legacy error";

        case ERROR_NET_HS_PROTO:
          return "legacy error";

        case ERROR_INV_ARG_COUNT:
          return "legacy error";

        case ERROR_INV_ARG_TYPE:
          return "legacy error";

        case ERROR_CLIENT_INV_NET_REPLY:
          return "legacy error";

        case ERROR_INVALID_WORKER_REPLY:
          return "illegal worker response";

      }

      return "internal error in ErrorException::getDescriptionErrorType";
    }
    
  public:
    ErrorException (ErrorType type, const string& message)
      : type(type), message(message) {
    }

    virtual ~ErrorException () {}
    
  public:   

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get error message
    ////////////////////////////////////////////////////////////////////////////////

    virtual const string& getMessage () const { 
      return message;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get error details
    ////////////////////////////////////////////////////////////////////////////////

    virtual const string& getDetails () const {
      return details;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get error type
    ////////////////////////////////////////////////////////////////////////////////

    virtual ErrorType getErrorType () const {
      return type;
    };

  protected:
    ErrorType type;
    string message;
    string details;
  };

}

#endif
