////////////////////////////////////////////////////////////////////////////////
/// @brief base class for parser source or destination node
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

#include "Parser/AreaNode.h"

namespace palo {
  AreaNode::AreaNode (vector< pair<string*, string*> * > * elements, vector< pair<int, int> * > * elementsIds)
    : elements(elements), elementsIds(elementsIds), unrestrictedDimensions(false), nodeArea(0) {
  }



  AreaNode::~AreaNode () {
    if (elements) {
      for (vector<pair<string*, string*>*>::iterator i = elements->begin(); i != elements->end(); i++) {
        if (*i != 0) {
          if ((*i)->first) {
            delete (*i)->first;
          }

          if ((*i)->second) {
            delete (*i)->second;
          }

          delete *i;
        }
      }

      delete elements;
    }

    if (elementsIds) {
      for (vector<pair<int, int>*>::iterator i = elementsIds->begin(); i != elementsIds->end(); i++ ) {
        if (*i != 0) {
          delete *i;
        }
      }

      delete elements;
    }

    if (nodeArea) {
      delete nodeArea;
    }
  }



  bool AreaNode::validate (Server* server, Database* database, Cube* cube, Node* area, string& error) {
    if (database == 0 || cube == 0) {
      valid = true;
    }
    else if (elements) {
      valid = validateNames(server, database, cube, area, error);
    }
    else if (elementsIds) {
      valid = validateIds(server, database, cube, area, error);
    }
    else {
      valid = false;
    }

    return valid;
  }



  bool AreaNode::validateNamesArea (Server* server, Database* database, Cube* cube, Node* area, string& error) {
    const vector<Dimension*>* dimensions = cube->getDimensions();

    // list of dimension identifiers
    dimensionIDs.resize(dimensions->size(), 0);

    // list of element identifiers
    elementIDs.resize(dimensions->size(), 0);

    // is restricted, i. e. DIMENSION:ELEMENT or ELEMENT
    isRestricted.resize(dimensions->size(), false);

    // is qualified, i. e. DIMENSION:ELEMENT
    isQualified.resize(dimensions->size(), false);

    // sequence given by user
    elementSequence.resize(dimensions->size(), -1);

    // set up dimension map and keep track of the remaining dimensions
    map<Dimension*, int> dim2pos;
    set<Dimension*> remainingDimensions;

    int pos = 0;

    for (vector<Dimension*>::const_iterator i = dimensions->begin(); i != dimensions->end(); i++) {
      dim2pos[*i] = pos++;
      remainingDimensions.insert(*i);
    }

    // first: process elements with a dimension name
    pos = 0;

    for (vector<pair<string*, string*>*>::iterator i = elements->begin(); i != elements->end(); i++, pos++) {
      if (*i != 0) {
        string* dimName = (*i)->first;
        string* elmName = (*i)->second;

        // element has a name
        if (dimName) {
          Dimension* dim = database->lookupDimensionByName(*dimName);

          // error: dimension not found
          if (!dim) {
            error = "dimension '" + *dimName + "' not found";
            return false;
          }

          if (dim2pos.find(dim) == dim2pos.end()) {
            error = "dimension '" + *dimName + "' is not a cube dimension";
            return false;
          }

          int dimp = dim2pos[dim];

          if (elmName) {
            Element* elm = dim->lookupElementByName(*elmName);

            // error: element not found
            if (!elm) {
              error = "element '" + *elmName + "' not found";
              return false;
            }

            elementIDs[dimp] = elm->getIdentifier();
            isRestricted[dimp] = true;
            isQualified[dimp] = true;
            elementSequence[pos] = dimp;
          }
          else {
            isRestricted[dimp] = false;
            unrestrictedDimensions = true;
          }

          dimensionIDs[dimp] = dim->getIdentifier();
          remainingDimensions.erase(dim);
        }
      }
    }

    // process elements without dimension name
    pos = 0;

    for (vector<pair<string*, string*>*>::iterator i = elements->begin(); i != elements->end(); i++, pos++) {
      if (*i != 0) {
        string* dimName = (*i)->first;
        string* elmName = (*i)->second;

        // element has no name
        if (!dimName && elmName) {
          Element* elm = 0;

          // 1. lookup element in dimension number "pos"
          if (pos >= (int) dimensions->size()) {
            error = "too many dimensions";
            return false;
          }

          Dimension* dim = dimensions->at(pos);
          set<Dimension*>::iterator d = remainingDimensions.find(dim);

          if (d != remainingDimensions.end()) {
            elm = dim->lookupElementByName(*elmName); // if element is not found, fall through next if statement
          }

          // 2. lookup element in remaining dimensions
          if (!elm) {
            for (int offset = 1;  offset < (int) dimensions->size();  offset++) {
              int dimp = (pos + offset) % (int) dimensions->size();

              if (dimp >= (int) dimensions->size()) {
                error = "too many dimensions";
                return false;
              }

              dim = dimensions->at(dimp);

              if (remainingDimensions.find(dim) != remainingDimensions.end()) {
                elm = dim->lookupElementByName(*elmName);

                // element found
                if (elm) {

                  break;
                }
              }
            }

            // error: element not found
            if (!elm) {
              error = "element '" + *elmName + "' not found";
              return false;
            }
          }

          int dimp = dim2pos[dim];

          remainingDimensions.erase(dim);
          dimensionIDs[dimp] = dim->getIdentifier();
          elementIDs[dimp] = elm->getIdentifier();
          isRestricted[dimp] = true;
          isQualified[dimp] = false;
          elementSequence[pos] = dimp;
        }
      }
    }

    for (set<Dimension*>::iterator j = remainingDimensions.begin(); j != remainingDimensions.end(); j++) {
      Dimension* dim = *j;
      dimensionIDs[dim2pos[dim]] = dim->getIdentifier();
      isRestricted[dim2pos[dim]] = false;
      unrestrictedDimensions = true;
    }

    // construct area
    nodeArea = new AreaNode::Area();    

    vector<IdentifierType>::iterator e = elementIDs.begin();
    vector<bool>::iterator r = isRestricted.begin();

    for (;  r != isRestricted.end();  e++, r++) {
      set<IdentifierType> v;

      if (*r) {
        v.insert(*e);
        nodeArea->push_back(v);
      }
      else {
        nodeArea->push_back(v);
      }
    }        

    return true;
  }



  bool AreaNode::validateIdsArea (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
    const vector<Dimension*>* dimensions = cube->getDimensions();

    // list of dimension identifiers
    dimensionIDs.resize(dimensions->size(), 0);

    // list of element identifiers
    elementIDs.resize(dimensions->size(), 0);

    // is restricted, i. e. DIMENSION:ELEMENT or ELEMENT
    isRestricted.resize(dimensions->size(), false);

    // is qualified, i. e. DIMENSION:ELEMENT
    isQualified.resize(dimensions->size(), false);

    // sequence given by user
    elementSequence.resize(dimensions->size(), -1);

    // set up dimension map and keep track of the remaining dimensions
    map<Dimension*, int> dim2pos;
    set<Dimension*> remainingDimensions;

    int pos = 0;

    for (vector<Dimension*>::const_iterator i = dimensions->begin(); i != dimensions->end(); i++) {
      dim2pos[*i] = pos++;
      remainingDimensions.insert(*i);
    }

    // process element identifiers of the area
    pos = 0;

    for (vector<pair<int, int>*>::iterator i = elementsIds->begin(); i != elementsIds->end(); i++, pos++) {
      pair<int,int>* ei = *i;

      if (ei != 0) {
        int dimId = (*i)->first;
        int elmId = (*i)->second;
        bool qualified = false;

        if (dimId < 0) {
          dimId = -(dimId + 1);
        }
        else {
          qualified = true;
        }

        Dimension * dim = database->lookupDimension(dimId);

        if (dim == 0) {
          error = "dimension not found";
          return false;
        }

        remainingDimensions.erase(dim);

        int dimp = dim2pos[dim];

        if (elmId < 0) {
          isRestricted[dimp] = false;
          unrestrictedDimensions = true;
        }
        else {
          Element * elm = dim->lookupElement(elmId);

          if (elm == 0) {
            error = "element not found";
            return false;
          }

          elementIDs[dimp] = elmId;
          isRestricted[dimp] = true;
          isQualified[dimp] = qualified;
          elementSequence[pos] = dimp;
        }

        dimensionIDs[dimp] = dimId;
      }
    }

    for (set<Dimension*>::iterator j = remainingDimensions.begin(); j != remainingDimensions.end(); j++) {
      Dimension* dim = *j;
      dimensionIDs[dim2pos[dim]] = dim->getIdentifier();
      isRestricted[dim2pos[dim]] = false;
      unrestrictedDimensions = true;
    }

    nodeArea = new AreaNode::Area();    

    vector<IdentifierType>::iterator e = elementIDs.begin();
    vector<bool>::iterator r = isRestricted.begin();

    for (;  r != isRestricted.end();  e++, r++) {
      set<IdentifierType> v;

      if (*r) {
        v.insert(*e);
        nodeArea->push_back(v);
      }
      else {
        nodeArea->push_back(v);
      }
    }        
    
    return true;
  }


  void AreaNode::appendXmlRepresentationType (StringBuffer* sb, int indent, bool names, const string& type) {
    identXML(sb, indent);

    sb->appendText("<" + type + ">");
    sb->appendEol();

    if (names) {
      for (vector<pair<string*, string*>*>::iterator i = elements->begin();  i != elements->end();  i++) {
        identXML(sb, indent + 1);

        sb->appendText("<dimension");

        if (*i != 0) {
          if ((*i)->first) {
            sb->appendText(" id=\"");
            sb->appendText(StringUtils::escapeXml(*(*i)->first));
            sb->appendText("\"");
          }

          if ((*i)->second) {
            sb->appendText(" restriction=\"");
            sb->appendText(StringUtils::escapeXml(*(*i)->second));
            sb->appendText("\"");
          }
        }

        sb->appendText("/>");
        sb->appendEol();
      }
    }
    else {
      vector<IdentifierType>::iterator d = dimensionIDs.begin();
      vector<IdentifierType>::iterator e = elementIDs.begin();
      vector<bool>::iterator r = isRestricted.begin();

      for (; d != dimensionIDs.end(); d++, e++, r++) {
        identXML(sb, indent + 1);

        sb->appendText("<dimension id=\"");
        sb->appendInteger(*d);

        if (*r) {
          sb->appendText("\" restriction=\"");
          sb->appendInteger(*e);
          sb->appendText("\"/>");
        }
        else {
          sb->appendText("\"/>");
        }

        sb->appendEol();
      }
    }

    identXML(sb,indent);
    sb->appendText("</" + type + ">");
    sb->appendEol();
  }



  void AreaNode::appendRepresentation (StringBuffer* sb, Cube* cube, bool isMarker) const {
    if (cube) {
      sb->appendText(isMarker ? "[[" : "[");

      if (! elementSequence.empty()) {
        Database * database = cube->getDatabase();
        vector<int>::const_iterator esb = elementSequence.begin();
        vector<int>::const_iterator ese = elementSequence.end() - 1;
        
        // find first entry from the end which is not equal -1
        bool found = false;

        // MSVC++ does not allow ese to be incremented below esb
        for (; esb <= ese; ese--) {
          if (*ese != -1) {
            found= true;
            break;
          }

          if (esb == ese) {
            break;
          }
        }
        
        // produce restrictions within this range
        if (found) {
          for (vector<int>::const_iterator iter = esb;  iter <= ese;  iter++) {
            if (iter != esb) {
              sb->appendChar(',');
            }
            
            int pos = *iter;
            
            if (pos != -1 && isRestricted[pos]) {
              bool qualified = isQualified[pos];
              
              IdentifierType d = dimensionIDs[pos];
              IdentifierType e = elementIDs[pos];
              Dimension * dim = database->findDimension(d, 0);
              Element * elm = dim->findElement(e, 0);
              
              if (qualified) {
                sb->appendText(StringUtils::escapeStringSingle(dim->getName()));
                sb->appendChar(':');
                sb->appendText(StringUtils::escapeStringSingle(elm->getName()));
              }
              else {
                sb->appendText(StringUtils::escapeStringSingle(elm->getName()));
              }
            }
          }
        }
      }

      sb->appendText(isMarker ? "]]" : "]");
    }
    else {
      sb->appendText(isMarker ? "{{" : "{");

      if (! elementSequence.empty()) {
        vector<int>::const_iterator esb = elementSequence.begin();
        vector<int>::const_iterator ese = elementSequence.end() - 1;
        
        // find first entry from the end which is not equal -1
        bool found = false;

        // MSVC++ does not allow ese to be incremented below esb
        for (; esb <= ese; ese--) {
          if (*ese != -1) {
            found = true;
            break;
          }

          if (esb == ese) {
            break;
          }
        }
        
        // produce restrictions within this range
        if (found) {
          for (vector<int>::const_iterator iter = esb;  iter <= ese;  iter++) {
            if (iter != esb) {
              sb->appendChar(',');
            }
            
            int pos = *iter;
            
            if (pos != -1 && isRestricted[pos]) {
              bool qualified = isQualified[pos];
              
              IdentifierType d = dimensionIDs[pos];
              IdentifierType e = elementIDs[pos];
              
              if (qualified) {
                sb->appendInteger(d);
                sb->appendChar(':');
                sb->appendInteger(e);
              }
              else {
                sb->appendInteger(d);
                sb->appendChar('@');
                sb->appendInteger(e);
              }
            }
          }
        }
      }

      sb->appendText(isMarker ? "}}" : "}");
    }
  }
}
