////////////////////////////////////////////////////////////////////////////////
/// @brief palo export page
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

#include "Olap/ExportPage.h"

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

  ExportPage::ExportPage (CubeIndex* index,
                size_t keySize,
                size_t valueSize,
                size_t numValues)
    : index(index),
      sorted(true),
      keySize(keySize), 
      valueSize(valueSize),
      numberElements(numValues) {

    rowSize = ((valueSize + keySize + sizeof(uint32_t) + ELEMENT_ALIGNMENT - 1) / ELEMENT_ALIGNMENT) * ELEMENT_ALIGNMENT;

    totalSize = numValues * rowSize;
    usedElements = 0;

    try {
      buffer = new uint8_t [totalSize];
      memset(buffer, 0, totalSize);
    }
    catch (std::bad_alloc) {
      throw ErrorException(ErrorException::ERROR_OUT_OF_MEMORY, "out of memory");
    }
    tmpBuffer = new uint8_t [rowSize];
    memset(tmpBuffer, 0, rowSize);
  }



  ExportPage::~ExportPage () {
    delete [] buffer;
    delete [] tmpBuffer;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // add and remove entries
  ////////////////////////////////////////////////////////////////////////////////

  ExportPage::element_t ExportPage::addElement (element_t row) {    
    if (usedElements == numberElements) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "in ExportPage::addElement too many elements");
    }

    element_t result = buffer + usedElements * rowSize;
    memcpy(result, row, valueSize + keySize);

    index->addElement(result);

    usedElements++;
    sorted = false;

    return result;
  }



  ExportPage::element_t ExportPage::removeElement (element_t row) {
    size_t pos = (row - buffer) / rowSize;

    if (pos >= usedElements) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "in ExportPage::removeElement");
    }

    usedElements--;

    index->removeKey(row + valueSize);

    if (usedElements == 0) {
      return 0;
    }

    if (pos != usedElements) {
      index->removeKey(buffer + usedElements * rowSize + valueSize);
      copyElement(pos, usedElements);
      index->addElement(row);
    }

    sorted = false;


    return row;
  }



  ExportPage::element_t ExportPage::removeLastElement () {
    if (usedElements == 0) {
      return 0;
    }
     
    size_t pos = usedElements-1;
    element_t row = buffer + pos * rowSize;

    usedElements--;

    index->removeKey(row + valueSize);

    if (usedElements == 0) {
      return 0;
    }

    return row;
  }

  ExportPage::element_t ExportPage::getLastElement () {
    if (usedElements == 0) {
      return 0;
    }
     
    size_t pos = usedElements-1;
    return buffer + pos * rowSize;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // sort and find
  ////////////////////////////////////////////////////////////////////////////////

  void ExportPage::sort () {
    if (sorted) {
      return;
    }

    // remove old elements from index
    CubePage::buffer_t ptr = buffer;
    CubePage::buffer_t end = buffer + usedElements * rowSize;

    for (;  ptr < end;  ptr += rowSize) {
      index->removeKey(((CubePage::element_t) ptr) + valueSize);
    }

    // use a shell sort to sort the values
    size_t N = usedElements;
    size_t H = 1;

    while (9 * H + 4 < N)  { H = 3 * H + 1; };

    /* sort according to the path */
    size_t h = H;

    while (0 < h) {
      for (size_t i = h + 1;  i <= N;  i++) {
        saveElement(i - 1);
        size_t j = i;

        while (h < j && isLessElement(j - h - 1)) {
          copyElement(j - 1, j - h - 1);
          j = j - h;
        }

        restoreElement(j - 1);
      }

      h = h / 3;
    }
    
    sorted = true;

    // add new elements to index
    ptr = buffer;

    for (;  ptr < end;  ptr += rowSize) {
      index->addElement((CubePage::element_t) ptr);
    }


    // update change number
    CubePage::buffer_t ptr1 = buffer + valueSize;  // first row
    CubePage::buffer_t ptr2 = ptr1 + rowSize;      // second row
    CubePage::buffer_t num  = ptr2 + keySize;      // change number    
    
    size_t numDims = keySize / sizeof(uint32_t);
    size_t numDimsM = numDims-1;

    uint32_t* firstNum = (uint32_t*) (ptr1+keySize);
    *firstNum = (IdentifierType) numDimsM;

    for (;  ptr2 < end;  ptr1 += rowSize, ptr2 += rowSize, num += rowSize) {
      uint32_t* last = (uint32_t*) ptr1;
      uint32_t* next = (uint32_t*) ptr2;
      
      for (size_t i = numDimsM; i > 1; i--) {
        if (* (last+i) != * (next+i) ) {
          uint32_t* n = (uint32_t*) num;
          *n = (uint32_t) i;
          break;
        }
      }
    }
  }


  void ExportPage::saveElement (size_t pos) {
    memcpy(tmpBuffer, buffer + pos * rowSize, rowSize);
  }



  void ExportPage::restoreElement (size_t pos) {
    memcpy(buffer + pos * rowSize, tmpBuffer, rowSize);
  }



  void ExportPage::copyElement (size_t dst, size_t src) {
    memcpy(buffer + dst * rowSize, buffer + src * rowSize, rowSize);
  }



  bool ExportPage::isLessElement (size_t right) {
    /*IdentifierType * leftElement =
      (IdentifierType*) (tmpBuffer + valueSize);

    IdentifierType * rightElement =
      (IdentifierType*) (buffer + right * rowSize + valueSize);

    IdentifierType * rightEnd =
      (IdentifierType*) (tmpBuffer + valueSize + keySize);

    for (;  leftElement < rightEnd;  leftElement++, rightElement++) {
      if (*leftElement < *rightElement) {
        return true;
      }
      else if (*leftElement > *rightElement) {
        return false;
      }
    }

    return false;*/
	  return isLess(tmpBuffer, buffer + right * rowSize);
  }

  bool ExportPage::isLess(element_t left, element_t right) const {
    IdentifierType * leftElement =
      (IdentifierType*) (left + valueSize);

    IdentifierType * rightElement =
      (IdentifierType*) (right + valueSize);

    IdentifierType * rightEnd =
      (IdentifierType*) (right + valueSize + keySize);

    for (;  leftElement < rightEnd;  leftElement++, rightElement++) {
      if (*leftElement < *rightElement) {
        return true;
      }
      else if (*leftElement > *rightElement) {
        return false;
      }
    }

    return false;
  }
}
