////////////////////////////////////////////////////////////////////////////////
/// @brief palo user info dimension
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

#include "Olap/UserInfoDimension.h"

#include "InputOutput/FileWriter.h"

#include "Olap/AttributesCube.h"
#include "Olap/AttributesDimension.h"
#include "Olap/NormalDatabase.h"
#include "Olap/SystemDatabase.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // notification callbacks
  ////////////////////////////////////////////////////////////////////////////////

  void UserInfoDimension::notifyAddDimension () {
    // create attribute dimension and cube
    AttributedDimension::notifyAddDimension(database, name, this);
  }



  void UserInfoDimension::beforeRemoveDimension () {
    // remove attribute dimension and cube
    AttributedDimension::beforeRemoveDimension(database, name);    
  }



  void UserInfoDimension::notifyRenameDimension (const string& oldName) {
    // rename attribute dimension and cube
    AttributedDimension::notifyRenameDimension(database, name, oldName);    
  }


  AttributesDimension* UserInfoDimension::getAttributesDimension () {    
    return AttributedDimension::getAttributesDimension(database, name);    
  }
    

  AttributesCube* UserInfoDimension::getAttributesCube () {
    return AttributedDimension::getAttributesCube(database, name);
  }


  ////////////////////////////////////////////////////////////////////////////////
  // functions to load and save a dimension
  ////////////////////////////////////////////////////////////////////////////////

  void UserInfoDimension::saveDimensionType (FileWriter* file) {
    file->appendIdentifier(identifier);
    file->appendEscapeString(name);
    file->appendInteger(DIMENSION_TYPE);
    file->appendInteger(deletable ? 1 : 0);
    file->appendInteger(renamable ? 1 : 0);
    file->appendInteger(changable ? 1 : 0);
    file->nextLine();
  }
  
}
