////////////////////////////////////////////////////////////////////////////////
/// @brief palo configuration cube
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

#include "Olap/ConfigurationCube.h"
#include "Olap/Lock.h"
#include "Olap/PaloSession.h"
#include "Olap/Server.h"

namespace palo {
    
  ConfigurationCube::ConfigurationCube (IdentifierType identifier,
                const string& name,
                Database* database,
                vector<Dimension*>* dimensions)
      : SystemCube (identifier, name, database, dimensions) {

      if (CubeWorker::useCubeWorker()) {
        Scheduler* scheduler = Scheduler::getScheduler();
        PaloSession* session = PaloSession::createSession(database->getServer(), 0, true, 0);        
        cubeWorker = new CubeWorker(scheduler, session->getEncodedIdentifier(), database, this);
        scheduler->registerWorker(cubeWorker);
      }
    }
    
  void ConfigurationCube::saveCubeType (FileWriter* file) {
    file->appendIdentifier(identifier);
    file->appendEscapeString(name);

    IdentifiersType identifiers;

    for (vector<Dimension*>::const_iterator i = dimensions.begin(); i != dimensions.end(); i++) {
      identifiers.push_back((*i)->getIdentifier());
    }

    file->appendIdentifiers(&identifiers); 
    file->appendInteger(CUBE_TYPE);
    file->appendBool(isDeletable());
    file->appendBool(isRenamable());
      
    file->nextLine();
  }
  
  
  semaphore_t ConfigurationCube::setCellValue (CellPath* cellPath, const string& value, User* user, PaloSession * session, bool checkArea, bool sepRight, Lock * lock) {

    string newValue = value;
    bool updateCacheType = false;
	bool updateHideElements = false;

    const vector<Element*>* elements = cellPath->getPathElements();    
    if (elements->at(0)->getName() == SystemDatabase::NAME_CLIENT_CACHE_ELEMENT)
		updateCacheType = true;
	else if (elements->at(0)->getName() == SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT) 
		updateHideElements = true;

	if (updateCacheType || updateHideElements)
	{
        size_t l = value.length();
        
        if (l != 1) {
          throw ParameterException(ErrorException::ERROR_INVALID_PERMISSION,
                                   "value not allowed here",
                                   "value", value);
        }        
        
        string okStrings = "NYE";

        char valueChar = ::toupper(value[0]);
        newValue = string(1, valueChar);

        if (newValue.find_first_not_of(okStrings) != string::npos) {
          throw ParameterException(ErrorException::ERROR_INVALID_PERMISSION,
                                   "value not allowed here",
                                   "value", value);
        }
        
    }

    semaphore_t semaphore;
    if (updateCacheType) {
      semaphore = Cube::setCellValue(cellPath, newValue, user, session, false, false, 0);
      updateDatabaseClientCacheType(cellPath);
    } else if (updateHideElements) {
      semaphore = Cube::setCellValue(cellPath, newValue, user, session, false, false, 0);
      updateDatabaseHideElements(cellPath);
    } else {
      semaphore = Cube::setCellValue(cellPath, newValue, user, session, checkArea, sepRight, 0);
    }
    return semaphore;
  }


  semaphore_t ConfigurationCube::clearCellValue (CellPath* cellPath, 
                                                 User* user, 
                                                 PaloSession * session, 
                                                 bool checkArea, 
                                                 bool sepRight,
                                                 Lock * lock) {

    const vector<Element*>* elements = cellPath->getPathElements();    
    if (elements->at(0)->getName() == SystemDatabase::NAME_CLIENT_CACHE_ELEMENT) {
      throw ParameterException(ErrorException::ERROR_INVALID_PERMISSION,
                                   "cannot clear client cache cell",
                                   "value", SystemDatabase::NAME_CLIENT_CACHE_ELEMENT);
	} else if (elements->at(0)->getName() == SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT) {
      throw ParameterException(ErrorException::ERROR_INVALID_PERMISSION,
                                   "cannot clear hide elements cell",
                                   "value", SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT);
    }

    return Cube::clearCellValue(cellPath, user, session, checkArea, sepRight, lock);
  }


  void ConfigurationCube::clearCells (User* user) {
    vector<Element*> path(1);
    vector<Element*> elements = dimensions.at(0)->getElements(user);
    for (vector<Element*>::iterator i = elements.begin(); i != elements.end(); i++) {
      
      // do not clear client cache cell
      if ( (*i)->getName() != SystemDatabase::NAME_CLIENT_CACHE_ELEMENT && (*i)->getName() != SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT ) {
        path.at(0) = *i;
        CellPath cp(this, &path);
        Cube::clearCellValue(&cp, user, 0, false, false, 0);
      }
    }
  }


  void ConfigurationCube::clearCells (vector<IdentifiersType> * baseElements,
                                      User * user) {
    throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                             "configuration cube cannot be cleared",
                             "user", (int) user->getIdentifier());
  }


  void ConfigurationCube::updateDatabaseClientCacheType () {
    Element * e =  dimensions.at(0)->findElementByName(SystemDatabase::NAME_CLIENT_CACHE_ELEMENT, 0);      
    vector<Element*> path;
    path.push_back(e);
    CellPath cp(this, &path);
    updateDatabaseClientCacheType(&cp);
  }

  void ConfigurationCube::updateDatabaseClientCacheType (CellPath *path) {
    
    bool found;
    set< pair<Rule*, IdentifiersType> > ruleHistory;    
    Cube::CellValueType value = getCellValue(path, &found, 0, 0, &ruleHistory);
    
    if (found && value.type == Element::STRING && value.charValue.length() == 1) {
      char c = value.charValue[0];
      switch (c) {
        case 'N' :
          database->setClientCacheType(ConfigurationCube::NO_CACHE);
          break;
        case 'Y' :
          database->setClientCacheType(ConfigurationCube::NO_CACHE_WITH_RULES);
          break;
        case 'E' :
          database->setClientCacheType(ConfigurationCube::CACHE_ALL);
          break;
        default :
          database->setClientCacheType(ConfigurationCube::NO_CACHE);
      }
    }
    else {
      Logger::info << "setting client cache to default = 'N'" << endl; 
      semaphore_t semaphore = Cube::setCellValue(path, "N", 0, 0, false, false, 0);
      saveCube();
      database->setClientCacheType(ConfigurationCube::NO_CACHE);
    }
    
  }

  void ConfigurationCube::checkDatabaseHideElementsElement() {
	  try {
		//add HideElement element if neccessary
		if (dimensions.at(0)->lookupElementByName(SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT)==0)
			dimensions.at(0)->addElement(SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT, Element::STRING, 0); 
	  } catch(...) {

	  }
  }

  void ConfigurationCube::updateDatabaseHideElements () {   
    
	//make sure element hideElements exists
	checkDatabaseHideElementsElement();

    Element * e =  dimensions.at(0)->findElementByName(SystemDatabase::NAME_HIDE_ELEMENTS_ELEMENT, 0);      
    vector<Element*> path;
    path.push_back(e);
    CellPath cp(this, &path);
    updateDatabaseHideElements(&cp);
  }

  void ConfigurationCube::updateDatabaseHideElements (CellPath *path) {
    
    bool found;
    set< pair<Rule*, IdentifiersType> > ruleHistory;    
    Cube::CellValueType value = getCellValue(path, &found, 0, 0, &ruleHistory);
    
    if (found && value.type == Element::STRING && value.charValue.length() == 1) {
      char c = value.charValue[0];
      switch (c) {        
        case 'Y' :
          database->setHideElements(true);
          break;    
		case 'N':
		  database->setHideElements(false);
          break;    
        default :
          database->setHideElements(false);
      }
    }
    else {
      Logger::info << "setting hide elements to default = 'N'" << endl; 
      semaphore_t semaphore = Cube::setCellValue(path, "N", 0, 0, false, false, 0);
      saveCube();
      database->setHideElements(false);
    }
    
  }
  
}
