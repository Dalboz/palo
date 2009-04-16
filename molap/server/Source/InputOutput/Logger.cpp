////////////////////////////////////////////////////////////////////////////////
/// @brief logger
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

#include "InputOutput/Logger.h"

#include <iostream>
#include <fstream>

namespace palo {
  
  // do not change the order, because the log streams need to initialized
  // before the constructor of the logger is called.
  Logger::LoggerStreamHeader Logger::error("ERROR", cerr);
  Logger::LoggerStreamHeader Logger::warning("WARNING", cerr);
  Logger::LoggerStreamHeader Logger::info("INFO", cerr);
  Logger::LoggerStreamHeader Logger::debug("DEBUG", cerr);
  Logger::LoggerStreamHeader Logger::trace("TRACE", cerr);
  Logger Logger::logger;



  Logger::Logger () {
  }



  void Logger::setLogLevel (const string& level) {
    error.deactivate();
    warning.deactivate();
    info.deactivate();
    debug.deactivate();  
    trace.deactivate();

    if (level == "error") {
      error.activate();
    }
    else if (level == "warning") {
      error.activate();
      warning.activate();
    }
    else if (level == "info") {
      error.activate();
      warning.activate();
      info.activate();
    }
    else if (level == "debug") {
      error.activate();
      warning.activate();
      info.activate();
      debug.activate();
    }
    else if (level == "trace") {
      error.activate();
      warning.activate();
      info.activate();
      debug.activate();
      trace.activate();
    }
    else {
      error.activate();
      warning.activate();
      Logger::error << "strange log level '" << level << "', going to 'warning'" << endl;
    }
  }



  void Logger::setLogFile (const string& file) {
    ostream * os;

    if (file == "-") {
      os = &cout;
    }
    else if (file == "+") {
      os = &cerr;
    }
    else {
      os = new ofstream(file.c_str(), ios::app);
    }

    if (! *os) {
      Logger::error << "cannot open log file '" << file << "'" << endl;
    }
    else {
      Logger::error.setStream(*os);
      Logger::warning.setStream(*os);
      Logger::info.setStream(*os);
      Logger::debug.setStream(*os);
      Logger::trace.setStream(*os);
    }
  }



  Logger::LoggerStreamHeader::LoggerStreamHeader (const string& prefix, ostream& output)
    : on(true), prefix(prefix), stream(output) {
  }
}
