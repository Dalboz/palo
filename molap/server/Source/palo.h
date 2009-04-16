////////////////////////////////////////////////////////////////////////////////
/// @brief main config file
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

#ifndef PALO_H
#define PALO_H 1

#if defined(_MSC_VER)
#include "System/system-vs-net-2003.h"
#elif __MINGW32__
#include "System/system-mingw32.h"
#else
#include "config.h"
#include "System/system-unix.h"
#endif

#include <stdlib.h>
#include <stddef.h>

#include <algorithm>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <map>

namespace palo {
  using namespace std;

  typedef uint32_t dtime_t;
  typedef uint32_t DepthType;
  typedef uint32_t IdentifierType;
  typedef uint32_t IndentType;
  typedef uint32_t LevelType;
  typedef uint32_t PositionType;

#define NO_IDENTIFIER ((IdentifierType) ~(uint32_t)0)

  class Element;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type of semaphore
  ////////////////////////////////////////////////////////////////////////////////

  typedef uint32_t semaphore_t;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief path type
  ////////////////////////////////////////////////////////////////////////////////

  typedef vector<Element*> PathType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief element list
  ////////////////////////////////////////////////////////////////////////////////

  typedef vector<Element*> ElementsType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief identifier list
  ////////////////////////////////////////////////////////////////////////////////

  typedef vector<IdentifierType> IdentifiersType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for elements and weights
  ////////////////////////////////////////////////////////////////////////////////

  typedef vector< pair<Element*, double> > ElementsWeightType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for elements and weights
  ////////////////////////////////////////////////////////////////////////////////

  typedef map<Element*, double> ElementsWeightMap;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for identifiers and weights
  ////////////////////////////////////////////////////////////////////////////////

  typedef vector< pair<IdentifierType, double> > IdentifiersWeightType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for path and weight
  ////////////////////////////////////////////////////////////////////////////////

  typedef pair< vector<Element*>, double > PathWeightType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for filenames
  ////////////////////////////////////////////////////////////////////////////////

  struct FileName {
    FileName () 
      : path("."), name("ERROR"), extension("") {
    }

    FileName (const string& path, const string& name, const string& extension)
      : path(path.empty() ? "." : path), name(name), extension(extension) {
    }

    FileName (const FileName& old)
      : path(old.path), name(old.name), extension(old.extension) {
    }

    FileName (const FileName& old, const string& extension)
      : path(old.path), name(old.name), extension(extension) {
    }

    string fullPath () const {
      if (path.empty()) {
        if (extension.empty()) {
          return name;
        }
        else {
          return name + "." + extension;
        }
      }
      else if (extension.empty()) {
        return path + "/" + name;
      }
      else {
        return path + "/" + name + "." + extension;
      }
    }

    string path;
    string name;
    string extension;
  };
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type of database, dimension or cubes
  ////////////////////////////////////////////////////////////////////////////////
  
  enum ItemType {
    NORMAL,
    SYSTEM,
    ATTRIBUTE,
    USER_INFO
  };
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for rights
  ////////////////////////////////////////////////////////////////////////////////
  
  enum RightsType {
    RIGHT_NONE,
    RIGHT_READ,
    RIGHT_WRITE,
    RIGHT_DELETE,
    RIGHT_SPLASH
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief type for worker login mode
  ////////////////////////////////////////////////////////////////////////////////
  
  enum WorkerLoginType {
    WORKER_NONE,
    WORKER_INFORMATION,
    WORKER_AUTHENTICATION,
    WORKER_AUTHORIZATION
  };

}

#endif
