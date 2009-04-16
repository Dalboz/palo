////////////////////////////////////////////////////////////////////////////////
/// @brief collections of file functions
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

#ifndef INPUT_OUTPUT_FILE_UTILS_H
#define INPUT_OUTPUT_FILE_UTILS_H 1

#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "InputOutput/Logger.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief collections of file functions
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS FileUtils {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a new ifstream or 0
    ////////////////////////////////////////////////////////////////////////////////

    static ifstream * newIfstream (const string& filename) {
#if defined(_MSC_VER)
      FILE * file = fopen(filename.c_str(), "rN");

      if (file == 0) {
        return 0;
      }

      return new ifstream(file);
#else
      ifstream * s = new ifstream(filename.c_str());

      if (! *s) {
        delete s;
        return 0;
      }

      return s;
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a new ofstream or 0
    ////////////////////////////////////////////////////////////////////////////////

    static ofstream * newOfstream (const string& filename) {
#if defined(_MSC_VER)
      FILE * file = fopen(filename.c_str(), "wN");

      if (file == 0) {
        return 0;
      }

      return new ofstream(file);
#else
      ofstream * s = new ofstream(filename.c_str());

      if (! *s) {
        delete s;
        return 0;
      }

      return s;
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a new ofstream or 0
    ////////////////////////////////////////////////////////////////////////////////

    static ofstream * newOfstreamAppend (const string& filename) {
#if defined(_MSC_VER)
      FILE * file = fopen(filename.c_str(), "aN");

      if (file == 0) {
        return 0;
      }

      return new ofstream(file);
#else
      ofstream * s = new ofstream(filename.c_str(), ios::app);

      if (! *s) {
        delete s;
        return 0;
      }

      return s;
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if a file is readable
    ////////////////////////////////////////////////////////////////////////////////

    static bool isReadable (const FileName& fileName) {
      FILE* file = fopen(fileName.fullPath().c_str(), "r");

      if (file == 0) {
        return false;
      }
      else {
        fclose(file);
#if defined(_MSC_VER)
        usleep(100);

        file = fopen(fileName.fullPath().c_str(), "r");

        if (file == 0) {
          Logger::warning << "encountered MS file-system change-log bug during isReadable" << endl;
          return false;
        }
        else {
          fclose(file);
        }
#endif
        return true;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if a file could be removed
    ////////////////////////////////////////////////////////////////////////////////

    static bool remove (const FileName& fileName) {
      int result = std::remove(fileName.fullPath().c_str());

#if defined(_MSC_VER)
    if (result != 0 && errno == EACCES) {
      for (int ms = 0;  ms < 5;  ms++) {
        Logger::warning << "encountered MS file-system change-log bug during remove, sleeping to recover" << endl;
        usleep(1000);

        result = std::remove(fileName.fullPath().c_str());

        if (result == 0 || errno != EACCES) {
          break;
        }
      }
    }
#endif

      return (result != 0) ? false : true;  
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if a file could be renamed
    ////////////////////////////////////////////////////////////////////////////////

    static bool rename (const FileName& oldName, const FileName& newName) {
      int result = std::rename(oldName.fullPath().c_str(), newName.fullPath().c_str());

#if defined(_MSC_VER)
    if (result != 0 && errno == EACCES) {
      for (int ms = 0;  ms < 5;  ms++) {
        Logger::warning << "encountered MS file-system change-log bug during rename, sleeping to recover" << endl;
        usleep(1000);

        result = std::rename(oldName.fullPath().c_str(), newName.fullPath().c_str());

        if (result == 0 || errno != EACCES) {
          break;
        }
      }
    }
#endif

      return (result != 0) ? false : true;  
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if a file directory be renamed
    ////////////////////////////////////////////////////////////////////////////////

    static bool renameDirectory (const FileName& oldName, const FileName& newName) {
      int result = std::rename(oldName.path.c_str(), newName.path.c_str());

#if defined(_MSC_VER)
    if (result != 0 && errno == EACCES) {
      for (int ms = 0;  ms < 5;  ms++) {
        Logger::warning << "encountered MS file-system change-log bug during rename directory, sleeping to recover" << endl;
        usleep(1000);

        result = std::rename(oldName.path.c_str(), newName.path.c_str());

        if (result == 0 || errno != EACCES) {
          break;
        }
      }
    }
#endif

      return (result != 0) ? false : true;  
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if a directory could be removed
    ////////////////////////////////////////////////////////////////////////////////

    static bool removeDirectory (const FileName& name) {
      int result = rmdir(name.path.c_str());

#if defined(_MSC_VER)
      if (result != 0 && (errno == EACCES || errno == ENOTEMPTY)) {
        for (int ms = 0;  ms < 5;  ms++) {
          Logger::warning << "encountered MS file-system change-log bug during rmdir, sleeping to recover" << endl;
          usleep(1000);
          
          result = rmdir(name.path.c_str());
          
          if (result == 0 || (errno != EACCES && errno != ENOTEMPTY)) {
            break;
          }
        }
      }
#endif

      return (result != 0) ? false : true;  
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates a new directory
    ////////////////////////////////////////////////////////////////////////////////

    static bool createDirectory (const string& name) {
#if defined(_MSC_VER)
      int result = mkdir(name.c_str());

      if (result != 0 && errno == EACCES) {
        for (int ms = 0;  ms < 5;  ms++) {
          Logger::warning << "encountered MS file-system change-log bug during mkdir, sleeping to recover" << endl;
          usleep(1000);

          result = mkdir(name.c_str());
          
          if (result == 0 || errno != EACCES) {
            break;
          }
        }
      }
#else
      int result = mkdir(name.c_str(), 0777);
#endif

      if (result != 0 && errno == EEXIST && isDirectory(name)) {
        result = 0;
      }

      return (result != 0) ? false : true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns list of files
    ////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

    static vector<string> _stdcall listFiles (const string& directory) {
      vector<string> result;

      struct _finddata_t fd;
      intptr_t handle;

      string filter = directory + "\\*";
      handle = _findfirst(filter.c_str(), &fd);

      if (handle == -1) {
        return result;
      }

      do {
        if (strcmp(fd.name, ".") != 0 && strcmp(fd.name, "..") != 0) {
          result.push_back(fd.name);
        }
      } while(_findnext(handle, &fd) != -1);

      _findclose(handle);

      return result;
    }

#elif defined(__MINGW32__)

    static vector<string> listFiles (const string& directory) {
      vector<string> result;
      // TODO
      return result;
    }

#else

    static vector<string> listFiles (const string& directory) {
      vector<string> result;

      DIR * d = opendir(directory.c_str());

      if (d == 0) {
        return result;
      }

      struct dirent * de = readdir(d);

      while (de != 0) {
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
          result.push_back(de->d_name);
        }

        de = readdir(d);
      }

      closedir(d);

      return result;
    }

#endif

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if path is a directory
    ////////////////////////////////////////////////////////////////////////////////

    static bool isDirectory (const string& path) {
      struct stat stbuf;
      stat(path.c_str(), &stbuf);

      return (stbuf.st_mode & S_IFMT) == S_IFDIR; 
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if path is a regular file
    ////////////////////////////////////////////////////////////////////////////////

    static bool isRegularFile (const string& path) {
      struct stat stbuf;
      stat(path.c_str(), &stbuf);

      return (stbuf.st_mode & S_IFMT) == S_IFREG;
    }
  };
}

#endif
