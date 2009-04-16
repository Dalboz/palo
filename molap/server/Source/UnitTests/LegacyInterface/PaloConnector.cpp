////////////////////////////////////////////////////////////////////////////////
/// @brief connector to libpalo 1.0
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

#include "assert.h"

#include <string>
#include <iostream>

#include "PaloConnector.h"

namespace palo {
  using namespace std;


  PaloConnector::PaloConnector (const string &serverName, const string &serverPort, const string &userName, const string &password)
    : serverName(serverName),
      serverPort(serverPort),
      userName(userName),
      password(password),
      connected(false),
      initialized(false) {
    init();
    connect();
  }



  PaloConnector::~PaloConnector () {
    cleanup();  
  }



  void PaloConnector::handleError (const string& where, libpalo_err *err) {
    cout << where << ", ";

    printError(err);

    cleanup();

    cout << "press return to exit..." << endl;
    getchar();
    
    exit(1);
  }



  void PaloConnector::printError (libpalo_err *err) {
    char *errmsg;

    if (err != NULL) {
      errmsg = libpalo_err_get_string_a(err);

      libpalo_err_free_contents(err);

      cerr << "error: " << (errmsg == NULL ? "unknown error" : errmsg) << endl;

      if(errmsg != NULL) {
        free_arg_str_a(errmsg);
      }
    } 
    else {
      cerr << "error (no detailed error description available)" << endl;
    }
  }



  void PaloConnector::init () {
    libpalo_err err;

    // set locale according to environment variables
    setlocale(LC_ALL, "en_US.iso88591");
  
    /* initialize libpalo */
    cout << "initializing libpalo..." << endl;

    if (! LIBPALO_SUCCESSFUL(libpalo_init(&err))) {
      handleError("init", &err);
    }

    initialized = true;
  }



  void PaloConnector::connect() {
    libpalo_err err;

    // establish a connection to the server
    cout << "connecting to server..." << endl;

    if (! LIBPALO_SUCCESSFUL(palo_connect_a(&err, serverName.c_str() , serverPort.c_str(), &so))) {
      handleError("connect", &err);
    }

    // authenticate (should be executed directly after connecting
    cout << "authenticating..." << endl;

    if (! LIBPALO_SUCCESSFUL(palo_auth_a(&err, &so, userName.c_str(), password.c_str()))) {
      handleError("authentication", &err);
    }

    connected = true;
  }



  void PaloConnector::cleanup () {
    if (connected) {
      cout << "disconnecting..." << endl;
      palo_disconnect(&so);
      connected = false;
    }

    if(initialized) {
      libpalo_cleanup();
      initialized = false;
    }
  }



  void PaloConnector::createDatabase (const string &database) {
    libpalo_err err;

    deleteDatabase(database);

    // create database
    cout << "creating database \"" << database << "\"..." << endl;

    if(! LIBPALO_SUCCESSFUL(palo_root_add_database_a(&err, &so, database.c_str()))) {
      handleError("create database", &err);
    }
  }



  void PaloConnector::deleteDatabase (const string &database) {
    libpalo_err err;

    cout << "trying to remove database \"" << database << "\"..." << endl;

    if (! LIBPALO_SUCCESSFUL(palo_database_delete_a(&err, &so, database.c_str()))) {
      printError(&err);
    }
  }



  void PaloConnector::createDimension (const string &database, const string &dimension) {
    libpalo_err err;

    // create dimension
    cout << "creating dimension \"" << dimension << "\" in database \"" << database << "\"..." << endl;

    if (! LIBPALO_SUCCESSFUL(palo_database_add_dimension_a(&err, &so, database.c_str(), dimension.c_str()))) {
      handleError("create dimension", &err);
    }
  }



  void PaloConnector::addElement (const string &database,
                                  const string &dimension,
                                  const string &element,
                                  de_type type, 
                                  unsigned int num_consolidation_elements, 
                                  const struct arg_consolidation_element_info_a *consolidation_elements) {
    libpalo_err err;

    if (! LIBPALO_SUCCESSFUL(palo_eadd_or_update_a(&err, &so, database.c_str(), dimension.c_str(), element.c_str(),
                                                  dimension_add_or_update_element_mode_add, type,
                                                  libpalo_make_arg_consolidation_element_info_array_a(num_consolidation_elements,
                                                                                                      consolidation_elements),
                                                  PALO_FALSE))) {
      handleError("add element", &err);
    }
  }



  void PaloConnector::appendElement (const string &database,
                                     const string &dimension,
                                     const string &element,
                                     de_type type, 
                                     unsigned int num_consolidation_elements, 
                                     const struct arg_consolidation_element_info_a *consolidation_elements) {
    libpalo_err err;

    if (! LIBPALO_SUCCESSFUL(palo_eadd_or_update_a(&err, &so, database.c_str(), dimension.c_str(), element.c_str(),
                                                  dimension_add_or_update_element_mode_add, type,
                                                  libpalo_make_arg_consolidation_element_info_array_a(num_consolidation_elements,
                                                                                                      consolidation_elements),
                                                  PALO_TRUE))) {
      handleError("append element", &err);
    }
  }



  void PaloConnector::addNumericElement (const string &database, const string &dimension, const string &element) {
    addElement(database, dimension, element, de_n, 0, NULL);
  }



  void PaloConnector::addConsolidateElement (const string &database,
                                             const string &dimension,
                                             const string &element, 
                                             vector<string> &subElements,
                                             de_type type) {
    int count = (int) subElements.size();
  
    arg_consolidation_element_info_a *aceia = new arg_consolidation_element_info_a[count];
    
    for (int i = 0;  i < count;  i++) {
      aceia[i].factor = 0.1;
      aceia[i].type   = type; 
      aceia[i].name   = (char*) subElements[i].c_str();       
    }
    
    addElement(database, dimension, element, de_c, count, aceia);
  
    delete[] aceia;
  }



  void PaloConnector::appendConsolidateElement (const string &database,
                                                const string &dimension,
                                                const string &element,
                                                vector<string> &subElements,
                                                de_type type) {
    int count = (int) subElements.size();
 
    arg_consolidation_element_info_a *aceia = new arg_consolidation_element_info_a[count];
   
    for (int i = 0;  i < count;  i++) {
      aceia[i].factor = 0.1;
      aceia[i].type   = type;
      aceia[i].name   = (char*) subElements[i].c_str();  
    }
   
    appendElement(database, dimension, element, de_c, count, aceia);
 
    delete[] aceia;
  } 



  void PaloConnector::deleteElement (const string &database,
                                     const string &dimension,
                                     const string &element) {
    libpalo_err err;

    if (! LIBPALO_SUCCESSFUL(palo_edelete_a(&err, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("delete element", &err);
    }
  }



  void PaloConnector::renameElement (const string &database,
                                     const string &dimension,
                                     const string &element,
                                     const string& newName) {
    libpalo_err err;

    if (! LIBPALO_SUCCESSFUL(palo_erename_a(&err, &so, database.c_str(), dimension.c_str(), element.c_str(), newName.c_str()))) {
      handleError("delete element", &err);
    }
  }



  void PaloConnector::appendChildren (const string &database,
                                      const string &dimension,
                                      const string &element, 
                                      vector<string> &subElements,
                                      de_type type) {
    int count = (int) subElements.size();
  
    arg_consolidation_element_info_a *aceia = new arg_consolidation_element_info_a[count];
    
    for (int i = 0;  i < count;  i++) {
      aceia[i].factor = 0.1;
      aceia[i].type   = type; 
      aceia[i].name   = (char*) subElements[i].c_str();       
    }
    
  
    libpalo_err err;

    if (! LIBPALO_SUCCESSFUL(palo_eadd_or_update_a(&err, &so, database.c_str(), dimension.c_str(), element.c_str(),
                                                   dimension_add_or_update_element_mode_update, de_c,
                                                   libpalo_make_arg_consolidation_element_info_array_a(count, aceia),
                                                   PALO_TRUE))) {
      handleError("append children", &err);
    }

    delete[] aceia;
  }



  void PaloConnector::createCube (const string &database, const string &cube, vector<string> &dimensions) {
    libpalo_err err;
    unsigned int i;
    unsigned int num_dimensions = (unsigned int) dimensions.size();
    const char ** di;
  
    di = new const char* [num_dimensions];

    // create cube
    cout << "creating cube \"" << cube << "\" in database \"" << database 
         << "\" using " << num_dimensions << " dimensions: ";

    for(i = 0;  i < num_dimensions;  i++) {
      di[i] = dimensions[i].c_str();
      cout << di[i] << " ";
    }

    cout << "..." << endl;

    if (! LIBPALO_SUCCESSFUL(palo_database_add_cube_a(&err, &so, database.c_str(), cube.c_str(),
                                                     libpalo_make_arg_str_array_a(num_dimensions, di)))) {
      handleError("create cube", &err);
    }
      
    delete[] di;
  }



  void PaloConnector::setData (const string &database, const string &cube, vector<string>coordinates, double dbl_val) {
    libpalo_err err;

    int num_coordinates = (int) coordinates.size();
    const char **co;  
    co = new const char* [num_coordinates]; 

    for(int i=0; i<num_coordinates; i++) {
      co[i] = coordinates[i].c_str();
    }

    if (! LIBPALO_SUCCESSFUL(palo_setdata_a(&err, &so, database.c_str(), cube.c_str(),
                                           libpalo_make_arg_palo_dataset_a(libpalo_make_arg_str_array_a(num_coordinates, co),
                                                                           libpalo_make_arg_palo_value_a(NULL, dbl_val)),
                                           splash_mode_disable))) {
      handleError("set data", &err);
    }
  }



  double PaloConnector::getData (const string &database, const string &cube, const vector<string>& coordinates) {
    libpalo_err err;
    struct arg_palo_value_a val;

    int num_coordinates = (int) coordinates.size();
    const char **co;  
    co = new const char* [num_coordinates]; 

    for(int i = 0;  i < num_coordinates;  i++) {
      co[i] = coordinates[i].c_str();
    }

    if (! LIBPALO_SUCCESSFUL(palo_getdata_a(&err, &val, &so, database.c_str(), cube.c_str(),
                                           libpalo_make_arg_str_array_a(num_coordinates, co)))) {
      handleError("get data", &err);
    }

    double d = 0;

    if (val.type == argPaloValueTypeStr) {
      // printf("got: \"%s\" (string)\n", val.val.s);
    }
    else if (val.type == argPaloValueTypeDouble) {
      d = val.val.d;
    }
    else {
      cout << "got value of unknown type" << endl;
    }
  
    free_arg_palo_value_contents_a(&val);
  
    delete[] co;
  
    return d;
  }



  double PaloConnector::getDataMulti (const string& database, 
              const string& cube, 
              const vector< vector<string> >& coordinates) {
    libpalo_err err;
    struct arg_palo_value_array_a val;
    struct arg_str_array_2d_a array;
  
    array.rows = (unsigned int) coordinates.size();
    array.cols = (unsigned int) coordinates[0].size();
    array.a    = new char * [array.rows * array.cols];
  
    for (unsigned int i = 0;  i < array.rows;  i++) {
      for (unsigned int j = 0;  j < array.cols;  j++) {
        array.a[i * array.cols + j] = (char*) coordinates[i][j].c_str();
      }
    }

    if (! LIBPALO_SUCCESSFUL(palo_getdata_multi_a(&err, &val, &so, database.c_str(), cube.c_str(), array))) {
      handleError("get data multi", &err);
    }

    double result = 0;

    for (unsigned int i = 0;  i < val.len;  i++) {
      result += val.a[i].val.d;
    }

    cout << "getdata_multi " << val.len << " values, sum " << result << endl;

    return result;
  }



  double PaloConnector::getArea (const string& database, 
         const string& cube, 
         const vector< vector<string> >& coordinates) {
    libpalo_err err;
    struct arg_palo_value_array_a val;
    struct arg_str_array_array_a array;
  
    array.len = (unsigned int) coordinates.size();
    array.a = new arg_str_array_a[array.len];

    for (unsigned int i = 0;  i < array.len;  i++) {
      array.a[i].len = (unsigned int) coordinates[i].size();
      array.a[i].a   = new char * [array.a[i].len];

      for (unsigned int j = 0;  j < array.a[i].len;  j++) {
        array.a[i].a[j] = (char*) coordinates[i][j].c_str();
      }
    }

    palo_bool guess = PALO_FALSE;

    if (! LIBPALO_SUCCESSFUL(palo_getdata_area_a(&err, &val, &so, database.c_str(), cube.c_str(), array, guess))) {
      handleError("get area", &err);
    }

    double result = 0;

    for (unsigned int i = 0;  i < val.len;  i++) {
      result += val.a[i].val.d;
    }

    cout << "getdata_area " << val.len << " values, sum " << result << endl;

    return result;
  }



  void PaloConnector::serverShutdown () {
    libpalo_err err;

    if (! LIBPALO_SUCCESSFUL(palo_server_shutdown_a(&err, &so))) {
      handleError("server shutdown", &err);
    }
  }



  vector<string> PaloConnector::getCubeDimensions (const string &database, const string &cube) {
    vector<string> resultV;
 
    libpalo_err err;
    struct arg_str_array_a result;

    if (! LIBPALO_SUCCESSFUL(palo_cube_list_dimensions_a(&err, &result, &so, database.c_str(), cube.c_str()))) {  
      handleError("cube dimensions", &err);
    }
  
    for (unsigned int i = 0;  i < result.len;  i++) {
      char * c = (result.a)[i];
      resultV.push_back(c);
    }
  
    // free_arg_str_array_contents_a(result);  
  
    return resultV;
  }



  vector<string> PaloConnector::getNumericElements (const string &database, const string &dimension) { 
    vector<string> resultV;
 
    libpalo_err err;
    struct arg_dim_element_info_array_a result;
    
    if (! LIBPALO_SUCCESSFUL(palo_dimension_list_elements_a(&err, &result, &so, database.c_str(), dimension.c_str()))) {  
      handleError("numeric elements", &err);
    }
  
    for (unsigned int i = 0;  i < result.len;  i++) {
      string str = (result.a)[i].name;

      if ((result.a)[i].type == de_n) {
        resultV.push_back(str);
      }
    }
  
    // do not use free_arg...!
    // free_arg_dim_element_info_array_contents_a(result);  
  
    return resultV;
  }



  vector<string> PaloConnector::getConsolidatedElements (const string &database, const string &dimension) { 
    vector<string> resultV;
 
    libpalo_err err;
    struct arg_dim_element_info_array_a result;

    if (! LIBPALO_SUCCESSFUL(palo_dimension_list_elements_a(&err, &result, &so, database.c_str(), dimension.c_str()))) {  
      handleError("consolidated elements", &err);
    }
  
    for (unsigned int i = 0;  i< result.len;  i++) {
      string str = (result.a)[i].name;

      if ((result.a)[i].type == de_c && str != dimension) {
        resultV.push_back(str);
      }
    }
  
    // do not use free_arg...!
    // free_arg_dim_element_info_array_contents_a(result);  
  
    return resultV;
  }



  unsigned int PaloConnector::eindent (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eindent_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("eindent", &err);
    }

    return result;
  }



  unsigned int PaloConnector::elevel (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_elevel_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("elevel", &err);
    }

    return result;
  }



  unsigned int PaloConnector::eindex (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eindex_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("eindex", &err);
    }

    return result;
  }



  double PaloConnector::eweight (const string& database, const string& dimension, const string& e1, const string& e2) {
    libpalo_err err;
    double result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eweight_a(&err, &result, &so, database.c_str(), dimension.c_str(), e1.c_str(), e2.c_str()))) {
      return -9999;
    }

    return result;
  }



  string PaloConnector::ename (const string& database, const string& dimension, unsigned int offset) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_ename_a(&err, &result, &so, database.c_str(), dimension.c_str(), offset))) {
      handleError("ename", &err);
    }

    return result;
  }



  string PaloConnector::enext (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_enext_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      return "ERROR";
    }

    return result;
  }



  string PaloConnector::eprev (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eprev_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      return "ERROR";
    }

    return result;
  }



  int PaloConnector::echildcount (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_echildcount_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("echildcount", &err);
    }

    return result;
  }



  int PaloConnector::eparentcount (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eparentcount_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("eparentcount", &err);
    }

    return result;
  }



  string PaloConnector::esibling (const string& database, const string& dimension, const string& element, int offset) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_esibling_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str(), offset))) {
      return "ERROR";
    }

    return result;
  }



  string PaloConnector::echildname (const string& database, const string& dimension, const string& element, int offset) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_echildname_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str(), offset))) {
      return "ERROR";
    }

    return result;
  }



  string PaloConnector::eparentname (const string& database, const string& dimension, const string& element, int offset) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eparentname_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str(), offset))) {
      return "ERROR";
    }

    return result;
  }



  int PaloConnector::eischild (const string& database, const string& dimension, const string& e1, const string& e2) {
    libpalo_err err;
    palo_bool result;
    
    if (! LIBPALO_SUCCESSFUL(palo_eischild_a(&err, &result, &so, database.c_str(), dimension.c_str(), e1.c_str(), e2.c_str()))) {
      handleError("eischild", &err);
    }

    return result ? 1 : 0;
  }



  string PaloConnector::efirst (const string& database, const string& dimension) {
    libpalo_err err;
    char * result;
    
    if (! LIBPALO_SUCCESSFUL(palo_efirst_a(&err, &result, &so, database.c_str(), dimension.c_str()))) {
      handleError("efirst", &err);
    }

    return result;
  }



  void PaloConnector::emove (const string& database, const string& dimension, const string& element, int pos) {
    libpalo_err err;
    
    if (! LIBPALO_SUCCESSFUL(palo_emove_a(&err, &so, database.c_str(), dimension.c_str(), element.c_str(), pos))) {
      handleError("emove", &err);
    }
  }



  int PaloConnector::ecount (const string& database, const string& dimension) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_ecount_a(&err, &result, &so, database.c_str(), dimension.c_str()))) {
      handleError("ecount", &err);
    }

    return result;
  }



  int PaloConnector::etype (const string& database, const string& dimension, const string& element) {
    libpalo_err err;
    de_type result;
    
    if (! LIBPALO_SUCCESSFUL(palo_etype_a(&err, &result, &so, database.c_str(), dimension.c_str(), element.c_str()))) {
      handleError("etype", &err);
    }

    return (int) result;
  }



  int PaloConnector::etoplevel (const string& database, const string& dimension) {
    libpalo_err err;
    int result;
    
    if (! LIBPALO_SUCCESSFUL(palo_etoplevel_a(&err, &result, &so, database.c_str(), dimension.c_str()))) {
      handleError("etoplevel", &err);
    }

    return result;
  }



  string PaloConnector::edimension (const string &database, const vector<string>& elements, bool unique) {
    libpalo_err err;

    int num = (int) elements.size();
    const char **co = new const char* [num]; 

    for(int i = 0;  i < num;  i++) {
      co[i] = elements[i].c_str();
    }

    palo_bool u = unique ? PALO_TRUE : PALO_FALSE;
    char * result;

    if (! LIBPALO_SUCCESSFUL(palo_edimension_a(&err, &result, &so, database.c_str(), libpalo_make_arg_str_array_a(num, co), u))) {
      return "ERROR";
    }

    return result;
  }
}

