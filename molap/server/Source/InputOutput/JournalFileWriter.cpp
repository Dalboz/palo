////////////////////////////////////////////////////////////////////////////////
/// @brief writes journals as csv data to files
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

#include "InputOutput/JournalFileWriter.h"

#include "Collections/StringUtils.h"

#include "Exceptions/FileOpenException.h"

#include "InputOutput/FileReader.h"
#include "InputOutput/FileUtils.h"
#include "InputOutput/Logger.h"

extern "C" {
#include <time.h>
}

#include <iostream>

namespace palo {
  using namespace std;
  


  JournalFileWriter::JournalFileWriter (const FileName& fileName, bool bufferOutput)
    : FileWriter (fileName, bufferOutput), lastFileName() {      
    lastFileId = 0;
    count = 0;
  }



  void JournalFileWriter::openFile () {
    
    // find last journal file 
    bool lastJournalFound = false;
    int last = 0;
    int next = 0;    

    while (! lastJournalFound) {
      stringstream se;
      se << fileName.name << "_" << next;
    
      FileName jf(fileName.path, se.str(), fileName.extension);

      if (! FileUtils::isReadable(jf)) {
        lastJournalFound = true;
      }
      else {
        last = next;
        next++;
      }
    }

    lastFileId = last;    

    stringstream se;
    se << fileName.name << "_" << lastFileId;

    lastFileName = FileName(fileName.path, se.str(), fileName.extension);
    
    // open last file
    outputFile = FileUtils::newOfstreamAppend(lastFileName.fullPath());
    
    if (outputFile == 0) {
      Logger::warning << "could not write to file '" << lastFileName.fullPath() << "'" << endl;
      throw FileOpenException("could not open file for writing", lastFileName.fullPath());
    }

    // check size of found journal file
    if (last != next) {
      checkFileSize();
    }    
  }



  void JournalFileWriter::closeFile () {
    if (outputFile != 0) {
      nextLine();
      writeBuffer();    
      outputFile->close();   

      delete outputFile;

      outputFile = 0;
    }
  }


    
  void JournalFileWriter::appendCommand (const string& user, const string& event, const string& command) {
    if (outputFile == 0) {
      Logger::error << "journal file is closed" << endl;
      return;
    }

    if (!isFirstValue) {
      nextLine();
    }
    
    // check file size of journal file after 10000 lines
    if ((count % 10000) == 0) {
      checkFileSize();
    }
    
    appendTimeStamp();
        
    *outputFile << StringUtils::escapeString(user) << ";" << StringUtils::escapeString(event) << ";" << command << ";";
  }



  void JournalFileWriter::checkFileSize () {
    if (outputFile == 0) {
      Logger::error << "journal file is closed" << endl;
      return;
    }

    int32_t bytes = getFileSize(lastFileName);
    
    if (bytes > 100000000) { 
      if (*outputFile) {
        writeBuffer();    
        outputFile->close();
        delete outputFile;
      }

      lastFileId++;

      stringstream se;
      se << fileName.name << "_" << lastFileId;

      lastFileName = FileName(fileName.path, se.str(), fileName.extension);

      outputFile = FileUtils::newOfstreamAppend(lastFileName.fullPath());

      if (outputFile == 0) {
        Logger::warning << "could not write to file '" << lastFileName.fullPath() << "'" << endl;
        throw FileOpenException("Could not open file for writing.", lastFileName.fullPath());
      }      
    }             
  }



  void JournalFileWriter::deleteJournalFiles (const FileName& fileName, bool deleteArchiv) {

    // find last journal file 
    bool lastJournalFound = false;
    int num = 0;

    while (! lastJournalFound) {
      stringstream se;
      se << fileName.name << "_" << num;      
      
      FileName jf(fileName.path, se.str(), fileName.extension);

      if (! FileUtils::isReadable(jf)) {
        lastJournalFound = true;
      }
      else {
        deleteFile(jf);
        num++;
      }
    }

    if (deleteArchiv) {
      try {
        deleteFile(FileName(fileName.path, fileName.name, "archived"));
      }
      catch (const FileOpenException& e) {
        Logger::info << "ignoring " << e.getMessage() << "/" << e.getDetails() << endl;
      }
    }
  }



  void JournalFileWriter::archiveJournalFiles (const FileName& fileName) {

    // find last journal file 
    bool lastJournalFound = false;
    int num = 0;

    FileName af(fileName.path, fileName.name, "archived");
    ofstream outputFile(af.fullPath().c_str(), ios::app);

    while (! lastJournalFound) {
      stringstream se;
      se << fileName.name << "_" << num;      
      
      FileName jf(fileName.path, se.str(), fileName.extension);

      if (! FileUtils::isReadable(jf)) {
        lastJournalFound = true;
      }
      else {
        {
          ifstream inputFile(jf.fullPath().c_str());

          while (inputFile) {
            string line;
            getline(inputFile, line);

            if (inputFile) {
              outputFile << line << "\n";
            }
          }
        }

        deleteFile(jf);
        num++;
      }
    }
  }
}
