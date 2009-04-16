////////////////////////////////////////////////////////////////////////////////
/// @brief palo subset and view cube
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

#include "Olap/SubsetViewCube.h"
#include "Olap/PaloSession.h"
#include "Olap/Server.h"

namespace palo {
    
  SubsetViewCube::SubsetViewCube (IdentifierType identifier,
                const string& name,
                Database* database,
                vector<Dimension*>* dimensions)
      : SystemCube (identifier, name, database, dimensions) {

      if (CubeWorker::useCubeWorker()) {
        Scheduler* scheduler = Scheduler::getScheduler();
        PaloSession* session = PaloSession::createSession(database->getServer(), 0, true, 0);        
        cubeWorker = new CubeWorker(scheduler, session->getEncodedIdentifier(), database, this);
        scheduler->registerWorker(cubeWorker);
      }
      
      if (name == SystemCube::NAME_SUBSET_GLOBAL_CUBE || name == SystemCube::NAME_VIEW_GLOBAL_CUBE) {
        isGlobalCube = true;
      } 
      else {
        isGlobalCube = false;
      }
  }
    
  void SubsetViewCube::saveCubeType (FileWriter* file) {
    file->appendIdentifier(identifier);
    file->appendEscapeString(name);

    IdentifiersType identifiers;

    for (vector<Dimension*>::const_iterator i = dimensions.begin(); i != dimensions.end(); i++) {
      identifiers.push_back((*i)->getIdentifier());
    }

    file->appendIdentifiers(&identifiers); 
    file->appendInteger(CUBE_TYPE);
    file->appendBool(isDeletable());
    file->appendBool(isRenamable());
      
    file->nextLine();
  }
  
}
