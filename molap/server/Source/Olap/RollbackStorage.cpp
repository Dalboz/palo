////////////////////////////////////////////////////////////////////////////////
/// @brief palo rollback storage
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

#include <limits>
#include <math.h>

#if defined(_MSC_VER)
#include <float.h>
#endif

#include "Olap/RollbackStorage.h"
#include "Olap/Rule.h"
#include "Olap/Cube.h"

#include "Exceptions/KeyBufferException.h"

#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"
#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"


#include <iostream>

namespace palo {

  size_t RollbackStorage::maximumFileRollbackSize = 100 * 1024 * 1024;  
  size_t RollbackStorage::maximumMemoryRollbackSize = 10 * 1024 * 1024;

  RollbackStorage::RollbackStorage (const vector<Dimension*>* dimensions, FileName* cubeFileName, IdentifierType id)
    : numberElements(0) {
    
    fileName = new FileName(cubeFileName->path, cubeFileName->name + "_lock_" + StringUtils::convertToString(id), cubeFileName->extension);
        
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

    // create first (in memory) rollback page
    currentPage = new RollbackPage(keySize, valueSize, maximumMemoryRollbackSize);
    numPages = 1;     
        
    sizeSavedPages = 0;
  }

  RollbackStorage::~RollbackStorage () {
    delete[] tmpKeyBuffer;
    delete[] tmpElementBuffer;

    for (size_t num = 0; num < numPages; num++) {
      FileName pageFileName = computePageFileName(num);
      FileUtils::remove(pageFileName);
    }
    
    // remove all values of current page
    RollbackPage::buffer_t ptr = currentPage->begin();
    RollbackPage::buffer_t end = currentPage->end();
    for (;  ptr < end;  ptr = currentPage->next(ptr)) {
      Cube::CellValueType* value = * (Cube::CellValueType* *) ptr;
      delete value;
    }

    // remove page
    delete currentPage;

    delete fileName;    
  }

  
  void RollbackStorage::addRollbackStep () {
    if (currentPage->isFull()) {
      step2pos.push_back(make_pair(numPages, 0));
    }    
    else {
      step2pos.push_back(make_pair(numPages-1, currentPage->size()));
    }
  }
  
  
  void RollbackStorage::checkCurrentPage () {
    if (currentPage->isFull()) {
      // save curentPage to disk and delete it from memory
      savePageToFile(currentPage, numPages-1);
      delete currentPage;
      
      // create new page in memory
      currentPage = new RollbackPage(keySize, valueSize, maximumMemoryRollbackSize);     
      numPages++;      
    }
  }
 

  void RollbackStorage::addCellValue (const IdentifiersType* path, double * value) {

    checkCurrentPage();
    
    RollbackPage::key_t keyBuffer = tmpElementBuffer + valueSize;      
    fillKeyBuffer(keyBuffer, path);

    // create new value
    Cube::CellValueType* newValue = new Cube::CellValueType();
    newValue->type = Element::NUMERIC;
    newValue->rule = Rule::NO_RULE;
    if (value) {
      newValue->doubleValue = *value;
    }      
    else {
      newValue->doubleValue = numeric_limits<double>::quiet_NaN();
    }
      
    memcpy(tmpElementBuffer, &newValue, valueSize);

    currentPage->addElement(tmpElementBuffer);        
    numberElements++;

    // print();
  }

  void RollbackStorage::addCellValue (const IdentifiersType* path, char * * value) {    

    checkCurrentPage();
    
    RollbackPage::key_t keyBuffer = tmpElementBuffer + valueSize;      
    fillKeyBuffer(keyBuffer, path);

    // create new value
    Cube::CellValueType* newValue = new Cube::CellValueType();
    newValue->type = Element::STRING;
    newValue->rule = Rule::NO_RULE;

    if (value == 0) {
      newValue->charValue.clear();
    }
    else {
      newValue->charValue.assign(*value);
    }    
      
    memcpy(tmpElementBuffer, &newValue, valueSize);

    currentPage->addElement(tmpElementBuffer);        
    numberElements++;

    // print();
  }


  void RollbackStorage::addCellValue (RollbackPage::buffer_t element) {    

    checkCurrentPage();
    
    RollbackPage::key_t keyBufferSrc = element + sizeof(double);      
    RollbackPage::key_t keyBufferDst = tmpElementBuffer + valueSize;      
    memcpy(keyBufferDst, keyBufferSrc, keySize);

    // create new value
    Cube::CellValueType* newValue = new Cube::CellValueType();
    newValue->type = Element::NUMERIC;
    newValue->rule = Rule::NO_RULE;
    newValue->doubleValue = * (double*) element;

    memcpy(tmpElementBuffer, &newValue, valueSize);

    currentPage->addElement(tmpElementBuffer);        
    numberElements++;

    // print();
  }




  Cube::CellValueType* RollbackStorage::getCellValue (RollbackPage* page, IdentifiersType* path, size_t pos) {
    RollbackPage::buffer_t ptr = page->getCell(pos);
    if (ptr) {
      fillPath(ptr, path);      
      return * (Cube::CellValueType* *) ptr;
    }
    else {
      Logger::error << "RollbackStorage::getCell internal error: value not found" << endl;
      return 0;
    }
  }
  
  
  

  void RollbackStorage::rollback (Cube* cube, size_t numSteps, User* user) {
    size_t max = getNumberSteps(); 

    if (max == 0) {
      return;
    }        
        
    pair<size_t,size_t> endPair;

    if (max < numSteps) {
      endPair = step2pos.at(0);
      step2pos.clear();
    }
    else {
      endPair = step2pos.at(max - numSteps);
      step2pos.erase(step2pos.end() - numSteps, step2pos.end());
    }
    
    size_t lowestPageNum = endPair.first;
    size_t lowestIndex   = endPair.second;
    
    size_t highestPageNum = numPages;
    size_t highestIndex = currentPage->size();

    if (lowestPageNum > highestPageNum) {
      return;
    }
    
    bool printDebug = false;

    if (printDebug) {
      if (highestPageNum == 0 || highestIndex == 0) {
        cout << "rollback from last element down to <" 
             << lowestPageNum << "," << lowestIndex << ">" << endl;
      }
      else {
        cout << "rollback from element <" << (highestPageNum-1) << "," << (highestIndex-1) << ">  down to <" 
             << lowestPageNum << "," << lowestIndex << ">" << endl;
      }
    }        
    
    IdentifiersType path(maxima.size());

    for (size_t pageNum = highestPageNum; pageNum>lowestPageNum; pageNum--) {
      size_t min = 0;

      if (pageNum-1 == lowestPageNum) {
        min = lowestIndex;
      } 
      
      for (size_t i = currentPage->size(); i > min; i--) {
        Cube::CellValueType* value = getCellValue(currentPage, &path, i-1);
        
        if (printDebug) {
          cout << "rollback Element: <" << (pageNum-1) << "," << (i-1) << "> path: <";        

          for (IdentifiersType::iterator it =  path.begin(); it != path.end(); it++) {
            cout << " " << *it;
          }

          cout << "> = ";
        }
        
        if (value) {
          CellPath cellPath(cube, &path);

          switch (value->type) {
            case (Element::NUMERIC) : 
              if (printDebug) cout << value->doubleValue;
              if (isnan(value->doubleValue)) {
                cube->clearCellValue(&cellPath, user, 0, false, false, 0);
              }
              else {
                cube->setCellValue(&cellPath, value->doubleValue, user, 0, false, false, false, Cube::DEFAULT, 0);
              }
              break;
              
            case (Element::STRING) : 
              if (printDebug) cout << value->charValue;
              cube->setCellValue(&cellPath, value->charValue, user, 0, false, false, 0);          
              break;
              
            default:
              if (printDebug) cout << "unknown element type " << endl;
              break;
          }
          
          delete value;
        }
        else {
          if (printDebug) {
            cout << "is null! " << endl;
          }
        }
        
        currentPage->removeLastElement();
        numberElements--;
      }
      
      if (pageNum-1 > lowestPageNum) {
        delete currentPage;
        numPages--;
        currentPage = loadPageFromFile(numPages-1);
      }
    }
  }
  
  void RollbackStorage::print () {
    cout << "show RollbackStorage: " << endl;
    cout << "number elements: " << numberElements << endl;
    cout << "number steps: " << getNumberSteps() << endl;

    size_t pos2=0;
    for (vector<pair <size_t,size_t> >::iterator i = step2pos.begin(); i != step2pos.end(); i++, pos2++) {
      cout << "step " << pos2 << " => <" << i->first << "," << i->second << ">" << endl;
    }

    //for (size_t pageNum = 0; pageNum < pages.size(); pageNum++) {
    //RollbackPage* page = pages.at(pageNum);
      RollbackPage* page = currentPage;
      
      RollbackPage::buffer_t ptr = page->begin();
      RollbackPage::buffer_t end = page->end();

      size_t pos = 0;

      cout << "current page has '" << numberElements << "' elements " << endl;
    
    
      for (;  ptr < end;  ptr = page->next(ptr)) {
              
        IdentifiersType path(maxima.size());
        fillPath(ptr, &path);
        
        cout << pos++;
  
        cout << "  Element <";
        for (IdentifiersType::iterator i =  path.begin(); i != path.end(); i++) {
          cout << " " << *i;
        }
        cout << "> = ";
        
        Cube::CellValueType* * value = (Cube::CellValueType* *) ptr;
        if (value) {
          if (*value) {
            switch ((*value)->type) {
              case (Element::NUMERIC) :
                  if (isnan((*value)->doubleValue)) {
                    cout << "not set" << endl;
                  }
                  else {
                    cout << (*value)->doubleValue << endl;
                  }
                  break;
              case (Element::STRING) :
                  cout << (*value)->charValue << endl;
                  break;
              default :
                  cout << "unknown" << endl;
            }
          }
          else {
            cout << "unset" << endl;
          }
        }
        else {
          cout << "not found" << endl;
        }      
      }
    //}
  }


  FileName RollbackStorage::computePageFileName (size_t identifier) {
    return FileName(fileName->path, fileName->name + "_" + StringUtils::convertToString((uint32_t) identifier), fileName->extension);
  }



  void RollbackStorage::savePageToFile (RollbackPage* page, size_t identifier) {
    
    FileName pageFileName = computePageFileName(identifier);
    
    // open a new temp-cube file
    FileWriter fw(FileName(pageFileName, "csv"), false);    
    
    fw.openFile();
    
    // database->getServer()->makeProgress();
    fw.appendComment("PALO ROLLBACK PAGE DATA");
    fw.appendComment("");
    
    fw.appendComment("Description of data: ");
    fw.appendComment("PATH;TYPE;VALUE ");
    fw.appendSection("VALUES");

    RollbackPage::buffer_t ptr = page->begin();
    RollbackPage::buffer_t e = page->end();

    for (;  ptr < e;  ptr = page->next(ptr)) {
              
      IdentifiersType path(maxima.size());
      fillPath(ptr, &path);
        
      Cube::CellValueType* * value = (Cube::CellValueType* *) ptr;
      if (value && *value) {
        fw.appendIdentifiers(&path);
        fw.appendInteger((*value)->type);
        switch ((*value)->type) {
          case (Element::NUMERIC) :
              if (isnan((*value)->doubleValue)) {
                fw.appendString("");
              }
              else {
                fw.appendDouble((*value)->doubleValue);
              }
              break;
          case (Element::STRING) :
              fw.appendEscapeString((*value)->charValue);
              break;
          default :
              fw.appendString("");
        }
        fw.nextLine();
      }
    }

    // that's it
    fw.appendComment("");
    fw.appendComment("PALO CUBE DATA END");

    fw.closeFile();
    
    sizeSavedPages += FileWriter::getFileSize(pageFileName);
    
    Logger::debug << "rollback page '" << identifier << "' saved (sizeSavedPages = " << sizeSavedPages << ")" << endl;
    
  }
    
  RollbackPage* RollbackStorage::loadPageFromFile (size_t identifier) {
    RollbackPage* page = new RollbackPage(keySize, valueSize, maximumMemoryRollbackSize);
        
    FileName pageFileName = computePageFileName(identifier);
    
    sizeSavedPages -= FileWriter::getFileSize(pageFileName);
    
    FileReader fr(pageFileName);
    fr.openFile();
        
    // database->getServer()->makeProgress();

    RollbackPage::key_t keyBuffer = tmpElementBuffer + valueSize;      

    if (fr.isSectionLine() && fr.getSection() == "VALUES") {
      fr.nextLine();

      while (fr.isDataLine()) {
        IdentifiersType path = fr.getDataIdentifiers(0);
        int type             = fr.getDataInteger(1);
        string value         = fr.getDataString(2);

        fillKeyBuffer(keyBuffer, &path);

        // create new value
        Cube::CellValueType* newValue = new Cube::CellValueType();
        newValue->rule = Rule::NO_RULE;
      
        if (type == 1) {
          // integer
          newValue->type = Element::NUMERIC;
          char *p;    
          double d = strtod(value.c_str(), &p);    
      
          if (*p != '\0') {
            // set value to nan (not set)
            newValue->doubleValue = numeric_limits<double>::quiet_NaN();
          }    
          else {
            // use value of d 
            newValue->doubleValue = d;
          }      
        }
        else {
          // string          
          newValue->type = Element::STRING;
          newValue->charValue.assign(value);
        }        

        memcpy(tmpElementBuffer, &newValue, valueSize);
        if (!page->isFull()) {
          page->addElement(tmpElementBuffer);
        }
        else {
          // this should not happen
          Logger::info << "RollbackStorage::loadPageFromFile: too many elements in file. Page is full!" << endl;
        }
        
        // get next line of saved data
        fr.nextLine();        
      }
    }
    
    
    FileUtils::remove(pageFileName);
    
    Logger::debug << "rollback page '" << identifier << "' loaded (sizeSavedPages = " << sizeSavedPages << ")" << endl;

    return page;
  }
  
  
  
  bool RollbackStorage::hasCapacity (double num) {

    size_t rowSize = currentPage->getRowSize();

    double max = (maximumFileRollbackSize + maximumMemoryRollbackSize) * 1.0;
        
    double used = (sizeSavedPages + rowSize * currentPage->size()) * 1.0;
    
    double needed = num * rowSize;
    
    //Logger::debug << "Rollback memory usage '" << used << "' max = '" << max << "' needed '" << needed << "'" <<  endl;    
    
    return max > used + needed; 
  }
  
}
