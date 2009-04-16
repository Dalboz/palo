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

#ifndef OLAP_LOCK_H
#define OLAP_LOCK_H 1

#include <set>
#include <string>
#include <vector>

#include "palo.h"
#include "Olap/RollbackStorage.h"

namespace palo {
  class Cube;
  class CellPath;
  class Dimension;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP cube area lock
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Lock { 

  public:
    static Lock * checkLock;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new locked area
    ////////////////////////////////////////////////////////////////////////////////

    Lock (IdentifierType identifier, 
          Cube* cube, 
          vector<IdentifiersType>* areaVector, 
          const string& areaString,
          IdentifierType userId,
          FileName* cubeFileName);


    
    Lock () {
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Destructor
    ////////////////////////////////////////////////////////////////////////////////

    virtual ~Lock ();
    
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name getter and setter
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets cube of rule
    ////////////////////////////////////////////////////////////////////////////////

    Cube* getCube () const {
      return cube;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets identifier of rule
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType getIdentifier () const {
      return identifier;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get lock area as String
    ////////////////////////////////////////////////////////////////////////////////

    const string& getAreaString () const {
      return areaString;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get the identifier of the user
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType getUserIdentifier () {
      return userId;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief get contains area
    ////////////////////////////////////////////////////////////////////////////////

    const vector< set<IdentifierType> >& getContainsArea () {
      return containsArea;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if a locked area is touched by a cell path 
    ///
    /// Returns true if the cell path changes values in the locked area.
    ////////////////////////////////////////////////////////////////////////////////

    bool contains (CellPath* cellPath);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if a locked area is blocking a cell path 
    ///
    /// Returns true if the cell path splashes or set values in the locked area.
    ////////////////////////////////////////////////////////////////////////////////

    bool blocks (CellPath* cellPath);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if another locked area overlaps to the locked area 
    ////////////////////////////////////////////////////////////////////////////////

    bool overlaps (const vector< set<IdentifierType> >& area);
    
    
    RollbackStorage* getStorage () {
      return storage;
    }
    
  private:

    void computeContains (vector<IdentifiersType>* area);
    
    void computeChildren (set<IdentifierType>*, Dimension*, Element*);

    void computeAncestors (set<IdentifierType>*, Dimension*, Element*);
    
  private:
    Cube* cube;
    IdentifierType identifier;
    string areaString;
    IdentifierType userId;

    // area elements and all children 
    vector< set<IdentifierType> > containsArea;

    // ancestor elements of the locked area
    vector< set<IdentifierType> > ancestorsIdentifiers;

    // area elements, all children and ancestor elements  
    vector< set<IdentifierType> > overlapArea;
    
    RollbackStorage* storage;
    
  };
}

#endif
