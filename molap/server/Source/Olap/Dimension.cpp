////////////////////////////////////////////////////////////////////////////////
/// @brief palo dimension
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

#include "Olap/BasicDimension.h"

#include "Exceptions/FileFormatException.h"

#include "InputOutput/FileReader.h"

#include "Olap/AliasDimension.h"
#include "Olap/AttributesDimension.h"
#include "Olap/ConfigurationDimension.h"
#include "Olap/Cube.h"
#include "Olap/CubeDimension.h"
#include "Olap/Database.h"
#include "Olap/DimensionDimension.h"
#include "Olap/NormalDimension.h"
#include "Olap/RightsDimension.h"
#include "Olap/Server.h"
#include "Olap/SubsetViewDimension.h"
#include "Olap/UserInfoDimension.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  Dimension* Dimension::loadDimensionFromType (FileReader* file,
                                               Database* database,
                                               IdentifierType identifier,
                                               const string& name,
                                               uint32_t type) {
    int isDeletable = file->getDataInteger(3);
    int isRenamable = file->getDataInteger(4);
    int isChangable = file->getDataInteger(5);

    Dimension * dimension;

    switch(type) {
      case AliasDimension::DIMENSION_TYPE: {
        string nameAliasDatabase = file->getDataString(3);
        string nameAliasDimension = file->getDataString(4);
        Database* aliasDatabase = database->getServer()->findDatabaseByName(nameAliasDatabase, 0);
        Dimension* aliasDimension = aliasDatabase->findDimensionByName(nameAliasDimension, 0);

        dimension = new AliasDimension(identifier, name, database, aliasDimension);
        break;
      }

      case NormalDimension::DIMENSION_TYPE:
        dimension = new NormalDimension(identifier, name, database);
        break;

      case RightsDimension::DIMENSION_TYPE:
        if (name == SystemDatabase::NAME_CUBE_DIMENSION) {
          dimension = new CubeDimension(identifier, name, database);
        }
        else {
          dimension = new RightsDimension(identifier, name, database);
        }
        break;
        
      case AttributesDimension::DIMENSION_TYPE:
        dimension = new AttributesDimension(identifier, name, database);
        break;

      case CubeDimension::DIMENSION_TYPE:
        dimension = new CubeDimension(identifier, name, database);
        break;

      case ConfigurationDimension::DIMENSION_TYPE:
        dimension = new ConfigurationDimension(identifier, name, database);
        break;

      case DimensionDimension::DIMENSION_TYPE:
        dimension = new DimensionDimension(identifier, name, database);
        break;

      case SubsetViewDimension::DIMENSION_TYPE:
        dimension = new SubsetViewDimension(identifier, name, database);
        break;

      case UserInfoDimension::DIMENSION_TYPE:
        dimension = new UserInfoDimension(identifier, name, database);
        break;

      default:
          Logger::error << "found unknow dimension type '" << type << "' for dimension '" << name << "'" << endl;
          throw FileFormatException("unknown dimension type", file);
    }

    dimension->setDeletable(isDeletable != 0);
    dimension->setRenamable(isRenamable != 0);
    dimension->setChangable(isChangable != 0);

    return dimension;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // getter and setter
  ////////////////////////////////////////////////////////////////////////////////

  void Dimension::setStatus(DimensionStatus status) {
    this->status = status;

    if (status == CHANGED) {
      database->setStatus(Database::CHANGED);
    }
  }



  vector<Cube*> Dimension::getCubes (User* user) {
    checkCubeAccessRight(user, RIGHT_READ);
    
    vector<Cube*> result;
    vector<Cube*> cubes = database->getCubes(0);

    for (vector<Cube*>::const_iterator c = cubes.begin(); c != cubes.end(); c++) {
      const vector<Dimension*>* dimensions = (*c)->getDimensions();
      vector<Dimension*>::const_iterator d = find(dimensions->begin(), dimensions->end(), this);

      if (d != dimensions->end()) {
        result.push_back(*c);
      }      
    }

    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to update internal structures
  ////////////////////////////////////////////////////////////////////////////////

  void Dimension::updateToken () {
    token++;
    database->updateToken();
  }


  ////////////////////////////////////////////////////////////////////////////////
  // internal function
  ////////////////////////////////////////////////////////////////////////////////

  bool Dimension::isLockedByCube () {
    vector<Cube*> cubes = database->getCubes(0);

    for (vector<Cube*>::const_iterator c = cubes.begin(); c != cubes.end(); c++) {
      const vector<Dimension*>* dimensions = (*c)->getDimensions();
      vector<Dimension*>::const_iterator d = find(dimensions->begin(), dimensions->end(), this);

      if (d != dimensions->end()) {
        if ((*c)->hasLockedArea()) {
          return true;
        }
      }      
    }

    return false;
  }


}

