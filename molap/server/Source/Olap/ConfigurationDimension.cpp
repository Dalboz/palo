////////////////////////////////////////////////////////////////////////////////
/// @brief palo configuration dimension
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

#include "Olap/CubeDimension.h"

#include "InputOutput/FileWriter.h"

#include "Olap/SystemDatabase.h"
#include "Olap/ConfigurationDimension.h"

namespace palo {
  
  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void ConfigurationDimension::saveDimensionType (FileWriter* file) {
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


  Element* ConfigurationDimension::addElement (const string& name, Element::ElementType elementType, User* user, bool useJournal) {
    return SystemDimension::addElement(name, Element::STRING, user, useJournal);
  }


  void ConfigurationDimension::deleteElement (Element * element, User* user, bool useJournal) {
    
    // do not delete CLIENT_CACHE_ELEMENT nor HIDE_ELEMENTS_ELEMENT
	  if (element && (element->getName() == SystemDatabase::NAME_CLIENT_CACHE_ELEMENT || element->getName() == SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT)) {
      if (user) {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_DELETABLE,
                                 "element is not deletable",
                                 "user", (int) user->getIdentifier());
      }
      else {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_DELETABLE,
                                 "element is not deletable",
                                 "user", 0);
      }      
    }
    
    return SystemDimension::deleteElement(element, user, useJournal);
  }



  void ConfigurationDimension::changeElementName (Element * element, const string& name, User* user) {

    // do not change CLIENT_CACHE_ELEMENT nor HIDE_ELEMENTS_ELEMENT
    if (element && (element->getName() == SystemDatabase::NAME_CLIENT_CACHE_ELEMENT || element->getName() == SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT)) {
      if (user) {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_RENAMABLE,
                                 "element is not renamable",
                                 "user", (int) user->getIdentifier());
      }
      else {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_RENAMABLE,
                                 "element is not renamable",
                                 "user", 0);
      }      
    }
    
    return SystemDimension::changeElementName(element, name, user);
  }
  
  
  void ConfigurationDimension::changeElementType (Element * element, Element::ElementType elementType, User* user,
                                    bool setConsolidated) {

    // do not change CLIENT_CACHE_ELEMENT nor HIDE_ELEMENTS_ELEMENT
    if (element && (element->getName() == SystemDatabase::NAME_CLIENT_CACHE_ELEMENT || element->getName() == SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT)) {
      if (user) {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_CHANGABLE,
                                 "element is not changable",
                                 "user", (int) user->getIdentifier());
      }
      else {
        throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_CHANGABLE,
                                 "element is not changable",
                                 "user", 0);
      }      
    }
    
    return SystemDimension::changeElementType(element, elementType, user, setConsolidated);
  }
  
  
  void ConfigurationDimension::checkElementAccessRight (User* user, RightsType minimumRight) {
      
    SystemDatabase* system = (database->getServer())->getSystemDatabase();
    if (user == 0 || system == 0) {
       return;
    }
      
    RightsType rt = user->getRoleElementRight();
      
    if (minimumRight > RIGHT_READ && rt < minimumRight) {
      throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                 "insufficient access rights for dimension element",
                                 "user", (int) user->getIdentifier());
    }
  }  
  

}
