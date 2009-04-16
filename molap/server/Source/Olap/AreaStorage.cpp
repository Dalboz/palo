////////////////////////////////////////////////////////////////////////////////
/// @brief palo area storage
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

#include "Olap/AreaStorage.h"

#include "Exceptions/KeyBufferException.h"

#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Olap/CellPath.h"
#include "Olap/Cube.h"
#include "Olap/CubeIndex.h"
#include "Olap/Dimension.h"
#include "Olap/Rule.h"

#include <iostream>

namespace palo {


  AreaStorage::AreaStorage (const vector<Dimension*>* dimensions,
                            vector<IdentifiersType>* area,
                            size_t numElements, 
                            bool isPathList)
    : numberElements(0) {
        
    valueSize = sizeof(Cube::CellValueType*);
    maxima.clear();
    
    size_t totalNumberBits = 0;
      
    for (vector<Dimension*>::const_iterator i = dimensions->begin();
         i != dimensions->end();
         i++) {
      uint32_t bitsDimension = 32;
      uint32_t maximum = ~0;

      maxima.push_back(maximum);
      totalNumberBits += bitsDimension;
    }
    
    keySize = ((totalNumberBits + 31) / 32) * 4; // normalise to 32 bit, convert to byte
    elementSize = keySize + valueSize;

    //Logger::trace << "creating new AreaStorage: key size = " << keySize 
    //              << ", value size = " <<  valueSize << endl;
         
    tmpKeyBuffer = new uint8_t [keySize];
    tmpElementBuffer = new uint8_t [elementSize];

    // generate index for base elements
    index = new CubeIndex(keySize, valueSize);
    
    maximumNumberElements = (numElements > MAXIMUM_AREA_SIZE) ? MAXIMUM_AREA_SIZE : numElements;
    
    page = new AreaPage(index, keySize, valueSize, maximumNumberElements);
    
    if (maximumNumberElements > 0) {
      if (isPathList) {
        addPathCells(area);
      }
      else {
        addAreaCells(area);
      }
    }
  }



  AreaStorage::~AreaStorage () {
    delete[] tmpKeyBuffer;
    delete[] tmpElementBuffer;


    size_t size;
    AreaPage::element_t const * array = index->getArray(size);

    for (size_t i = 0;  i < size;  array++) {
      Cube::CellValueType ** ptr = ( Cube::CellValueType**) (*array);
      if (ptr != 0) {
        delete *ptr;
        i++;
      }
    }

    for (vector<Cube::CellValueType*>::iterator i = errorValues.begin(); i != errorValues.end(); i++) {
      delete (*i);
    }
    
    delete page;
    delete index;
  }
  
  


  void AreaStorage::addCell (IdentifiersType* path) {
    if (maximumNumberElements == numberElements) {
      return;
    }
    
    AreaPage::key_t keyBuffer = tmpElementBuffer + valueSize;      
    fillKeyBuffer(keyBuffer, path);

    AreaPage::element_t element = index->lookupKey(keyBuffer);

    if (element == 0) {
      // create new value
      Cube::CellValueType* value = new Cube::CellValueType();
      value->type = Element::UNDEFINED;
      value->doubleValue = 0.0;
	  value->rule = Rule::NO_RULE;
      
      memcpy(tmpElementBuffer, &value, valueSize);
    }
    else {
      // use found value
      memcpy(tmpElementBuffer, element, valueSize);
    }

    page->addElement(tmpElementBuffer);        
    numberElements++;
  }



  void AreaStorage::addErrorCell () {
    if (maximumNumberElements == numberElements) {
      return;
    }

    Cube::CellValueType* value = new Cube::CellValueType();
    value->type = Element::UNDEFINED;
    value->doubleValue = 0.0;
    memcpy(tmpElementBuffer, &value, valueSize);
    page->addDummyElement(tmpElementBuffer);        
    numberElements++;
    errorValues.push_back(value);
  }




  void AreaStorage::addAreaCells (vector<IdentifiersType>* area) {
     vector<size_t> combinations(area->size(), 0);
     loop(area, combinations);
  }




  void AreaStorage::loop (const vector< IdentifiersType >* paths, vector<size_t>& combinations) {

    bool eval  = true;
    int length = (int) paths->size();

    for (int i = 0;  i < length;) {
      
      if (eval) {
        // construct path
        IdentifiersType path;

        for (size_t i = 0;  i < paths->size();  i++) {
          IdentifierType id = paths->at(i)[combinations[i]];
          path.push_back(id);
        }
        addCell(&path);
      }

      size_t position = combinations[i];
      const IdentifiersType& dim = paths->at(i);

      if (position + 1 < dim.size()) {
        combinations[i] = (int) position + 1;
        i = 0;
        eval = true;
      }
      else {
        i++;

        for (int k = 0;  k < i;  k++) {
          combinations[k] = 0;
        }

        eval = false;
      }
    }
  }
  
  
  
  void AreaStorage::addPathCells (vector<IdentifiersType>* paths) {
    size_t s = maxima.size();
    
    for (vector<IdentifiersType>::iterator i = paths->begin(); i != paths->end(); i++) {
      if (i->size() != s) {
        addErrorCell();
      }
      else {
        addCell( &(*i) );
      }
    }
  }
  

  void AreaStorage::print () {
    AreaPage::buffer_t ptr = page->begin();
    AreaPage::buffer_t end = page->end();

    for (;  ptr < end;  ptr = page->next(ptr)) {
            
      IdentifiersType path(maxima.size());
      fillPath(ptr, &path);
      
      cout << "Element <";
      for (IdentifiersType::iterator i =  path.begin(); i != path.end(); i++) {
        cout << " " << *i;
      }
      cout << "> = ";
      
      Cube::CellValueType* * value = (Cube::CellValueType* *) ptr;
      if (value) {
        switch ((*value)->type) {
          case (Element::NUMERIC) :
              cout << (*value)->doubleValue << endl;
              break;
          case (Element::STRING) :
              cout << (*value)->charValue << endl;
              break;
          default :
              cout << "unknown" << endl;
        }
      }
      else {
        cout << "not found" << endl;
      }      
    }

  }
  
  void AreaStorage::addDoubleValue(IdentifiersType* path, double value) {
    Cube::CellValueType* *v = (Cube::CellValueType* *) getCellValue(path);
    
    if (v) {
      (*v)->type = Element::NUMERIC;
      (*v)->doubleValue += value;
    }
  }
  
  
  void AreaStorage::addDoubleValue(uint8_t * keyBuffer, double value) {
    Cube::CellValueType* *v = (Cube::CellValueType* *) index->lookupKey(keyBuffer);
    
    if (v) {
      (*v)->type = Element::NUMERIC;
      (*v)->doubleValue += value;
    }
  }
  
  
  Cube::CellValueType* AreaStorage::getCell (size_t pos) {
    AreaPage::buffer_t ptr = page->getCell(pos);

    if (ptr) {
      return * (Cube::CellValueType* *) ptr;
    }
    else {
      Logger::error << "AreaStorage::getCell internal error: value not found" << endl;
      return 0;
    }
  }     
  
  Cube::CellValueType* AreaStorage::getCell (size_t pos, IdentifiersType* path) {
    AreaPage::buffer_t ptr = page->getCell(pos);

    if (ptr) {
      fillPath(ptr, path);      
      return * (Cube::CellValueType* *) ptr;
    }
    else {
      Logger::error << "AreaStorage::getCell internal error: value not found" << endl;
      return 0;
    }
  }     

}
