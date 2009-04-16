////////////////////////////////////////////////////////////////////////////////
/// @brief https server
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

#include "HttpsServer/PaloHttpsServer.h"

#include <openssl/err.h>

#include <iostream>

#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"

#include "HttpServer/PaloHttpHandler.h"

#if defined(ENABLE_HTTPS_MODULE)
#include "Programs/extension.h"
#endif

#include "HttpsServer/HttpsServerTask.h"

namespace palo {

  /////////////////////
  // SSL help functions
  /////////////////////

  static string lastSSLError () {
    char buf[121];

    unsigned long err = ERR_get_error();
    string errstr = ERR_error_string(err, buf);

    return errstr;
  }


  static string DefaultPassword;

  // this is not thread safe
  static int passwordCallback (char * buffer, int num, int rwflag, void* userdata) {
    if (num < (int) DefaultPassword.size() + 1) {
      return 0;
    }

    strcpy(buffer, DefaultPassword.c_str());

    return (int) strlen(buffer);
  }



  static SSL_CTX * initializeCTX (const string& rootfile, const string& keyfile, const string& password) {
    static bool initialized = false;
    
    // global system initialization
    if (! initialized) {
      SSL_library_init();
      SSL_load_error_strings();
      
      initialized = true;
    }

    // Create our context
    SSL_METHOD* meth = SSLv23_method();
    SSL_CTX* sslctx = SSL_CTX_new(meth);

    // Load our keys and certificates
    if (! SSL_CTX_use_certificate_chain_file(sslctx, keyfile.c_str())) {
      Logger::error << "cannot read certificate from '" << keyfile << "'" << endl;
      Logger::error << lastSSLError() << endl;
      return 0;
    }

    DefaultPassword = password;
    SSL_CTX_set_default_passwd_cb(sslctx, passwordCallback);

    if (! SSL_CTX_use_PrivateKey_file(sslctx, keyfile.c_str(), SSL_FILETYPE_PEM)) {
      Logger::error << "cannot read key from '" << keyfile << "'" << endl;
      Logger::error << lastSSLError() << endl;
      return 0;
    }

    // load the CAs we trust
    if (! SSL_CTX_load_verify_locations(sslctx, rootfile.c_str(), 0)) {
      Logger::error << "cannot read CA list from '" << rootfile << "'" << endl;
      Logger::error << lastSSLError() << endl;
      return 0;
    }

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
    SSL_CTX_set_verify_depth(sslctx, 1);
#endif
    
    return sslctx;
  }
     


  static void destroyCTX (SSL_CTX* sslctx) {
    SSL_CTX_free(sslctx);
  }



  static bool loadDHParameters (SSL_CTX* sslctx, const string& file) {
    BIO* bio = BIO_new_file(file.c_str(), "r");

    if (bio == NULL) {
      Logger::error << "cannot open diffie-hellman file '" << file << "'" << endl;
      Logger::error << lastSSLError() << endl;
      return false;
    }

    DH * ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    BIO_free(bio);

    if (SSL_CTX_set_tmp_dh(sslctx,ret) < 0) {
      Logger::error << "cannot set diffie-hellman parameters" << endl;
      Logger::error << lastSSLError() << endl;
      return false;
    }

    return true;
  }

  /////////////////////
  // static constructor
  /////////////////////

  HttpServer * ConstructPaloHttpsServer (Scheduler * scheduler,
                                         Server * server,
                                         const string& rootfile,
                                         const string& keyfile,
                                         const string& password,
                                         const string& dhfile,
                                         const string& templateDirectory,
                                         const string& address,
                                         int port,
                                         Encryption_e encryption) {

    // initialize the session context
    SSL_CTX * sslctx = initializeCTX(rootfile, keyfile, password);

    if (sslctx == 0) {
      return 0;
    }

    // load the diffie-hellman paramaters
    loadDHParameters(sslctx, dhfile);

    // construct a new https server
    PaloHttpsServer * https = new PaloHttpsServer(server, scheduler, sslctx);

    // add all handlers
    AddPaloHttpHandlers(server, https, scheduler, templateDirectory, false, encryption, port, true);

    // connect server with the listing port
    bool ok;

    if (address.empty()) {
      ok = scheduler->addListenTask(https, port);
    }
    else {
      ok = scheduler->addListenTask(https, address, port);
    }

    if (! ok) {
      Logger::error << "cannot open port '" << port << "' on address '" << address << "'" << endl;
      return 0;
    }
    else {
      Logger::info << "https port '" << port << "' on address '" << address << "' open" << endl;
      return https;
    }
  }

  ///////////////
  // constructors
  ///////////////

  PaloHttpsServer::PaloHttpsServer (Server * server, Scheduler* scheduler, SSL_CTX* ctxParam)
    : IoTask(INVALID_SOCKET, INVALID_SOCKET),
      HttpServer(scheduler),
      server(server),
      scheduler(scheduler),
      ctx(ctxParam) {
  }



  PaloHttpsServer::~PaloHttpsServer () {
    destroyCTX(ctx);
  }



  void PaloHttpsServer::handleConnected (socket_t fd) {
    Logger::info << "trying to establish secure connection" << endl;

    // convert in a SSL BIO structure
    BIO * sbio = BIO_new_socket((int) fd, BIO_NOCLOSE);

    if (sbio == 0) {
      Logger::warning << "cannot build new SSL BIO" << endl;
      Logger::warning << lastSSLError() << endl;
      closesocket(fd);
      return;
    }

    // build a new connection
    SSL * ssl = SSL_new(ctx);

    if (ssl == 0) {
      BIO_free_all(sbio);
      Logger::warning << "cannot build new SSL connection" << endl;
      Logger::warning << lastSSLError() << endl;
      closesocket(fd);
      return;
    }

    // with the above bio
    SSL_set_bio(ssl, sbio, sbio);

    // the HTTP server task will try to accept the incomming connection
    HttpsServerTask * task = new HttpsServerTask(fd, this, scheduler, ssl, sbio);

    bool success = scheduler->addIoTask(task);

    if (success) {
      // TODO: tasks.insert(task);
      Logger::info << "got new client connection on " << fd << endl;
    }
    else {
      delete task;
      closesocket(fd);
    }
  }



  void PaloHttpsServer::handleClientHangup (HttpsServerTask * task) {
    Logger::info << "lost client connection on " << task->getReadSocket() << endl;

    scheduler->removeIoTask(task);
    
    closesocket(task->getReadSocket());
    // TODO tasks.erase(task);
  }



  void PaloHttpsServer::handleShutdown () {
#if 0
    Logger::trace << "beginning shutdown of listener on " << readSocket << endl;

    closesocket(readSocket);

    scheduler->removeListenTask(this);
#endif
  }

}

///////////////////
// module extension
///////////////////

#if defined(ENABLE_HTTPS_MODULE)
using namespace palo;

static bool PaloInterface (ServerInfo_t* info) {

  // check version
  if (strcmp(info->version, Server::getVersion()) == 0) {
    Logger::info << "version " << Server::getVersion() << " match in PaloInterface" << endl;
  }
  else {
    Logger::warning << "version " << Server::getVersion() << " mismatch in PaloInterface" << endl;
    return false;
  }

  // check revision
  if (strcmp(info->revision, Server::getRevision()) == 0) {
    Logger::info << "revision " << Server::getRevision() << " match in PaloInterface" << endl;
  }
  else {
    Logger::warning << "revision " << Server::getRevision() << " mismatch in PaloInterface" << endl;
    return false;
  }

  // return info about extension
  string description = "palo https interface";

  info->description = new char[description.size() + 1];
  strcpy(info->description, description.c_str());

  info->httpsInterface = &ConstructPaloHttpsServer;

  return true;
}

INIT_EXTENSION(PaloInterface);

#endif
