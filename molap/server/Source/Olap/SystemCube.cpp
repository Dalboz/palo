////////////////////////////////////////////////////////////////////////////////
/// @brief palo system cube
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
#include "InputOutput/FileWriter.h"

#include "Olap/SystemCube.h"
#include "Olap/Server.h"

namespace palo {
  const string SystemCube::PREFIX_GROUP_DIMENSION_DATA = "#_GROUP_DIMENSION_DATA_";
  const string SystemCube::GROUP_CUBE_DATA             = "#_GROUP_CUBE_DATA";
  const string SystemCube::CONFIGURATION_DATA          = "#_CONFIGURATION";
  const string SystemCube::NAME_VIEW_LOCAL_CUBE        = "#_VIEW_LOCAL";
  const string SystemCube::NAME_VIEW_GLOBAL_CUBE       = "#_VIEW_GLOBAL";
  const string SystemCube::NAME_SUBSET_LOCAL_CUBE      = "#_SUBSET_LOCAL";
  const string SystemCube::NAME_SUBSET_GLOBAL_CUBE     = "#_SUBSET_GLOBAL";
  
  
  void SystemCube::saveCube () {
    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "cube file name not set");
    }
     
    database->getServer()->makeProgress();
    database->saveDatabase();

    if (status == LOADED) {
      return;
    }

    // open a new temp-cube file
    FileWriter fw(FileName(*fileName, "tmp"), false);
    fw.openFile();
    
    // save overview
    database->getServer()->makeProgress();
    saveCubeOverview(&fw);

    // and dimensions
    database->getServer()->makeProgress();
    saveCubeDimensions(&fw);

    // and cells
    database->getServer()->makeProgress();

    saveCubeCells(&fw);

    // that's it
    fw.appendComment("");
    fw.appendComment("PALO CUBE DATA END");

    fw.closeFile();
    
    // delete journal files
    bool journalOpen = false;

    if (journal != 0) {
      closeJournal();
      journalOpen = true;
    }

    // archive journal
    JournalFileWriter::archiveJournalFiles(FileName(*fileName, "log"));
    // remove old cube file
    FileUtils::remove(*fileName);

    // delete journal
    JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

    // rename temp-cube file
    FileUtils::rename(FileName(*fileName, "tmp"), *fileName);
    
    // reopen journal
    if (journalOpen) {
      openJournal();
    }

    // the cube is now loaded
    database->getServer()->makeProgress();
    status = LOADED;
  }


  void SystemCube::loadCube (bool processJournal) {
    if (status == LOADED) {
      return;
    }

    if (fileName == 0) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "cube file name not set");
    }
     
    updateToken();
    updateClientCacheToken();
    
    if (!FileUtils::isReadable(*fileName) && FileUtils::isReadable(FileName(*fileName, "tmp"))) {
      Logger::warning << "using temp file for cube '" << name << "'" << endl;

      // delete journal
      JournalFileWriter::deleteJournalFiles(FileName(*fileName, "log"), false);

      // rename temp file
      FileUtils::rename(FileName(*fileName, "tmp"), *fileName);
    } 
    
    FileReader fr(*fileName);
    fr.openFile();
        
    // load overview
    database->getServer()->makeProgress();
    loadCubeOverview(&fr);

    // and cell values
    database->getServer()->makeProgress();

    if (fr.isSectionLine()) {
      loadCubeCells(&fr);
    }
    else {
      Logger::info << "section line not found for cube '" << getName() << "'" << endl; 
    }

    if (processJournal) {
      database->getServer()->makeProgress();
      processCubeJournal(status);
    }

    // the cube is now loaded
    database->getServer()->makeProgress();
    status = LOADED;

    // we can now load the rules
    loadCubeRules();
  }
  
}
