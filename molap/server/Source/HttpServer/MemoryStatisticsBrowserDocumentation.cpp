////////////////////////////////////////////////////////////////////////////////
/// @brief provides documentation from statistics
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

#include "HttpServer/MemoryStatisticsBrowserDocumentation.h"

#include <iostream>
#include <sstream>

#include "Olap/Dimension.h"

namespace palo {
  MemoryStatisticsBrowserDocumentation::MemoryStatisticsBrowserDocumentation (Server* server,
                                                                              const string& message)
    : BrowserDocumentation(message) {
    long int serverTotal = 0;

    vector<string> databaseName;
    vector<string> dimensionName;
    vector<string> cubeName;
    vector<string> memUsageIndex;
    vector<string> memUsageStorage;
    vector<string> memUsageTotal;
    
    vector<Database*> databases = server->getDatabases(0);
    for (vector<Database*>::iterator databaseIter = databases.begin(); databaseIter != databases.end(); databaseIter++) {
      string dName = (*databaseIter)->getName();

      if ( (*databaseIter)->getStatus() != Database::UNLOADED) {

        // database is in memory
        long int totalIndex = 0;
        long int totalStorage = 0;
        long int total = 0;
                      
        vector<Dimension*> dimensions = (*databaseIter)->getDimensions(0);    

        if (dimensions.size() > 0) {
          for(vector<Dimension*>::const_iterator i = dimensions.begin(); i != dimensions.end(); i++) {
            Dimension * dimension = *i;

            databaseName.push_back( StringUtils::escapeHtml(dName));
            dimensionName.push_back(StringUtils::escapeHtml(dimension->getName()));
            cubeName.push_back(     "");
            
            size_t index   = dimension->getMemoryUsageIndex();
            size_t storage = dimension->getMemoryUsageStorage();
            

            memUsageIndex.push_back(  StringUtils::convertToString((uint64_t) index));
            memUsageStorage.push_back(StringUtils::convertToString((uint64_t) storage));
            memUsageTotal.push_back(  StringUtils::convertToString((uint64_t) (index + storage)));

            totalIndex   += (long) index;
            totalStorage += (long) storage;
            total        += (long) (index + storage);
          }
        }
        
        vector<Cube*> cubes = (*databaseIter)->getCubes(0);    

        if (cubes.size() > 0) {
          for(vector<Cube*>::const_iterator i = cubes.begin(); i != cubes.end(); i++) {
            Cube * cube = *i;

            databaseName.push_back( StringUtils::escapeHtml(dName));
            dimensionName.push_back("");
            cubeName.push_back(     StringUtils::escapeHtml(cube->getName()));

            if (cube->getStatus() != Cube::UNLOADED) {
              memUsageIndex.push_back("-");
              memUsageStorage.push_back("-");
              memUsageTotal.push_back("-");
            }
            else {
              memUsageIndex.push_back("-");
              memUsageStorage.push_back("-");
              memUsageTotal.push_back("-");
            }
          }
        }

        // insert sum over all database data        
        databaseName.push_back( StringUtils::escapeHtml(dName));
        dimensionName.push_back("");
        cubeName.push_back(     "");

        memUsageIndex.push_back(  "&Sigma;&nbsp;" + StringUtils::convertToString((uint64_t) totalIndex));
        memUsageStorage.push_back("&Sigma;&nbsp;" + StringUtils::convertToString((uint64_t) totalStorage));
        memUsageTotal.push_back(  "&Sigma;&nbsp;" + StringUtils::convertToString((uint64_t) total));
        
        serverTotal += total;
      }
      else {

        // database is not in memory
        databaseName.push_back(   StringUtils::escapeHtml(dName));
        dimensionName.push_back(  "");
        cubeName.push_back(       "");
        memUsageIndex.push_back(  "-");
        memUsageStorage.push_back("-");
        memUsageTotal.push_back(  "-");
      }

      // add empty line
      databaseName.push_back(   "");
      dimensionName.push_back(  "");
      cubeName.push_back(       "");
      memUsageIndex.push_back(  "");
      memUsageStorage.push_back("");
      memUsageTotal.push_back(  "");
      
    }

    databaseName.push_back(   "");
    dimensionName.push_back(  "");
    cubeName.push_back(       "");
    memUsageIndex.push_back(  "");
    memUsageStorage.push_back("");
    memUsageTotal.push_back(  "&Sigma;&nbsp;" + StringUtils::convertToString((uint64_t) serverTotal));
      
    values["@database_name"]  = databaseName;
    values["@dimension_name"] = dimensionName;
    values["@cube_name"]      = cubeName;
    values["@memory_index"]   = memUsageIndex;
    values["@memory_storage"] = memUsageStorage;
    values["@memory_total"]   = memUsageTotal;
  }
}
