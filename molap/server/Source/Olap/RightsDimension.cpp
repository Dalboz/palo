////////////////////////////////////////////////////////////////////////////////
/// @brief palo rights dimension
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

#include "Olap/RightsDimension.h"

#include "InputOutput/FileWriter.h"

#include "Olap/SystemDatabase.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void RightsDimension::saveDimensionType (FileWriter* file) {
    file->appendIdentifier(identifier);
    file->appendEscapeString(name);
    file->appendInteger(DIMENSION_TYPE);
    file->appendInteger(deletable ? 1 : 0);
    file->appendInteger(renamable ? 1 : 0);
    file->appendInteger(changable ? 1 : 0);
    file->nextLine();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // element functions
  ////////////////////////////////////////////////////////////////////////////////


  Element* RightsDimension::addElement (const string& name, Element::ElementType elementType, User* user, bool useJournal) {
    SystemDatabase * system = dynamic_cast<SystemDatabase*>(database);

    if (system != 0) {
      if (this == system->getUserDimension()) {
        elementType = Element::STRING;
      }
      else if (this == system->getGroupDimension()) {
        elementType = Element::STRING;
      }
      else if (this == system->getRoleDimension()) {
        elementType = Element::STRING;
      }
    }

    return SystemDimension::addElement(name, elementType, user, useJournal);
  }


  void RightsDimension::deleteElement (Element * element, User* user, bool useJournal) {
    SystemDatabase * system = database->getType() == SYSTEM ? dynamic_cast<SystemDatabase*>(database) : 0;

    if (system != 0) {
      if (   this == system->getUserDimension() 
          || this == system->getGroupDimension()
          || this == system->getRoleDimension()) {
        if (element->getName() == "admin") {
          throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_DELETABLE,
                                   "admin is not deletable",
                                   "element", name);
        }
      }
    }

    return SystemDimension::deleteElement(element, user, useJournal);
  }



  void RightsDimension::changeElementName (Element * element, const string& name, User* user) {
    SystemDatabase * system = database->getType() == SYSTEM ? dynamic_cast<SystemDatabase*>(database) : 0;

    if (system != 0) {
      if (   this == system->getUserDimension() 
          || this == system->getGroupDimension()
          || this == system->getRoleDimension()) {
        if (element->getName() == "admin") {
          throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_RENAMABLE,
                                   "admin is not renamable",
                                   "element", name);
        }
      }
    }

    return SystemDimension::changeElementName(element, name, user);
  }
  
  
  void RightsDimension::changeElementType (Element * element, Element::ElementType elementType, User* user,
                                    bool setConsolidated) {
    SystemDatabase * system = dynamic_cast<SystemDatabase*>(database);

    if (system != 0) {
      if (this == system->getUserDimension()) {
        elementType = Element::STRING;
      }
      else if (this == system->getGroupDimension()) {
        elementType = Element::STRING;
      }
      else if (this == system->getRoleDimension()) {
        elementType = Element::STRING;
      }
    }

    return SystemDimension::changeElementType(element, elementType, user, setConsolidated);
  }


  void RightsDimension::clearElements (User* user) {
    SystemDatabase * system = database->getType() == SYSTEM ? dynamic_cast<SystemDatabase*>(database) : 0;

    if (system != 0) {
      if (   this == system->getUserDimension() 
          || this == system->getGroupDimension()
          || this == system->getRoleDimension()) {
            
        vector<Element*> elements = getElements(0);
        for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
          if ((*i)->getName() != "admin") {
            SystemDimension::deleteElement(*i, user, true);
          }
        }        
      }
    }
    else {
      SystemDimension::clearElements(user);
    }
  }
  
  
  
}
