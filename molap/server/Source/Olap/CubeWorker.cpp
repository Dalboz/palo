////////////////////////////////////////////////////////////////////////////////
/// @brief cube worker
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

#include "Olap/CubeWorker.h"

#include "Collections/StringBuffer.h"
#include "Collections/StringUtils.h"

#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"

#include "Olap/Cube.h"
#include "Olap/Database.h"
#include "Olap/Server.h"

namespace palo {
  bool CubeWorker::useWorkers = false;


  
  void CubeWorker::setUseCubeWorker (bool use) {
    CubeWorker::useWorkers = use;
  }  


  
  bool CubeWorker::useCubeWorker () {
    return CubeWorker::useWorkers;
  }


  
  CubeWorker::CubeWorker (Scheduler* scheduler, const string& sessionString, Database* database, Cube* cube) 
    : Worker(scheduler, sessionString), database(database), cube(cube){
    waitForAreasSemaphore = (semaphore_t) -1;
  }



  bool CubeWorker::start () {
    if (waitForShutdownTimeout || scheduler->shutdownInProgress()) {
      return false;
    }    
    
    Logger::trace << "starting worker for cube '" << cube->getName() << "'" << endl;          

    bool ok = Worker::start();
    if (ok) {
      executeCube();
    }
    return ok;
  }



  void CubeWorker::executeCube () {
    IdentifierType databaseId = database->getIdentifier();
    IdentifierType cubeId = cube->getIdentifier();    
    semaphore_t semaphore = scheduler->createSemaphore();
    semaphores.insert(semaphore);    
    waitForAreasSemaphore = semaphore;
    execute(semaphore, "CUBE;" + StringUtils::convertToString(databaseId) + ";" + StringUtils::convertToString(cubeId));
  }
  
  

  semaphore_t CubeWorker::executeSetCell (const string& areaIdentifier, const string& sessionIdentifier, const IdentifiersType* path, double value) {
    IdentifierType databaseId = database->getIdentifier();
    IdentifierType cubeId = cube->getIdentifier();
    semaphore_t semaphore = scheduler->createSemaphore();
    semaphores.insert(semaphore);
    
    StringBuffer sb;
    sb.initialize();

    sb.appendText("DOUBLE;");
    sb.appendInteger(databaseId);
    sb.appendChar(';');
    sb.appendInteger(cubeId);
    sb.appendChar(';');
    sb.appendText(StringUtils::escapeString(areaIdentifier));
    sb.appendChar(';');
    sb.appendText(StringUtils::escapeString(sessionIdentifier));
    sb.appendChar(';');
    sb.appendText(buildPathString(path));
    sb.appendChar(';');
    sb.appendDecimal(value);
    execute(semaphore, sb.c_str());
      
    sb.free();
      
    return semaphore;
  }
  
    

  semaphore_t CubeWorker::executeSetCell (const string& areaIdentifier, const string& sessionIdentifier, const IdentifiersType* path, const string& value) {
    IdentifierType databaseId = database->getIdentifier();
    IdentifierType cubeId = cube->getIdentifier();
    semaphore_t semaphore = scheduler->createSemaphore();
    semaphores.insert(semaphore);
    execute(semaphore, "STRING;"  
      + StringUtils::convertToString(databaseId) + ";" 
      + StringUtils::convertToString(cubeId) + ";" 
      + StringUtils::escapeString(areaIdentifier) + ";"
      + StringUtils::escapeString(sessionIdentifier) + ";"
      + buildPathString(path) + ";" 
      + StringUtils::escapeString(value));
    return semaphore;
  }
  
  

  semaphore_t CubeWorker::executeAreaBuildOk () {
    semaphore_t semaphore = scheduler->createSemaphore();
    semaphores.insert(semaphore);
    execute(semaphore, "AREA-BUILD;OK");
    return semaphore;
  }
  

  
  semaphore_t CubeWorker::executeAreaBuildError (const string& id1, const string& id2) {
    semaphore_t semaphore = scheduler->createSemaphore();
    semaphores.insert(semaphore);
    execute(semaphore, "AREA-BUILD;ERROR;"  
      + StringUtils::escapeString(id1) + ";"
      + StringUtils::escapeString(id2));
    return semaphore;
  }



  semaphore_t CubeWorker::executeShutdown () {
    semaphore_t semaphore = scheduler->createSemaphore();
    semaphores.insert(semaphore);
    execute(semaphore, "TERMINATE");
    waitForShutdownTimeout = true;
    scheduler->registerTimoutWorker(this);
    return semaphore;
  }



  string CubeWorker::buildPathString (const IdentifiersType* path) {
    bool first = true;
    string pathString;
    for (IdentifiersType::const_iterator i = path->begin(); i != path->end(); i++) {
      if (first) {
        first = false;
      } 
      else {
        pathString += ",";
      }
      pathString += StringUtils::convertToString(*i);
    }
    return pathString;
  }
  


  bool CubeWorker::computeArea (vector<string>& answer, string& areadId, vector< IdentifiersType >* area) {
    // has no area
    // AREA;<id_database>;<id_cube>
    // has area
    // AREA;<id_database>;<id_cube>;<id_area>;<list_elements>;<list_elements>;...;<list_elements>
    
    if (answer.size() < 4) {
      return false;
    }

    // we start one worker for each normal cube
    // so we can check the cube and databse identifier here
    IdentifierType databaseId = StringUtils::stringToInteger(answer[2]);
    if (databaseId != database->getIdentifier()) {
      Logger::error << "wrong database id '" << databaseId << "' in worker response" << endl;
      return false;
    }
    IdentifierType cubeId = StringUtils::stringToInteger(answer[3]); 
    if (cubeId != cube->getIdentifier()) {
      Logger::error << "wrong cube id '" << cubeId << "' in worker response" << endl;
      return false;
    }
    
    // get area identifier and dimension elements    
    if (answer.size() > 4) {
      areadId = answer[1];

      for (int i = 4; i < (int) answer.size(); i++) {        
        if (answer[i] == "*") {
          // use empty vector for all elements          
          vector<IdentifierType> ids;
          area->push_back(ids);
        }
        else {          
          vector<string> idStrings;
          StringUtils::splitString(answer[i], &idStrings, ',');
        
          vector<IdentifierType> ids;
          for (vector<string>::iterator i = idStrings.begin(); i != idStrings.end(); i++) {
            ids.push_back(StringUtils::stringToInteger(*i));
          }
          area->push_back(ids);
        }
      }
    }

    return true;
  }



  bool CubeWorker::readAreaLines (const vector<string>& result, vector<string>* areaIds, vector< vector< IdentifiersType > >* areas) {
    for (vector<string>::const_iterator i = result.begin(); i != result.end(); i++) {

      vector<string> values;
      StringUtils::splitString(*i, &values, ';');          
      // check for AREA string    
      if (values[0] == "AREA" || values.size()<6) {
        string areaId;
        vector< IdentifiersType > area;

        try {
          bool ok = computeArea(values, areaId, &area); 
          if (ok) {
            if (area.size() > 0) {
              areaIds->push_back(areaId);
              areas->push_back(area);
            }
          }
          else {
            Logger::error << "error in worker response AREA: '" << *i << "'" << endl;
            return false;
          }
        }
        catch (...) {
          Logger::error << "error in worker response AREA: '" << *i << "'" << endl;
          return false;
        }
      }
      else {
        Logger::error << "error in worker response: '" << *i << "'" << endl;
        return false;
      }     

    }
    
    if (areaIds->size()>0) {
      return true;
    }
    
    return false;
  }    



  bool CubeWorker::isOverlapping (vector< IdentifiersType >* area1,  vector< IdentifiersType >* area2) {
    vector< IdentifiersType >::iterator a1 = area1->begin();
    vector< IdentifiersType >::iterator a2 = area2->begin();
    
    for (; a1 != area1->end(); a1++, a2++) {
      if (a1->size() > 0 && a2->size() > 0) {      
        sort( a1->begin(), a1->end());
        sort( a2->begin(), a2->end());

        vector<IdentifierType> result;
        insert_iterator<vector<IdentifierType> > resIns(result, result.begin());
                         
        set_intersection( a1->begin(), a1->end(), a2->begin(), a2->end(), resIns);
        
        if (result.size()==0) {
          // result is empty
          // no overlapping possible
          return false;
        }
        
      }  
    }        
    return true;
  }



  bool CubeWorker::checkAreas (vector<string>* areaIds, vector< vector< IdentifiersType > >* areas) {    
    const vector< Dimension* >* dimensions = cube->getDimensions();
    size_t numDimension = dimensions->size();
    
    size_t num = areas->size();
    
    for (size_t i = 0; i < num; i++) {
      vector< IdentifiersType > area = areas->at(i);

      if (areaIds->at(i) == "") {
        Logger::error << "name of area is empty" << endl;
        // TODO: send error message to worker
        return false;
      }
      
      if (numDimension != area.size()) {
        Logger::error << "error in size of dimension elements in AREA: '" << areaIds->at(i) << "'" << endl;
        // TODO: send error message to worker
        return false;
      }
    }

    // check for overlapping areas
    for (size_t i = 0; i < num-1; i++) {
      for (size_t j = i+1; j < num; j++) {
        bool overlaps = isOverlapping( &(areas->at(i)), &(areas->at(j)));
        if (overlaps) {
          Logger::error << "error overlapping area '" << areaIds->at(i) << "' and '" << areaIds->at(j) << "'" << endl;
          executeAreaBuildError(areaIds->at(i), areaIds->at(j));
          return false;
        }
      }      
    }   
    
    return true; 
  }


  
  void CubeWorker::executeFinished (const vector<string>& result) {    
    vector<string> areaIds;
    vector< vector< IdentifiersType > > areas;        
    bool hasAreasResponse = false;
    bool sendAreaOk = false;

    // delete semaphores of the worker message
    bool deleteSemaphore = true;
    
    string* msg = 0;
    
    if (waitForSessionDone) {
      // ignore the result of the session message
      // and do not delete the semaphore  
      deleteSemaphore = false;
    }
    else {
      if (result.size() > 0) {
        if (result.at(0).substr(0,6) == "ERROR;") {
          msg = new string(result.at(0).substr(6));
        }
        else {               
          hasAreasResponse = readAreaLines(result, &areaIds, &areas);
        }        
      }
    }
    
    semaphore_t executedSemaphore = this->semaphore;
    
    if (status != WORKER_WORKING) {
      Logger::error << "Worker::executeFinished called, but worker not working" << endl;
      status = WORKER_NOT_RUNNING;
    }
    else {
      this->result = result;
      resultStatus = RESULT_OK;
      status = WORKER_RUNNING;
      workDone(msg);
    }
    
    if (msg) {
      delete msg;
    }

    // delete semaphore of the worker message
    if (deleteSemaphore) {
      if (semaphores.size() == 0) {
        Logger::error << "semaphores.size()==0" << endl;
      }
      else {
        if (executedSemaphore == waitForAreasSemaphore) {
          waitForAreasSemaphore = (semaphore_t) -1;
          sendAreaOk = true;
        }
        
        semaphores.erase(executedSemaphore);
        scheduler->deleteSemaphore(executedSemaphore);
      }
    }

    if (hasAreasResponse) {
      waitForAreasSemaphore = (semaphore_t) -1;
      bool ok = checkAreas(&areaIds, &areas);

      if (ok) {
        Logger::info << "got area for cube '" << cube->getName() << "'" << endl;
        for (vector<string>::const_iterator i = result.begin(); i != result.end(); i++) {
          Logger::info <<  *i << endl;
        }             

        cube->setWorkerAreas(&areaIds, &areas);
        sendAreaOk = true;
      }
      else {
        sendAreaOk = false;
      }
    }
    
    if (sendAreaOk) {
      
      if (hasAreasResponse) {
        executeAreaBuildOk();
      }
      else {
        Logger::info << "cube '" << cube->getName() << "' has no worker area. Register worker for shutdown" << endl;
        executeShutdown();
        return;
      }      
    }
  }

  void CubeWorker::timeoutReached () {
    if (waitForShutdownTimeout) {
      cube->removeWorker();
    }
  }

}
