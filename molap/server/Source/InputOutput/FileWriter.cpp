////////////////////////////////////////////////////////////////////////////////
/// @brief writes csv data to files
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

#include "InputOutput/FileWriter.h"

#include "Exceptions/FileOpenException.h"

#include "Collections/StringBuffer.h"

#include "InputOutput/FileUtils.h"
#include "InputOutput/Logger.h"

#include <stdio.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

extern "C" {
#include <sys/stat.h>
}

#include <iostream>

namespace palo {
  using namespace std;
  


  FileWriter::FileWriter (const FileName& fileName, bool bufferOutput)
    : fileName(fileName) {
    isFirstValue = true;
    isBuffered   = bufferOutput;
    outputFile   = 0;
  }




  FileWriter::~FileWriter () {
    closeFile();
  }
    


    
  void FileWriter::openFile () {
    outputFile = FileUtils::newOfstream(fileName.fullPath());

#if defined(_MSC_VER)
    if (outputFile == 0 && errno == EACCES) {
      for (int ms = 0;  ms < 5;  ms++) {
        Logger::warning << "encountered MS file-system change-log bug during open, sleeping to recover" << endl;
        usleep(1000);

        outputFile = FileUtils::newOfstream(fileName.fullPath());

        if (outputFile != 0 || errno != EACCES) {
          break;
        }
      }
    }
#endif
    
    if (outputFile == 0) {
      Logger::warning << "could not write to file '" << fileName.fullPath() << "'" 
                      << " (" << strerror(errno) << ")" << endl;
      throw FileOpenException("could not open file for writing", fileName.fullPath());
    }
  }



    
  void FileWriter::closeFile () {
    if (outputFile != 0) {
      nextLine();
      writeBuffer();    
      outputFile->close();   
      delete outputFile;
      outputFile = 0;
    }
  }



    
  void FileWriter::appendComment (const string& value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    if (!isFirstValue) {
      nextLine();
    }

    // delete line feeds
    size_t pos = value.find_first_of("\r\n");

    if (pos != string::npos) {
      string copy = value;
      size_t pos = copy.find_first_of("\r\n");

      while (pos != string::npos) {
        copy[pos] = ' ';
        pos = copy.find_first_of("\r\n", pos);
      }

      *outputFile << "# " << copy;
    }
    else {
      *outputFile << "# " << value;
    }

    isFirstValue = false;

    nextLine();      
  }




  void FileWriter::appendSection (const string& value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    *outputFile << "[" << value << "]";
    isFirstValue = false;
    nextLine();
  }



    
  void FileWriter::appendString (const string& value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    *outputFile << value << ";";
    isFirstValue = false;
  }



    
  void FileWriter::appendEscapeString (const string& value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    *outputFile << escapeString(value) << ";";
    isFirstValue = false;
  }



    
  void FileWriter::appendEscapeStrings (const vector<string>* value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    bool first = true;

    for (vector<string>::const_iterator i = value->begin(); i != value->end(); i++) {
      if (first) {
        first = false;
      } 
      else {
        *outputFile << ",";
      }
      *outputFile << escapeString(*i);
    }

    *outputFile << ";";        

    isFirstValue = false;
  }




  void FileWriter::appendInteger (const int32_t value){
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    *outputFile << value << ";";
    isFirstValue = false;
  }



  void FileWriter::appendIntegers (const vector<int32_t>* value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    bool first = true;

    for (vector<int32_t>::const_iterator i = value->begin(); i != value->end(); i++) {
      if (first) {
        first = false;
      } 
      else {
        *outputFile << ",";
      }
      *outputFile << *i;
    }

    *outputFile << ";";        

    isFirstValue = false;
  }
    



  void FileWriter::appendIntegers (const vector<size_t>* value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    bool first = true;

    for (vector<size_t>::const_iterator i = value->begin(); i != value->end(); i++) {
      if (first) {
        first = false;
      } 
      else {
        *outputFile << ",";
      }

      *outputFile << (uint32_t) *i;
    }

    *outputFile << ";";        

    isFirstValue = false;
  }
    



  void FileWriter::appendIdentifier (const IdentifierType value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    *outputFile << value << ";";        

    isFirstValue = false;
  }
    



  void FileWriter::appendIdentifiers (const IdentifiersType* value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    bool first = true;

    for (IdentifiersType::const_iterator i = value->begin(); i != value->end(); i++) {
      if (first) {
        first = false;
      } 
      else {
        *outputFile << ",";
      }

      *outputFile << (uint32_t) *i;
    }

    *outputFile << ";";        

    isFirstValue = false;
  }
    



  void FileWriter::appendDouble (const double value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    outputFile->precision(15);

    *outputFile << value << ";";
    isFirstValue = false;
  }
    


  void FileWriter::appendDoubles (const vector< double >* value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    bool first = true;

    for (vector<double>::const_iterator i = value->begin(); i != value->end(); i++) {
      if (first) {
        first = false;
      } 
      else {
        *outputFile << ",";
      }
      *outputFile << *i;
    }

    *outputFile << ";";        

    isFirstValue = false;
  }


  void FileWriter::appendBool (const bool value) {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    if (value) {
      *outputFile << "1;";
    }
    else {
      *outputFile << "0;";
    }

    isFirstValue = false;
  }
    



  void FileWriter::appendTimeStamp () {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    timeval tv;
    gettimeofday(&tv, 0);
        
    *outputFile << tv.tv_sec << "." << tv.tv_usec << ";";
    isFirstValue = false;
  }
    



  void FileWriter::nextLine () {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    if (!isFirstValue) {
      *outputFile << endl;

      if (!isBuffered) {
        outputFile->flush();        
      }
    }

    isFirstValue = true;
  }
    
  void FileWriter::writeBuffer () {
    if (outputFile == 0) {
      Logger::error << "file writer is closed" << endl;
      return;
    }

    outputFile->flush();        
  }
    

  
  string FileWriter::escapeString (const string& text) {
    StringBuffer sb;
    sb.initialize();
    size_t begin = 0;
    size_t end   = text.find("\"");
    
    sb.appendText("\"");        

    while (end != string::npos) {

      sb.appendText(text.substr(begin,end-begin));
      sb.appendText("\"\"");        

      begin = end+1;
      end   = text.find("\"", begin);       
    }


    sb.appendText(text.substr(begin,end-begin));
    sb.appendText("\"");        
    string result = sb.c_str();
    sb.free();
    return result;
  }
  
  
  
  void FileWriter::deleteFile (const FileName& fileName) {
    if (!FileUtils::remove(fileName)) {
      throw FileOpenException("could not delete file", fileName.fullPath());
    }
  }
  
  
  
  int32_t FileWriter ::getFileSize (const FileName& fileName) {
    struct stat fileStat;
    int result = stat(fileName.fullPath().c_str(), &fileStat);
    
    if (result < 0) {
      return 0;
    }

    return (int32_t) fileStat.st_size;
  }
}
