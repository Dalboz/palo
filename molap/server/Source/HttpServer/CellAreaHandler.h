////////////////////////////////////////////////////////////////////////////////
/// @brief cell area
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

#ifndef HTTP_SERVER_CELL_AREA_HANDLER_H
#define HTTP_SERVER_CELL_AREA_HANDLER_H 1

#include "palo.h"

#include <string>

#include "Olap/AreaStorage.h"
#include "Olap/CellPath.h"
#include "Olap/Rule.h"

#include "HttpServer/PaloHttpHandler.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

namespace palo {

	////////////////////////////////////////////////////////////////////////////////
	/// @brief cell area
	////////////////////////////////////////////////////////////////////////////////

	class SERVER_CLASS CellAreaHandler : public PaloHttpHandler {
	public:
		CellAreaHandler (Server * server) 
			: PaloHttpHandler(server) {
		}

	public:
		HttpResponse * handlePaloRequest (const HttpRequest * request) {
			Database* database = findDatabase(request, 0);
			Cube* cube         = findCube(database, request, 0);

			checkToken(cube, request);

			const vector<Dimension*> * dimensions = cube->getDimensions();

			uint32_t numResult;
			vector<IdentifiersType> area = getArea(request, dimensions, numResult, false);

			AreaStorage storage(dimensions, &area, numResult, false);

			vector<IdentifiersType> resultPaths;
			bool storageFilled = cube->getAreaCellValues(&storage, &area, &resultPaths, user);

			string showRuleParam = request->getValue(SHOW_RULE);
			bool showRule = false;

			if (! showRuleParam.empty() && showRuleParam == "1") {
				showRule = true;
			}

			const string& showLockParam = request->getValue(SHOW_LOCK_INFO);
			bool showLockInfo = showLockParam=="1";

			IdentifierType userId = user ? user->getIdentifier() : 0;

			HttpResponse * response = new HttpResponse(HttpResponse::OK);
			response->setToken(X_PALO_CUBE, cube->getToken());

			if (storageFilled) {                      
				size_t last = storage.size();
				vector<IdentifiersType>::iterator iter = resultPaths.begin();

				for (size_t i = 0; i < last; i++, iter++) {
					Cube::CellValueType* value = storage.getCell(i); 
					buildResponse(cube, value, &(*iter), &response->getBody(), showRule, showLockInfo, userId);
				}      
			}
			else {
				if (0 < numResult) {
					vector<size_t> combinations(area.size(), 0);

					loop(cube, area, combinations, &response->getBody(), showRule, showLockInfo, userId);
				}
			}

			return response;
		}

	private:
		void buildResponse (Cube* cube, Cube::CellValueType* value,
			const IdentifiersType* path,
			StringBuffer* sb,
			bool showRule, bool showLockInfo, IdentifierType userId) {

				bool validCell = true;

				if (value && value->type != Element::UNDEFINED) {
					appendCsvInteger(sb, (int32_t) value->type);

					switch(value->type) {
		  case Element::NUMERIC: 
			  appendCsvString(sb, "1");
			  appendCsvDouble(sb, value->doubleValue);
			  break;

		  case Element::STRING:
			  if (value->charValue.empty()) {
				  appendCsvString(sb, "0;");
			  }
			  else {
				  appendCsvString(sb, "1");
				  appendCsvString(sb, StringUtils::escapeString(value->charValue));
			  }
			  break;

		  default:
			  appendCsvString(sb, "1");
			  appendCsvString(sb, "");
			  break;
					}
				}
				else if (value && value->type == Element::UNDEFINED) {
					if (value->doubleValue == 0.0) {
						// no value found
						appendCsvInteger(sb, (int32_t) Element::NUMERIC);
						appendCsvString(sb, "0;");
					}
					else {
						// error
						validCell = false;
						appendCsvInteger(sb, (int32_t) 99);
						appendCsvInteger(sb, (int32_t) value->doubleValue);
						appendCsvString( sb, StringUtils::escapeString(value->charValue));
					}
				}
				else {
					// this should not happen:
					validCell = false;
					appendCsvInteger(sb, (int32_t) 99);
					appendCsvInteger(sb, (int32_t) ErrorException::ERROR_INTERNAL);
					appendCsvString( sb, ErrorException::getDescriptionErrorType(ErrorException::ERROR_INTERNAL));
				}

				for (size_t i = 0;  i < path->size();  i++) {
					if (0 < i) { sb->appendChar(','); }
					sb->appendInteger(path->at(i));
				}

				sb->appendChar(';');

				if (showRule) {
					if (value && value->rule != Rule::NO_RULE) {
						appendCsvInteger(sb, (uint32_t) value->rule);
					}
					else {
						sb->appendChar(';');
					}
				}

				if (showLockInfo) {	
					Cube::CellLockInfo lockInfo = 0;
					if (validCell) {
						try {
							CellPath cp(cube, path); //may throw an exception if path is not in correct format
							lockInfo = cube->getCellLockInfo(&cp, userId);
						} catch(...) {}
					} 
					appendCsvInteger(sb, (uint32_t) lockInfo);
				}

				sb->appendEol();
		}


		////////////////////////////////////////////////////////////////////////////////
		/// @brief Loop through all possible combinations allowed by the
		///        element lists defined by paths. Each combination indirectly defines a cell-path.
		///        Each cell path is "evaluated" which means that a CSV answer is appended
		///        to our buffer.
		/// @param[in] paths List of element identifier lists. Defines the area.
		/// @param[out] combinations Zero initialized vector of ints of size = no. of dimensions
		/// @param[in] cube The cube our area lies in
		/// @param[out] sb String buffer that holds our CSV answer
		////////////////////////////////////////////////////////////////////////////////

		void loop (Cube * cube,
			const vector< IdentifiersType >& paths,
			vector<size_t>& combinations, 
			StringBuffer* sb,
			bool showRule, bool showLockInfo, IdentifierType userId) {

				bool eval  = true;
				int length = (int) paths.size();

				for (int i = 0;  i < length;) {
					if (eval) {
						evaluate(cube, paths, combinations, sb, showRule, showLockInfo, userId);
					}

					size_t position = combinations[i];
					const IdentifiersType& dim = paths[i];

					if (position + 1 < dim.size()) {
						combinations[i] = (int) position + 1;
						i = 0;
						eval = true;
					}
					else {
						i++;

						for (int k = 0;  k < i;  k++) {
							combinations[k] = 0;
						}

						eval = false;
					}
				}
		}

		////////////////////////////////////////////////////////////////////////////////
		/// @brief evaluate exactly one cell-path. The cell-path is defined by the
		///        array parameter combinations and parameter path. We write the value
		///        of the cell plus its ID-path to the buffer that holds our CSV-reply.
		/// @param[in] cube Pointer to the cube our area belongs to
		/// @param[in] paths Paths that together define the area
		/// @param[in] combinations  Array of dimension indices.
		///            the first element indexes the first dimension etc.
		/// @param[out] StringBuffer The buffer that holds our CSV-reply to the area request.
		////////////////////////////////////////////////////////////////////////////////

		void evaluate (Cube * cube,
			const vector< IdentifiersType >& paths,
			vector<size_t>& combinations,
			StringBuffer* sb,
			bool showRule, bool showLockInfo, IdentifierType userId) {

				// construct path
				IdentifiersType path;

				for (size_t i = 0;  i < paths.size();  i++) {
					IdentifierType id = paths[i][combinations[i]];
					path.push_back(id);
				}

				// get value
				Cube::CellValueType value;
				value.rule = Rule::NO_RULE;

				CellPath cp(cube, &path);

				bool cellValueSucceed;
				try {       

					bool found;
					set< pair<Rule*, IdentifiersType> > ruleHistory;
					value = cube->getCellValue(&cp, &found, user, 0, &ruleHistory);

					cellValueSucceed = true;

					appendCsvInteger(sb, (int32_t) value.type);

					if (found) {
						appendCsvString(sb, "1");

						switch(value.type) {
			case Element::NUMERIC: 
				appendCsvDouble(sb, value.doubleValue);
				break;

			case Element::STRING:
				appendCsvString(sb, StringUtils::escapeString(value.charValue));
				break;

			default:
				appendCsvString(sb, "");
				break;
						}
					}
					else {
						appendCsvString(sb, "0;");
					}
				}
				catch (const ParameterException& error) {
					cellValueSucceed = false;
					appendCsvInteger(sb, (int32_t) 99);
					appendCsvInteger(sb, (int32_t) error.getErrorType());
					appendCsvString( sb, "cannot evaluate path");
				}

				for (size_t i = 0;  i < paths.size();  i++) {
					if (0 < i) { sb->appendChar(','); }
					IdentifierType id = paths[i][combinations[i]];
					sb->appendInteger(id);
				}

				sb->appendChar(';');

				if (showRule) {
					if (value.rule != Rule::NO_RULE) {
						appendCsvInteger(sb, (uint32_t) value.rule);
					}
					else {
						sb->appendChar(';');
					}
				}

				if (showLockInfo) {		
					Cube::CellLockInfo lockInfo = 0;
					if ( cellValueSucceed ) 
						lockInfo = cube->getCellLockInfo(&cp, userId); //safe to call getCellLockInfo because cp is valid path
					appendCsvInteger(sb, (uint32_t) lockInfo);					
				}

				sb->appendEol();
		}
	};

}

#endif
