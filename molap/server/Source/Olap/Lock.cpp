////////////////////////////////////////////////////////////////////////////////
/// @brief palo cube area lock
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

#include "Olap/Lock.h"
#include "Olap/Cube.h"
#include "Olap/Dimension.h"
#include "Olap/Element.h"

namespace palo {
  Lock * Lock::checkLock = new Lock();



  Lock::Lock (IdentifierType identifier, 
                Cube* cube, 
                vector<IdentifiersType>* areaVector, 
                const string& areaString,
                IdentifierType userId,
                FileName* cubeFileName)    
        : cube(cube), identifier(identifier), areaString(areaString), userId(userId) {
      
    computeContains(areaVector);
    storage = new RollbackStorage(cube->getDimensions(), cubeFileName, identifier);
  }



  Lock::~Lock () {
    delete storage;
  } 

  ////////////////////////////////////////////////////////////////////////////////
  // destination related methods
  ////////////////////////////////////////////////////////////////////////////////

  bool Lock::contains (CellPath* cellPath) {
    return Cube::isInArea(cellPath, &containsArea);
  }



  bool Lock::blocks (CellPath* cellPath) {
    return Cube::isInArea(cellPath, &overlapArea);
  }



  void Lock::computeContains (vector<IdentifiersType>* area) {
    containsArea.clear();
    containsArea.resize(area->size());

    ancestorsIdentifiers.clear();
    ancestorsIdentifiers.resize(area->size());
    
    overlapArea.clear();
    overlapArea.resize(area->size());

    vector< set<IdentifierType> >::iterator containsIterator = containsArea.begin();
    vector< set<IdentifierType> >::iterator ancestorsIterator = ancestorsIdentifiers.begin();
    vector< set<IdentifierType> >::iterator overlapIterator = overlapArea.begin();
    vector< IdentifiersType >::iterator src = area->begin();
    vector<Dimension*>::const_iterator dims = cube->getDimensions()->begin();

    for (; src != area->end(); containsIterator++, ancestorsIterator++, overlapIterator++, src++, dims++) {
      set<IdentifierType>& children = *containsIterator;
      set<IdentifierType>& ancestors = *ancestorsIterator;
      set<IdentifierType>& overlap = *overlapIterator;
      IdentifiersType& sst = *src;

      if (sst.empty()) {
      }
      else {
        Dimension * d = *dims;

        // compute childen
        for (IdentifiersType::iterator iter = sst.begin();  iter != sst.end();  iter++) {
          IdentifierType id = *iter;
          Element * e = d->findElement(id, 0);

          // simple children
          children.insert(e->getIdentifier()); 

          if (e->getElementType() == Element::CONSOLIDATED && !d->isStringConsolidation(e)) {
            computeChildren(&children, d, e);
          }
        }

        // compute (not string) ancestors
        for (IdentifiersType::iterator iter = sst.begin();  iter != sst.end();  iter++) {
          IdentifierType id = *iter;
          Element * e = d->findElement(id, 0);
          
          // do not add parents of string nodes
          if (e->getElementType() == Element::NUMERIC ||
             (e->getElementType() == Element::CONSOLIDATED && !d->isStringConsolidation(e))) {
            computeAncestors(&ancestors, d, e);
          }
        }
        
        // copy children and ancestors to the overlap set
        overlap.insert(children.begin(), children.end());
        overlap.insert(ancestors.begin(), ancestors.end());
      }
    }
  }



  void Lock::computeChildren (set<IdentifierType>* children, Dimension* dimension, Element* parent) {
    
    const ElementsWeightType * childEdges = dimension->getChildren(parent);

    for (ElementsWeightType::const_iterator i = childEdges->begin(); i != childEdges->end(); i++) {
      Element* child = i->first;
      children->insert(child->getIdentifier());
      
      if (child->getElementType() == Element::CONSOLIDATED) {
        computeChildren(children, dimension, child);
      }
    } 
  }



  void Lock::computeAncestors (set<IdentifierType>* ancestors, Dimension* dimension, Element* child) {
    
    const vector<Element*>* parents = dimension->getParents(child);

    for (vector<Element*>::const_iterator i = parents->begin(); i != parents->end(); i++) {
      Element* parent = *i;
 
      if (!dimension->isStringConsolidation(parent)) {
        ancestors->insert(parent->getIdentifier());      
        computeAncestors(ancestors, dimension, parent);
      }
      
    } 
  }

  
  
  bool Lock::overlaps (const vector< set<IdentifierType> >& area) {
    
    vector< set<IdentifierType> >::const_iterator dimIterA = area.begin();
    vector< set<IdentifierType> >::const_iterator dimIterB = overlapArea.begin();
    
    for (; dimIterA != area.end(); dimIterA++, dimIterB++) {
      const set<IdentifierType>& identifiersA = (*dimIterA);
      const set<IdentifierType>& identifiersB = (*dimIterB);
      
      bool foundIdentifier = false;
      
      for (set<IdentifierType>::const_iterator idIterA = identifiersA.begin(); idIterA != identifiersA.end(); idIterA++) {
        set<IdentifierType>::const_iterator found =  identifiersB.find(*idIterA);
        if (found != identifiersB.end()) {
          foundIdentifier = true;
          break;
        }
      }
      
      if (!foundIdentifier) {
        // no dimension element of area found in containsArea
        return false;
      }
    }
    
    return true;
  }
}
