////////////////////////////////////////////////////////////////////////////////
/// @brief palo marker storage
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

#ifndef OLAP_MARKER_STORAGE_H
#define OLAP_MARKER_STORAGE_H 1

#include "palo.h"

#include <set>
#include <map>

#include "Exceptions/ErrorException.h"

#include "Olap/CellPath.h"
#include "Olap/CubeIndex.h"
#include "Olap/CubePage.h"
#include "Olap/Element.h"

namespace palo {
  class AreaStorage;
  class CubeLooper;
  class ExportStorage;
  class HashAreaStorage;
  class Lock;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo cube storage
  ///
  /// A cube storage uses cube pages with a fixed memory size to store the values.
  /// Each cube page is divided into rows. A row consits of a value (double value
  /// or char*) and a key. The memory size needed for the row is calculated
  /// by the constructor.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS MarkerStorage {
  public:
    static const uint32_t NO_PERMUTATION = ~0;
    static const uint32_t NO_MAPPING;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Creates an empty cube storage
    ////////////////////////////////////////////////////////////////////////////////

    MarkerStorage (size_t numberDimensions,
                   const uint32_t* fixed,
                   const uint32_t* permutations,
                   const vector< vector<uint32_t> >*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Remove a cube storage
    ////////////////////////////////////////////////////////////////////////////////

    ~MarkerStorage ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief adds a coordinate vector of a cell
    ////////////////////////////////////////////////////////////////////////////////

    void setCellValue (const uint8_t * path) {
      setCellValue((const uint32_t*) path);
    }

    void setCellValue (const uint32_t * path) {
      uint32_t * ptr = tmpKeyBuffer;
      uint32_t * perm = permutations;

      if (maps == 0) {
        for (; ptr < tmpKeyBufferEnd;  ptr++, perm++) {
          if (*perm != NO_PERMUTATION) {
            *ptr = path[*perm];
          }
        }
      }
      else {
        vector< vector<uint32_t> >::const_iterator m = maps->begin();

        for (; ptr < tmpKeyBufferEnd;  ptr++, perm++, m++) {
          if (*perm != NO_PERMUTATION) {
            const vector<uint32_t>& mapping = *m;
            uint32_t id = path[*perm];

            if (id >= mapping.size() || mapping[id] == NO_MAPPING) {
              return;
            }

            *ptr = mapping[id];
          }
        }
      }

      CubePage::element_t element = index->lookupKey((uint8_t*) tmpKeyBuffer);

      if (element == 0) {
        cubePage->addElement((uint8_t*) tmpKeyBuffer, false);

        numberElements++;
      }
    }

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief number of used rows
    ////////////////////////////////////////////////////////////////////////////////

    size_t size () const {
      return numberElements;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief begin of storage
    ////////////////////////////////////////////////////////////////////////////////

    const uint8_t* begin () const {
      return cubePage->begin();
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief end of storage
    ////////////////////////////////////////////////////////////////////////////////

    const uint8_t* end () const {
      return cubePage->end();
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief rowsize
    ////////////////////////////////////////////////////////////////////////////////

    size_t getRowSize () const {
      return cubePage->getRowSize();
    };

  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief total number of elements
    ////////////////////////////////////////////////////////////////////////////////

    size_t numberElements;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief total number of dimensions in destination cube
    ////////////////////////////////////////////////////////////////////////////////

    size_t numberDimensions;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a path
    ////////////////////////////////////////////////////////////////////////////////

    size_t keySize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Pointer to a cube index object
    ////////////////////////////////////////////////////////////////////////////////

    CubeIndex * index;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief temporary buffer
    ////////////////////////////////////////////////////////////////////////////////

    uint32_t * tmpKeyBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief end of temporary buffer
    ////////////////////////////////////////////////////////////////////////////////

    uint32_t * tmpKeyBufferEnd;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief cube page
    ////////////////////////////////////////////////////////////////////////////////

    CubePage * cubePage;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief permutation of keys
    ////////////////////////////////////////////////////////////////////////////////

    uint32_t * permutations;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief mapping of identifiers between cubes
    ///
    /// The mapping is not owned by MarkerStorage. It must not be deleted.
    ////////////////////////////////////////////////////////////////////////////////

    const vector< vector<uint32_t> > * maps;
  };

}

#endif
