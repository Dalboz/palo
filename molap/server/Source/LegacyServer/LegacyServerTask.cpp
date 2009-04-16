////////////////////////////////////////////////////////////////////////////////
/// @brief read/write task of a legacy server
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

#include "LegacyServer/LegacyServerTask.h"

#include <algorithm>
#include <iostream>

#include "Exceptions/CommunicationException.h"
#include "Exceptions/ParameterException.h"

#include "Collections/DeleteObject.h"

#include "InputOutput/Logger.h"

#include "Olap/Server.h"

#include "HttpServer/EventBeginHandler.h"

#include "LegacyServer/ArgumentString.h"
#include "LegacyServer/ArgumentVoid.h"
#include "LegacyServer/LegacyArgument.h"
#include "LegacyServer/LegacyServer.h"
#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {
  using namespace std;


  static const int DELAY_COMMAND = 0;
  static const int DELAY_AUTH = 1;
  static const int DELAY_EVENT = 2;



  const char * LegacyServerTask::NET_SERVER_HELLO = "JDX_MP_OLAP_SERVER\0            ";
  const char * LegacyServerTask::NET_CLIENT_HELLO = "JDX_MP_OLAP_CLIENT\0            ";


  
  LegacyServerTask::LegacyServerTask (socket_t fd, LegacyServer* server)
    : IoTask(fd, fd),
      ReadTask(fd),
      WriteTask(fd),
      ReadWriteTask(fd, fd),
      session(0), 
      server(server),
      state(CONNECTING), 
      control(false),
      user(0) {
    outputBuffer.initialize();
    getdataHandler = server->lookupGetdataHandler();
    setdataHandler = server->lookupSetdataHandler();

    loginSemaphore = server->getScheduler()->createSemaphore();
  }



  LegacyServerTask::~LegacyServerTask () {
    if (session != 0) {
      PaloSession::deleteSession(session);
    }

    outputBuffer.free();

    server->getScheduler()->deleteSemaphore(loginSemaphore);
  }



  bool LegacyServerTask::canHandleRead () {
    switch (state) {
      case WAIT_FOR_CONNECT_ACK: return true;
      case WAIT_FOR_CONTROL_REQUEST: return true;
      case WAIT_FOR_REQUEST_HEADER: return true;
      case WAIT_FOR_REQUEST_BODY: return true;
      default: return false;
    }
  }



  bool LegacyServerTask::canHandleWrite () {
    if (state == CONNECTING) {
      LegacyHeader header;
    
      header.protocolVersion = htonl(NET_PROTOCOL_VERSION);
      memcpy(&header.hello, NET_SERVER_HELLO, sizeof(header.hello));
      
      outputBuffer.replaceText((char*) &header, sizeof(header));

      setWriteBuffer(&outputBuffer, false);

      state = CONNECTED;
    }
    else if (state == WAIT_FOR_SEMAPHORE) {
      return false;
    }

    return WriteTask::canHandleWrite();
  }



  void LegacyServerTask::waitForConnectAck () {
    if (readBuffer.length() < sizeof(LegacyHeader)) {
      return;
    }

    const LegacyHeader * header = reinterpret_cast<const LegacyHeader*>(readBuffer.c_str());

    if (strncmp(header->hello, NET_CLIENT_HELLO, sizeof(header->hello)) != 0) {
      Logger::error << "invalid hello from client on " << readSocket << endl;
      handleHangup(readSocket);
      // the above statement will delete the current legacy server task!
      return;
    }

    uint32_t pv = ntohl(header->protocolVersion);

    if (pv != NET_PROTOCOL_VERSION) {
      Logger::error << "client requested unknown protocol version " << pv << endl;
      handleHangup(readSocket);
      // the above statement will delete the current legacy server task!
      return;
    }

    state = WAIT_FOR_REQUEST_HEADER;

    readBuffer.erase_front(sizeof(LegacyHeader));

    if (! readBuffer.empty()) {
      waitForRequestHeader();
    }
  }



  void LegacyServerTask::waitForControlRequest () {
    if (readBuffer.length() < sizeof(int16_t)) {
      return;
    }

    const ControlType cmd  = ntohs((reinterpret_cast<const int16_t*>(readBuffer.c_str()))[0]);

    readBuffer.erase_front(sizeof(int16_t));

    processControlRequest(cmd);
  }



  void LegacyServerTask::waitForRequestHeader () {
    if (readBuffer.length() < sizeof(int32_t)) {
      return;
    }

    const int32_t * length = reinterpret_cast<const int32_t*>(readBuffer.c_str());

    requestLength = ntohl(*length);
    state = WAIT_FOR_REQUEST_BODY;

    if (sizeof(uint32_t) < readBuffer.length()) {
      waitForRequestBody();
    }
  }



  void LegacyServerTask::waitForRequestBody () {
    if (readBuffer.length() < sizeof(int32_t) + requestLength) {
      return;
    }

    try {
      if (Server::isBlocking()) {
        server->getScheduler()->waitOnSemaphore(Server::getSemaphore(), this, DELAY_EVENT);
      }
      else {
        processRequest(readBuffer.c_str() + sizeof(int32_t));
      }
    }
    catch (const ErrorException& ex) {
      Logger::error << ex.getMessage() << " " << ex.getDetails() << endl;
      handleHangup(readSocket);
      // the above statement will delete the current legacy server task!
      return;
    }
    catch (...) {
      handleHangup(readSocket);
      // the above statement will delete the current legacy server task!
      return;
    }
  }



  bool LegacyServerTask::handleRead () {
    bool res = fillReadBuffer();

    if (! res) {
      return false;
    }

    switch (state) {

      // if we are connected or connecting, wait until the server greeting has been sent
      case CONNECTED:
      case CONNECTING:
        return true;

      // if we are closing, all hope is lost
      case CLOSING:
        return true;

      // if we have sent the server greeting, wait for the client response
      case WAIT_FOR_CONNECT_ACK:
        waitForConnectAck();
        return true;

      // wait for control header
      case WAIT_FOR_CONTROL_REQUEST:
        waitForControlRequest();
        return true;

      // wait for a request header
      case WAIT_FOR_REQUEST_HEADER:
        waitForRequestHeader();
        return true;

      // wait for a request header
      case WAIT_FOR_REQUEST_BODY:
        waitForRequestBody();
        return true;

      // do nothing, we must send the response first
      case SEND_RESPONSE:
        return true;

      default:
        Logger::error << "handle read in strange state " << state << " on " << readSocket << endl;
        return false;
        
    }
  }



  void LegacyServerTask::handleShutdown () {
    Logger::trace << "beginning shutdown of connection on " << readSocket << endl;

    state = CLOSING;
    server->handleClientHangup(this);
    // the above statement will delete the current legacy server task!
  }



  void LegacyServerTask::handleHangup (socket_t fd) {
    state = CLOSING;
    server->handleClientHangup(this);
    // the above statement will delete the current legacy server task!
  }



  void LegacyServerTask::completedWriteBuffer () {
    outputBuffer.clear();

    if (state == CONNECTED) {
      state = WAIT_FOR_CONNECT_ACK;
    }
    else if (state == SEND_RESPONSE) {
      if (control) {
        state = WAIT_FOR_CONTROL_REQUEST;

        if (! readBuffer.empty()) {
          waitForControlRequest();
        }
      }
      else {
        state = WAIT_FOR_REQUEST_HEADER;

        if (! readBuffer.empty()) {
          waitForRequestHeader();
        }
      }
    }
    else {
      Logger::error << "completed write buffer in strange state " << state << endl;
      server->handleClientHangup(this);
      // the above statement will delete the current legacy server task!
      return;
    }
  }



  LegacyArgument* LegacyServerTask::processAuthentication (const vector<LegacyArgument*>* arguments) {
    if (arguments->size() != 2) {
      throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                               "expecing two string arguments for authentication",
                               "number of arguments", (int) arguments->size());
    }

    ArgumentString * name = dynamic_cast<ArgumentString*>((*arguments)[0]);
    ArgumentString * pass = dynamic_cast<ArgumentString*>((*arguments)[1]);

    if (name == 0) {
      throw ParameterException(ErrorException::ERROR_INV_ARG_TYPE,
                               "expecing a string as first argument for authentication",
                               "number of arguments", (int) arguments->size());
    }

    username = name->getValue();

    if (pass == 0) {
      throw ParameterException(ErrorException::ERROR_INV_ARG_TYPE,
                               "expecing a string as second argument for authenfication",
                               "number of arguments", (int) arguments->size());
    }

    password = pass->getValue();

    Logger::debug << "got authentication request for user " << username << endl;

    //  get user identifier and check password 
    SystemDatabase* sd = server->getServer()->getSystemDatabase();

    if (! sd) {
      throw ErrorException(ErrorException::ERROR_DATABASE_NOT_FOUND,
                           "system database not found");
    }

    LoginWorker* loginWorker = server->getServer()->getLoginWorker();

    if (loginWorker) {
      Logger::debug << "got login worker" << endl;
    }
          
    //  check user name and password by worker 
    if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHORIZATION) {
      loginWorker->executeLogin(loginSemaphore, username, password);
      return new ArgumentDelayed(loginSemaphore);
    }

    //  get user  
    if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHENTICATION) {
      user = sd->getUser(username);
    }
    else {
      user = sd->getUser(username, password, false);
    }

    // unknown user
    if (user == 0 || !user->canLogin()) {
      throw ParameterException(ErrorException::ERROR_AUTHORIZATION_FAILED,
                               "login error",
                               "username", username);
    }

    //  check password by worker 
    if (loginWorker && loginWorker->getLoginType() == WORKER_AUTHENTICATION) {
      loginWorker->executeLogin(loginSemaphore, username, password);
      return new ArgumentDelayed(loginSemaphore);
    }

    //  send user login 
    if (loginWorker && loginWorker->getLoginType() == WORKER_INFORMATION) {
      loginWorker->executeLogin(loginSemaphore, username, password);
      return new ArgumentDelayed(loginSemaphore);
    }
        
    // a legacy session is never timed out, the connection will be closed on timeout
    // this will destroy the session
    session = PaloSession::createSession(server->getServer(), user, false, 0);

    return new ArgumentVoid();
  }



  void LegacyServerTask::processControlRequest (ControlType cmd) {
    switch (cmd) {
      case netCtlCmdTypeStatus:
        setWriteBuffer(&outputBuffer, false);
        state = SEND_RESPONSE;
        control = false;
        break;

      case netCtlCmdTypeCancel: {
        outputBuffer.clear();
        ReplyType netReply = ntohl(netReplyTypeCancellationSuccess);
        outputBuffer.appendData((void*) &netReply, sizeof(ReplyType));
        state = SEND_RESPONSE;
        control = false;
        break;
      }

      default:
        throw CommunicationException(ErrorException::ERROR_INV_CMD_CTL,
                                     "unknown control request");
    }
  }


  LegacyArgument* LegacyServerTask::processLegacyRequest (const vector<LegacyArgument*>* arguments) {
    if (arguments->size() < 1) {
      throw ParameterException(ErrorException::ERROR_INV_ARG_TYPE,
                               "expecing function name as argument",
                               "number of arguments", (int) arguments->size());
    }

    ArgumentString * func = dynamic_cast<ArgumentString*>((*arguments)[0]);

    if (func == 0) {
      throw ParameterException(ErrorException::ERROR_INV_ARG_TYPE,
                               "expecting a string as function name",
                               "number of arguments", (int) arguments->size());
    }

    Logger::debug << "received " << func->getValue() << " with " << (arguments->size() - 1) << " arguments" << endl;

    LegacyRequestHandler * handler = server->lookupHandler(func->getValue());

    if (handler == 0) {
      throw ParameterException(ErrorException::ERROR_INV_CMD,
                               "unknown function called",
                               "function", func->getValue());
    }

    return handler->handleLegacyRequest(arguments, user, session);
  }



  int32_t LegacyServerTask::legacyErrorType (const ErrorException& error) {
    switch (error.getErrorType()) {
      case ErrorException::ERROR_UNKNOWN:
        return PALO_ERR_NOT_IMPLEMENTED;

      case ErrorException::ERROR_INTERNAL:
        return PALO_ERR_NOT_IMPLEMENTED;


      case ErrorException::ERROR_ID_NOT_FOUND:
        return PALO_ERR_ID_NOT_FOUND;

      case ErrorException::ERROR_INVALID_FILENAME:
        return PALO_ERR_FILE;

      case ErrorException::ERROR_MKDIR_FAILED:
        return PALO_ERR_MKDIR;

      case ErrorException::ERROR_RENAME_FAILED:
        return PALO_ERR_FILE_RENAME;

      case ErrorException::ERROR_AUTHORIZATION_FAILED:
        return PALO_ERR_AUTH_NA;

      case ErrorException::ERROR_INVALID_TYPE:
        return PALO_ERR_TYPE;

      case ErrorException::ERROR_INVALID_COORDINATES:
        return PALO_ERR_INV_ARG;

      case ErrorException::ERROR_CONVERSION_FAILED:
        return PALO_ERR_TYPE;

      case ErrorException::ERROR_FILE_NOT_FOUND_ERROR:
        return PALO_ERR_FILE;

      case ErrorException::ERROR_NOT_AUTHORIZED:
        return PALO_ERR_AUTH;

      case ErrorException::ERROR_CORRUPT_FILE:
        return PALO_ERR_FILE;

      case ErrorException::ERROR_WITHIN_EVENT:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_NOT_WITHIN_EVENT:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_INVALID_PERMISSION:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_INVALID_SERVER_PATH:
        return PALO_ERR_FILE;

      case ErrorException::ERROR_INVALID_SESSION:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_PARAMETER_MISSING:
        return PALO_ERR_INV_ARG_COUNT;
        
      case ErrorException::ERROR_SERVER_TOKEN_OUTDATED:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_INVALID_SPLASH_MODE:
        return PALO_ERR_INV_ARG_TYPE;

      case ErrorException::ERROR_INVALID_DATABASE_NAME:
        return PALO_ERR_INV_DATABASE_NAME;

      case ErrorException::ERROR_DATABASE_NOT_FOUND:
        return PALO_ERR_DATABASE_NOT_FOUND;

      case ErrorException::ERROR_DATABASE_NOT_LOADED:
        return PALO_ERR_DATABASE_NOT_FOUND;

      case ErrorException::ERROR_DATABASE_UNSAVED:
        return PALO_ERR_DATABASE_NOT_FOUND;

      case ErrorException::ERROR_DATABASE_STILL_LOADED:
        return PALO_ERR_DATABASE_NOT_FOUND;

      case ErrorException::ERROR_DATABASE_NAME_IN_USE:
        return PALO_ERR_INV_DATABASE_NAME;

      case ErrorException::ERROR_DATABASE_UNDELETABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DATABASE_UNRENAMABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DATABASE_TOKEN_OUTDATED:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DIMENSION_EMPTY:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DIMENSION_EXISTS:
        return PALO_ERR_DIMENSION_EXISTS;

      case ErrorException::ERROR_DIMENSION_NOT_FOUND:
        return PALO_ERR_DIMENSION_NOT_FOUND;

      case ErrorException::ERROR_INVALID_DIMENSION_NAME:
        return PALO_ERR_INV_DIMENSION_NAME;

      case ErrorException::ERROR_DIMENSION_UNCHANGABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DIMENSION_NAME_IN_USE:
        return PALO_ERR_INV_DIMENSION_NAME;

      case ErrorException::ERROR_DIMENSION_IN_USE:
        return PALO_ERR_IN_USE;

      case ErrorException::ERROR_DIMENSION_UNDELETABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DIMENSION_UNRENAMABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_DIMENSION_TOKEN_OUTDATED:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_ELEMENT_EXISTS:
        return PALO_ERR_DIMENSION_ELEMENT_EXISTS;

      case ErrorException::ERROR_ELEMENT_CIRCULAR_REFERENCE:
        return PALO_ERR_CIRCULAR_REF;

      case ErrorException::ERROR_ELEMENT_NAME_IN_USE:
        return PALO_ERR_DIMENSION_ELEMENT_EXISTS;

      case ErrorException::ERROR_ELEMENT_NAME_NOT_UNIQUE:
        return PALO_ERR_NAME_NOT_UNIQUE;

      case ErrorException::ERROR_ELEMENT_NOT_FOUND:
        return PALO_ERR_DIM_ELEMENT_NOT_FOUND;

      case ErrorException::ERROR_ELEMENT_NO_CHILD_OF:
        return PALO_ERR_DIM_ELEMENT_NOT_CHILD;

      case ErrorException::ERROR_INVALID_ELEMENT_NAME:
        return PALO_ERR_INV_DIMENSION_ELEMENT_NAME;

      case ErrorException::ERROR_INVALID_OFFSET:
        return PALO_ERR_INV_OFFSET;

      case ErrorException::ERROR_INVALID_ELEMENT_TYPE:
        return PALO_ERR_DIM_ELEMENT_INV_TYPE;

      case ErrorException::ERROR_INVALID_POSITION:
        return PALO_ERR_INV_OFFSET;

      case ErrorException::ERROR_ELEMENT_NOT_RENAMABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_ELEMENT_NOT_DELETABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_CUBE_NOT_FOUND:
        return PALO_ERR_CUBE_NOT_FOUND;

      case ErrorException::ERROR_INVALID_CUBE_NAME:
        return PALO_ERR_INV_CUBE_NAME;

      case ErrorException::ERROR_CUBE_NOT_LOADED:
        return PALO_ERR_CUBE_NOT_FOUND;

      case ErrorException::ERROR_CUBE_EMPTY:
        return PALO_ERR_EMPTY_CUBE;

      case ErrorException::ERROR_CUBE_UNSAVED:
        return PALO_ERR_CUBE_NOT_FOUND;

      case ErrorException::ERROR_SPLASH_DISABLED:
        return PALO_ERR_DIM_ELEMENT_INV_TYPE;

      case ErrorException::ERROR_COPY_PATH_NOT_NUMERIC:
        return PALO_ERR_DIM_ELEMENT_INV_TYPE;

      case ErrorException::ERROR_INVALID_COPY_VALUE:
        return PALO_ERR_DIM_ELEMENT_INV_TYPE;

      case ErrorException::ERROR_CUBE_NAME_IN_USE:
        return PALO_ERR_INV_CUBE_NAME;

      case ErrorException::ERROR_CUBE_UNDELETABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_CUBE_UNRENAMABLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_CUBE_TOKEN_OUTDATED:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_NET_ARG:
        return PALO_ERR_NET_ARG;

      case ErrorException::ERROR_INV_CMD:
        return PALO_ERR_INV_CMD;

      case ErrorException::ERROR_INV_CMD_CTL:
        return PALO_ERR_INV_CMD_CTL;

      case ErrorException::ERROR_NET_SEND:
        return PALO_ERR_NET_SEND;

      case ErrorException::ERROR_NET_CONN_TERM:
        return PALO_ERR_NET_CONN_TERM;

      case ErrorException::ERROR_NET_RECV:
        return PALO_ERR_NET_RECV;

      case ErrorException::ERROR_NET_HS_HALLO:
        return PALO_ERR_NET_HS_HELLO;

      case ErrorException::ERROR_NET_HS_PROTO:
        return PALO_ERR_NET_HS_PROTO;

      case ErrorException::ERROR_INV_ARG_COUNT:
        return PALO_ERR_INV_ARG_COUNT;

      case ErrorException::ERROR_INV_ARG_TYPE:
        return PALO_ERR_INV_ARG_TYPE;

      case ErrorException::ERROR_CLIENT_INV_NET_REPLY:
        return PALO_ERR_CLIENT_INV_NET_REPLY;


      case ErrorException::ERROR_INVALID_WORKER_REPLY:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_WORKER_AUTHORIZATION_FAILED:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_WORKER_MESSAGE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_SPLASH_NOT_POSSIBLE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_PARSING_RULE:
        return PALO_ERR_SYSTEM;

      case ErrorException::ERROR_RULE_NOT_FOUND:
        return PALO_ERR_SYSTEM;
    }

    return PALO_ERR_NOT_IMPLEMENTED;
  }



  void LegacyServerTask::processRequest (const char * buffer) {
    CommandType command = ntohs(reinterpret_cast<const CommandType*>(buffer)[0]);
    size_t numberArguments = ntohs(reinterpret_cast<const uint16_t*>(buffer)[1]);

    vector<LegacyArgument*> arguments = LegacyArgument::unserializeArguments(buffer + sizeof(CommandType) + sizeof(uint16_t),
                                                                             buffer + requestLength,
                                                                             numberArguments);


    readBuffer.erase_front(sizeof(int32_t) + requestLength);

    LegacyArgument * result = 0;
    IdentifierType delayType = DELAY_COMMAND;

    try {
      ReplyType reply = netReplyTypeResult;

      try {
        switch (command) {
          case CMD_AUTH:
            result = processAuthentication(&arguments);
            delayType = DELAY_AUTH;
            break;

          case CMD_NI:
            if (user == 0) {
              throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                       "login error",
                                       "function", "");
            }

            result  = processLegacyRequest(&arguments);
            break;

          case CMD_FNI:
            if (user == 0) {
              throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                       "login error",
                                       "function", "");
            }

            result = processLegacyRequest(&arguments);
            break;

          case CMD_SETDATA:
            if (user == 0) {
              throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                       "login error",
                                       "function", "setdata");
            }

            result = setdataHandler->handleLegacyRequest(&arguments, user, session);
            break;
          
          case CMD_GETDATA:
            if (user == 0) {
              throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                       "login error",
                                       "function", "getdata");
            }

            result = getdataHandler->handleLegacyRequest(&arguments, user, session);
            break;
          
          default:
            Logger::error << "illegal command " << command << " on " << readSocket << endl;
            throw CommunicationException(ErrorException::ERROR_INV_CMD,
                                         "illegal command encountered");
            break;
        }
      }
      catch (const ParameterException& error) {
        Logger::info << error.getMessage() << ", " << error.getDetails() << endl;
        reply = netReplyTypeError;

        if (result != 0) {
          delete result;
        }

        ArgumentMulti * multi = new ArgumentMulti();
        multi->append(new ArgumentInt32(legacyErrorType(error)));

        result = multi;
      }

      outputBuffer.clear();
        
      if (result->getArgumentType() == LegacyArgument::NET_ARG_TYPE_DELAYED) {
        state   = WAIT_FOR_SEMAPHORE;
        control = false;

        server->getScheduler()->waitOnSemaphore(((ArgumentDelayed*) result)->getSemaphore(), this, delayType);
      }
      else {
        ReplyType netReply = ntohl(reply);
        outputBuffer.appendData((void*) &netReply, sizeof(ReplyType));

        // the length will be fixed later
        uint32_t length = 0;
        outputBuffer.appendData((void*) &length, sizeof(length));
        
        if (result == 0) {
          throw ErrorException(ErrorException::ERROR_INTERNAL,
                               "expecting a result");
        }
        
        result->serialize(outputBuffer);
        
        size_t offset = sizeof(netReply) + sizeof(length);
        
        length = ntohl((uint32_t) (outputBuffer.length() - offset));
        memcpy((void*)(outputBuffer.begin() + sizeof(netReply)), (void*) &length, sizeof(length));
      
        if (command == CMD_NI) {
          state   = WAIT_FOR_CONTROL_REQUEST;
          control = true;
        }
        else {
          state   = SEND_RESPONSE;
          control = false;

          setWriteBuffer(&outputBuffer, false);
        }
      }
    }
    catch (...) {
      if (result != 0) {
        delete result;
      }

      for_each(arguments.begin(), arguments.end(), DeleteObject());

      throw;
    }

    if (result != 0) {
      delete result;
    }

    for_each(arguments.begin(), arguments.end(), DeleteObject());

    if (control && ! readBuffer.empty()) {
      waitForControlRequest();
    }
  }



  void LegacyServerTask::handleSemaphoreRaised (semaphore_t semaphore, IdentifierType type, const string* msg) {
    try {

      // wait on event
      if (type == (IdentifierType) DELAY_EVENT) {
        if (Server::isBlocking()) {
          server->getScheduler()->waitOnSemaphore(Server::getSemaphore(), this, DELAY_EVENT);
        }
        else {
          processRequest(readBuffer.c_str() + sizeof(int32_t));
        }

        return;
      }

      // login request
      if (type == (IdentifierType) DELAY_AUTH) {
        LoginWorker* loginWorker = server->getServer()->getLoginWorker();

        if (loginWorker->getResultStatus() != Worker::RESULT_OK) {
          throw ErrorException(ErrorException::ERROR_INVALID_WORKER_REPLY, "login worker error");
        }
  
        SystemDatabase* sd = server->getServer()->getSystemDatabase();

        if (loginWorker->getLoginType() == WORKER_INFORMATION) {
          user = sd->getUser(username, password, false);
        }
        else if (loginWorker->getLoginType() == WORKER_AUTHENTICATION) {
          bool canLogin;                 

          loginWorker->getAuthentication(&canLogin);

          if (!canLogin) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     "username", username);
          }

          user = sd->getUser(username);

          if (user && !user->canLogin()) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     "username", username);
          }
        }
        
        else if (loginWorker->getLoginType() == WORKER_AUTHORIZATION) {
          bool canLogin;          
          vector<string> groups;          

          loginWorker->getAuthorization(&canLogin, &groups);

          if (!canLogin) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     "username", username);
          }

          // TODO: create user without system database!
          user = sd->getExternalUser(username, &groups);

          if (user && !user->canLogin()) {
            throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
                                     "login error",
                                     "username", username);
          }
        }
        else {
          throw ErrorException(ErrorException::ERROR_INTERNAL, "unknown login mode");
        }

        // a legacy session is never timed out, the connection will be closed on timeout
        // this will destroy the session
        session = PaloSession::createSession(server->getServer(), user, false, 0);
      }
    

      ReplyType netReply = ntohl(netReplyTypeResult);
      outputBuffer.appendData((void*) &netReply, sizeof(ReplyType));

      // the length will be fixed later
      uint32_t length = 0;
      outputBuffer.appendData((void*) &length, sizeof(length));
        
      ArgumentVoid result;

      result.serialize(outputBuffer);
        
      size_t offset = sizeof(netReply) + sizeof(length);
        
      length = ntohl((uint32_t) (outputBuffer.length() - offset));
      memcpy((void*)(outputBuffer.begin() + sizeof(netReply)), (void*) &length, sizeof(length));

      state   = SEND_RESPONSE;
      control = false;
      
      setWriteBuffer(&outputBuffer, false);
    }
    catch (...) {
      server->handleClientHangup(this);
    }
  }



  void LegacyServerTask::handleSemaphoreDeleted (semaphore_t semaphore, IdentifierType) {
    server->handleClientHangup(this);
  }
}
