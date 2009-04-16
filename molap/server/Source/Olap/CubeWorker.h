////////////////////////////////////////////////////////////////////////////////
/// @brief cube worker
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

#ifndef OLAP_CUBE_WORKER_H
#define OLAP_CUBE_WORKER_H 1

#include "palo.h"

#include <set>

#include "InputOutput/Worker.h"

namespace palo {
  class Database;
  class Cube;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief cube worker
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CubeWorker : public Worker {
  public:

    static bool useCubeWorker ();

    static void setUseCubeWorker (bool);
    
  public:
    CubeWorker (Scheduler*, const string&, Database*, Cube*);

  public:
    void executeCube ();
    
    bool start ();
    
    semaphore_t executeSetCell (const string&, const string&, const IdentifiersType*, double);
    
    semaphore_t executeSetCell (const string&, const string&, const IdentifiersType*, const string&);

    semaphore_t executeAreaBuildOk ();
    semaphore_t executeAreaBuildError (const string&, const string&);
    
    semaphore_t executeShutdown ();
    
    void timeoutReached ();
    
  private:
  
    string buildPathString(const IdentifiersType*);

    bool computeArea (vector<string>& answer, string& areaId, vector< IdentifiersType >* area);    
    bool isOverlapping (vector< IdentifiersType >* area1,  vector< IdentifiersType >* area2);
    bool checkAreas (vector<string>* areaIds, vector< vector< IdentifiersType > >* areas);
    bool readAreaLines (const vector<string>& result, vector<string>* areaIds, vector< vector< IdentifiersType > >* areas);    
    
    void executeFinished (const vector<string>& result);

  private:
    set<semaphore_t> semaphores;
    Database* database;    
    Cube* cube;
    
    semaphore_t waitForAreasSemaphore;
    
    static bool useWorkers;
  };

}

#endif
