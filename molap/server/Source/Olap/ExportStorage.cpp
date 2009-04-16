////////////////////////////////////////////////////////////////////////////////
/// @brief palo export storage
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

#include "Olap/ExportStorage.h"

#include "Exceptions/KeyBufferException.h"

#include "InputOutput/Condition.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Olap/CellPath.h"
#include "Olap/Cube.h"
#include "Olap/CubeIndex.h"
#include "Olap/Dimension.h"

#include <iostream>

namespace palo {


  ExportStorage::ExportStorage (const vector<Dimension*>* dimensions,
                vector<IdentifiersType>* area,
                size_t blocksize)
    : numberElements(0), blocksize(blocksize) {

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

    //Logger::trace << "creating new ExportStorage: key size = " << keySize
    //              << ", value size = " <<  valueSize << endl;

    tmpKeyBuffer = new uint8_t [keySize];
    tmpElementBuffer = new uint8_t [elementSize];

    // generate index for base elements
    index = new CubeIndex(keySize, valueSize);

    maximumNumberElements = blocksize;
    try {
      page = new ExportPage(index, keySize, valueSize, maximumNumberElements);
    }
    catch (ErrorException e) {
      delete index;
      delete tmpKeyBuffer;
      delete tmpElementBuffer;
      if (blocksize > 10000 && e.getErrorType() == ErrorException::ERROR_OUT_OF_MEMORY) {
        throw ErrorException(ErrorException::ERROR_OUT_OF_MEMORY, "reduce the blocksize");
      }
      throw e;
    }
    
    firstPath.resize(dimensions->size());
    lastPath.resize(dimensions->size());
    setArea(area);

    lastCheckedElement = 0;

    for (vector<IdentifiersType>::iterator i = area->begin(); i != area->end(); i++) {
      set<IdentifierType> s;
      for (IdentifiersType::iterator j = i->begin(); j != i->end(); j++) {
        s.insert(*j);
      }
      areaSets.push_back(s);
    }

  }



  ExportStorage::~ExportStorage () {
    delete[] tmpKeyBuffer;
    delete[] tmpElementBuffer;


    size_t size;
    ExportPage::element_t const * array = index->getArray(size);

    for (size_t i = 0;  i < size;  array++) {
      Cube::CellValueType ** ptr = ( Cube::CellValueType**) (*array);
      if (ptr != 0) {
        delete *ptr;
        i++;
      }
    }

    delete page;
    delete index;
  }




  void ExportStorage::increasePage () {
    // delete old index and page

    size_t size;
    ExportPage::element_t const * array = index->getArray(size);

    for (size_t i = 0;  i < size;  array++) {
      Cube::CellValueType ** ptr = ( Cube::CellValueType**) (*array);
      if (ptr != 0) {
        delete *ptr;
        i++;
      }
    }

    delete page;
    delete index;

    lastCheckedElement = 0;
    numberElements = 0;


    // generate new index and a bigger page

    index = new CubeIndex(keySize, valueSize);

    if (blocksize < 50) {
      maximumNumberElements = 4 * blocksize;
    }
    else if (blocksize < 300) {
      maximumNumberElements = 3 * blocksize;
    }
    else {
      maximumNumberElements = 2 * blocksize;
    }

    page = new ExportPage(index, keySize, valueSize, maximumNumberElements);
  }



  void ExportStorage::setArea (vector<IdentifiersType>* area) {
    this->area = area;

    for (size_t i = 0; i < firstPath.size(); i++) {
      if (! area->at(i).empty()) {
        firstPath[i] = *(area->at(i).begin());
        lastPath[i]  = *( --(area->at(i).end()) );
      }
    }
  }



  void ExportStorage::print () {
    ExportPage::buffer_t ptr = page->begin();
    ExportPage::buffer_t end = page->end();

    cout << "blocksize = " << blocksize << endl;
    cout << "numberElements = " << numberElements << endl;
    cout << "maximumNumberElements = " << maximumNumberElements << endl;

    cout << "first path <";
    for (IdentifiersType::iterator i =  firstPath.begin(); i != firstPath.end(); i++) {
      cout << " " << *i;
    }
    cout << " >" << endl;


    cout << "last path <";
    for (IdentifiersType::iterator i =  lastPath.begin(); i != lastPath.end(); i++) {
      cout << " " << *i;
    }
    cout << " >" << endl;


    for (;  ptr < end;  ptr = page->next(ptr)) {

      IdentifiersType path(maxima.size());
      fillPath(ptr, &path);

      cout << "Element <";
      for (IdentifiersType::iterator i =  path.begin(); i != path.end(); i++) {
        cout << " " << *i;
      }
      cout << " > = ";

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

  void ExportStorage::addDoubleValue (uint8_t * keyBuffer, double value) {
    //  check isInStorageArea in CubeStorage::setExportValue
    //  if (! isInStorageArea(keyBuffer)) {
    //    // path is not between first and last path of storage
    //    return;
    //  }

    Cube::CellValueType* *v = (Cube::CellValueType* *) index->lookupKey(keyBuffer);

    if (v) {
      (*v)->type = Element::NUMERIC;
      (*v)->doubleValue += value;
    }
    else {
      Cube::CellValueType* newValue = new Cube::CellValueType();
      newValue->type = Element::NUMERIC;
      newValue->doubleValue = value;

      memcpy(tmpElementBuffer, (ExportPage::value_t) &newValue, valueSize);

      CubePage::key_t tmpKeyBuffer = tmpElementBuffer + valueSize;
      memcpy(tmpKeyBuffer, keyBuffer, keySize);

      addNewCell(tmpElementBuffer);
    }
  }


  void ExportStorage::addStringValue (uint8_t * keyBuffer, string& value) {
    //  check isInStorageArea in CubeStorage::computeExportStringCellValue
    //  if (! isInStorageArea(keyBuffer)) {
    //    // path is not between first and last path of storage
    //    return;
    //  }


    Cube::CellValueType* *v = (Cube::CellValueType* *) index->lookupKey(keyBuffer);

    if (v) {
      (*v)->type = Element::STRING;
      (*v)->charValue = value;
    }
    else {
      Cube::CellValueType* newValue = new Cube::CellValueType();
      newValue->type = Element::STRING;
      newValue->charValue = value;

      memcpy(tmpElementBuffer, (ExportPage::value_t) &newValue, valueSize);

      CubePage::key_t tmpKeyBuffer = tmpElementBuffer + valueSize;
      memcpy(tmpKeyBuffer, keyBuffer, keySize);

      addNewCell(tmpElementBuffer);
    }
  }


  Cube::CellValueType* ExportStorage::getCell (size_t pos) {
    ExportPage::buffer_t ptr = page->getCell(pos);
    if (ptr) {
      return * (Cube::CellValueType* *) ptr;
    }
    else {
      Logger::error << "ExportStorage::getCell internal error: value not found" << endl;
      return 0;
    }
  }



  Cube::CellValueType* ExportStorage::getCell (size_t pos, IdentifiersType* path) {
    ExportPage::buffer_t ptr = page->getCell(pos);
    if (ptr) {
      fillPath(ptr, path);
      return * (Cube::CellValueType* *) ptr;
    }
    else {
      Logger::error << "ExportStorage::getCell internal error: value not found" << endl;
      return 0;
    }
  }



  void ExportStorage::removeCell (size_t pos) {
    ExportPage::buffer_t element = page->getCell(pos);
    if (element) {
      // delete old value
      delete * ( Cube::CellValueType**) element;

      page->removeElement(element);
      numberElements--;
    }
    else {
      Logger::error << "ExportStorage::removeCell internal error: value not found" << endl;
    }
  }



  bool ExportStorage::setStartPath (IdentifiersType* path) {
    size_t max = area->size();

    vector< IdentifiersType::iterator > iv(area->size());

    // wrong number of elements
    if (path->size() != iv.size()) {
      return false;
    }

    // is the given cell path an element of the area?
    for (size_t i = 0; i < max; i++) {
      iv[i] = find(area->at(i).begin(), area->at(i).end(), path->at(i));

      // path not found in area
      if (iv[i] == area->at(i).end()) {
        return false;
      }
    }

    IdentifiersType newPath(*path);

    size_t pos = max-1;

    while (pos >= 0 && pos < path->size()) {
      iv[pos]++;

      if (iv[pos] != area->at(pos).end()) {
        newPath[pos] = * (iv[pos]);
        firstPath = newPath;

        return true;
      }

      // error in new start path!
      if (pos == 0) {
        return false;
      }

      newPath[pos] = *(area->at(pos).begin());

      // next dimension
      pos--;
    }

    return false;
  }


  int ExportStorage::posArea (ExportPage::key_t  keyBuffer) {
    IdentifierType * buffer = (IdentifierType*) keyBuffer;
    IdentifiersType::const_iterator firstPathIter = firstPath.begin();

    for (;  firstPathIter != firstPath.end();  firstPathIter++) {
      if (*buffer < *firstPathIter) {
        return -1;
      }
      else if (*buffer++ > *firstPathIter) {
        break;
      }
    }

    buffer = (IdentifierType*) keyBuffer;
    IdentifiersType::const_iterator lastPathIter = lastPath.begin();
    for (;  lastPathIter != lastPath.end();  lastPathIter++) {
      if (*buffer < *lastPathIter) {
        break;
      }
      else if (*buffer++ > *lastPathIter) {
        return 1;
      }
    }

    return 0;
  }



  bool ExportStorage::isInStorageArea (ExportPage::key_t  keyBuffer) {
    IdentifierType * buffer = (IdentifierType*) keyBuffer;
    size_t max = firstPath.size();

    for (size_t i = 0; i < max; i++) {
      set<IdentifierType>::iterator find = (areaSets[i]).find(*buffer++);

      if (find == (areaSets[i]).end()) {
        return false;
      }
    }

//    return true;


//    IdentifierType * buffer = (IdentifierType*) keyBuffer;
    buffer = (IdentifierType*) keyBuffer;
    IdentifiersType::const_iterator firstPathIter = firstPath.begin();

    for (;  firstPathIter != firstPath.end();  firstPathIter++) {
      if (*buffer < *firstPathIter) {
        return false;
      }
      else if (*buffer++ > *firstPathIter) {
        break;
      }
    }

    buffer = (IdentifierType*) keyBuffer;
    IdentifiersType::const_iterator lastPathIter = lastPath.begin();
    for (;  lastPathIter != lastPath.end();  lastPathIter++) {
      if (*buffer < *lastPathIter) {
        break;
      }
      else if (*buffer++ > *lastPathIter) {
        return false;
      }
    }

    return true;

  }



  bool ExportStorage::isInStorageArea (ExportPage::key_t  keyBuffer, size_t& errorPos) {
    IdentifierType * buffer = (IdentifierType*) keyBuffer;
    size_t max = firstPath.size();

    for (size_t i = 0; i < max; i++) {
      set<IdentifierType>::iterator find = (areaSets[i]).find(*buffer++);

      if (find == (areaSets[i]).end()) {
        errorPos = i;
        return false;
      }
    }

//    return true;

//    IdentifierType * buffer = (IdentifierType*) keyBuffer;
//    size_t max = firstPath.size();
    buffer = (IdentifierType*) keyBuffer;

    for (size_t i = 0; i < max; i++) {
      if (*buffer < firstPath[i]) {
        errorPos = i;
        return false;
      }
      else if (*buffer++ > firstPath[i]) {
        break;
      }
    }

    buffer = (IdentifierType*) keyBuffer;
    for (size_t i = 0; i < max; i++) {
      if (*buffer < lastPath[i]) {
        break;
      }
      else if (*buffer++ > lastPath[i]) {
        errorPos = i;
        return false;
      }
    }

    return true;

  }



  bool ExportStorage::isInStorageArea (IdentifierType id1) {
    set<IdentifierType>::iterator find = (areaSets[0]).find(id1);

    if (find == (areaSets[0]).end()) {
      return false;
    }

    return true;
  }



  bool ExportStorage::isInStorageArea (IdentifierType id1, IdentifierType id2) {

    set<IdentifierType>::iterator find = (areaSets[0]).find(id1);
    if (find == (areaSets[0]).end()) {
      return false;
    }

    find = (areaSets[1]).find(id2);
    if (find == (areaSets[1]).end()) {
      return false;
    }

    return true;
  }



  void ExportStorage::updateLastPath (ExportPage::key_t  keyBuffer) {
    IdentifierType * buffer = (IdentifierType*) keyBuffer;

    // cout << "new lastPath = < " ;
    for (size_t i = 0; i < lastPath.size(); i++) {
      lastPath[i] = *(buffer++);
      // cout << lastPath[i] << " ";
    }

    // cout << ">" << endl;
  }



  void ExportStorage::addNewCell (uint8_t * elementBuffer) {
    
	  bool add = true;
	if (numberElements == maximumNumberElements) {
		page->sort();
		ExportPage::element_t lastElement = page->getLastElement();
		if (page->isLess(elementBuffer, lastElement)) {
		  // remove last element in page as this element is less than last		  
		  page->removeLastElement();
		  numberElements--;		  
		} else 
			add = false;
	} 
	if (add) {
		page->addElement(elementBuffer);
		numberElements++;
	}

    if (numberElements == maximumNumberElements) {
      page->sort();
      // get last element and set lastPath
      ExportPage::element_t elementBuffer = page->getLastElement();
      updateLastPath((ExportPage::key_t) elementBuffer + valueSize);
      // print();
    }
  }



  void ExportStorage::processCondition (Condition * condition) {

    for (size_t i = numberElements; i > lastCheckedElement; i--) {

      // get value
      ExportPage::buffer_t element = page->getCell(i-1);
      Cube::CellValueType* value = * (Cube::CellValueType* *) element;

      if (value) {

        // check value
        if ( (value->type == Element::STRING  && ! condition->check(value->charValue)) ||
             (value->type == Element::NUMERIC && ! condition->check(value->doubleValue))) {
          delete value;
          page->removeElement(element);
          numberElements--;
        }
      }
    }

    page->sort();
  }



  bool ExportStorage::resetForNextLoop () {
    // set first path
    bool hasArea = setStartPath(&lastPath);

    // reset last path
    for (size_t i = 0; i < lastPath.size(); i++) {
      lastPath[i]  = *( --(area->at(i).end()) );
    }

    if (numberElements > 0) {
      lastCheckedElement = numberElements-1;
    }
    else {
      lastCheckedElement = 0;
    }

    return hasArea;
  }


  void ExportStorage::fillWithEmptyElements () {
    IdentifiersType path(firstPath);

    size_t max = area->size();
    vector< IdentifiersType::iterator > iv(max);

    for (size_t i = 0; i < max; i++) {
      iv[i] = area->at(i).begin();
      while ( iv[i] != area->at(i).end() &&  *(iv[i]) != path[i]) {
        iv[i]++;
      }
    }

    size_t pos = max-1;

    do {
      Cube::CellValueType* newValue = new Cube::CellValueType();
      newValue->type = Element::UNDEFINED;
      newValue->doubleValue = 0.0;

      setCellValue(&path, (ExportPage::value_t) &newValue);

      pos = max-1;
      bool found = false;
      while (!found && pos >= 0 && pos < max) {
        iv[pos]++;
        if (iv[pos] != area->at(pos).end()) {
          path[pos] = * (iv[pos]);
          found = true;
        }
        else {
          iv[pos] = area->at(pos).begin();
          path[pos] = * (iv[pos]);
          pos--;
        }
      }

    } while (pos >= 0 && pos < max && numberElements < maximumNumberElements);

  }



  double ExportStorage::getAreaSize () {
    double result = 1.0;
    size_t max = area->size();

    for (size_t i = 0; i < max; i++) {
      result *= area->at(i).size();
    }

    return result;
  }



  double ExportStorage::getPosNumber () {
    if (numberElements == 0 || numberElements < blocksize) {
      return getAreaSize();
    }

    size_t pos = size()-1;
    IdentifiersType path(area->size());
    Cube::CellValueType* value = getCell(pos, &path);

    double result = 1.0;
    vector< double > m(path.size());

    size_t max = area->size();

    for (size_t i = max; i > 0; i--) {
      m[i-1] = result;
      result *= area->at(i-1).size();
    }

    result = 1.0;

    for (size_t i = 0; i < max; i++) {

      for (size_t j = 0; j < area->at(i).size(); j++) {
        if (path[i] == area->at(i)[j]) {
          result += j * m[i];
        }
      }
    }

    return result;
  }

}
