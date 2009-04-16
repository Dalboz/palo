////////////////////////////////////////////////////////////////////////////////
/// @brief palo area page
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

#include "Olap/AreaPage.h"

#include <iostream>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#include "Olap/CubeIndex.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  AreaPage::AreaPage (CubeIndex * index,
                      size_t keySize,
                      size_t valueSize,
                      size_t numElements)
    : index(index),
      keySize(keySize), 
      valueSize(valueSize) {

    rowSize = ((valueSize + keySize + 3) / 4) * 4; // align row size to 4 byte

    totalSize = rowSize * numElements;
    numberElements = totalSize / rowSize;
    usedElements = 0;

    buffer = new uint8_t [totalSize];
    memset(buffer, 0, totalSize);

    tmpBuffer = new uint8_t [rowSize];
    memset(tmpBuffer, 0, rowSize);
  }



  AreaPage::~AreaPage () {
    delete [] buffer;
    delete [] tmpBuffer;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // add and remove entries
  ////////////////////////////////////////////////////////////////////////////////

  AreaPage::element_t AreaPage::addElement (element_t row) {
    if (usedElements == numberElements) {
      Logger::info << "internal error in AreaPage::addElement()" << endl;
      return 0;
    }

    element_t result = buffer + usedElements * rowSize;
    memcpy(result, row, valueSize + keySize);

    index->addElement(result);

    usedElements++;

    return result;
  }  


  AreaPage::element_t AreaPage::addDummyElement (element_t row) {
    if (usedElements == numberElements) {
      Logger::info << "internal error in AreaPage::addElement()" << endl;
      return 0;
    }

    element_t result = buffer + usedElements * rowSize;
    memcpy(result, row, valueSize + keySize);

    usedElements++;

    return result;
  }  

}
