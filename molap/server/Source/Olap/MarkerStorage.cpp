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

#include "Olap/MarkerStorage.h"

#include "InputOutput/Logger.h"

namespace palo {
  const uint32_t MarkerStorage::NO_MAPPING = ~0;

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  MarkerStorage::MarkerStorage (size_t numberDimensions,
                                const uint32_t* fixed,
                                const uint32_t* perms,
                                const vector< vector< uint32_t > > * maps)
    : numberElements(0),
      numberDimensions(numberDimensions),
      maps(maps) {
    keySize = numberDimensions * 32 / 8;

    Logger::trace << "creating new MarkerStorage: key size = " << keySize << endl;

    // generate index for base elements
    index = new CubeIndex(keySize, 0);

    // generate cube page
    cubePage = new CubePage(index, keySize, 0, 0, 1);

    // copy permutations
    permutations = new uint32_t[numberDimensions];
    memcpy(permutations, perms, numberDimensions * 32 / 8);

    // create key buffer and add fixed keys
    tmpKeyBuffer = new uint32_t [numberDimensions];
    tmpKeyBufferEnd = tmpKeyBuffer + numberDimensions;

    uint32_t* ptr = tmpKeyBuffer;
    const uint32_t* qtr = fixed;

    for (const uint32_t* perm = perms;  perm < perms + numberDimensions;  perm++, ptr++, qtr++) {
      if (*perm == NO_PERMUTATION) {
        *ptr = *qtr;
      }
    }
  }



  MarkerStorage::~MarkerStorage () {
    delete[] tmpKeyBuffer;
    delete cubePage;
    delete index;
    delete[] permutations;
  }
}
