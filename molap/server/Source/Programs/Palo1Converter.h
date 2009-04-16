////////////////////////////////////////////////////////////////////////////////
/// @brief converts a Palo 1.0 database
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

#ifndef PROGRAMS_PALO1_CONVERTER_H
#define PROGRAMS_PALO1_CONVERTER_H 1

#include "palo.h"

#include "Olap/Element.h"

namespace palo {
  class Cube;
  class Database;
  class Dimension;
  class Server;

  class SERVER_CLASS Palo1Converter {
  public:
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief suffix of the original directory
    ////////////////////////////////////////////////////////////////////////////////

    static const string backupSuffix;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks for palo 1.0 data directory
    ////////////////////////////////////////////////////////////////////////////////

    static bool isPalo1Directory (const string& dataDirectory);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief lists palo 1.0 data directories
    ////////////////////////////////////////////////////////////////////////////////

    static vector<string> listDirectories (const string& dataDirectory);

  public:
    Palo1Converter (Server * palo, const string& path, const string& dir);

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts Palo 1.0 database directory
    ////////////////////////////////////////////////////////////////////////////////

    bool convertDatabase (const string& importLogFile);

  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @bried finds or adds an element
    ////////////////////////////////////////////////////////////////////////////////

    Element* findOrAddElement (Dimension* dimension, const string& name, Element::ElementType type, bool changeType);

    ////////////////////////////////////////////////////////////////////////////////
    /// @bried finds an element and checks the type
    ////////////////////////////////////////////////////////////////////////////////

    Element* findElement (Dimension* dimension, const string& name, Element::ElementType type);    
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @bried updates the position of an element
    ////////////////////////////////////////////////////////////////////////////////

    void updateElementPosition (Dimension* dimension, const string& name, PositionType position);

    ////////////////////////////////////////////////////////////////////////////////
    /// @bried processes dimension file
    ////////////////////////////////////////////////////////////////////////////////

    void processDimension (Database* database, const string& name, ifstream& file);

    ////////////////////////////////////////////////////////////////////////////////
    /// @bried reads dimension from cube file
    ////////////////////////////////////////////////////////////////////////////////

    void readDimension (ifstream &file, Dimension* dimension, vector<Element*>* elements);

  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief processes xml cube description
    ////////////////////////////////////////////////////////////////////////////////

    Cube* processCubeDescription (Database* database, const string& name, ifstream& file);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief processes cube data file
    ////////////////////////////////////////////////////////////////////////////////

    void processCubeData (Database* database, Cube* cube, ifstream& file);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief decodes a name from a palo file name
    ////////////////////////////////////////////////////////////////////////////////
    string decodeNameFromFileName (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief decodes palo base64 to a string
    ////////////////////////////////////////////////////////////////////////////////

    static uint8_t * decodePaloBase64 (const string& str, size_t& length);
    static uint8_t paloBase64ToInt (const uint8_t c);

  private:
    Server * palo;
    const string palo1Path;
    const string palo1Directory;

    ostream* logFile;
  };

}

#endif
