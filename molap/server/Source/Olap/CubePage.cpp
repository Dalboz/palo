////////////////////////////////////////////////////////////////////////////////
/// @brief palo cube page
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

#include "Olap/CubePage.h"

#include <iostream>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#include "Olap/CubeIndex.h"

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

static const bool DEBUG_FILE = true;


namespace palo {

  static void PrintCubePage (CubePage::buffer_t start, 
                             CubePage::buffer_t stop,
                             size_t valueSize,
                             size_t keySize,
                             size_t rowSize) {
    cout << "--------------------------------------------------------------------------------\n";

    for (; start < stop;  start += rowSize) {
      cout << *(double*) start << " ";

      CubePage::key_t key = start + valueSize;

      for (int i = 0;  i < (int) keySize;  i += 4, key += 4) {
        cout << * (uint32_t*) key << " ";
      }

      cout << " @ " << * (uint32_t*) key << "\n";
    }

    cout << "--------------------------------------------------------------------------------\n";
  }

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  CubePage::CubePage (CubeIndex * index,
                      size_t keySize,
                      size_t valueSize,
                      IdentifierType first,
                      IdentifierType second)
    : index(index),
      first(first),
      second(second),
      sorted(true),
      keySize(keySize), 
      valueSize(valueSize) {

    rowSize = ((valueSize + keySize + sizeof(uint32_t) + ELEMENT_ALIGNMENT - 1) / ELEMENT_ALIGNMENT) * ELEMENT_ALIGNMENT;

    totalSize = PAGE_SIZE;
    numberElements = totalSize / rowSize;
    usedElements = 0;

    buffer = new uint8_t [totalSize];
    memset(buffer, 0, totalSize);

    tmpBuffer = new uint8_t [rowSize];
    memset(tmpBuffer, 0, rowSize);
  }



  CubePage::~CubePage () {
    delete [] buffer;
    delete [] tmpBuffer;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // add and remove entries
  ////////////////////////////////////////////////////////////////////////////////

  CubePage::element_t CubePage::addElement (element_t row, bool isMarked) {
    if (usedElements == numberElements) {
      grow();
    }

    element_t result = buffer + usedElements * rowSize;
    memcpy(result, row, valueSize + keySize);

    if (isMarked) {
      * (uint32_t*) (result + valueSize + keySize) = 0x80000000;
    }
    else {
      * (uint32_t*) (result + valueSize + keySize) = 0;
    }

    index->addElement(result);

    usedElements++;
    sorted = false;

    return result;
  }



  CubePage::element_t CubePage::removeElement (element_t row) {
    size_t pos = (row - buffer) / rowSize;

    if (pos >= usedElements) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "in CubePage::removeElement");
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

  ////////////////////////////////////////////////////////////////////////////////
  // sort and find
  ////////////////////////////////////////////////////////////////////////////////

  void CubePage::sort () {
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

    // sort according to the path
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
    usedElements = 0;

    for (;  ptr < end;  ptr += rowSize) {
      if (isDeleted((CubePage::element_t) ptr)) {
        break;
      }

      index->addElement((CubePage::element_t) ptr);
      usedElements++;
    }

    if (DEBUG_FILE) {
      if (ptr < end) {
        Logger::trace << "cube page reduced to " << usedElements
                      << " rows from " << ((end - buffer) / rowSize)
                      << endl;
      }
    }

    // update change number
    CubePage::buffer_t ptr1 = buffer + valueSize;  // first row
    CubePage::buffer_t ptr2 = ptr1 + rowSize;      // second row
    CubePage::buffer_t num  = ptr2 + keySize;      // change number    
    
    size_t numDims = keySize / sizeof(uint32_t);
    size_t numDimsM = numDims - 1;

    uint32_t* firstNum = (uint32_t*) (ptr1 + keySize);
    *firstNum = (((uint32_t) numDimsM) & 0x3fffffff) | (*firstNum & 0xc0000000);

    for (;  ptr2 < end;  ptr1 += rowSize, ptr2 += rowSize, num += rowSize) {
      uint32_t* last = (uint32_t*) ptr1;
      uint32_t* next = (uint32_t*) ptr2;
      
      for (size_t i = numDimsM;  1 < i;  i--) {
        if (* (last + i) != * (next + i) ) {
          uint32_t* n = (uint32_t*) num;
          *n = (((uint32_t) i) & 0x3fffffff) | (*n & 0xC0000000);
          break;
        }
      }
    }
  }



  void CubePage::sortExport () {
    sorted = false;

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

    // sort according to the path
    size_t h = H;

    while (0 < h) {
      for (size_t i = h + 1;  i <= N;  i++) {
        saveElement(i - 1);
        size_t j = i;

        while (h < j && isLessElementExport(j - h - 1)) {
          copyElement(j - 1, j - h - 1);
          j = j - h;
        }

        restoreElement(j - 1);
      }

      h = h / 3;
    }
    
    // add new elements to index
    ptr = buffer;
    usedElements = 0;

    for (;  ptr < end;  ptr += rowSize) {
      if (isDeleted((CubePage::element_t) ptr)) {
        break;
      }

      index->addElement((CubePage::element_t) ptr);
      usedElements++;
    }

    if (DEBUG_FILE) {
      if (ptr < end) {
        Logger::trace << "cube page reduced to " << usedElements
                      << " rows from " << ((ptr - buffer) / rowSize)
                      << endl;
      }
    }

    // update change number
    CubePage::buffer_t ptr1 = buffer + valueSize;  // first row
    CubePage::buffer_t ptr2 = ptr1 + rowSize;      // second row
    CubePage::buffer_t num  = ptr2 + keySize;      // change number    
    
    size_t numDims = keySize / sizeof(uint32_t);
    size_t numDimsM = numDims-1;

    uint32_t* firstNum = (uint32_t*) (ptr1 + keySize);
    *firstNum = (((uint32_t) 2) & 0x3fffffff) | (*firstNum & 0xc0000000);

    for (;  ptr2 < end;  ptr1 += rowSize, ptr2 += rowSize, num += rowSize) {
      uint32_t* last = (uint32_t*) ptr1;
      uint32_t* next = (uint32_t*) ptr2;
      
      for (size_t i = 2;  i <= numDimsM;  i++) {
        if (* (last+i) != * (next+i) ) {
          uint32_t* n = (uint32_t*) num;
          *n = (((uint32_t) i) & 0x3fffffff) | (*n & 0xC0000000);
          break;
        }
      }
    }
  }



  CubePage::buffer_t CubePage::lower_bound (buffer_t first,
                                            buffer_t last,
                                            IdentifierType value,
                                            size_t offset) {
    size_t len = (last - first) / rowSize;

    while (len > 0) {
      size_t half = len >> 1;
      buffer_t middle = first + half * rowSize;
      IdentifierType current = * (IdentifierType*) (middle + offset);

      if (current < value) {
        first = middle + rowSize;
        len = len - half - 1;
      }
      else {
        len = half;
      }
    }

    return first;
  }



  CubePage::buffer_t CubePage::upper_bound (buffer_t first,
                                            buffer_t last,
                                            IdentifierType value,
                                            size_t offset) {
    size_t len = (last - first) / rowSize;

    while (len > 0) {
      size_t half = len >> 1;
      buffer_t middle = first + half * rowSize;
      IdentifierType current = * (IdentifierType*) (middle + offset);

      if (value < current) {
        len = half;
      }
      else {
        first = middle + rowSize;
        len = len - half - 1;
      }
    }

    return first;
  }



  void CubePage::equal_range (buffer_t * firstArg,
                              buffer_t * lastArg,
                              IdentifierType value,
                              size_t offset) {
    buffer_t first = *firstArg;
    buffer_t last  = *lastArg;

    size_t len = (last - first) / rowSize;

    while (len > 0) {
      size_t half = len >> 1;
      buffer_t middle = first + half * rowSize;
      IdentifierType current = * (IdentifierType*) (middle + offset);

      if (current < value) {
        first = middle + rowSize;
        len = len - half - 1;
      }
      else if (value < current) {
        len = half;
      }
      else {
        buffer_t left = lower_bound(first, middle, value, offset);
        buffer_t right = upper_bound(middle + rowSize, first + len * rowSize, value, offset);

        *firstArg = left;
        *lastArg = right;

        return;
      }
    }

    *firstArg = first;
    *lastArg  = first;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // internal methods
  ////////////////////////////////////////////////////////////////////////////////

  void CubePage::grow () {

    // create new buffer
    size_t newTotalSize = ((3 * totalSize / 2 + PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE;
    buffer_t newBuffer = new uint8_t [newTotalSize];

    memset(newBuffer, 0, newTotalSize);
    memcpy(newBuffer, buffer, totalSize);

    // reindex
    CubePage::buffer_t ptr = newBuffer;
    CubePage::buffer_t end = newBuffer + usedElements * rowSize;

    for (;  ptr < end;  ptr += rowSize) {
      index->addElement((CubePage::element_t) ptr);
    }

    // delete old buffer
    delete[] buffer;

    buffer = newBuffer;
    totalSize = newTotalSize;
    numberElements = newTotalSize / rowSize;
  }



  void CubePage::saveElement (size_t pos) {
    memcpy(tmpBuffer, buffer + pos * rowSize, rowSize);
  }



  void CubePage::restoreElement (size_t pos) {
    memcpy(buffer + pos * rowSize, tmpBuffer, rowSize);
  }



  void CubePage::copyElement (size_t dst, size_t src) {
    memcpy(buffer + dst * rowSize, buffer + src * rowSize, rowSize);
  }



  bool CubePage::isLessElement (size_t right) {
    CubePage::element_t rightBuffer = buffer + right * rowSize;

    if (isDeleted(tmpBuffer)) {
      return false;
    }
    else if (isDeleted(rightBuffer)) {
      if (! isDeleted(tmpBuffer)) {
        return true;
      }

      return false;
    }

    IdentifierType * leftElement =
      (IdentifierType*) (tmpBuffer + valueSize + keySize - 4);

    IdentifierType * rightElement =
      (IdentifierType*) (buffer + right * rowSize + valueSize + keySize - 4);

    IdentifierType * leftEnd =
      (IdentifierType*) (tmpBuffer + valueSize);

    for (;  leftEnd <= leftElement;  leftElement--, rightElement--) {
      if (*leftElement < *rightElement) {
        return true;
      }
      else if (*leftElement > *rightElement) {
        return false;
      }
    }

    return false;
  }
  


  bool CubePage::isLessElementExport (size_t right) {
    IdentifierType * leftElement =
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

    return false;
  }
  
}
