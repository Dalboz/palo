////////////////////////////////////////////////////////////////////////////////
/// @brief element bulk create
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
/// Developed by Marko Stijak, Banja Luka on behalf of Jedox AG.
/// Copyright and exclusive worldwide exploitation right has
/// Jedox AG, Freiburg.
///
/// @author Marko Stijak, Banja Luka, Bosnia and Herzegovina
////////////////////////////////////////////////////////////////////////////////

#ifndef HTTP_SERVER_ELEMENT_BULK_CREATE_HANDLER_H
#define HTTP_SERVER_ELEMENT_BULK_CREATE_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "HttpServer/PaloHttpHandler.h"

#include "Exceptions/ParameterException.h"

namespace palo {

	////////////////////////////////////////////////////////////////////////////////
	/// @brief element create bulk
	////////////////////////////////////////////////////////////////////////////////

	class SERVER_CLASS ElementCreateBulkHandler : public PaloHttpHandler {
	public:
		ElementCreateBulkHandler (Server * server) 
			: PaloHttpHandler(server) {
		}

	public:
		//
		HttpResponse * handlePaloRequest (const HttpRequest * request) {

			//example url
			///element/create_bulk?sid=NvUl&database=3&dimension=11&name_elements=abc,def&type=4&name_children=a,b,c:d,e,f,r&weights=1,1,2:1,2,1

			Database* database   = findDatabase(request, 0);
			Dimension* dimension = findDimension(database, request, 0);

			checkToken(dimension, request);

			const string& nameElementsString = request->getValue(NAME_ELEMENTS);
			const string& typeString   = request->getValue(ID_TYPE);      

			Element::ElementType elementType = getElementTypeByIdentifier(typeString);

			if (elementType == Element::UNDEFINED) {
				throw ParameterException(ErrorException::ERROR_INVALID_ELEMENT_TYPE,
					"wrong value for element type",
					ID_TYPE, typeString);
			}

			vector<string> elementName;

					
			string elementsBuffer = nameElementsString;

			for (size_t pos = 0; pos < elementsBuffer.size(); ) { 
				string name = StringUtils::getNextElement(elementsBuffer, pos, ',', true);		
				elementName.push_back(name);
			}

			vector<ElementsWeightType> children;
			bool useDefaultWeights = false;


			if (elementType == Element::CONSOLIDATED) {
				const string& childrenIds  = request->getValue(ID_CHILDREN);
				string weightString = request->getValue(WEIGHTS);
				bool useChildrenIds = ! childrenIds.empty();
				string childrenString = useChildrenIds ? childrenIds : request->getValue(NAME_CHILDREN);

				vector<vector<string> > elementChildrenIdStrings;
				vector<vector<string> > elementChildrenWeightStrings;
				
				
				if (!StringUtils::splitString2Sep(childrenString, &elementChildrenIdStrings, ':', ',', !useChildrenIds))
					throw ParameterException(ErrorException::ERROR_PARAMETER_MISSING,
						"invalid children string",
						ID_CHILDREN, childrenString);

				if (elementChildrenIdStrings.size()<elementName.size())
					throw ParameterException(ErrorException::ERROR_PARAMETER_MISSING,
						"missing children string",
						ID_CHILDREN, childrenString);
				

				if (!StringUtils::splitString2Sep(weightString, &elementChildrenWeightStrings, ':', ',', false))
					throw ParameterException(ErrorException::ERROR_PARAMETER_MISSING,
						"invalid weights string",
						WEIGHTS, weightString);	

				if (elementChildrenWeightStrings.size()>0 && elementChildrenWeightStrings.size()<elementName.size())
					throw ParameterException(ErrorException::ERROR_PARAMETER_MISSING,
						"missing weights string",
						WEIGHTS, weightString);

				useDefaultWeights = elementChildrenWeightStrings.size()==0;

				for (size_t i = 0; i<elementName.size(); i++) {

					ElementsWeightType elementChildren;

					for (size_t j = 0; j<elementChildrenIdStrings[i].size(); j++)
					{
						const string& idString = elementChildrenIdStrings[i][j];

						Element * child = useChildrenIds 
							? dimension->findElement((IdentifierType) StringUtils::stringToInteger(idString), user)
							: dimension->findElementByName(idString, user);

						double weight;

						// read weight from parameter
						if (!useDefaultWeights) {
							if (j >= elementChildrenWeightStrings[i].size()) {
								throw ParameterException(ErrorException::ERROR_PARAMETER_MISSING,
									"missing weight",
									WEIGHTS, weightString);
							}
							const string& weightStr = elementChildrenWeightStrings[i][j];
							weight = StringUtils::stringToDouble(weightStr);
						}
						// default: use weight = 1
						else {
							weight = 1;
						}

						elementChildren.push_back(pair<Element*, double>(child, weight));
					}

					children.push_back(elementChildren);
				}
			}

			//everything seems ok, now create elements
			vector<Element*> createdElement;
			
			try {		
				createdElement.reserve(elementName.size());

				for (size_t i = 0; i<elementName.size(); ++i) {
					Element* element = dimension->addElement(elementName[i], elementType, user);
					if (elementType==Element::CONSOLIDATED)
						dimension->addChildren(element, &children[i], user);
					createdElement.push_back(element);
				}
			}
			catch(...) {
				for (size_t i = 0; i<createdElement.size(); i++)
					dimension->deleteElement(createdElement[i], 0);				
				throw;
			}
			return generateOkResponse(dimension);
		}
	};

}

#endif
