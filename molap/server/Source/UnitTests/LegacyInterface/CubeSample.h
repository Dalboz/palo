////////////////////////////////////////////////////////////////////////////////
/// @brief sample cube using libpalo 1.0
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

#ifndef UNIT_TESTS_TIMING_CUBE_SAMPLE_H
#define UNIT_TESTS_TIMING_CUBE_SAMPLE_H 1

#include <string>
#include <vector>

#include "PaloConnector.h"

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sample cube using libpalo 1.0
  ////////////////////////////////////////////////////////////////////////////////

  class CubeSample {
  public:
    CubeSample (PaloConnector &p);

    ~CubeSample();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief generates sample cube using libpalo 1.0
    ////////////////////////////////////////////////////////////////////////////////

    void createDatabase (const string& databaseName, 
                         const string& cubeName, 
                         int numDimension, 
                         int numConsolidateElements, 
                         int numElements, 
                         int extraElements, 
                         int posExtraElements);
     
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief loads database information
    ////////////////////////////////////////////////////////////////////////////////

    void loadDatabase (const string& databaseName,
                       const string& cubeName);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes generated database, do nothing otherwise
    ////////////////////////////////////////////////////////////////////////////////

    void deleteDatabase ();
     
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets data
    ////////////////////////////////////////////////////////////////////////////////

    void setData (double rate, double * runtime = 0);
                 
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets data using getdata or getdata_multi
    ////////////////////////////////////////////////////////////////////////////////

    double getData1 (bool useMulti, double * runtime = 0);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets data using getdata or getdata_multi
    ////////////////////////////////////////////////////////////////////////////////

    double getData2 (int dim, bool useMulti, double * runtime = 0);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets data using getdata or getdata_multi
    ////////////////////////////////////////////////////////////////////////////////

    double getData3 (int dim1, int dim2, bool useMulti, double * runtime = 0);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets data using getdata_area
    ////////////////////////////////////////////////////////////////////////////////

    double getArea1 (int dim, double * runtime = 0);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets data using getdata_area
    ////////////////////////////////////////////////////////////////////////////////

    double getArea2 (int dim, double * runtime = 0);
    
  private:
    string databaseName;
    string cubeName;

    bool addedDB;
    bool infoLoaded;         

    PaloConnector * palo;
       
    vector<string>           dimensions;
    vector< vector<string> > consolidatedElements;
    vector< vector<string> > baseElements;
  };
}

#endif
