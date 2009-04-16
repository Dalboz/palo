////////////////////////////////////////////////////////////////////////////////
/// @brief support for dynamic extensions
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

#ifndef PROGRAMS_EXTENSION_H
#define PROGRAMS_EXTENSION_H 1

#include "palo.h"

#include "HttpServer/PaloHttpHandler.h"

namespace palo {
  class HttpServer;
  class LegacyServer;
  class Server;
  class Scheduler;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief init function for an http interface
  ////////////////////////////////////////////////////////////////////////////////

  typedef HttpServer* (*InitHttpInterface_fptr)(Scheduler*,
                                                Server*,
                                                const string& templateDirectory,
                                                const string& address,
                                                int port,
                                                bool admin,
                                                bool requireUserLogin,
                                                Encryption_e,
                                                int https);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief init function for an https interface
  ////////////////////////////////////////////////////////////////////////////////

  typedef HttpServer* (*InitHttpsInterface_fptr)(Scheduler*,
                                                 Server*,
                                                 const string& rootfile,
                                                 const string& keyfile,
                                                 const string& password,
                                                 const string& dhfile,
                                                 const string& templateDirectory,
                                                 const string& address,
                                                 int port,
                                                 Encryption_e);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief init function for a legacy interface
  ////////////////////////////////////////////////////////////////////////////////

  typedef LegacyServer* (*InitLegacyInterface_fptr)(Scheduler*,
                                                    Server*,
                                                    const string& address,
                                                    int port);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief init function for additional http commands
  ////////////////////////////////////////////////////////////////////////////////

  typedef void (*InitHttpExtensions_fptr)(Scheduler*,
                                          Server*,
                                          HttpServer*,
                                          bool admin,
                                          bool https);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief close extension function
  ////////////////////////////////////////////////////////////////////////////////

  typedef void (*CloseExtension_fptr)();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief information about the server
  ////////////////////////////////////////////////////////////////////////////////

  struct ServerInfo_t {
    const char *                revision;        // input
    const char *                version;         // input
    Server *                    server;          // input
    Scheduler *                 scheduler;       // input
    const char *                modules;         // input

#if defined(_MSC_VER)
    HINSTANCE                   handle;          // handle to DLL
#else
    void *                      handle;          // handle to shared library
#endif    

    char *                      description;     // output
    InitHttpInterface_fptr      httpInterface;   // output
    InitHttpsInterface_fptr     httpsInterface;  // output
    InitLegacyInterface_fptr    legacyInterface; // output 
    InitHttpExtensions_fptr     httpExtensions;  // output
    CloseExtension_fptr         closeExtension;  // output
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief init function
  ////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#define INIT_EXTENSION(a) extern "C" __declspec(dllexport) InitExtension_fptr InitExtension = &a
#else
#define INIT_EXTENSION(a) InitExtension_fptr InitExtension = &a
#endif

  typedef bool (*InitExtension_fptr)(ServerInfo_t*);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief checks the existance of external modules
  ////////////////////////////////////////////////////////////////////////////////

  vector<ServerInfo_t*> CheckExternalModules (Scheduler* scheduler,
                                              Server* server,
                                              const string& directory);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief checks the existance of external modules
  ////////////////////////////////////////////////////////////////////////////////

  void CloseExternalModules ();
}

#endif

