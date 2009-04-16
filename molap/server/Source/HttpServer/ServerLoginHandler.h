////////////////////////////////////////////////////////////////////////////////
/// @brief palo server login handler
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

#ifndef HTTP_SERVER_SERVER_LOGIN_HANDLER_H
#define HTTP_SERVER_SERVER_LOGIN_HANDLER_H 1

#include "palo.h"

#include <string>
#include <iostream>

#include "HttpServer/PaloHttpHandler.h"
#include "Exceptions/ParameterException.h"
#include "InputOutput/Worker.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief login to a palo server
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS ServerLoginHandler : public PaloHttpHandler, public PaloSemaphoreHandler {
  public:
    ServerLoginHandler (Server * server, Scheduler * scheduler) 
      : PaloHttpHandler(server), scheduler(scheduler) {
      semaphore = scheduler->createSemaphore();
    }

  public:
    HttpResponse * handleHttpRequest (const HttpRequest * request) {
      checkToken(server, request);

      try {
        if (requireUser) {

          // we need a user name and a passoword
          const string& username = request->getValue(NAME_USER);
          const string& intern_password = request->getValue(PASSWORD);
          const string& extern_password = request->getValue(EXTERN_PASSWORD);           

          string password = intern_password;

          LoginWorker* loginWorker = server->getLoginWorker();
          
          if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHENTICATION) {
            if (! extern_password.empty()) {
              password = extern_password;
            }
          }

          if (username.empty()) {
            throw ParameterException(ErrorException::ERROR_AUTHORIZATION_FAILED,
                                     "missing username",
                                     NAME_USER, "");
          }
        
          if (password.empty()) {
            throw ParameterException(ErrorException::ERROR_AUTHORIZATION_FAILED,
                                     "missing password",
                                     PASSWORD, "");
          }

          // we need a system database
          SystemDatabase* sd = server->getSystemDatabase();

          if (!sd) {
            throw ErrorException(ErrorException::ERROR_DATABASE_NOT_FOUND,
                                 "system database not found");
          }
        
          if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHORIZATION) {

            //  check user name and password by worker 
            loginWorker->executeLogin(semaphore, username, password);
            return new HttpResponse(semaphore, 0);
          }


          //  get user  
          if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHENTICATION) {
            user = sd->getUser(username);
          }
          else {
            user = sd->getUser(username, password);
          }
          
          if (user == 0) {

            // unknown user
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     NAME_USER, username);
          }

          
          if (!user->canLogin()) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     NAME_USER, username);
          }

          
          if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHENTICATION) {

            //  check password by worker 
            loginWorker->executeLogin(semaphore, username, password);
            return new HttpResponse(semaphore, 0);
          }

          if (loginWorker && loginWorker->getLoginType() == WORKER_INFORMATION) {

            //  send user login 
            loginWorker->executeLogin(semaphore, username, password);
            return new HttpResponse(semaphore, 0);
          }

        }
        else {
          user = 0;
        }
        
        return generateLoginResponse(server, PaloSession::createSession(server, user, false, getDefaultTtl()));
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
    
    
    
    HttpResponse * handlePaloRequest (const HttpRequest * request) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "check ServerLoginHandler.h");
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
  
        SystemDatabase* sd = server->getSystemDatabase();
        User* user;

        if (loginWorker->getLoginType() == WORKER_INFORMATION) {
          const string& username = request->getValue(NAME_USER);
          const string& password = request->getValue(PASSWORD);

          user = sd->getUser(username, password);

          if (user == 0 || !user->canLogin()) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     NAME_USER, username);
          }

          return generateLoginResponse(server, PaloSession::createSession(server, user, false, getDefaultTtl()));
        }
        else if (loginWorker->getLoginType() == WORKER_AUTHENTICATION) {
          bool canLogin;                

          loginWorker->getAuthentication(&canLogin);

          const string& username = request->getValue(NAME_USER);

          if (!canLogin) {
            throw ParameterException(ErrorException::ERROR_WORKER_AUTHORIZATION_FAILED,
                                     "login error",
                                     NAME_USER, username);
          }

          user = sd->getUser(username);
          
          if (user == 0 || !user->canLogin()) {
            throw ParameterException(ErrorException::ERROR_WORKER_AUTHORIZATION_FAILED,
                                     "login error",
                                     NAME_USER, username);
          }

          return generateLoginResponse(server, PaloSession::createSession(server, user, false, getDefaultTtl()));
        }
        
        else if (loginWorker->getLoginType() == WORKER_AUTHORIZATION) {
          bool canLogin;         
          vector<string> groups;          

          loginWorker->getAuthorization(&canLogin, &groups);

          const string& username = request->getValue(NAME_USER);

          if (!canLogin) {
            throw ParameterException(ErrorException::ERROR_WORKER_AUTHORIZATION_FAILED,
                                     "login error",
                                     NAME_USER, username);
          }

          user = sd->getExternalUser(username, &groups);

          if (user == 0 || !user->canLogin()) {
            throw ParameterException(ErrorException::ERROR_WORKER_AUTHORIZATION_FAILED,
                                     "login error",
                                     NAME_USER, username);
          }

		  return generateLoginResponse(server, PaloSession::createSession(server, user, false, getDefaultTtl()));
        }

        throw ErrorException(ErrorException::ERROR_INTERNAL, "unknown login mode");

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

    void checkLogin (const HttpRequest * request) {};

  private:
    Scheduler * scheduler;
    semaphore_t semaphore;

  };
}

#endif
