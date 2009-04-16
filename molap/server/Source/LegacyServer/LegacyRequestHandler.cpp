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

#include "LegacyServer/LegacyRequestHandler.h"

#include "Exceptions/CommunicationException.h"

#include "Olap/Dimension.h"
#include "Olap/Server.h"

#include "LegacyServer/ArgumentString.h"
#include "LegacyServer/ArgumentMulti.h"
#include "LegacyServer/ArgumentInt32.h"

namespace palo {
  int32_t LegacyRequestHandler::legacyType (Element::ElementType type) {
    switch (type) {
      case Element::UNDEFINED:     return 0;
      case Element::NUMERIC:       return 1;
      case Element::STRING:        return 2;
      case Element::CONSOLIDATED:  return 3;
    }

    return 0;
  }



  LegacyRequestHandler::UpdateElementMode LegacyRequestHandler::legacyUpdateElementMode (ArgumentInt32 * argument) {
    switch (argument->getValue()) {
      case COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_ADD:            return ADD;
      case COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_FORCE_ADD:      return FORCE_ADD;
      case COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_UPDATE:         return UPDATE;
      case COMM_DIMENSION_ADD_OR_UPDATE_ELEMENT_MODE_ADD_OR_UPDATE:  return ADD_OR_UPDATE;
    }

    throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "unknown update element mode");
  }



  Element::ElementType LegacyRequestHandler::legacyElementType (ArgumentInt32 * argument) {
    switch (argument->getValue()) {
      case COMM_DE_TYPE_N:  return Element::NUMERIC;
      case COMM_DE_TYPE_S:  return Element::STRING;
      case COMM_DE_TYPE_C:  return Element::CONSOLIDATED;
    }

    throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "unknown element type");
  }



  bool LegacyRequestHandler::legacyBool (ArgumentInt32 * argument) {
    return argument->getValue() != 0;
  }



  Cube::SplashMode LegacyRequestHandler::legacySplashMode (ArgumentInt32 * argument) {
    switch (argument->getValue()) {
      case COMM_SPLASH_MODE_DISABLE:   return Cube::DISABLED;
      case COMM_SPLASH_MODE_DEFAULT:   return Cube::DEFAULT;
      case COMM_SPLASH_MODE_BASE_SET:  return Cube::SET_BASE;
      case COMM_SPLASH_MODE_BASE_ADD:  return Cube::ADD_BASE;
    }

    throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "unknown splash mode");
  }


  Database * LegacyRequestHandler::findDatabase (const vector<LegacyArgument*>* arguments,
                                                 size_t pos,
                                                 User* user, 
                                                 bool requireLoad) {
    ArgumentString * name = dynamic_cast<ArgumentString*>((*arguments)[pos]);

    if (name == 0) {
      throw CommunicationException(ErrorException::ERROR_INV_ARG_COUNT, "expecting database name as argument");
    }

    return server->findDatabaseByName(name->getValue(), user, requireLoad);
  }



  Cube * LegacyRequestHandler::findCube (Database * database,
                                         const vector<LegacyArgument*>* arguments,
                                         size_t pos,
                                         User* user,
                                         bool requireLoad) {
    ArgumentString * name = dynamic_cast<ArgumentString*>((*arguments)[pos]);

    if (name == 0) {
      throw CommunicationException(ErrorException::ERROR_INV_ARG_COUNT, "expecting cube name as argument");
    }

    return database->findCubeByName(name->getValue(), user, requireLoad);
  }



  Dimension * LegacyRequestHandler::findDimension (Database * database,
                                                   const vector<LegacyArgument*>* arguments, 
                                                   size_t pos,
                                                   User* user) {
    ArgumentString * name = dynamic_cast<ArgumentString*>((*arguments)[pos]);

    if (name == 0) {
      throw CommunicationException(ErrorException::ERROR_INV_ARG_COUNT, "expecting dimension name as argument");
    }

    return database->findDimensionByName(name->getValue(), user);
  }



  Element * LegacyRequestHandler::findElement (Dimension * dimension,
                                               const vector<LegacyArgument*>* arguments,
                                               size_t pos,
                                               User* user) {
    ArgumentString * name = dynamic_cast<ArgumentString*>((*arguments)[pos]);

    if (name == 0) {
      throw CommunicationException(ErrorException::ERROR_INV_ARG_COUNT, "expecting element name as argument");
    }

    return dimension->findElementByName(name->getValue(), user);
  }



  LegacyArgument * LegacyRequestHandler::generateDimensionsResult (const vector<Dimension*>* dimensions) {
    ArgumentMulti * multi = new ArgumentMulti();

    for (vector<Dimension*>::const_iterator iter = dimensions->begin();  iter != dimensions->end();  iter++) {
      const string& name = (*iter)->getName();

      multi->append(new ArgumentString(name));
    }

    return multi;
  }



  LegacyArgument * LegacyRequestHandler::generateElementsResult (const vector<Element*>* elements) {
    ArgumentMulti * multi = new ArgumentMulti();

    for (vector<Element*>::const_iterator iter = elements->begin();  iter != elements->end();  iter++) {
      ArgumentMulti * element = new ArgumentMulti();

      element->append(new ArgumentString((*iter)->getName()));

      int32_t type = legacyType((*iter)->getElementType());

      element->append(new ArgumentInt32(type));

      multi->append(element);
    }

    return multi;
  }



  LegacyArgument * LegacyRequestHandler::generateElementResult (Element* element) {
    return new ArgumentString(element->getName());
  }
}
