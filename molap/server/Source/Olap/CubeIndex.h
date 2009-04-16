////////////////////////////////////////////////////////////////////////////////
/// @brief palo cube index
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

#ifndef OLAP_CUBE_INDEX_H
#define OLAP_CUBE_INDEX_H 1

#include "palo.h"

#include <string>
#include <vector>

#include "Collections/AssociativeArray.h"
#include "Collections/StringUtils.h"

#include "Olap/CubePage.h"

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP cube index
  ///
  /// An OLAP cube stores the index of a cube storage
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CubeIndex {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates a new index
    ////////////////////////////////////////////////////////////////////////////////

    CubeIndex (size_t keySize, size_t valueSize) 
      : keySize(keySize),
        valueSize(valueSize),
        index(1000, IndexDesc(keySize, valueSize)) {
    }

  public:
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief find a key in index
    ///
    /// Returns "0" 
    ////////////////////////////////////////////////////////////////////////////////    

    CubePage::element_t lookupKey (CubePage::key_t key) {
      return index.findKey(key);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds or updates an element in the index
    ////////////////////////////////////////////////////////////////////////////////    

    void addElement (CubePage::element_t element) {
      index.addElement(element);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief find a key in index and remove entry in storage
    ////////////////////////////////////////////////////////////////////////////////    

    CubePage::element_t removeKey (CubePage::key_t key) {
      return index.removeKey(key);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a pointer to the associative array
    ///
    /// @warning if you add elements to the index the pointer is invalidated.
    ////////////////////////////////////////////////////////////////////////////////    

    CubePage::element_t const * getArray (size_t& size) {
      size = index.size();
      return index.getTable();
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief clears the associative array
    ///
    /// @warning if you add elements to the index the pointer is invalidated.
    ////////////////////////////////////////////////////////////////////////////////    

    void clear () {
      index.clear();
    }
    
  private:
    struct IndexDesc {
      IndexDesc (size_t keySize, size_t valueSize) 
        : keySize(keySize), valueSize(valueSize) {
      }

      uint32_t hash (CubePage::key_t key) {
        return StringUtils::hashValue32((uint32_t*) key, keySize / 4);
      }

      bool isEmptyElement (CubePage::element_t & element) {
        return element == 0;
      }

      uint32_t hashElement (CubePage::element_t const & element) {
        return hash(element + valueSize);
      }

      uint32_t hashKey (CubePage::key_t key) {
        return hash(key);
      }

      bool isEqualElementElement (CubePage::element_t const & left, CubePage::element_t const & right) {
        return memcmp(left + valueSize, right + valueSize, keySize) == 0;
      }

      bool isEqualKeyElement (CubePage::key_t key, CubePage::element_t const & element) {
        return memcmp(key, element + valueSize, keySize) == 0;
      }

      void clearElement (CubePage::element_t & element) {
        element = 0;
      }

    private:
      size_t keySize;
      size_t valueSize;
    };

  private:
    size_t keySize;
    size_t valueSize;
    AssociativeArray<CubePage::key_t, CubePage::element_t, IndexDesc> index;
  };
  
}

#endif 
