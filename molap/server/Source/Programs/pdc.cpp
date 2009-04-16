////////////////////////////////////////////////////////////////////////////////
/// @brief main program
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

#include "palo.h"

#include <iostream>
#include <fstream>

#include "Exceptions/FileFormatException.h"
#include "Exceptions/FileOpenException.h"

#include "InputOutput/FileUtils.h"
#include "InputOutput/Logger.h"

#include "Options/Options.h"

#include "Olap/Server.h"

#include "Programs/Palo1Converter.h"

using namespace std;
using namespace palo;

////////////////////////////////////////////////////////////////////////////////
/// @brief data directory
////////////////////////////////////////////////////////////////////////////////

static string DataDirectory = "./Data";

////////////////////////////////////////////////////////////////////////////////
/// @brief program logfile
////////////////////////////////////////////////////////////////////////////////

static string LogFile = "-";

////////////////////////////////////////////////////////////////////////////////
/// @brief import logfile
////////////////////////////////////////////////////////////////////////////////

static string ImportLogFile = "import_logfile.log";

////////////////////////////////////////////////////////////////////////////////
/// @brief palo server
////////////////////////////////////////////////////////////////////////////////

static Server * Palo = 0;

////////////////////////////////////////////////////////////////////////////////
/// @brief loops over all sub-directories
////////////////////////////////////////////////////////////////////////////////

static void processDirectories (const string& path, vector<string>& dirs) {
  for (vector<string>::iterator i = dirs.begin(); i != dirs.end(); i++) {
    const string& dir = *i;

    Palo1Converter converter(Palo, path, dir);

    Logger::info << "converting directory " << dir << endl;

    converter.convertDatabase(ImportLogFile);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief construct palo server
////////////////////////////////////////////////////////////////////////////////

static void ConstructPaloServer () {
  bool loaded = false;

  Palo = new Server(0, FileName(".", "palo", "csv"));

  try {
    Palo->loadServer(0);
    loaded = true;
  }
  catch (const FileFormatException&) {
    Logger::error << "server file is corrupted, giving up!" << endl;
    exit(1);
  }
  catch (const FileOpenException& e) {
    loaded = false;
    Logger::warning << "failed to load server '" << e.getMessage() << "' (" << e.getDetails() << ")" << endl;
  }

  if (! loaded) {
    try {
      Palo->saveServer(0);
    }
    catch (const FileOpenException&) {
      Logger::error << "cannot save server file, giving up!" << endl;
      exit(1);
    }
  }

  // load or generate system databases
  Palo->addSystemDatabase();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief prints usage
////////////////////////////////////////////////////////////////////////////////

static void usage (Options& opts) {
  cout << opts.usage("") << "\n"
       << "data-directory: " << DataDirectory << "\n"
       << endl;

  exit(1);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parse options from command line or file
////////////////////////////////////////////////////////////////////////////////

static void parseOptions (Options& opts, OptionsIterator& iter) {
  opts.clear();

  int optchar;
  string optarg;

  while ((optchar = opts(iter, optarg)) != Options::OPTIONS_END_OPTIONS) {
    switch (optchar) {
      case 'd':
        DataDirectory = optarg;
        break;

      case 'o':
        LogFile = optarg;
        break;

      case 'v':
        Logger::setLogLevel(optarg);
        break;

      case Options::OPTIONS_POSITIONAL:
      case Options::OPTIONS_MISSING_VALUE:
      case Options::OPTIONS_EXTRA_VALUE:
      case Options::OPTIONS_AMBIGUOUS:
      case Options::OPTIONS_BAD_KEYWORD:
      case Options::OPTIONS_BAD_CHAR:
        cout << "\n";
        usage(opts);

      default:
        cout << "Oops! Got '" << optchar << "'\n" << endl;
        usage(opts);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parse command line options
////////////////////////////////////////////////////////////////////////////////

static void parseOptions (int argc, char * argv []) {

  // build options parser
  const char * optv [] = {
    "d:data-directory      <directory>",
    "o:log                 <logfile>",
    "v:verbose             <level>",
    0
  };

  Options opts(argv[0], optv);
  opts.setControls(OptionSpecification::NOGUESSING);

  // parse command line options
  OptionsArgvIterator iter(--argc, ++argv);

  parseOptions(opts, iter);

  if (iter.size() != 0) {
    cout << "unknown argument " << *iter.getCurrent() << "\n" << endl;
    usage(opts);
  }

  // change into data directory
  if (chdir(DataDirectory.c_str()) == -1) {
    Logger::error << "cannot change into directory '" << DataDirectory << "'" << endl;
    usage(opts);
  }

  // set logfile
  Logger::setLogFile(LogFile);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief main
////////////////////////////////////////////////////////////////////////////////

int main (int argc, char * argv []) {
  parseOptions(argc, argv);

  // create server
  ConstructPaloServer();

  // process palo 1.0 directories
  vector<string> directories = Palo1Converter::listDirectories(".");
  processDirectories(".", directories);  
}
