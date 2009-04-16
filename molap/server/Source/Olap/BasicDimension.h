////////////////////////////////////////////////////////////////////////////////
/// @brief palo basic dimension
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

#ifndef OLAP_BASIC_DIMENSION_H
#define OLAP_BASIC_DIMENSION_H 1

#include <deque>

#include "palo.h"

#include "Exceptions/ParameterException.h"

#include "Collections/AssociativeArray.h"
#include "Collections/StringUtils.h"

#include "Olap/Dimension.h"
#include "Olap/Element.h"

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief basic dimension
  ///
  /// A basic dimension is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS BasicDimension : public Dimension {
    private:
      struct Name2ElementDesc {
          uint32_t hash (const string& name) {
            return StringUtils::hashValueLower(name.c_str(), name.size());
          }

          bool isEmptyElement (Element * const & element) {
            return element == 0;
          }

          uint32_t hashElement (Element * const & element) {
            return hash(element->getName());
          }

          uint32_t hashKey (const string& key) {
            return hash(key);
          }

          bool isEqualElementElement (Element * const & left, Element * const & right) {
            return left->getName() == right->getName();
          }

          bool isEqualKeyElement (const string& key, Element * const & element) {
            const string& name = element->getName();

            return key.size() == name.size() && strncasecmp(key.c_str(), name.c_str(), key.size()) == 0;
          }

          void clearElement (Element * & element) {
            element = 0;
          }
      };



      struct Position2ElementDesc {
          bool isEmptyElement (Element * const & element) {
            return element == 0;
          }

          uint32_t hashElement (Element * const & element) {
            return element->getPosition();
          }

          uint32_t hashKey (const PositionType& key) {
            return key;
          }

          bool isEqualElementElement (Element * const & left, Element * const & right) {
            return left->getPosition() == right->getPosition();
          }

          bool isEqualKeyElement (const PositionType& key, Element * const & element) {
            return key == element->getPosition();
          }

          void clearElement (Element * & element) {
            element = 0;
          }
      };



#if defined(_MSC_VER)
#pragma warning( disable : 4311 )
#endif

      struct ParentChildrenPair {
          ParentChildrenPair (Element * parent) : parent(parent) {
          }

          Element * parent;
          ElementsWeightType children;
      };

      struct Parent2ChildrenDesc {
          bool isEmptyElement (ParentChildrenPair * const & element) {
            return element == 0;
          }

          uint32_t hashElement (ParentChildrenPair * const & element) {
#if SIZEOF_VOIDP == 8
            uint64_t p = reinterpret_cast<uint64_t>(element->parent);
            return (uint32_t)((p & 0xFFFFFF) ^ (p >> 32));
#else
            return reinterpret_cast<uint32_t>(element->parent);
#endif
          }

          uint32_t hashKey (Element * const & key) {
#if SIZEOF_VOIDP == 8
            uint64_t p = reinterpret_cast<uint64_t>(key);
            return (uint32_t)((p & 0xFFFFFF) ^ (p >> 32));
#else
            return reinterpret_cast<uint32_t>(key);
#endif
          }

          bool isEqualElementElement (ParentChildrenPair * const & left, ParentChildrenPair * const & right) {
            return left->parent == right->parent;
          }

          bool isEqualKeyElement (Element * const & key, ParentChildrenPair * const & element) {
            return key == element->parent;
          }

          void clearElement (ParentChildrenPair * & element) {
            element = 0;
          }

          void deleteElement (ParentChildrenPair * & element) {
            delete element;
          }
      };



      struct ChildParentsPair {
          ChildParentsPair (Element * child) : child(child) {
          }

          Element * child;
          ParentsType parents;
      };

      struct Child2ParentsDesc {
          bool isEmptyElement (ChildParentsPair * const & element) {
            return element == 0;
          }

          uint32_t hashElement (ChildParentsPair * const & element) {
#if SIZEOF_VOIDP == 8
            uint64_t p = reinterpret_cast<uint64_t>(element->child);
            return (uint32_t)((p & 0xFFFFFF) ^ (p >> 32));
#else
            return reinterpret_cast<uint32_t>(element->child);
#endif
          }

          uint32_t hashKey (Element * const & key) {
#if SIZEOF_VOIDP == 8
            uint64_t p = reinterpret_cast<uint64_t>(key);
            return (uint32_t)((p & 0xFFFFFF) ^ (p >> 32));
#else
            return reinterpret_cast<uint32_t>(key);
#endif
          }

          bool isEqualElementElement (ChildParentsPair * const & left, ChildParentsPair * const & right) {
            return left->child == right->child;
          }

          bool isEqualKeyElement (Element * const & key, ChildParentsPair * const & element) {
            return key == element->child;
          }

          void clearElement (ChildParentsPair * & element) {
            element = 0;
          }

          void deleteElement (ChildParentsPair * & element) {
            delete element;
          }
      };



      struct ElementSetDesc {
          bool isEmptyElement (Element * const & element) {
            return element == 0;
          }

          uint32_t hashElement (Element * const & element) {
#if SIZEOF_VOIDP == 8
            uint64_t p = reinterpret_cast<uint64_t>(element);
            return (uint32_t)((p & 0xFFFFFF) ^ (p >> 32));
#else
            return reinterpret_cast<uint32_t>(element);
#endif
          }

          uint32_t hashKey (Element * const & key) {
#if SIZEOF_VOIDP == 8
            uint64_t p = reinterpret_cast<uint64_t>(key);
            return (uint32_t)((p & 0xFFFFFF) ^ (p >> 32));
#else
            return reinterpret_cast<uint32_t>(key);
#endif
          }

          bool isEqualElementElement (Element * const & left, Element * const & right) {
            return left == right;
          }

          bool isEqualKeyElement (Element * const & key, Element * const & element) {
            return key == element;
          }

          void clearElement (Element * & element) {
            element = 0;
          }
      };

    public:

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief creates new dimension with given identifier
      ////////////////////////////////////////////////////////////////////////////////

      BasicDimension (IdentifierType identifier, const string& name, Database* database);
    
      ////////////////////////////////////////////////////////////////////////////////
      /// @brief deletes dimension
      ////////////////////////////////////////////////////////////////////////////////

      ~BasicDimension ();

    public:  
      void loadDimension (FileReader* file);

      void saveDimension (FileWriter* file);
      
    public:  
      LevelType getLevel ();

      IndentType getIndent ();

      DepthType getDepth ();

      size_t getMemoryUsageIndex ();

      size_t getMemoryUsageStorage ();

      vector<Element*> getElements (User* user);

      IdentifierType getMaximalIdentifier ();

    public:

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief updates level, indent, and depth information
      ////////////////////////////////////////////////////////////////////////////////

      void updateLevelIndentDepth ();
      
      ////////////////////////////////////////////////////////////////////////////////
      /// @brief updates base element list of each element
      ////////////////////////////////////////////////////////////////////////////////

      void updateElementBaseElements ();

      ////////////////////////////////////////////////////////////////////////////////
      /// @brief updates the list of topological sorted elements
      ////////////////////////////////////////////////////////////////////////////////

      void updateTopologicalSortedElements ();

	  void _destroyElementLeavePosition(User* user, Element* element);

    public:
      void clearElements (User* user);

      Element* addElement (const string& name, Element::ElementType elementType, User* user, bool useJournal = true);

      void deleteElement (Element * element, User* user, bool useJournal = true);

      void deleteElements (std::vector<Element *> elements, User* user, bool useJournal = true);
	  void deleteElements2(std::vector<Element *> elementList, User* user, bool useJournal = true);

      void changeElementName (Element * element, const string& name, User* user);

      void changeElementType (Element * element, Element::ElementType elementType, User* user, bool setConsolidated);

      void moveElement (Element * element, PositionType newPosition, User* user);

      void addChildren (Element * parent, const ElementsWeightType * children, User* user);

      void removeChildrenNotIn (User* user, Element * parent, set<Element*> * keep);

      void removeChildren (User* user, Element * parent);

      set<Element*> getBaseElements (Element* parent, bool* multiple);

      size_t sizeElements () {
        return numElements;
      }

      set<Element*> ancestors (Element * child);

      const ParentsType* getParents (Element * child) {
        ChildParentsPair * cpp = childToParents.findKey(child);
        return cpp == 0 ? &emptyParents : &cpp->parents;
      }

      const ElementsWeightType * getChildren (Element* parent) {
        ParentChildrenPair * pcp = parentToChildren.findKey(parent);
        return pcp == 0 ? &emptyChildren : &pcp->children;
      }

      bool isStringConsolidation (Element * element) {
        if (element->getElementType() != Element::CONSOLIDATED) {
          return false;
        }

        return stringConsolidations.findKey(element) != 0;
      }
    
      Element * lookupElement (IdentifierType elementIdentifier) {
        return elementIdentifier < elements.size() ? elements[elementIdentifier] : 0;
      }

      Element * lookupElementByName (const string& name) {
        return nameToElement.findKey(name);
      }
    
      Element * lookupElementByPosition (PositionType position) {
        return positionToElement.findKey(position);
      }

      Element* findElement (IdentifierType elementIdentifier, User* user) {
        checkElementAccessRight(user, RIGHT_READ);

        Element * element = lookupElement(elementIdentifier);      

        if (element == 0) {
          throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_FOUND,
                                   "element with identifier " + StringUtils::convertToString((uint32_t) elementIdentifier) + " not found",
                                   "elementIdentifier", (int) elementIdentifier); 
        }
      
        return element;
      }
    
      Element * findElementByName (const string& name, User* user) {
        checkElementAccessRight(user, RIGHT_READ);

        Element * element = lookupElementByName(name);      

        if (element == 0) {
          throw ParameterException(ErrorException::ERROR_ELEMENT_NOT_FOUND,
                                   "element with name '" + name + "' not found",
                                   "name", name);
        }

        return element;
      }
    

      Element * findElementByPosition (PositionType position, User* user) {
        checkElementAccessRight(user, RIGHT_READ);

        Element * element = lookupElementByPosition(position);

        if (element == 0) {
          throw ParameterException(ErrorException::ERROR_INVALID_POSITION,
                                   "element with position " + StringUtils::convertToString((uint32_t) position) + " not found",
                                   "position", (int) position); 
        }

        return element;
      }

	protected:
		virtual bool hasStringAttributes() { return false; }

    private:
      static ElementsWeightType emptyChildren;
      static ParentsType emptyParents;

    private:
      uint32_t loadDimensionOverview (FileReader* file);

      void loadDimensionElementParents (FileReader* file,
                                        Element* element,
                                        IdentifiersType* parents,
                                        uint32_t numElements);

      void loadDimensionElementChildren (FileReader* file,
                                         Element* element,
                                         IdentifiersType* children,
                                         vector<double>* weights,
                                         uint32_t numElements);

      void loadDimensionElement (FileReader* file,
                                 uint32_t numElements);

      void loadDimensionElements (FileReader* file,
                                  uint32_t numElements);

      void saveDimensionOverview (FileWriter* file);

      void saveDimensionElement (FileWriter* file, Element* element);

      void saveDimensionElements (FileWriter* file);

      void checkUpdateConsolidationType (User* user, Element* element);

      void removeParentInChildrenNotIn (Element* parent, ElementsWeightType* ew, set<Element*> * keep);

      void removeParentInChildren (Element* parent, ElementsWeightType* ew);

      void removeChildInParents (User*, Element* element, bool isString);

      void addChildrenNC (User*, Element * parent, const ElementsWeightType * children);

      bool isCycle (const ParentsType*, const ElementsWeightType*);

      void removeElementFromCubes(User* user, Element * element, 
                                  bool processStorageDouble, 
                                  bool processStorageString, 
                                  bool deleteRules,
								  bool skipStorageStringInAttributeCubes = false);

	  void removeElementFromCubes (User* user,
									std::vector<Element*> elements, 
									bool processStorageDouble,
									bool processStorageString,
									bool deleteRules,
									bool skipStorageStringInAttributeCubes = false);
    
      void updateLevel ();

      void checkElementName (const string& name);
      
      void addParrentsToSortedList (Element* child, set<Element*>* knownElements);
    
    private:
      vector<Element*> elements;        // list of elements
      size_t numElements;               // number of used elements in the list of elements
      set<IdentifierType> freeElements; // list of unused elements in the list of elements

      AssociativeArray<Element*, ParentChildrenPair*, Parent2ChildrenDesc> parentToChildren;
      AssociativeArray<Element*, ChildParentsPair*, Child2ParentsDesc> childToParents;
      AssociativeArray<string, Element*, Name2ElementDesc> nameToElement;
      AssociativeArray<PositionType, Element*, Position2ElementDesc> positionToElement;
      AssociativeArray<Element*, Element*, ElementSetDesc> stringConsolidations;

      bool isValidLevel;    // true if maxLevel, maxDepth, and maxIndent are valid
      LevelType maxLevel;   // max level of elements, 0 = base element, levelParent = max(levelChild) + 1
      IndentType maxIndent; // max indents of elements, 1 = element has no father, indentChild = indent of first father + 1
      DepthType maxDepth;   // max depth of elements, 0 = element has no father, depthChild = max(depthFather) + 1

      bool isValidBaseElements;        // true if the list of base elements off all elements is valid       
      
      bool isValidSortedElements;      // true if the list of topological elements is valid 
      deque<Element*> sortedElements;  // list of topological sorted elements
  
  };

}

#endif
