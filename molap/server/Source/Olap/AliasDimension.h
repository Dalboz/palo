////////////////////////////////////////////////////////////////////////////////
/// @brief palo alias dimension
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

#ifndef OLAP_ALIAS_DIMENSION_H
#define OLAP_ALIAS_DIMENSION_H 1

#include "palo.h"

#include "Olap/SystemDimension.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief alias OLAP dimension
  ///
  /// An OLAP dimension is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS AliasDimension : public SystemDimension {
  public:
    static const uint32_t DIMENSION_TYPE = 3;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new dimension with given identifier
    ////////////////////////////////////////////////////////////////////////////////

    AliasDimension (IdentifierType identifier, const string& name, Database* database, Dimension* alias)
      : SystemDimension(identifier, name, database),
        alias(alias) {
      status = LOADED;
    }
    
  public:  
    void loadDimension (FileReader* file) {
      return;
    }

    void saveDimensionType (FileWriter* file);

    void saveDimension (FileWriter* file) {
      return;
    }
      
  public:  
    ItemType getType() {
      return SYSTEM;
    }

    SystemDimensionType getSubType () const {
      return ALIAS_DIMENSION;
    }

    LevelType getLevel () {
      return alias->getLevel();
    }

    IndentType getIndent () {
      return alias->getIndent();
    }

    DepthType getDepth () {
      return alias->getDepth();
    }

    size_t getMemoryUsageIndex () {
      return alias->getMemoryUsageIndex();
    }

    size_t getMemoryUsageStorage () {
      return alias->getMemoryUsageStorage();
    }

  public:
    void updateLevelIndentDepth () {
      alias->updateLevelIndentDepth();
    }

  public:
    vector<Element*> getElements (User* user) {
      return alias->getElements(user);
    }

    void clearElements (User* user) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      alias->clearElements(user);
    }

    Element* addElement (const string& name,
                         Element::ElementType elementType,
                         User* user,
                         bool useJournal = true) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      return alias->addElement(name, elementType, user, useJournal);
    }

    void deleteElement (Element * element, User* user, bool useJournal = true) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      alias->deleteElement(element, user, useJournal);
    }

    void changeElementName (Element * element, const string& name, User* user) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      alias->changeElementName(element, name, user);
    }

    void changeElementType (Element * element, Element::ElementType elementType, User* user, bool setConsolidated) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      alias->changeElementType(element, elementType, user, setConsolidated);
    }

    void moveElement (Element * element, PositionType newPosition, User* user) {
      alias->moveElement(element, newPosition, user);
    }

    void addChildren (Element * parent, const ElementsWeightType * children, User* user) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      alias->addChildren(parent, children, user);
    }

    void removeChildren (User* user, Element * parent) {
      if (! changable) {
        throw ParameterException(ErrorException::ERROR_DIMENSION_UNCHANGABLE,
                                 "dimension cannot be changed",
                                 "dimension", name);
      }

      alias->removeChildren(user, parent);
    }

    size_t sizeElements () {
      return alias->sizeElements();
    }

    const ParentsType* getParents (Element * child) {
      return alias->getParents(child);
    }

    const ElementsWeightType * getChildren (Element* parent) {
      return alias->getChildren(parent);
    }

    set<Element*> getBaseElements (Element* parent, bool* multiple) {
      return alias->getBaseElements(parent, multiple);
    }

    bool isStringConsolidation (Element * element) {
      return alias->isStringConsolidation(element);
    }
    
    Element * lookupElement (IdentifierType elementIdentifier) {
      return alias->lookupElement(elementIdentifier);
    }

    Element * lookupElementByName (const string& name) {
      return alias->lookupElementByName(name);
    }
    
    Element * lookupElementByPosition (PositionType position) {
      return alias->lookupElementByPosition(position);
    }

    Element* findElement (IdentifierType elementIdentifier, User* user) {
      return alias->findElement(elementIdentifier, user);
    }
    
    Element * findElementByName (const string& name, User* user) {
      return alias->findElementByName(name, user);
    }
    
    Element * findElementByPosition (PositionType position, User* user) {
      return alias->findElementByPosition(position, user);
    }

  private:
    Dimension* alias;
  };

}

#endif
