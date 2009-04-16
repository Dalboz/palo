////////////////////////////////////////////////////////////////////////////////
/// @brief palo normal dimension
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

#include "Olap/NormalDimension.h"

#include "InputOutput/FileWriter.h"

#include "Olap/AttributesCube.h"
#include "Olap/AttributesDimension.h"
#include "Olap/NormalDatabase.h"
#include "Olap/RightsCube.h"
#include "Olap/SystemDatabase.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // notification callbacks
  ////////////////////////////////////////////////////////////////////////////////

  void NormalDimension::notifyAddDimension () {

    // create attribute dimension and cube
    AttributedDimension::notifyAddDimension(database, name, this);

    if (!this->rightsCube) {
      // create dimension rights cube     
      const string cubeName     = SystemCube::PREFIX_GROUP_DIMENSION_DATA + name;
      Dimension* groupDimension = database->findDimensionByName(SystemDatabase::NAME_GROUP_DIMENSION, 0);    
      vector<Dimension*> dimensions;
      dimensions.push_back(groupDimension);
      dimensions.push_back(this);
  
      IdentifierType cubeIdentifier = database->fetchCubeIdentifier();
      RightsCube* cube = new RightsCube(cubeIdentifier, cubeName, database, &dimensions);
      cube->setDeletable(false);
      cube->setRenamable(false);
  
      try {
        // and add cube to database
        database->addCube(cube, false);
      }
      catch (...) {
        attributesCube->setDeletable(true);
        database->deleteCube(attributesCube, false);
        attributesDimension->setDeletable(true);
        database->deleteDimension(attributesDimension, false);
        delete cube;
        throw;
      }
  
      this->rightsCube = cube;
    }
    
    NormalDatabase * db = dynamic_cast<NormalDatabase*>(database);
    if (db) {
      // add dimension to list of dimensions in #_DIMENSION_
      Dimension * dimensionDimension = db->getDimensionDimension();
      if (dimensionDimension) {
        dimensionDimension->addElement(name, Element::STRING, 0, false);
      }
    }    
  }



  void NormalDimension::beforeRemoveDimension () {
    // note: delete all system cubes using this dimension 

    // remove attribute dimension and cube
    AttributedDimension::beforeRemoveDimension(database, name);
    
    // delete dimension rights cube
    deleteAssociatedCubeByName(database, SystemCube::PREFIX_GROUP_DIMENSION_DATA + name);

    NormalDatabase * db = dynamic_cast<NormalDatabase*>(database);
    if (db) {
      // delete dimension from list of dimensions in #_DIMENSION_
      Dimension * dimensionDimension = db->getDimensionDimension();
      if (dimensionDimension) {
        Element * e = dimensionDimension->lookupElementByName(name);
        if (e) {
          dimensionDimension->deleteElement(e, 0, false);
        }
      }
    }    
  }



  void NormalDimension::notifyRenameDimension (const string& oldName) {
    // rename attribute dimension and cube
    AttributedDimension::notifyRenameDimension(database, name, oldName);    
        
    // rename dimension rights cube 
    const string oldCubeName = SystemCube::PREFIX_GROUP_DIMENSION_DATA + oldName;
    const string newCubeName = SystemCube::PREFIX_GROUP_DIMENSION_DATA + name;
    renameAssociatedCube(database, oldCubeName, newCubeName);    

    NormalDatabase * db = dynamic_cast<NormalDatabase*>(database);
    if (db) {
      // rename dimension in list of dimensions in #_DIMENSION_
      Dimension * dimensionDimension = db->getDimensionDimension();
      if (dimensionDimension) {
        Element * e = dimensionDimension->lookupElementByName(oldName);
        if (e) {
          dimensionDimension->changeElementName(e, name, 0);
        }
      }
    }    
  }


  AttributesDimension* NormalDimension::getAttributesDimension () {    
    return AttributedDimension::getAttributesDimension(database, name);    
  }
    

  AttributesCube* NormalDimension::getAttributesCube () {
    return AttributedDimension::getAttributesCube(database, name);
  }


  RightsCube* NormalDimension::getRightsCube () {
    if (rightsCube) {
      return rightsCube;
    }

    const string str = SystemCube::PREFIX_GROUP_DIMENSION_DATA + name;
    Cube* c = database->lookupCubeByName(str);
    if (c && c->getType()==SYSTEM) {
      SystemCube* sc = dynamic_cast<SystemCube*>(c);
      if (sc->getSubType() == RightsCube::RIGHTS_CUBE) {
        rightsCube = dynamic_cast<RightsCube*>(sc); 
        return rightsCube;
      }
    }
    return 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void NormalDimension::saveDimensionType (FileWriter* file) {
    file->appendIdentifier(identifier);
    file->appendEscapeString(name);
    file->appendInteger(DIMENSION_TYPE);
    file->appendInteger(deletable ? 1 : 0);
    file->appendInteger(renamable ? 1 : 0);
    file->appendInteger(changable ? 1 : 0);
    file->nextLine();
  }

  vector<Element*> NormalDimension::getElements(User* user) {    
		vector<Element*> allElements = BasicDimension::getElements(user);
		if (user) {
			//hide elements wich user should not see
			bool hide_elements = database->getHideElements();

			if (hide_elements) {
				vector<Element*> result; result.reserve(allElements.size());
				for (size_t i = 0; i<allElements.size(); i++)
					if (user->getDimensionDataRight(database, this, allElements[i])>RIGHT_NONE)
						result.push_back(allElements[i]);
				return result;
			}
		}
		return allElements;	
	}
	
	Element* NormalDimension::findElement (IdentifierType elementIdentifier, User* user) {
		Element* e = BasicDimension::findElement(elementIdentifier, user);
		if (user && database->getHideElements() && !(user->getDimensionDataRight(database, this, e)>RIGHT_NONE))
			throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
			"insufficient access rights for element",
			"user", (int) user->getIdentifier());
		return e;
	}
	Element * NormalDimension::findElementByName (const string& name, User* user) {
		Element* e = BasicDimension::findElementByName(name, user);
		if (user && database->getHideElements() && !(user->getDimensionDataRight(database, this, e)>RIGHT_NONE))
			throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
			"insufficient access rights for element",
			"user", (int) user->getIdentifier());
		return e;
	}
	Element * NormalDimension::findElementByPosition (PositionType position, User* user){
		Element* e = BasicDimension::findElementByPosition(position, user);
		if (user && database->getHideElements() && !(user->getDimensionDataRight(database, this, e)>RIGHT_NONE))
			throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
			"insufficient access rights for element",
			"user", (int) user->getIdentifier());
		return e;
	}

	bool NormalDimension::hasStringAttributes() { 
		const std::vector<Element*> elements = getAttributesDimension()->getElements(0);
		for (std::vector<Element*>::const_iterator e = elements.begin(); e!=elements.end(); e++)
			if ((*e)->getElementType() == Element::STRING)
				return true;
		return false;
	}
}
