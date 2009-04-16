////////////////////////////////////////////////////////////////////////////////
/// @brief element create
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

#ifndef HTTP_SERVER_ELEMENT_CREATE_HANDLER_H
#define HTTP_SERVER_ELEMENT_CREATE_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "HttpServer/PaloHttpHandler.h"

#include "Exceptions/ParameterException.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief element create
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ElementCreateHandler : public PaloHttpHandler {
  public:
    ElementCreateHandler (Server * server, bool isReplaceHandler) 
      : PaloHttpHandler(server), isReplaceHandler(isReplaceHandler) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      bool hasChildren = false;
      bool isCreated   = false;
      
      Database* database   = findDatabase(request, 0);
      Dimension* dimension = findDimension(database, request, 0);
      
      checkToken(dimension, request);

      const string& name         = request->getValue(NEW_NAME);
      const string& typeString   = request->getValue(ID_TYPE);
      const string& childrenIds  = request->getValue(ID_CHILDREN);
      const string& weightString = request->getValue(WEIGHTS);

      bool useChildrenIds = ! childrenIds.empty();
      const string& childrenString = useChildrenIds ? childrenIds : request->getValue(NAME_CHILDREN);

      Element::ElementType elementType = getElementTypeByIdentifier(typeString);

      if (elementType == Element::UNDEFINED) {
         throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
                                  "wrong value for element type",
                                  ID_TYPE, typeString);
      }
      
      // construct element
      Element *      element;

      if (isReplaceHandler) {
        try {
          element = findElement(dimension, request, 0);
      
          if (element->getElementType() != elementType) {
            dimension->changeElementType(element, elementType, user, false);
          }
        }
        catch (ParameterException e) {
      
          // create element
          element = dimension->addElement(name, elementType, user);
          isCreated = true;
        }
      }
      else {   

        // create element
        element = dimension->addElement(name, elementType, user);
        isCreated = true;
      }
      
      // add children
      if (elementType == Element::CONSOLIDATED) {
        ElementsWeightType children;
		set<Element*> childrenSet;
      
        try {
          bool hasWeight = false;
    
          // parse children
          if (weightString != "") {
            hasWeight = true;
          }
              
          string bufferChildren = childrenString; 
          string bufferWeight   = weightString;
              
          for (size_t pos1 = 0, pos2 = 0;  pos1 < bufferChildren.size(); ) { 
            string idString = StringUtils::getNextElement(bufferChildren, pos1, ',');

            Element * child = useChildrenIds 
              ? dimension->findElement((IdentifierType) StringUtils::stringToInteger(idString), user)
              : dimension->findElementByName(idString, user);
                
            double weight;
    
            // read weight from parameter
            if (hasWeight) {
              if (pos2 >= bufferWeight.size()) {
                throw ParameterException(ErrorException::ERROR_PARAMETER_MISSING,
                                         "missing weight",
                                         WEIGHTS, weightString);
              }
    
              string weightStr = StringUtils::getNextElement(bufferWeight, pos2, ',');
              weight = StringUtils::stringToDouble(weightStr);
            }

            // default: use weight = 1
            else {
              weight = 1;
            }
            
            children.push_back(pair<Element*, double>(child, weight));
			childrenSet.insert(child);

            hasChildren = true;
          }       
        }
        catch (...) {
    
          // error found, we have to delete the new element
          if (isCreated) {
            dimension->deleteElement(element, 0);
          }
    
          throw;
        }


        const ElementsWeightType oldChildren = *(dimension->getChildren(element));
      
        if (! isCreated) {

          // we cannot remove all children, because this might change the parent order
          // therefore remove only unknown childs
          dimension->removeChildrenNotIn(user, element, &childrenSet);
        }
        
        if (hasChildren) {
          try {
          dimension->addChildren(element, &children, user);
        }     
          catch (...) {
            dimension->addChildren(element, &oldChildren, 0);
            throw;
      } 
        }     
      } 
      
      return generateElementResponse(dimension, element);
    }
    
  private:
    bool isReplaceHandler;
  };

}

#endif
