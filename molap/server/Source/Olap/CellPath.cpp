////////////////////////////////////////////////////////////////////////////////
/// @brief palo cell path
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

#include "Olap/CellPath.h"

#include "Exceptions/ParameterException.h"

#include "Olap/Cube.h"
#include "Olap/Dimension.h"

namespace palo {

  // The CellPath constructors are called quite often, so they have to be as fast as possible.
  // The three constructors are almost identical, but in order to allow optimisation the source
  // code is not shared.

  CellPath::CellPath (Cube* cube, const IdentifiersType * identifiers)
    : cube(cube),
      pathElements(identifiers->size(), 0),
      pathIdentifiers(*identifiers),
      pathType(Element::NUMERIC),
      base(true) {

    const vector<Dimension*> * dimensions = cube->getDimensions();
   
    // the number of dimensions has to match the number of identifiers in the given path
    if (identifiers->size() != dimensions->size()) {
      throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                               "number of dimensions does not match number of identifiers",
                               "path", (int) identifiers->size());
    }
    
    // convert identifiers into elements
    IdentifiersType::const_iterator    identifiersIter  = identifiers->begin();
    vector<Dimension*>::const_iterator dimensionIter    = dimensions->begin();
    PathType::iterator                 pathIter         = pathElements.begin();

    for (;  identifiersIter != identifiers->end();  identifiersIter++, dimensionIter++, pathIter++) { 
      Dimension * dimension = *dimensionIter;
      
      // find element from identifier
      Element * element = dimension->findElement(*identifiersIter, 0);
      
      // copy values to member variables
      *pathIter = element;
      
      // change type of string consolidations to STRING
      Element::ElementType type;

      if (dimension->isStringConsolidation(element)) {
        type = Element::STRING;
        base = false;
      }
      else {
        type = element->getElementType();
      }

      // compute path type
      if (type == Element::CONSOLIDATED) {
        if (pathType != Element::STRING) {
          pathType = Element::CONSOLIDATED;
        }

        base = false;
      }
      else if (type == Element::STRING) {
        pathType = Element::STRING;
      }
    }
  }
  


  CellPath::CellPath (Cube* cube, const IdentifierType * identifiers)
    : cube(cube),
      pathElements(cube->getDimensions()->size(), 0),
      pathIdentifiers(cube->getDimensions()->size(), 0),
      pathType(Element::NUMERIC),
      base(true) {

    const vector<Dimension*> * dimensions = cube->getDimensions();
   
    // convert identifiers into elements
    const IdentifierType *             identifiersIter = identifiers;
    vector<Dimension*>::const_iterator dimensionIter   = dimensions->begin();
    IdentifiersType::iterator          idIter          = pathIdentifiers.begin();
    PathType::iterator                 pathIter        = pathElements.begin();

    for (;  dimensionIter != dimensions->end();  identifiersIter++, dimensionIter++, pathIter++, idIter++) { 
      Dimension * dimension = *dimensionIter;
      
      // find element from identifier
      Element * element = dimension->findElement(*identifiersIter, 0);
      
      // copy values to member variables
      *pathIter = element;
      *idIter   = *identifiersIter;
      
      // change type of string consolidations to STRING
      Element::ElementType type;

      if (dimension->isStringConsolidation(element)) {
        type = Element::STRING;
        base = false;
      }
      else {
        type = element->getElementType();
      }

      // compute path type
      if (type == Element::CONSOLIDATED) {
        if (pathType != Element::STRING) {
          pathType = Element::CONSOLIDATED;
        }

        base = false;
      }
      else if (type == Element::STRING) {
        pathType = Element::STRING;
      }
    }
  }
  


  CellPath::CellPath (Cube* cube, const PathType * elements)
    : cube(cube),
      pathElements(*elements),
      pathIdentifiers(elements->size(), 0),
      pathType(Element::NUMERIC),
      base(true) {

    const vector<Dimension*> * dimensions = cube->getDimensions();
   
    // the number of dimensions has to match the number of identifiers in the given path
    if (elements->size() != dimensions->size()) {
      throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                               "number of dimensions does not match number of identifiers",
                               "path", (int) elements->size());
    }
    
    // convert identifiers into elements
    PathType::const_iterator           elementsIter    = elements->begin();
    vector<Dimension*>::const_iterator dimensionIter   = dimensions->begin();
    IdentifiersType::iterator          identifiersIter = pathIdentifiers.begin();

    for (;  elementsIter != elements->end();  elementsIter++, dimensionIter++, identifiersIter++) { 
      Dimension * dimension = *dimensionIter;
      Element * element     = *elementsIter;
      
      // copy values to member variables
      *identifiersIter = element->getIdentifier();
      
      // change type of string consolidations to STRING
      Element::ElementType type;

      if (dimension->isStringConsolidation(element)) {
        type = Element::STRING;
        base = false;
      }
      else {
        type = element->getElementType();
      }

      // compute path type
      if (type == Element::CONSOLIDATED) {
        if (pathType != Element::STRING) {
          pathType = Element::CONSOLIDATED;
        }

        base = false;
      }
      else if (type == Element::STRING) {
        pathType = Element::STRING;
      }
    }
  }
}
