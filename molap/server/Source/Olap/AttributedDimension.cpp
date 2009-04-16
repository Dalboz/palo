////////////////////////////////////////////////////////////////////////////////
/// @brief palo attributed dimension
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

#include "Olap/AttributedDimension.h"

#include "Olap/SystemDatabase.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // notification callbacks
  ////////////////////////////////////////////////////////////////////////////////

  void AttributedDimension::notifyAddDimension (Database* database, const string& name, Dimension* dimension) {

    if (!this->attributesDimension) {
      // create attributes dimension
      const string dimName          = AttributesDimension::PREFIX_ATTRIBUTE_DIMENSION + name 
                                    + AttributesDimension::SUFFIX_ATTRIBUTE_DIMENSION;
      IdentifierType dimIdentifier  = database->fetchDimensionIdentifier();
      AttributesDimension* attributesDimension = new AttributesDimension(dimIdentifier, dimName, database);
      attributesDimension->setDeletable(false);
      attributesDimension->setRenamable(false);
  
      try {
        // and add dimension to database
        database->addDimension(attributesDimension, false);
      }
      catch (...) {
        delete attributesDimension;
        throw;
      }
  
      this->attributesDimension = attributesDimension;
    }
    
    if (!this->attributesCube) {    
      // create attribute cube
      const string cubeName1 = AttributesCube::PREFIX_ATTRIBUTE_CUBE + name;
      vector<Dimension*> dimensions1;
      dimensions1.push_back(attributesDimension);
      dimensions1.push_back(dimension);
  
      IdentifierType cubeIdentifier = database->fetchCubeIdentifier();
      AttributesCube* attributesCube = new AttributesCube(cubeIdentifier, cubeName1, database, &dimensions1);
      attributesCube->setDeletable(false);
      attributesCube->setRenamable(false);
  
      try {
        // and add cube to database
        database->addCube(attributesCube, false);
      }
      catch (...) {
        attributesDimension->setDeletable(true);
        database->deleteDimension(attributesDimension,0);
        delete attributesCube;
        throw;
      }
      
      this->attributesCube = attributesCube;
    }
  }



  void AttributedDimension::beforeRemoveDimension (Database* database, const string& name) {
    // note: delete all system cubes using this dimension 

    // delete attribute cube
    deleteAssociatedCubeByName(database, AttributesCube::PREFIX_ATTRIBUTE_CUBE + name);

    // delete attribute dimension    
    const string dimName = AttributesDimension::PREFIX_ATTRIBUTE_DIMENSION + name + AttributesDimension::SUFFIX_ATTRIBUTE_DIMENSION;
    Dimension* attributesDimension = database->lookupDimensionByName(dimName);

    if (attributesDimension) {    
      attributesDimension->setDeletable(true);
      database->deleteDimension(attributesDimension, false);
    }
    
  }




  void AttributedDimension::deleteAssociatedCubeByName (Database* database, const string& name) {
    Cube* cube = database->lookupCubeByName(name);

    if (cube) {
      cube->setDeletable(true);
      database->deleteCube(cube, 0);
    }
  }



  void AttributedDimension::notifyRenameDimension (Database* database, const string& newName, const string& oldName) {
    // rename attribute cube 
    const string oldCubeName2 = AttributesCube::PREFIX_ATTRIBUTE_CUBE + oldName;
    const string newCubeName2 = AttributesCube::PREFIX_ATTRIBUTE_CUBE + newName;
    renameAssociatedCube(database, oldCubeName2, newCubeName2);

    // rename attribute dimension 
    const string oldDimName = AttributesDimension::PREFIX_ATTRIBUTE_DIMENSION + oldName + AttributesDimension::SUFFIX_ATTRIBUTE_DIMENSION;
    const string newDimName = AttributesDimension::PREFIX_ATTRIBUTE_DIMENSION + newName + AttributesDimension::SUFFIX_ATTRIBUTE_DIMENSION;
    Dimension* dim = database->lookupDimensionByName(oldDimName);

    if (dim) {    
      database->renameDimension(dim, newDimName, false);
    }
    
  }


  void AttributedDimension::renameAssociatedCube (Database* database, const string& oldName, const string& newName) {
    Cube* cube = database->lookupCubeByName(oldName);

    if (cube) {
      database->renameCube(cube, newName, false);
    }
  }



  AttributesDimension* AttributedDimension::getAttributesDimension (Database* database, const string& name) {
    if (attributesDimension) {
      return attributesDimension;
    }
    const string str = AttributesDimension::PREFIX_ATTRIBUTE_DIMENSION + name 
                     + AttributesDimension::SUFFIX_ATTRIBUTE_DIMENSION;
                         
    Dimension* d = database->lookupDimensionByName(str);
    if (d && d->getType() == SYSTEM) {
      SystemDimension* sd = dynamic_cast<SystemDimension*>(d);
      if (sd->getSubType() == AttributesDimension::ATTRIBUTE_DIMENSION) {              
        attributesDimension = dynamic_cast<AttributesDimension*>(sd);
        return attributesDimension;
      }
    }
    return 0;
  }
    

  AttributesCube* AttributedDimension::getAttributesCube (Database* database, const string& name) {
    if (attributesCube) {
      return attributesCube;
    }

    const string str = AttributesCube::PREFIX_ATTRIBUTE_CUBE + name;
    Cube* c = database->lookupCubeByName(str);
    if (c && c->getType()==SYSTEM) {
      SystemCube* sc = dynamic_cast<SystemCube*>(c);
      if (sc->getSubType() == AttributesCube::ATTRIBUTES_CUBE) {
        attributesCube = dynamic_cast<AttributesCube*>(sc); 
        return attributesCube;
      }
    }
    return 0;
  }


}
