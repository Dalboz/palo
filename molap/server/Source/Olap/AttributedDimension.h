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

#ifndef OLAP_ATTRIBUTED_DIMENSION_H
#define OLAP_ATTRIBUTED_DIMENSION_H 1

#include "palo.h"

#include "Olap/Database.h"
#include "Olap/AttributesDimension.h"
#include "Olap/AttributesCube.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief normal OLAP dimension
  ///
  /// An OLAP dimension is an ordered list of elements
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS AttributedDimension {

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new attributed dimension with given identifier
    ////////////////////////////////////////////////////////////////////////////////

    AttributedDimension () {
      attributesDimension=0;
      attributesCube=0;
    }
    
  public:
    void notifyAddDimension (Database* database, const string& name, Dimension* dimension);

    void beforeRemoveDimension (Database* database, const string& name);

    void notifyRenameDimension (Database* database, const string& newName, const string& oldName);
    
    AttributesDimension* getAttributesDimension (Database* database, const string& name);
    
    AttributesCube* getAttributesCube (Database* database, const string& name);
    

  protected:
    void deleteAssociatedCubeByName (Database* database, const string& name);
  
    void renameAssociatedCube (Database* database, const string& oldName, const string& newName);
    
    AttributesDimension* attributesDimension;
    
    AttributesCube* attributesCube;
  };

}

#endif
