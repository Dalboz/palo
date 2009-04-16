////////////////////////////////////////////////////////////////////////////////
/// @brief cell drillthrough
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

#ifndef HTTP_SERVER_CELL_DRILLTHROUGH_H
#define HTTP_SERVER_CELL_DRILLTHROUGH_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "HttpServer/PaloSemaphoreHandler.h"
#include "HttpServer/PaloHttpHandler.h"
#include "Exceptions/ParameterException.h"
#include "InputOutput/LoginWorker.h"

namespace palo {

	////////////////////////////////////////////////////////////////////////////////
	/// @brief login to a palo server
	////////////////////////////////////////////////////////////////////////////////

	class SERVER_CLASS CellDrillThroughHandler : public PaloHttpHandler, public PaloSemaphoreHandler {
	public:
		CellDrillThroughHandler (Server * server, Scheduler * scheduler) 
			: PaloHttpHandler(server), scheduler(scheduler) {
				semaphore = scheduler->createSemaphore();
		}

	public:
		HttpResponse * handlePaloRequest (const HttpRequest * request) {
			
			Database* database = findDatabase(request, 0);
			Cube* cube = findCube(database, request, 0);

			checkToken(cube, request);  


			// get path
			IdentifiersType path = getPath(request, cube->getDimensions());

			string modeString = request->getValue(ID_MODE);
			IdentifierType modeId;


			modeId = StringUtils::stringToInteger(modeString);
			if (modeId<1 || modeId>2) 
				throw new ParameterException(ErrorException::ERROR_INVALID_MODE, "unsupported cell/drillthrough mode", ID_MODE, modeId);

			if (user && user->getRoleCellDataRight() < RIGHT_SPLASH) {
				throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
					"insufficient access rights for path",
					"user", user->getIdentifier());
			}

			LoginWorker* loginWorker = server->getLoginWorker();

			if (loginWorker) {
				loginWorker->executeCellDrillThrough(semaphore, modeId, database->getIdentifier(), cube->getIdentifier(), path);
				return new HttpResponse(semaphore, 0);
			} else 
				throw ErrorException(ErrorException::ERROR_INTERNAL, "login worker null error");
		}

		HttpResponse * handlePaloSemaphoreDeleted (const HttpRequest * request, IdentifierType clientData) {
			throw ErrorException(ErrorException::ERROR_INTERNAL, "semaphore failed");
		}



		HttpResponse * handlePaloSemaphoreRaised (const HttpRequest * request, IdentifierType clientData, const string* msg) {
			try {
				LoginWorker* loginWorker = server->getLoginWorker();

				if (loginWorker->getResultStatus() != Worker::RESULT_OK) {
					throw ErrorException(ErrorException::ERROR_INTERNAL, "login worker error");
				}
				
				std::vector<std::string> result = loginWorker->getResult();
				if(result.size() < 2) {
					throw ErrorException(ErrorException::ERROR_INTERNAL, "login worker error");
				}

				HttpResponse * response = new HttpResponse(HttpResponse::OK);
				StringBuffer& sb = response->getBody();
				
				for(std::vector<std::string>::iterator i = result.begin(); i != result.end();i++) {
					appendCsvString(&sb, StringUtils::escapeString(i->c_str()));
					sb.appendEol();
				}

				return response;
			}
			catch (ErrorException e) {
				HttpResponse * response = new HttpResponse(HttpResponse::BAD);        

				StringBuffer& body = response->getBody();

				appendCsvInteger(&body, (int32_t) e.getErrorType());
				appendCsvString(&body, StringUtils::escapeString(ErrorException::getDescriptionErrorType(e.getErrorType())));
				appendCsvString(&body, StringUtils::escapeString(e.getMessage()));
				body.appendEol();

				return response;
			}
		}  

	private:
		Scheduler * scheduler;
		semaphore_t semaphore;

	};
}

#endif
