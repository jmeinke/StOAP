/* 
 *
 * Copyright (C) 2006-2015 Jedox AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as published
 * by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * If you are developing and distributing open source applications under the
 * GPL License, then you are free to use Palo under the GPL License.  For OEMs,
 * ISVs, and VARs who distribute Palo with their products, and do not license
 * and distribute their source code under the GPL, Jedox provides a flexible
 * OEM Commercial License.
 *
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#ifndef STOAP_STOAP_AGGREGATIONENVIRONMENT_H_
#define STOAP_STOAP_AGGREGATIONENVIRONMENT_H_ 1

// #include <gtest/gtest.h>
#include <vector>
#include <map>
#include <string>

#include "Olap.h"
#include "Collections/StringUtils.h"
#include "Olap/Area.h"
#include "Olap/Cube.h"
#include "Olap/Dimension.h"
#include "Exceptions/ParameterException.h"

// Singleton class
class AggrEnv {
 public:
  static AggrEnv& instance() {
    static AggrEnv _instance;
    return _instance;
  }

  // destructor
  ~AggrEnv();

  // Parse the command line arguments.
  // --server-mode, -s   : if set, a pipe for queries will be opened
  // --log-level, -v     : log-level can be [trace|debug|info|warning|error].
  void parseCommandLineArguments(int argc, char** argv);
  // FRIEND_TEST(AggregationEnvironmentTest, parseCommandLineArguments);

  // Read dimensions from the cmd-line specified csv-file.
  void loadDimensions();
  // FRIEND_TEST(AggregationEnvironmentTest, loadDimensions);

  // collect information about available cubes
  void collectCubeInfo();

  // if there are multiple cubes, let the user select the cube to load
  void selectAndLoadCube();

  // ask the user for an ggregation query or a command
  void askQuery();

  Dimension* lookupDimension(IdentifierType identifier);
  Dimension* lookupDimensionByName(const string& name);

  // stoapMain uses this to decide whether the user should be asked or pipes should be used
  bool isServerMode() {
    return _serverMode;
  }

  // open a pipe to allow enable with other processes
  void openPipe();

  // stop asking the user or handling pipe input
  void setExit() {
    _exitRequested = true;
  }

  Cube* getCube() {
    return _cube;
  }

 private:
  // private constructor prevents object creation from the outside of the class
  AggrEnv();

  // prevent creation of an instance through copy constructor
  AggrEnv(const AggrEnv&);
  AggrEnv & operator =(const AggrEnv&);

  // print usage info and exit.
  void printUsageAndExit();

  // add comments and tests
  void addDimension(Dimension* dimension);

  // process aggregation query (used in user mode)
  bool processQuery(const string& query);

  // process aggregation request (used in server mode)
  string handleRequest(const string& request);

  // validate the number of arguments for a given command
  bool checkNumArguments(const string& cmd, int given, int expected);

  // print information about the cube
  void printCubeInfo();

  // print information about the dimensions
  void printDimensionInfo();

  // print information about the cube storage
  void printStorageInfo();

  // print the value of a cube cell
  void printCellValue(const string& path);

  // print the values of an area
  void printAreaValues(const string& path);

  // construct the area path from a string
  vector<IdentifiersType> getAreaPathFromString(const string& path);

  // construct the area path from a cell value request
  vector<IdentifiersType> getCellsFromUrl(const string& path);

  // construct the area path from a cell area request
  vector<IdentifiersType> getAreaPathFromUrl(const string& path);

  vector<IdentifiersType> dotPaths(const vector<IdentifiersType>& in);

  // database folder containing database and cube files
  const char* _databasePath;

  // number of dimensions
  size_t _numDimensions;

  // map holding the dimensions
  map<IdentifierType, Dimension*> _dimensions;

  // map holding the cube candidates
  map<IdentifierType, Cube*> _cubes;

  // cube holding the actually used cube
  Cube* _cube;
  IdentifierType _cubeId;

  // decides on whether we want to open a pipe or ask the user
  bool _serverMode;
  bool _exitRequested;
};

#endif  // STOAP_STOAP_AGGREGATIONENVIRONMENT_H_
