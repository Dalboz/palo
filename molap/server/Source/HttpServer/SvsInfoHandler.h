////////////////////////////////////////////////////////////////////////////////
/// @brief element delete
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

#ifndef HTTP_SERVER_SVS_INFO_H
#define HTTP_SERVER_SVS_INFO_H 1

#include "palo.h"

#include <string>
#include "../InputOutput/LoginWorker.h"
#include "../Olap/CubeWorker.h"

namespace palo {

	////////////////////////////////////////////////////////////////////////////////
	/// @brief element delete
	////////////////////////////////////////////////////////////////////////////////

	class SERVER_CLASS SvsInfoHandler : public PaloHttpHandler {
	public:
		SvsInfoHandler (Server * server) 
			: PaloHttpHandler(server) {
		}

	public:
		HttpResponse * handlePaloRequest (const HttpRequest * request) {      

			LoginWorker* loginWorker = server->getLoginWorker();

			int svsActive = loginWorker!=NULL ? 1 : 0;
			int loginMode = svsActive ? loginWorker->getLoginType() : 0;
			int cubeWorker = CubeWorker::useCubeWorker();
			int drillThroughEnabled = svsActive ? loginWorker->getDrillThroughEnabled() : 0;


			HttpResponse * response = new HttpResponse(HttpResponse::OK);
			StringBuffer& sb = response->getBody();

			appendCsvInteger(&sb, svsActive);
			appendCsvInteger(&sb, loginMode);
			appendCsvInteger(&sb, cubeWorker);
			appendCsvInteger(&sb, drillThroughEnabled);

			return response;
		}
	};

}

#endif
