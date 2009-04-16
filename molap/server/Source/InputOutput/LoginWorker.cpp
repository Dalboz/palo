////////////////////////////////////////////////////////////////////////////////
/// @brief worker
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

#include "InputOutput/LoginWorker.h"

#include "Collections/StringUtils.h"

#include "InputOutput/Logger.h"

namespace palo {
	LoginWorker::LoginWorker( Scheduler* scheduler, const string& sessionString, WorkerLoginType loginType, bool drillThroughEnabled )
			: Worker( scheduler, sessionString ), loginType( loginType ), drillThroughEnabled(drillThroughEnabled) {}

	void LoginWorker::executeLogin( semaphore_t semaphore, const string& username, const string& password ) {
		switch ( loginType ) {
			case WORKER_AUTHORIZATION:
				execute( semaphore, "AUTHORIZATION;" + StringUtils::escapeString( username ) + ";" + StringUtils::escapeString( password ) );
				break;
			case WORKER_AUTHENTICATION:
				execute( semaphore, "AUTHENTICATION;" + StringUtils::escapeString( username ) + ";" + StringUtils::escapeString( password ) );
				break;
			case WORKER_INFORMATION:
				execute( semaphore, "LOGIN;" + StringUtils::escapeString( username ) );
				break;
			case WORKER_NONE:
				throw ErrorException( ErrorException::ERROR_INTERNAL, "executeLogin with login mode" );
		}
	}

	void LoginWorker::executeLogout( semaphore_t semaphore, const string& username ) {
		execute( semaphore, "USER LOGOUT;" + StringUtils::escapeString( username ) );
	}

	void LoginWorker::executeDatabaseSaved( semaphore_t semaphore, IdentifierType id ) {
		execute( semaphore, "DATABASE SAVED;" + StringUtils::convertToString( id ) );
	}

	void LoginWorker::executeCellDrillThrough( semaphore_t semaphore, IdentifierType modeId, IdentifierType databaseId, IdentifierType cubeID, IdentifiersType cellPath ) {

		if (!drillThroughEnabled)
			throw ErrorException( ErrorException::ERROR_INTERNAL, "executeCellDrillThrough disabled" );

		string path;
		for ( size_t i = 0; i < cellPath.size(); i++ )
			path += StringUtils::convertToString( cellPath[i] ) + ",";
		if ( !path.empty() )
			path.erase( path.length() - 1, 1 );
		execute( semaphore, "CELL DRILLTHROUGH;" + StringUtils::convertToString( modeId ) + ";" + StringUtils::convertToString( databaseId ) + ";" + StringUtils::convertToString( cubeID ) + ";" + path );
	}

	void LoginWorker::executeShutdown( semaphore_t semaphore ) {
		execute( semaphore, "SERVER SHUTDOWN" );
		waitForShutdownTimeout = true;
		scheduler->registerTimoutWorker( this );
	}

	bool LoginWorker::start() {
		if ( waitForShutdownTimeout || scheduler->shutdownInProgress() ) {
			return false;
		}

		Logger::trace << "starting login/info worker" << endl;

		return Worker::start();
	}

	void LoginWorker::getAuthentication( bool* canLogin ) {
		if ( result.size() != 1 ) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer of login worker" );
		}

		vector<string> answer;
		string line = result[0];
		StringUtils::splitString( line, &answer, ';' );

		if ( answer.size() != 2 && answer.size() != 4) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

		if ( answer[0] != "LOGIN" ) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

		if ( answer[1] == "TRUE" ) {
			*canLogin = true;
		} else if ( answer[1] == "FALSE" ) {
			*canLogin = false;
			return;
		} else {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

	}

	void LoginWorker::getAuthorization( bool* canLogin, vector<string>* groups ) {
		if ( result.size() != 1 && result.size() != 2 ) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer of login worker" );
		}

		vector<string> answer;
		string line = result[0];
		StringUtils::splitString( line, &answer, ';' );

		if ( answer.size() != 2 && answer.size() != 4) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

		if ( answer[0] != "LOGIN" ) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

		if ( answer[1] == "TRUE" ) {
			*canLogin = true;
		} else if ( answer[1] == "FALSE" ) {
			*canLogin = false;
			return;
		} else {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

		if ( result.size() != 2 ) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer of login worker" );
		}

		answer.clear();
		line = result[1];
		StringUtils::splitString( line, &answer, ';' );

		if ( answer.size() < 2 ) {
			throw ErrorException( ErrorException::ERROR_INVALID_WORKER_REPLY, "error in answer '" + line + "' of login worker" );
		}

		for ( vector<string>::iterator i = answer.begin() + 1; i != answer.end(); i++ ) {
			groups->push_back( *i );
		}

	}

	void LoginWorker::timeoutReached() {
		// todo delete login worker in server
		Logger::debug << "delete login worker" << endl;
		delete this;
	}

}
