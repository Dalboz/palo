////////////////////////////////////////////////////////////////////////////////
/// @brief parser source node
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

#include "Parser/SourceNode.h"

namespace palo {
  SourceNode::SourceNode (vector< pair<string*, string*> *> *elements, vector< pair<int, int> *> *elementsIds) 
    : AreaNode(elements, elementsIds) {
    server = 0;
    database = 0;
    cube = 0;
    fixedCellPath = 0;
    marker = false;
  }



  SourceNode::SourceNode (vector< pair<string*, string*> *> *elements, vector< pair<int, int> *> *elementsIds, bool marker) 
    : AreaNode(elements, elementsIds), marker(marker) {
    server = 0;
    database = 0;
    cube = 0;
    fixedCellPath = 0;
  }



  SourceNode::~SourceNode () {
    if (fixedCellPath) {
      delete fixedCellPath;
    }
  }



  Node * SourceNode::clone () {
    vector< pair<string*, string*> * > * clonedElements = 0;
    vector< pair<int, int> * > * clonedElementsIds = 0;
    
    // create a copy of the elements and elements identifiers
    if (elements != 0) {
      clonedElements = new vector<pair<string*, string*>*>;

      for (vector<pair<string*, string*>*>::iterator i = elements->begin(); i != elements->end(); i++) {
        if (*i != 0) {
          string * newFirst = (*i)->first == 0 ? 0 : new string(*((*i)->first));
          string * newSecond = (*i)->second == 0 ? 0 : new string(*((*i)->second));
          pair<string*, string*> * p = new pair<string*, string*>(newFirst, newSecond);

          clonedElements->push_back(p);
        }
        else {
          clonedElements->push_back(0);
        }
      }
    }

    if (elementsIds != 0) {
      clonedElementsIds = new vector<pair<int, int>*>;

      for (vector<pair<int, int>*>::iterator i = elementsIds->begin(); i != elementsIds->end(); i++ ) {
        if (*i != 0) {
          pair<int, int> * p = new pair<int, int>((*i)->first, (*i)->second);

          clonedElementsIds->push_back(p);
        }
        else {
          clonedElementsIds->push_back(0);
        }
      }
    }

    SourceNode * cloned = new SourceNode(clonedElements, clonedElementsIds);

    cloned->dimensionIDs           = this->dimensionIDs;
    cloned->elementIDs             = this->elementIDs;
    cloned->isRestricted           = this->isRestricted;
    cloned->isQualified            = this->isQualified;
    cloned->elementSequence        = this->elementSequence;
    cloned->unrestrictedDimensions = this->unrestrictedDimensions;

    cloned->nodeArea = new AreaNode::Area();

    for (AreaNode::Area::iterator i = this->nodeArea->begin();
         i != this->nodeArea->end();
         i++) {
      cloned->nodeArea->push_back(*i);
    }

    cloned->server   = this->server;
    cloned->database = this->database;
    cloned->cube     = this->cube;

    if (this->fixedCellPath != 0) {
      cloned->fixedCellPath = new CellPath(*this->fixedCellPath);
    }

    cloned->marker = this->marker;

    return cloned;
  }
  

  
  SourceNode::ValueType SourceNode::getValueType () {
    return Node::UNKNOWN;
  }



  bool SourceNode::validateNames (Server* server, Database* database, Cube* cube, Node* source, string& error) {
    this->server = server;
    this->database = database;
    this->cube = cube;
         
    bool result = validateNamesArea(server, database, cube, source, error);

    if (! result) {
      return false;
    }

    if (! unrestrictedDimensions) {
      // all dimensions are restricted, so we can build the CellPath here
      fixedCellPath = new CellPath(cube, &elementIDs);
    }
    
    return true;
  }
  
  
  bool SourceNode::validateIds (Server* server, Database* database, Cube* cube, Node* source, string& error) {
    this->server = server;
    this->database = database;
    this->cube = cube;

    bool result = validateIdsArea(server, database, cube, source, error);

    if (! result) {
      return false;
    }

    if (! unrestrictedDimensions) {
      // all dimensions are restricted, so we can build the CellPath here
      fixedCellPath = new CellPath(cube, &elementIDs);
    }
    
    return true;
  }

  
  
  Node::RuleValueType SourceNode::getValue (CellPath* cellPath, User* user, bool* usesOtherDatabase, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
    RuleValueType result;

    Cube::CellValueType value;
    Element::ElementType type;
    bool found;
    
    if (unrestrictedDimensions) {

      // compute source CellPath
      IdentifiersType elements;      
      const IdentifiersType *requestElements = cellPath->getPathIdentifier();      
      IdentifiersType::const_iterator source  = elementIDs.begin();
      IdentifiersType::const_iterator request = requestElements->begin();
      vector<bool>::const_iterator i = isRestricted.begin();      

      for (;i!=isRestricted.end(); i++, source++, request++) {
        if (*i) {
          elements.push_back(*source);
        }
        else {
          elements.push_back(*request);
        }
      }      
      CellPath cp(cube, &elements);
      type = cp.getPathType();
      
      value = cube->getCellValue(&cp, &found, user, 0, ruleHistory);
    }
    else {
      type = fixedCellPath->getPathType();
      value = cube->getCellValue(fixedCellPath, &found, user, 0, ruleHistory);  
    }
    
    if (found) {
      if (value.type == Element::STRING) {
        result.type = Node::STRING; 
        result.stringValue = value.charValue;
      }
      else {
        result.type = Node::NUMERIC;
        result.doubleValue = value.doubleValue;
      }
    }
    else {
      if (type == Element::STRING) {
        result.type = Node::STRING; 
        result.stringValue = "";
      }
      else {
        result.type = Node::NUMERIC;
        result.doubleValue = 0.0;
      }
    }
    
    return result;
  }  
  
  
  bool SourceNode::hasElement (Dimension* dimension, IdentifierType element) const {
    vector<IdentifierType>::const_iterator dIter = dimensionIDs.begin();
    vector<IdentifierType>::const_iterator eIter = elementIDs.begin();

    for (; dIter != dimensionIDs.end() && eIter != elementIDs.end(); dIter++, eIter++) {
      if (dimension->getIdentifier() == *dIter && element == *eIter) {
        return true;
      }
    }
    
    return false;
  }
  

  
  void SourceNode::appendXmlRepresentation (StringBuffer* sb, int indent, bool names) {
    if (marker) {
      appendXmlRepresentationType(sb, indent, names, "marker");
    }
    else {
      appendXmlRepresentationType(sb, indent, names, "source");
    }
  }
}
