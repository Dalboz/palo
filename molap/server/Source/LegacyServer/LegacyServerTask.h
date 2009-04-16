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

#ifndef LEGACY_SERVER_LEGACY_SERVER_TASK_H
#define LEGACY_SERVER_LEGACY_SERVER_TASK_H 1

#include "palo.h"

#include <deque>
#include <vector>

#include "InputOutput/ReadWriteTask.h"
#include "InputOutput/SemaphoreTask.h"

namespace palo {
  using namespace std;

  class ErrorException;
  class LegacyArgument;
  class LegacyRequestHandler;
  class LegacyServer;
  class PaloSession;
  class User;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief read/write task of a legacy server
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS LegacyServerTask : virtual public ReadWriteTask, virtual SemaphoreTask {
  private:
    enum LegacyServerState {
      CONNECTING,
      CONNECTED,
      CLOSING,
      WAIT_FOR_CONNECT_ACK,
      WAIT_FOR_CONTROL_REQUEST,
      WAIT_FOR_REQUEST_HEADER,
      WAIT_FOR_REQUEST_BODY,
      WAIT_FOR_SEMAPHORE,
      SEND_RESPONSE,
    };

    struct LegacyHeader {
      char hello [32];
      uint32_t protocolVersion;
    } header;

    typedef uint16_t CommandType;
    typedef uint32_t ReplyType;
    typedef uint16_t ControlType;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief constructs a new read/write task of a legacy server
    ///
    /// The legacy server constructs a LegacyServerTask for each client
    /// connection. This task is responsible for dealing for the input/output
    /// to/from the client.
    ////////////////////////////////////////////////////////////////////////////////

    LegacyServerTask (socket_t, LegacyServer*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes a read/write task of a legacy server
    ////////////////////////////////////////////////////////////////////////////////

    ~LegacyServerTask ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns associated user
    ////////////////////////////////////////////////////////////////////////////////

    User* getUser() {
      return user;
    }

  public:
    bool canHandleRead ();
    bool canHandleWrite ();
    bool handleRead ();
    void handleHangup (socket_t);
    void handleShutdown ();
    
  public:
    void handleSemaphoreRaised (semaphore_t, IdentifierType, const string*);
    void handleSemaphoreDeleted (semaphore_t, IdentifierType);

  protected:
    void completedWriteBuffer ();

  public:
    static const int32_t PALO_SUCCESS = 0;
    static const int32_t PALO_ERR_NO_MEM = -1;
    static const int32_t PALO_ERR_XML_SYNTAX = -2;
    static const int32_t PALO_ERR_XML_WRONG_VERSION = -5;
    static const int32_t PALO_ERR_INVALID_POINTER = -3;
    static const int32_t PALO_ERR_INVALID_FILENAME = -4;
    static const int32_t PALO_ERR_READ = -6;
    static const int32_t PALO_ERR_XML_NO_DTD = -7;
    static const int32_t PALO_ERR_XML_NO_DECL = -8;
    static const int32_t PALO_ERR_XML_INVALID_TAG = -9;
    static const int32_t PALO_ERR_XML_INVALID_CHAR = -10;
    static const int32_t PALO_ERR_BUFFER_OVERFLOW = -11;
    static const int32_t PALO_ERR_XML_NO_CLOSING_TAG = -12;
    static const int32_t PALO_ERR_XML_ATTR = -13;
    static const int32_t PALO_ERR_ID_IN_USE = -14;
    static const int32_t PALO_ERR_ID_NOT_IN_USE = -15;
    static const int32_t PALO_ERR_NO_FREE_ID = -16;
    static const int32_t PALO_ERR_THREAD = -17;
    static const int32_t PALO_ERR_MUTEX = -18;
    static const int32_t PALO_ERR_NET_ARG = -19;
    static const int32_t PALO_ERR_ALREADY_LOADED = -20;
    static const int32_t PALO_ERR_STRING = -21;
    static const int32_t PALO_ERR_FILE = -22;
    static const int32_t PALO_ERR_NOT_FOUND = -23;
    static const int32_t PALO_ERR_EMPTY_CUBE = -24;
    static const int32_t PALO_ERR_IM = -25;
    static const int32_t PALO_ERR_TYPE = -26;
    static const int32_t PALO_ERR_INV_ARG = -27;
    static const int32_t PALO_ERR_IN_USE = -28;
    static const int32_t PALO_ERR_QUERY = -29;
    static const int32_t PALO_ERR_NOT_IMPLEMENTED = -30;
    static const int32_t PALO_ERR_NETWORK = -31;
    static const int32_t PALO_ERR_SYSTEM = -32;
    static const int32_t PALO_ERR_ROOT_NOT_AVAILABLE = -33;
    static const int32_t PALO_ERR_DATABASE_NOT_FOUND = -34;
    static const int32_t PALO_ERR_DIMENSION_NOT_FOUND = -35;
    static const int32_t PALO_ERR_CUBE_NOT_FOUND = -36;
    static const int32_t PALO_ERR_DIM_ELEMENT_NOT_FOUND = -37;
    static const int32_t PALO_ERR_DIM_ELEMENT_INV_TYPE = -38;
    static const int32_t PALO_ERR_INV_OFFSET = -39;
    static const int32_t PALO_ERR_DIM_ELEMENT_NOT_CHILD = -40;
    static const int32_t PALO_ERR_DIM_EMPTY = -41;
    static const int32_t PALO_ERR_UNEXPECTED_THREAD_STATE = -42;
    static const int32_t PALO_ERR_AUTH_NA = -43;
    static const int32_t PALO_ERR_AUTH = -44;
    static const int32_t PALO_ERR_INVALID_DIRNAME = -45;
    static const int32_t PALO_ERR_INV_FILE_TYPE = -46;
    static const int32_t PALO_ERR_MKDIR = -47;
    static const int32_t PALO_ERR_FILE_RENAME = -48;
    static const int32_t PALO_ERR_NAME_NOT_UNIQUE = -49;
    static const int32_t PALO_ERR_DIMENSION_ELEMENT_EXISTS = -50;
    static const int32_t PALO_ERR_DIMENSION_EXISTS = -51;
    static const int32_t PALO_ERR_MISSING_COORDINATES = -52;
    static const int32_t PALO_ERR_CLEANUP_LIST_FULL = -53;
    static const int32_t PALO_ERR_INVALID_STR = -54;
    static const int32_t PALO_ERR_CIRCULAR_REF = -55;
    static const int32_t PALO_ERR_ID_NOT_FOUND = -56;
    static const int32_t PALO_ERR_UNKNOWN_OPERATION = -57;
    static const int32_t PALO_ERR_EOF = -58;
    static const int32_t PALO_ERR_INV_CMD = -59;
    static const int32_t PALO_ERR_INV_CMD_CTL = -60;
    static const int32_t PALO_ERR_NET_SEND = -61;
    static const int32_t PALO_ERR_NET_CONN_TERM = -62;
    static const int32_t PALO_ERR_NET_RECV = -63;
    static const int32_t PALO_ERR_NET_HS_HELLO = -64;
    static const int32_t PALO_ERR_NET_HS_PROTO = -65;
    static const int32_t PALO_ERR_CHILD_ELEMENT_EXISTS = -66;
    static const int32_t PALO_ERR_CHILD_ELEMENT_NOT_FOUND = -67;
    static const int32_t PALO_ERR_INV_DATABASE_NAME = -68;
    static const int32_t PALO_ERR_INV_CUBE_NAME = -69;
    static const int32_t PALO_ERR_INV_DIMENSION_NAME = -70;
    static const int32_t PALO_ERR_INV_DIMENSION_ELEMENT_NAME = -71;
    static const int32_t PALO_ERR_INV_ARG_COUNT = -72;
    static const int32_t PALO_ERR_INV_ARG_TYPE = -73;
    static const int32_t PALO_ERR_CLIENT_INV_NET_REPLY = -74;
    static const int32_t PALO_ERR_CLIENT_SERVER_RETURNED_ERROR = -75;
    static const int32_t PALO_ERR_CACHE_ENTRY_NOT_FOUND = -76;
    static const int32_t PALO_ERR_CACHE_GOT_ERROR = -77;
    static const int32_t PALO_ERR_CACHE = -78;
    static const int32_t PALO_ERR_SERVER_VERSION = -79;
    static const int32_t PALO_ERR_CONSOLE = -80;
    static const int32_t PALO_ERR_SYS_TIME = -81;
    static const int32_t PALO_ERR_INTEGER_OVERFLOW = -82;

  private:
    static const uint32_t NET_PROTOCOL_VERSION = 2;

    static const char * NET_SERVER_HELLO;
    static const char * NET_CLIENT_HELLO;

    static const CommandType CMD_GETDATA = 0;
    static const CommandType CMD_SETDATA = 1;
    static const CommandType CMD_NI      = 2;
    static const CommandType CMD_FNI     = 3;
    static const CommandType CMD_AUTH    = 4;

    static const ReplyType netReplyTypeError               = 1;
    static const ReplyType netReplyTypeInfo                = 2;
    static const ReplyType netReplyTypeResult              = 3;
    static const ReplyType netReplyTypeCancellationSuccess = 4;

    static const ControlType netCtlCmdTypeStatus = 0;
    static const ControlType netCtlCmdTypeCancel = 1;

  private:
    int32_t legacyErrorType (const ErrorException&);

    LegacyArgument* processAuthentication (const vector<LegacyArgument*>*);
    LegacyArgument* processLegacyRequest (const vector<LegacyArgument*>* arguments);

    void waitForConnectAck ();
    void waitForControlRequest ();
    void waitForRequestHeader ();
    void waitForRequestBody ();

    void fillWriteBuffer ();

    void processControlRequest (ControlType);
    void processRequest (const char * buffer);

  private:
    PaloSession * session;

    StringBuffer outputBuffer;

    LegacyServer * server;
    LegacyServerState state;
    size_t requestLength;

    LegacyRequestHandler * getdataHandler;
    LegacyRequestHandler * setdataHandler;

    bool control;

    semaphore_t loginSemaphore;
    string username;
    string password;
    User* user;
  };

}

#endif
