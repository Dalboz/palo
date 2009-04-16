////////////////////////////////////////////////////////////////////////////////
/// @brief reads journal data files
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

#include "InputOutput/JournalFileReader.h"

#include "Exceptions/FileOpenException.h"

#include "InputOutput/FileUtils.h"
#include "InputOutput/Logger.h"

#include <iostream>

namespace palo {
  using namespace std;
  


  JournalFileReader::JournalFileReader (const FileName& fileName) 
  : FileReader (fileName) {      
    lastFileId = 0;
  }

    

  void JournalFileReader::openFile () {
    
    // find first journal file 
    lastFileId = 0;    

    stringstream se;
    se << fileName.name << "_" << lastFileId;

    string filename = FileName(fileName.path, se.str(), fileName.extension).fullPath();
    
    inputFile = FileUtils::newIfstream(filename);
    
    if (inputFile == 0) {
      Logger::warning << "could not read from file '" << filename << "'" << endl;
      throw FileOpenException("could not open file for reading", filename);
    }

    // read first line 
    nextLine();
  }


  
  void JournalFileReader::nextLine (bool strip) {
    if (!endOfFile) {
      FileReader::nextLine(strip);

      if (endOfFile) {

        // check next journal file
        stringstream filename;
        filename <<  fileName.name << "_" << (lastFileId + 1);

        FileName jf(fileName.path, filename.str(), fileName.extension);

        if (FileUtils::isReadable(jf)) {
          string fn = jf.fullPath();

          // next journal file found
          inputFile->close();
          lastFileId++;
          inputFile->open(fn.c_str());
          endOfFile = false;

          // read first line 
          FileReader::nextLine(strip);
        }    
      }
    }
  }



  void JournalFileReader::gotoTimeStamp (long int seconds, long int useconds) {    
    while (isDataLine()) {       
      long int s, u;
      getTimeStamp(&s, &u, 0);

      if (s > seconds) {
        return;
      }
      else if (s == seconds && u >= useconds) {
        return;
      }      

      nextLine();
    }    
  }
}
