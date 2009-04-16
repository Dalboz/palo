////////////////////////////////////////////////////////////////////////////////
/// @brief converts a Palo 1.0 database
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

#include "Programs/Palo1Converter.h"

#include <iostream>
#include <fstream>

#include "Exceptions/FileOpenException.h"
#include "Exceptions/FileFormatException.h"

#include "Collections/StringUtils.h"

#include "InputOutput/FileDocumentation.h"
#include "InputOutput/FileUtils.h"
#include "InputOutput/HtmlFormatter.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/SignalTask.h"

#include "Olap/Database.h"
#include "Olap/Server.h"

namespace palo {
  const string Palo1Converter::backupSuffix = ".backup";



  static bool isBackupDirectory (const string& dataDirectory) {
    return StringUtils::isSuffix(dataDirectory, Palo1Converter::backupSuffix);
  } 



  bool Palo1Converter::isPalo1Directory (const string& dataDirectory) {
  
    // do not import backup directories  
    if (isBackupDirectory(dataDirectory)) {
      return false;
    }
  
    bool hasDimensionXML = false;
  
    vector<string> files = FileUtils::listFiles(dataDirectory);

    if (files.empty()) {
      return false;
    }

    for (vector<string>::iterator iter = files.begin();  iter != files.end();  iter++) {
      const string& filename = dataDirectory + "/" + *iter;
      bool isRegularFile = FileUtils::isRegularFile(filename);
      
      if (isRegularFile) {      
        if (filename == "database.csv") {
          return false;
        }
        else if (StringUtils::isSuffix(filename, ".dimension.xml")) {
          hasDimensionXML = true;
        }
      }
    }

    return hasDimensionXML;
  }



  vector<string> Palo1Converter::listDirectories (const string& dataDirectory) {
    vector<string> files = FileUtils::listFiles(dataDirectory);
    vector<string> result;

    for (vector<string>::iterator iter = files.begin();  iter != files.end();  iter++) {
      const string& filename = dataDirectory + "/" + *iter;
      bool isDirectory = FileUtils::isDirectory(filename);

      if (isDirectory && isPalo1Directory(filename)) { 
        result.push_back(filename);
      }
    }

    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and desctructors
  ////////////////////////////////////////////////////////////////////////////////

  Palo1Converter::Palo1Converter (Server * palo, const string& path, const string& directory) 
    : palo(palo), palo1Path((path == "." || path.empty()) ? "" : (path + "/")), palo1Directory(directory), logFile(&cout) {
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // utility functions for file reading
  ////////////////////////////////////////////////////////////////////////////////
  
  static char getNextChar (ifstream &file) {
    if (file.eof()) return 0;  
    return file.get();
  }
  
  

  static uint32_t readInteger (ifstream &file) {
    union {
      char c[4];
      uint32_t i;
    } buffer;
    
    buffer.c[3] = getNextChar(file);
    buffer.c[2] = getNextChar(file);
    buffer.c[1] = getNextChar(file);
    buffer.c[0] = getNextChar(file);

    return buffer.i;
  }



  static double readDouble (ifstream &file) {
    union {
      uint32_t i[2];
      double   d;
    } buffer;
  
    buffer.i[1] = readInteger(file);
    buffer.i[0] = readInteger(file);
  
    return buffer.d;
  }



  static string readString (ifstream &file) {
    uint32_t sizeElement = readInteger(file);

    if (sizeElement == 0) {
      return "";
    }

    char * name = new char [sizeElement+1];
    file.read(name, sizeElement);
    name[sizeElement] = 0;
    string result(name, sizeElement);
    delete [] name;

    return result;
  }



  static string replaceXmlChar (const string& str, const string& replace, const string& value) {
    string::size_type i = 0;

    string result;

    while (i != string::npos) {
      string::size_type f = str.find(replace, i);

      if (f != string::npos) {
        result += str.substr(i, f-i) + value;
        i = f + replace.length();
      }
      else {
        result += str.substr(i);
        i = f;
      }
    }
    
    return result;
  }



  static string replaceXmlChars (const string& str) {
    string result = str;

    result = replaceXmlChar(result, "&amp;", "&");
    result = replaceXmlChar(result, "&lt;", "<");
    result = replaceXmlChar(result, "&gt;", ">");
    result = replaceXmlChar(result, "&apos;", "'");
    result = replaceXmlChar(result, "&quot;", "\"");

    if (result.length()>1 && result[result.length()-1] == '\r') {
      result = result.substr(0,result.length()-1);
    }
    
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // dimension functions
  ////////////////////////////////////////////////////////////////////////////////

  Element* Palo1Converter::findOrAddElement (Dimension* dimension, const string& name, Element::ElementType type, bool changeType) {
    Element* element = dimension->lookupElementByName(name);
  
    if (element) {
      if (element->getElementType() != type && changeType) {
        dimension->changeElementType(element, type, 0, false);
      }
      return element;
    }
    else {
      return dimension->addElement(name, type, 0, true);
    }
  }



  void Palo1Converter::updateElementPosition (Dimension* dimension, const string& name, PositionType position) {
    Element* element = dimension->lookupElementByName(name);
  
    if (element) {
      dimension->moveElement(element, position, 0);
    }
  }



  Element* Palo1Converter::findElement (Dimension* dimension, const string& name, Element::ElementType type) {
    Element* result = dimension->lookupElementByName(name);

    if (result == 0) {
      *logFile << "ERROR Element '" << name << "' of dimension '" + dimension->getName() + "' not found." << endl;
      *logFile << "ERROR Is element '" << name << "' in dimension XML-File?" << endl;
      throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "Element '" + name + "' of dimension '" + dimension->getName() + "' not found.");      
    }
  
    if (result->getElementType() != type) {
      *logFile << "ERROR Element '" << result->getName() << "' has type '" << result->getElementType()
               << "' but should have type '" << type << "'" << endl;
      throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "wrong element type of element '" + name + "' in cube data file");      
    }
  
    return result;
  }



  void Palo1Converter::processDimension (Database* database, const string& name, ifstream& file) {
    Dimension* dimension = 0;
    string::size_type pos;
  
    bool foundDimensionTag = false;
    while (! file.eof() && !foundDimensionTag) {
      string line;
      getline(file, line);
    
      pos = line.find("<Dimension");

      if (pos != string::npos) {
        pos = line.find("\">", pos);

        if (pos != string::npos) {
          string dimensionName;
          string::size_type pos2 = line.find("</Dimension>", pos);
          if (pos2 != string::npos) {
            dimensionName = replaceXmlChars(line.substr(pos+2, pos2-pos-2));
          }
          else {
            dimensionName = replaceXmlChars(line.substr(pos+2));
          }
          
          foundDimensionTag = true;
          if (name != dimensionName) {
            *logFile << "WARNING dimension name (filename): '" << name << "'" << endl;
            *logFile << "WARNING dimension name (xml tag):  '" << dimensionName << "'" << endl;
            *logFile << "WARNING using dimension name:      '" << name << "'" << endl;
          }
        }
      }
    }
  
    if (!foundDimensionTag) {
      throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "dimension name not found"); 
    }

    dimension = database->addDimension(name, 0, false);
    
    vector<string> NameToPosition;
  
    // get elements  
    while (! file.eof()) {
      string line;

      getline(file, line);
      pos = line.find("<Element Type=\"N\">");

      if (pos != string::npos) {
        string::size_type pos2 = line.find("</Element>", pos);

        if (pos != string::npos) {
          string e = replaceXmlChars(line.substr(pos+18, pos2 - pos - 18));
          *logFile << "INFO    numeric element '" << e << "'" << endl;
          findOrAddElement(dimension, e, Element::NUMERIC, true);
          NameToPosition.push_back(e);
        }
      }

      pos = line.find("<Element Type=\"S\">");

      if (pos != string::npos) {
        string::size_type pos2 = line.find("</Element>", pos);

        if (pos != string::npos) {
          string e = replaceXmlChars(line.substr(pos+18, pos2 - pos - 18));
          *logFile << "INFO    string element '" << e << "'" << endl;
          findOrAddElement(dimension, e, Element::STRING, true);
          NameToPosition.push_back(e);
        }
      }

      pos = line.find("<Element Type=\"C\">");

      if (pos != string::npos) {
        string::size_type pos2 = line.find("</Element>", pos);

        if (pos2 != string::npos) {
          string e = replaceXmlChars(line.substr(pos+18, pos2 - pos - 18));
          findOrAddElement(dimension, e, Element::CONSOLIDATED, true);
          NameToPosition.push_back(e);
        }
        else {      
          ElementsWeightType children;
          string e = replaceXmlChars(line.substr(pos+18));
          *logFile << "INFO    consolidated element '" << e << "'" << endl;
          Element* con = findOrAddElement(dimension, e, Element::CONSOLIDATED, true);
          NameToPosition.push_back(e);
      
          // now get the children
          bool end = false;      

          while (! file.eof() && !end) {
            getline(file, line);
            
            pos = line.find("</Element>");

            if (pos != string::npos) {
              // last line found
              end = true;
            }
            else {
              string::size_type pos1 = line.find("<Consolidate Factor=\"");
              
              if (pos1 != string::npos) {
                string::size_type pos2 = line.find("\">", pos1);
                string::size_type pos3 = line.find("</Consolidate>", pos1);
                
                if (pos2 != string::npos && pos3 != string::npos) {
                  string factor = line.substr(pos1+21, pos2 - pos1 - 21);
                  string name   = replaceXmlChars(line.substr(pos2+2, pos3 - pos2 - 2));
                  *logFile << "INFO        child element '" << name << "' (factor: " << factor << ")" << endl;
                  Element* e = findOrAddElement(dimension, name, Element::NUMERIC, false); 
                  pair<Element*, double> p;
                  p.first  = e;
                  p.second = StringUtils::stringToDouble(factor);
                  children.push_back(p);              
                }
                else {
                  *logFile << "ERROR        error in file" << endl;
                  throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "child element not found (" + line + ")"); 
                  return;
                }
              }
              else {
                *logFile << "ERROR        tag '<Consolidate Factor=\"' or '</Element>' not found" << endl;
                throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "child element not found"); 
              }
            }
          }        
          
          dimension->addChildren(con, &children, 0);      
        }
      }
    }
    
    PositionType max = (PositionType) NameToPosition.size();
    
    for (PositionType i = 0; i < max; i++) {
      updateElementPosition(dimension, NameToPosition.at(i), i);
    }
  }



  void Palo1Converter::readDimension (ifstream &file, Dimension* dimension, vector<Element*>* elements) {
    // *logFile << "INFO   dimension = '" << dimension->getName() << "'" << endl;
  
    uint32_t numberNElements = readInteger(file);
    *logFile << "INFO    dimension has '" << numberNElements << "' numeric elements" << endl;

    elements->resize(dimension->sizeElements());

    // numeric elements
    for (uint32_t i = 0; i < numberNElements;  i++) {
      int num = readInteger(file);
      string name = readString(file);
      *logFile << "INFO    id '" << num << "' numeric element name = '" << name << "'" << endl;
      Element* e = findElement(dimension, name, Element::NUMERIC); 

      if (elements->size() <= (size_t) num) {
        *logFile << "WARNING    id > size of dimension" << endl;
        *logFile << "WARNING    size of dimension = " << (int) dimension->sizeElements() << endl;
        elements->resize(num+1);
      }

      elements->at(num) = e;
    } 

    uint32_t numberSElements = readInteger(file);
    *logFile << "INFO    dimension has '" << numberSElements << "' string elements" << endl;

    // string elements
    for (uint32_t i = 0;  i < numberSElements;  i++) {
      int num = readInteger(file);
      string name = readString(file);
      *logFile << "INFO    id '" << num << "' string element name = '" << name << "'" << endl;
      Element* e = findElement(dimension, name, Element::STRING);
      
      if (elements->size() <= (size_t) num) {
        *logFile << "WARNING    id > size of dimension" << endl;
        *logFile << "WARNING    size of dimension = " << (int) dimension->sizeElements() << endl;
        elements->resize(num+1);
      }
      
      elements->at(num) = e;
    }
  
    uint32_t numberCElements = readInteger(file);
    *logFile << "INFO    dimension has '" << numberCElements << "' consolidated elements" << endl;

    // consolidated elements
    for (uint32_t i=0; i<numberCElements; i++) {
      int num = readInteger(file);
      string name = readString(file);
      *logFile << "INFO    id '" << num << "' consolidated element name = '" << name << "'" << endl;
      Element* e = findElement(dimension, name, Element::CONSOLIDATED);
      
      if (elements->size() <= (size_t) num) {
        *logFile << "WARNING    id > size of dimension" << endl;
        *logFile << "WARNING    size of dimension = " << (int) dimension->sizeElements() << endl;
        elements->resize(num+1);
      }
      
      elements->at(num) = e;
      
      stringstream children;
      stringstream weights;
      uint32_t numberChildren = readInteger(file);
      
      for (uint32_t j = 0;  j < numberChildren;  j++) {
        string nameChild = readString(file);
        double weight    = readDouble(file);
        *logFile << "INFO       child element name = '" << nameChild << "' and weight = '" << weight << "'" << endl;
        // we can check children here
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // cube functions
  ////////////////////////////////////////////////////////////////////////////////

  Cube* Palo1Converter::processCubeDescription (Database* database, const string& name, ifstream& file) {
    string cubeName;
    string::size_type pos;
  
    bool cubeTagFound = false;
    while (! file.eof() && !cubeTagFound) {
      string line;
      getline(file, line);
    
      pos = line.find("<Cube>");

      if (pos != string::npos) {
        cubeName = replaceXmlChars(line.substr(pos+6));
        cubeTagFound = true;

        if (name != cubeName) {
          *logFile << "WARNING cube name (filename): '" << name << "'" << endl;
          *logFile << "WARNING cube name (xml tag):  '" << cubeName << "'" << endl;
          *logFile << "WARNING using cube name:      '" << name << "'" << endl;
        }
      }
    }
  
    if (!cubeTagFound) {
      throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "cube name not found"); 
    }

    vector<Dimension*> dimensions;
    
    int i = 0;
    while (! file.eof()) {
      string line;
      
      getline(file, line);
      
      pos = line.find("<Dimension>");
      
      if (pos != string::npos) {
        string::size_type j = line.find("</Dimension>");
        
        if (j == string::npos) {
          throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "tag </Dimension> not found"); 
        }
        
        string dimName = replaceXmlChars(line.substr(pos + 11, j - pos - 11));
        *logFile << "INFO   dimension # " << i++ << " = '" << dimName << "'" << endl;      
        Dimension* dimension = database->findDimensionByName(dimName, 0);
        dimensions.push_back(dimension);
      }
    }
    
    return database->addCube(name, &dimensions, 0, false);
  }



  void Palo1Converter::processCubeData (Database* database, Cube* cube, ifstream& file) {
    const vector<Dimension*>* dimensions = cube->getDimensions();
  
    // read tiemstamp
    readInteger(file);
    readInteger(file);
    
    // get number of dimensions in cube
    uint32_t numberDimension = readInteger(file);
    
    *logFile << "INFO number of dimensions: " << numberDimension << endl;
    
    if (numberDimension != dimensions->size()) {
      throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "wrong number of dimensions in cube data file"); 
    }
    
    // read dimensions
    vector< vector<Element*> > dimElements;  
    dimElements.resize(numberDimension);

    for (uint32_t i = 0;  i < numberDimension;  i++) {
      *logFile << "INFO reading dimension # " << i << " dimension name = '" << dimensions->at(i)->getName() << "'" << endl;
      readDimension(file, dimensions->at(i), &dimElements.at(i));
    }
    
    vector<uint32_t> ids;
    ids.resize(numberDimension);

    // read numeric values
    bool endFound = false;
    
    *logFile << "INFO reading numeric cell values" << endl;

    while (!file.eof() && !endFound) {
      vector<Element*> elements;
      
      // read ids of cell      
      for (uint32_t i = 0;  i < numberDimension;  i++) {
        ids.at(i) = readInteger(file);
      }

      // read value of cell      
      double value = readDouble(file);
      
      // 0.0 is end marker
      if (value == 0.0) {
        *logFile << "INFO found end marker" << endl;
        endFound = true;
      }
      else {
        // import cell value

        *logFile << "INFO set cell <";
      
        for (uint32_t i = 0;  i < numberDimension;  i++) {
          uint32_t id = ids.at(i);
        
          if (i != 0) {
            *logFile << ",";
          }
        
          *logFile << "(" << id << ") ";

          if (id > dimElements.at(i).size()) {
            *logFile << endl;
            *logFile << "ERROR id '" << id << "' is not defined in the list of elements of dimension # " << i << endl;
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "wrong id used for element in cube data file"); 
          }
          Element* e = dimElements.at(i).at(id);
          if (e == 0) {
            *logFile << endl;
            *logFile << "ERROR id '" << id << "' is not defined in the list of elements of dimension # " << i << endl;
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "wrong id used for element in cube data file"); 
          }
        
          *logFile << e->getName();
          elements.push_back(dimElements.at(i).at(id));
        }
      
        *logFile << "> = '" << value << "'" << endl;

        CellPath cp(cube, &elements);
        
        if (cp.getPathType() != Element::NUMERIC) {
          *logFile << endl << "element path is not NUMERIC but cube cell is NUMERIC" << endl;
          
          for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
            *logFile << "INFO      element '" << (*i)->getName() << "' type = " << (*i)->getElementType() << endl;
          }
        }
        
        cube->setCellValue(&cp, value, 0, 0, false, false, false, Cube::DEFAULT, 0);
      }
      
      
    }
    
    // read string values
    endFound = false;
    
    *logFile << "INFO reading string cell values" << endl;

    while (! file.eof() && ! endFound) {
      vector<Element*> elements;
      
      bool isNull = true;
      
      // read ids of cell      
      for (uint32_t i = 0;  i < numberDimension;  i++) {
        uint32_t id = readInteger(file); 
        ids.at(i) = id;
        if (id>0) {
          isNull = false;
        }
      }

      // read value of cell      
      string value = readString(file);
      
      // "" is end marker
      if (isNull && value.empty()) {
        *logFile << "INFO found end marker" << endl;
        endFound = true;
      }
      else {
        // import cell value

        *logFile << "INFO set cell <";

        for (uint32_t i = 0;  i < numberDimension;  i++) {
          uint32_t id = ids.at(i);
        
          if (i != 0) {
            *logFile << ",";
          }
        
          *logFile << "(" << id << ") ";

          if (id > dimElements.at(i).size()) {
            *logFile << endl;
            *logFile << "ERROR id '" << id << "' is not defined in the list of elements of dimension # " << i << endl;
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "wrong id used for element in cube data file"); 
          }
          Element* e = dimElements.at(i).at(id);
          if (e == 0) {
            *logFile << endl;
            *logFile << "ERROR id '" << id << "' is not defined in the list of elements of dimension # " << i << endl;
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "wrong id used for element in cube data file"); 
          }
        
          *logFile << e->getName();
          elements.push_back(dimElements.at(i).at(id));
        }
      
        *logFile << "> = '" << value << "'" << endl;

        CellPath cp(cube, &elements);
        
        if (cp.getPathType() != Element::STRING) {
          *logFile << endl << "element path is not STRING but cube cell is STRING" << endl;
          
          for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
            *logFile << "INFO      element '" << (*i)->getName() << "' type = " << (*i)->getElementType() << endl;
          }
        }
        
        cube->setCellValue(&cp, value, 0, 0, false, false, 0);
      }      
    }

    *logFile << "INFO file end reached" << endl;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // database functions
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief converts Palo 1.0 database directory
  ////////////////////////////////////////////////////////////////////////////////

  bool Palo1Converter::convertDatabase (const string& importLogFile) {

    // rename Palo 1.0 directory
    string oldDirName = palo1Path + palo1Directory;
    string newDirName = palo1Path + palo1Directory + backupSuffix;
  
    int ok = rename(oldDirName.c_str(),  newDirName.c_str());

    if (ok != 0) {
      Logger::error << "could not rename directory '" << oldDirName << "'" << endl;
      return false;
    }
  
    string databaseName = palo1Directory;
    Database* database = palo->lookupDatabaseByName(databaseName);

    // open log file in Palo 1.0 directory
    string logFileName = newDirName + "/" + importLogFile;

    ofstream logger(logFileName.c_str());

    if (! logger) {
      Logger::error << "cannot open log file '" << logFileName << "'" << endl;
      return false;
    }

    logFile = &logger;

    // check database name
    if (database) {
      *logFile << "ERROR found database '" << databaseName << "' in palo server." << endl;
      *logFile << "ERROR cannot import database directory '" << oldDirName << "'" << endl;
      rename(newDirName.c_str(), oldDirName.c_str());
      return false;
    }

    string aFileName;

    // convert Palo 1.0 data
    try {

      database = palo->addDatabase(databaseName, 0);
    
      vector<string> files = FileUtils::listFiles(newDirName);

      // import dimensions
      for (vector<string>::iterator i = files.begin(); i != files.end(); i++) {    
        if (StringUtils::isSuffix(*i, ".dimension.xml")) {
          aFileName = *i;
          string filename = newDirName + "/" + *i;      
          ifstream dimensionFile(filename.c_str(), ios::binary);

          *logFile << "INFO ----------------------------------------" << endl;  
          *logFile << "INFO processing dimension file '" <<  *i << "'" << endl;

          if (dimensionFile) {
            string dimensionName = decodeNameFromFileName(*i);
            *logFile << "INFO dimension name '" <<  dimensionName << "'" << endl;

            processDimension(database, dimensionName, dimensionFile);
          }
          else {
            *logFile << "INFO ----------------------------------------" << endl;  
            *logFile << "INFO could not open dimension file '" <<  *i << "'" << endl;
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "could not open dimension file '" + *i + "'"); 
          }
        }
      }

      // import cubes
      for (vector<string>::iterator i = files.begin(); i != files.end(); i++) {    
        if (StringUtils::isSuffix(*i, ".cube.xml")) {
          aFileName = *i;
          Cube* cube;
          string filename = newDirName + "/" + *i;      
          ifstream cubeDescriptionFile(filename.c_str(), ios::binary);

          if (cubeDescriptionFile) {
            *logFile << "INFO ----------------------------------------" << endl;  
            *logFile << "INFO processing cube description file '" <<  *i << "'" << endl;  
            string cubeName = decodeNameFromFileName(*i);
            *logFile << "INFO cube name '" <<  cubeName << "'" << endl;
            cube = processCubeDescription(database, cubeName, cubeDescriptionFile);
          }
          else {
            *logFile << "ERROR ----------------------------------------" << endl;  
            *logFile << "ERROR could not open cube description file '" <<  *i << "'" << endl;  
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "could not open cube description file'" + *i + "'"); 
          }

          string dataFile = (*i).substr(0,(*i).length()-3) + "data";
          aFileName = dataFile;
          string filename2 = newDirName + "/" + dataFile;     
          ifstream cubeDataFile(filename2.c_str(), ios::binary);
          
          if (cubeDataFile) {
            *logFile << "INFO ----------------------------------------" << endl;  
            *logFile << "INFO processing cube data file '" << dataFile << "'" << endl;  
            processCubeData(database, cube, cubeDataFile);
          }
          else {
            *logFile << "ERROR ----------------------------------------" << endl;  
            *logFile << "ERROR could not open cube data file '" <<  dataFile << "'" << endl;  
            throw ErrorException(ErrorException::ERROR_CORRUPT_FILE, "could not open cube data file '" + dataFile + "'"); 
          }
        }
      }

      // commit new Palo 1.5 database
      *logFile << "INFO commiting changes to database '" << database->getName() << "'" << endl;

      palo->saveDatabase(database, 0);

      vector<Cube*> cubes = database->getCubes(0);

      for (vector<Cube*>::iterator j = cubes.begin();  j != cubes.end();  j++) {
        Cube * cube = *j;

        if (cube->getStatus() == Cube::CHANGED) {
          *logFile << "INFO commiting changes to cube '" << cube->getName() << "'" << endl;
          database->saveCube(cube, 0);
        }
      }

      Logger::info << "converted palo 1.0 database '" << palo1Directory << "'" << endl;

      return true;
    }
  
    // cannot convert database
    catch (const ErrorException& e) {
      *logFile << "ERROR " << e.getMessage() << endl;  
      if (aFileName!="") {
        *logFile << "ERROR file: '" << aFileName << "'" << endl;  
      }
      *logFile << "ERROR error found, stopping import" << endl;  

      logger.close();

      database = palo->lookupDatabaseByName(databaseName);
      if (database) {
        palo->deleteDatabase(database, 0);
      }

      if (FileUtils::isDirectory(oldDirName.c_str())) {
        string failed = palo1Path + palo1Directory + ".failed" + backupSuffix;
        rename(oldDirName.c_str(), failed.c_str());
      }

      rename(newDirName.c_str(), oldDirName.c_str());

      Logger::error << "cannot convert palo 1.0 database '" << palo1Directory << "' ("
                    << e.getMessage() << ")" << endl;

      if (aFileName!="") {
        Logger::error << "error in file '" << aFileName << "'" << endl;  
      }

      return false;
    }
  }
  



  string Palo1Converter::decodeNameFromFileName (const string& str) {
    size_t end   = str.find_first_of(".");
    size_t size;
    char *c = (char*)decodePaloBase64(str.substr(0,end), size);
    string result(c, size);
    delete[] c;
    return result;
  }



  uint8_t Palo1Converter::paloBase64ToInt (const uint8_t c) {
    if (c >= 'A' && c <= 'Z') {
      return c - 'A';
    } 
    else if (c >= 'a' && c <= 'z') {
      return c - 'a' + 26;
    } 
    else if (c >= '0' && c <= '9') {
      return c - '0' + 52;
    }
    else if (c == '-') {
      return 62;
    } 
    else if (c == '_') {
      return 63;
    } 
    else if (c == '=') {
      return 0;
    } 

    throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                             "error decoding a base 64 value",
                             "c", c);
  }



  uint8_t * Palo1Converter::decodePaloBase64 (const string& str, size_t& length) {
    if (str.empty()) {
      length = 0;
      return 0;
    }

    size_t l = str.length();

    if ( (l % 4) != 0) {
      throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                               "error decoding a base 64 value",
                               "str", str);
    }
             
    StringBuffer s;
    s.initialize();

    char buffer[4] ;
    size_t j = 0; 

    for (size_t i = 0;  i < l;  i++) {
      buffer[j++] = str[i];

      if (j == 4) {
        s.appendChar((paloBase64ToInt(buffer[0]) << 2) + (paloBase64ToInt(buffer[1]) >> 4));

        if (buffer[1] != '=' && buffer[2] != '=') {
          s.appendChar(((paloBase64ToInt(buffer[1]) & 0x0f) << 4) + (paloBase64ToInt(buffer[2]) >> 2));
        }

        if (buffer[2] != '=' && buffer[3] != '=') {
          s.appendChar(((paloBase64ToInt(buffer[2]) & 0x03) << 6) + (paloBase64ToInt(buffer[3])));
        }
        
        j = 0;
      } 
    }

    length = s.length();

    uint8_t * result = new uint8_t[length];
    memcpy(result, s.c_str(), length);

    s.free();

    return result;
  }  

  
}
