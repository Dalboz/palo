////////////////////////////////////////////////////////////////////////////////
/// @brief statistics page
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

#ifndef HTTP_SERVER_STATISTICS_HANDLER_H
#define HTTP_SERVER_STATISTICS_HANDLER_H 1

#include "palo.h"

#include <string>

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief server browser
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS StatisticsHandler : public PaloHttpBrowserHandler {
  public:
    StatisticsHandler (Server * server, const string &path) 
      : PaloHttpBrowserHandler(server, path) {
    }

  public:
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      HttpResponse * response = new HttpResponse(HttpResponse::OK);

      StatisticsDocumentation fd(Statistics::statistics);
      HtmlFormatter hd(templatePath + "/statistics.tmpl");

      response->getBody().copy(hd.getDocumentation(&fd));
      response->setContentType("text/html;charset=utf-8");

      return response;
    }
  };
}

#endif
