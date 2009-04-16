////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for all legacy request handler
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

#ifndef LEGACY_SERVER_LEGACY_REQUEST_HANDLER_H
#define LEGACY_SERVER_LEGACY_REQUEST_HANDLER_H 1

#include <vector>

#include "Exceptions/CommunicationException.h"

#include "InputOutput/Statistics.h"

#include "Olap/Cube.h"
#include "Olap/Element.h"

#include "LegacyServer/ArgumentDelayed.h"
#include "LegacyServer/ArgumentDouble.h"
#include "LegacyServer/ArgumentError.h"
#include "LegacyServer/ArgumentInt32.h"
#include "LegacyServer/ArgumentMulti.h"
#include "LegacyServer/ArgumentString.h"
#include "LegacyServer/ArgumentUInt32.h"
#include "LegacyServer/ArgumentVoid.h"

namespace palo {
  using namespace std;

  class Database;
  class Dimension;
  class PaloSession;
  class Server;
  class User;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief abstract base class for all legacy request handler
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS LegacyRequestHandler {
  public:
    enum UpdateElementMode {
      UNKOWN,
      ADD,
      FORCE_ADD,
      UPDATE,
      ADD_OR_UPDATE
    };

  public:
    static int32_t legacyType (Element::ElementType type);

    static UpdateElementMode legacyUpdateElementMode (ArgumentInt32 * argument);
    static Element::ElementType legacyElementType (ArgumentInt32 * argument);
    static bool legacyBool (ArgumentInt32 * argument);
    static Cube::SplashMode legacySplashMode (ArgumentInt32 * argument);

  public:
    static const int32_t COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_ADD           = 1;
    static const int32_t COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_FORCE_ADD     = 2;
    static const int32_t COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_UPDATE        = 3;
    static const int32_t COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_ADD_OR_UPDATE = 4;

    static const int32_t COMM_DE_TYPE_N = 1;
    static const int32_t COMM_DE_TYPE_S = 2;
    static const int32_t COMM_DE_TYPE_C = 3;
    static const int32_t COMM_DE_TYPE_R = 4;

    static const int32_t COMM_SPLASH_MODE_DISABLE  = 1;
    static const int32_t COMM_SPLASH_MODE_DEFAULT  = 2;
    static const int32_t COMM_SPLASH_MODE_BASE_SET = 3;
    static const int32_t COMM_SPLASH_MODE_BASE_ADD = 4;

  public:
    static ArgumentString * asString (const vector<LegacyArgument*>* arguments, size_t pos) {
      ArgumentString * arg = dynamic_cast<ArgumentString*>((*arguments)[pos]);

      if (arg == 0) {
        throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "expecting a string argument");
      }

      return arg;
    }

    static ArgumentInt32 * asInt32 (const vector<LegacyArgument*>* arguments, size_t pos) {
      ArgumentInt32 * arg = dynamic_cast<ArgumentInt32*>((*arguments)[pos]);

      if (arg == 0) {
        throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "expecting an integer argument");
      }

      return arg;
    }

    static ArgumentUInt32 * asUInt32 (const vector<LegacyArgument*>* arguments, size_t pos) {
      ArgumentUInt32 * arg = dynamic_cast<ArgumentUInt32*>((*arguments)[pos]);

      if (arg == 0) {
        throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "expecting an unsigned integer argument");
      }

      return arg;
    }

    static ArgumentMulti * asMulti (const vector<LegacyArgument*>* arguments, size_t pos) {
      ArgumentMulti * arg = dynamic_cast<ArgumentMulti*>((*arguments)[pos]);

      if (arg == 0) {
        throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "expecting a multi argument");
      }

      return arg;
    }

    static ArgumentDouble * asDouble (const vector<LegacyArgument*>* arguments, size_t pos) {
      ArgumentDouble * arg = dynamic_cast<ArgumentDouble*>((*arguments)[pos]);

      if (arg == 0) {
        throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "expecting a double argument");
      }

      return arg;
    }

  public:
    LegacyRequestHandler (Server* server) 
      : server(server) {
    }

    virtual ~LegacyRequestHandler () {
    }

  public:
    virtual LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>*, User*, PaloSession*) = 0;

  protected:
    Database * findDatabase (const vector<LegacyArgument*>*, size_t pos, User*, bool requireLoad = true);
    Cube * findCube (Database * database, const vector<LegacyArgument*>* arguments, size_t pos, User*, bool requireLoad = true);
    Dimension * findDimension (Database * database, const vector<LegacyArgument*>* arguments, size_t pos, User*);
    Element * findElement (Dimension * dimension, const vector<LegacyArgument*>* arguments, size_t pos, User*);

    LegacyArgument * generateDimensionsResult (const vector<Dimension*>*);
    LegacyArgument * generateElementsResult (const vector<Element*>*);
    LegacyArgument * generateElementResult (Element*);

  protected:
    Server * server;
  };

}

#endif
