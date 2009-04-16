////////////////////////////////////////////////////////////////////////////////
/// @brief parser destination node
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

#include "Parser/DestinationNode.h"

namespace palo {
  DestinationNode::DestinationNode (vector< pair<string*, string*> *> *elements, vector< pair<int, int> *> *elementsIds) 
    : AreaNode(elements, elementsIds) {
  }



  DestinationNode::~DestinationNode () {
  }
  

  
  Node * DestinationNode::clone () {
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

    DestinationNode * cloned = new DestinationNode(clonedElements, clonedElementsIds);

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

    return cloned;
  }
  

  
  DestinationNode::ValueType DestinationNode::getValueType () {
    return Node::UNKNOWN;
  }
  


  bool DestinationNode::validateNames (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    return validateNamesArea(server, database, cube, destination, error);
  }



  bool DestinationNode::validateIds (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    return validateIdsArea(server, database, cube, destination, error);
  }



  bool DestinationNode::hasElement (Dimension* dimension, IdentifierType element) const {
    vector<IdentifierType>::const_iterator dIter = dimensionIDs.begin();
    vector<IdentifierType>::const_iterator eIter = elementIDs.begin();

    for (; dIter != dimensionIDs.end() && eIter != elementIDs.end(); dIter++, eIter++) {
      if (dimension->getIdentifier() == *dIter && element == *eIter) {
        return true;
      }
    }
    
    return false;
  }

  
  
  void DestinationNode::appendXmlRepresentation (StringBuffer* sb, int indent, bool names) {
    appendXmlRepresentationType(sb, indent, names, "destination");
  }
}
