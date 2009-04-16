////////////////////////////////////////////////////////////////////////////////
/// @brief palo rollback page
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

#include "Olap/RollbackPage.h"

#include <iostream>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#ifdef ALIGNOF_DOUBLE
# ifdef ALIGNOF_VOIDP
#   if ALIGNOF_DOUBLE < ALIGNOF_VOIDP
#     define ELEMENT_ALIGNMENT ALIGNOF_VOIDP
#   else
#     define ELEMENT_ALIGNMENT ALIGNOF_DOUBLE
#   endif
# else
#   define ELEMENT_ALIGNMENT ALIGNOF_DOUBLE
# endif
#else
# ifdef ALIGNOF_VOIDP
#   define ELEMENT_ALIGNMENT ALIGNOF_VOIDP
# else
#   define ELEMENT_ALIGNMENT 4
# endif
#endif


namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  RollbackPage::RollbackPage (size_t keySize, size_t valueSize, size_t maximumSize)
    : keySize(keySize), valueSize(valueSize), maximumSize(maximumSize) {

    rowSize = ((valueSize + keySize + sizeof(uint32_t) + ELEMENT_ALIGNMENT - 1) / ELEMENT_ALIGNMENT) * ELEMENT_ALIGNMENT;

    totalSize = PAGE_SIZE;
    numberElements = totalSize / rowSize;
    usedElements = 0;

    maximumElements = maximumSize / rowSize;

    buffer = new uint8_t [totalSize];
    memset(buffer, 0, totalSize);

    tmpBuffer = new uint8_t [rowSize];
    memset(tmpBuffer, 0, rowSize);
  }



  RollbackPage::~RollbackPage () {    
    delete [] buffer;
    delete [] tmpBuffer;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // add and remove entries
  ////////////////////////////////////////////////////////////////////////////////

  RollbackPage::element_t RollbackPage::addElement (element_t row) {
    if (usedElements == numberElements) {
      grow();
    }

    element_t result = buffer + usedElements * rowSize;
    memcpy(result, row, valueSize + keySize);

    usedElements++;

    return result;
  }



  void RollbackPage::removeLastElement () {
    usedElements--;
    
    // we can shrink here
  }


  RollbackPage::buffer_t RollbackPage::getCell (size_t pos) {
    if (pos < usedElements) {
      return buffer + pos * rowSize;
    }
    return 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // internal methods
  ////////////////////////////////////////////////////////////////////////////////

  void RollbackPage::grow () {

    // create new buffer
    size_t newTotalSize = ((3 * totalSize / 2 + PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE;
    
    if (newTotalSize > maximumSize) {
      newTotalSize = maximumSize;
    }
    
    buffer_t newBuffer = new uint8_t [newTotalSize];

    memset(newBuffer, 0, newTotalSize);
    memcpy(newBuffer, buffer, totalSize);

    // delete old buffer
    delete[] buffer;

    buffer = newBuffer;
    totalSize = newTotalSize;
    numberElements = newTotalSize / rowSize;
  }
  
}
