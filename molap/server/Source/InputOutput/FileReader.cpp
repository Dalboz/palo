////////////////////////////////////////////////////////////////////////////////
/// @brief reads csv data from files
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

#include "InputOutput/FileReader.h"

#include "Exceptions/FileOpenException.h"

#include "Collections/StringBuffer.h"

#include "InputOutput/FileUtils.h"
#include "InputOutput/Logger.h"

#include <iostream>
 
namespace palo {
  using namespace std;


  
  FileReader::FileReader (const FileName& fileName) 
    : data(false), section(false), endOfFile(false), sectionName(), fileName(fileName) {
    inputFile = 0;
    lineNumber = 0;
  }




  FileReader::~FileReader () {
    if (inputFile != 0) {
      inputFile->close();
      delete inputFile;
    }   
  }
    



  void FileReader::openFile() {
    inputFile = FileUtils::newIfstream(fileName.fullPath());
    
    if (inputFile == 0) {
      Logger::warning << "could not read from file '" << fileName.fullPath() << "'" 
                      << " (" << strerror(errno) << ")" << endl;
      throw FileOpenException("could not open file for reading", fileName.fullPath());
    }

    lineNumber = 0;

    // read first line
    nextLine();
  }




  void FileReader::nextLine (bool strip) {
    data = false;
    section = false;
    
    string line = "";

    while(line.empty() || line[0] == '#') {
      if (inputFile->eof()) {
        endOfFile=true;
        return;
      }
    
      lineNumber++;
      
      getline(*inputFile, line);

      if (! line.empty() && line[line.size()-1] == '\r') {
        line.erase(line.size()-1);
      }

      if (strip) {
        while (! line.empty() && (line[line.size()-1] == ' ' || line[line.size()-1] == '\t')) {
          line.erase(line.size()-1);
        }
      }
    }
    
    if (line[0]=='[') {
      processSection(line);
    }
    else {
      processDataLine(line);
    }
  }    
  


  
  void FileReader::processSection (const string& line) {
    size_t end   = line.find("]",1);
    if (end != string::npos) {
      section = true;
      sectionName = line.substr(1,end-1);
      //cout << sectionName;
    }
    //cout << endl;
  }



  
  void FileReader::processDataLine (string& line) {
    dataLine.clear();
    splitString(line, &dataLine, ';', true);    
    data = true;
  }



  void FileReader::splitString (string& line, vector<string>* elements, char seperator, bool readNextLine) {
    if (line.empty()) {
      return;
    }

    string s = "";
    bool first = true;
    bool escaped = false;
    bool endFound = false;

    while (! endFound) {    
      size_t len = line.length();
      size_t pos = 0;

      while (pos < len) {
        char c = line[pos];
      
        if (first) {
          if (line[pos] == '"') {
            escaped = true;
            first = false;
          }

          // empty value found
          else if (line[pos] == seperator) {
            elements->push_back("");
          }
          else {
            s += c;
            first = false;
          }
        }
        else {
          if (escaped) {
            if (line[pos] == '"') {
              if (pos+1 < len) {
                pos++;
                if (line[pos] == seperator) {
                  elements->push_back(s);
                  s = "";
                  first = true;
                  escaped = false;
                }
                else {
                  s += c;
                }
              }
              else {
                elements->push_back(s);
              }
            }
            else {
              s += c;
            }
          }
          else {
            if (line[pos] == seperator) {
              elements->push_back(s);
              s = "";
              first = true;
            }
            else {
              s += c;
            }
          }
        }
        pos++;
      }

      if (! first && readNextLine) {

        // error or string has a new line
        getline(*inputFile, line);
        
        lineNumber++;
        
        if (! line.empty() && line[line.size()-1] == '\r') {
          line.erase(line.size()-1);
        }

        if (inputFile->eof()) {

          // error
          elements->push_back(s);
          endFound = true;
        }
        else {
          s += '\n';
        }
      }
      else {
        elements->push_back(s);
        endFound = true;
      }
    }    
  }

  
  
  const string& FileReader::getDataString (int num) const {
    static const string empty;

    if (num >= 0 && (size_t) num < dataLine.size()) {
      return dataLine[num];
    }
    
    return empty;
  }
  


  
  long FileReader::getDataInteger (int num, int defaultValue) const {
    string x = getDataString(num);
    
    if (x == "") {
      return defaultValue;
    }    

    char *p;    
    long result = strtol(x.c_str(), &p, 10);    

    if (*p != '\0') {
      return defaultValue;
    }    

    return result;    
  }



  
  double FileReader::getDataDouble (int num) const {
    string x = getDataString(num);
    char *p;    
    double result = strtod(x.c_str(), &p);    

    if (*p != '\0') {
       return 0.0;
    }    

    return result;
  }




  bool FileReader::getDataBool (int num, bool defaultValue) const {
    string x = getDataString(num);
    if (x == "1") {
       return true;
    }    
    else if (x == "") {
       return defaultValue;
    } 
    return false;
  }




  const vector<string> FileReader::getDataStrings (int num) {
    string x = getDataString(num);
    vector<string> result;
    splitString(x, &result, ',');

    return result;    
  }
  
  
  
  const vector<int> FileReader::getDataIntegers (int num) {
    const vector<string> stringList = getDataStrings(num);
    vector<int> result;
    char *p;    

    for (vector<string>::const_iterator i = stringList.begin(); i!=stringList.end(); i++) {
      long li = strtol((*i).c_str(), &p, 10);    

      if (*p != '\0') {
        result.push_back(0);
      }
      else {
        result.push_back(li);
      }
    }

    return result;
  }
  
  

  const IdentifiersType FileReader::getDataIdentifiers (int num) {
    const vector<string> stringList = getDataStrings(num);
    IdentifiersType result;
    char *p;    

    for (vector<string>::const_iterator i = stringList.begin();  i != stringList.end();  i++) {
      long li = strtol((*i).c_str(), &p, 10);    

      if (*p != '\0') {
        result.push_back(0);
      }
      else {
        result.push_back(li);
      }
    }

    return result;
  }
  
  

  const vector<double> FileReader::getDataDoubles (int num) {
    const vector<string> stringList = getDataStrings(num);
    vector<double> result;
    char *p;    

    for (vector<string>::const_iterator i = stringList.begin();  i != stringList.end();  i++) {
      double d = strtod((*i).c_str(), &p);   

      if (*p != '\0') {
        result.push_back(0);
      }
      else {
        result.push_back(d);
      }
    } 

    return result;
  }
  
  void FileReader::getTimeStamp (long *seconds, long *useconds, int num) {
    string x = getDataString(num);
    vector<string> result;
    splitString(x, &result, '.');
    
    if (result.size() != 2) {
      *seconds = 0;
      *useconds = 0;
      return;
    }
    
    char *p;    
    *seconds = strtol(result.at(0).c_str(), &p, 10);    

    if (*p != '\0') {
      *seconds = 0;
      *useconds = 0;
      return;
    }    
    
    *useconds = strtol(result.at(1).c_str(), &p, 10);    

    if (*p != '\0') {
      *seconds = 0;
      *useconds = 0;
      return;
    }    
  }
}
