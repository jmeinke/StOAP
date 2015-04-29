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

#include "Stoap/AggregationEnvironment.h"

#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>

#include "Collections/StringUtils.h"
#include "Collections/DeleteObject.h"
#include "Collections/StringBuffer.h"
#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "InputOutput/FileUtils.h"
#include "Stoap/AggregationProcessor.h"
#include "Exceptions/FileFormatException.h"
#include "Exceptions/FileOpenException.h"
#include "Exceptions/ParameterException.h"

// Constructor without arguments
AggrEnv::AggrEnv() {
  _serverMode = false;
  _exitRequested = false;
  _databasePath = NULL;
  _cube = NULL;
  _cubeId = 0;
  _numDimensions = 0;
}

// Parse the command line arguments.
void AggrEnv::parseCommandLineArguments(int argc, char** argv) {
  struct option options[] = { { "server-mode", 0, NULL, 's' }, { "log-level", 1,
      NULL, 'v' }, { NULL, 0, NULL, 0 } };

  optind = 1;
  while (true) {
    char c = getopt_long(argc, argv, "v:s", options, NULL);
    if (c == -1)
      break;
    switch (c) {
      case 'v': {
          try {
            int level = std::stoi(string(optarg));
            if (level < 0 || level > 4) level = 0;
            FLAGS_minloglevel = level;
          } catch (const std::invalid_argument& ia) {
            cerr << "Invalid log-level: " << optarg << '\n';
            printUsageAndExit();
          }
        }
        break;
      case 's':
        _serverMode = true;
        break;
      default:
        printUsageAndExit();
    }
  }
  if (optind + 1 != argc) {
    printUsageAndExit();
  }
  cout << "Log-level: " << FLAGS_minloglevel << endl;
  _databasePath = argv[optind];
  if (_serverMode) {
    cout << "Server mode: enabled" << endl;
  } else {
    cout << "Server mode: disabled" << endl;
  }
  cout << "Database path: " << _databasePath << endl;
  cout << endl;
}

// Print the usage and exit
void AggrEnv::printUsageAndExit() {
  cerr << "" << endl;
  cerr << "Usage: ./stoapMain [options] <database-path>" << endl;
  cerr << "<database-path> path to a folder containing database and cube files."
       << endl;
  cerr << "Options:" << endl;
  cerr << " -v, --log-level: log-level can be [0|1|2|3|4]."
       << endl;
  cerr
      << " -s, --server-mode: if specified, an input and output FIFO file will be created" << endl
      << "                    in /tmp/stoap-in and /tmp/stoap-out. All logger output goes to" << endl
      << "                    a log file called 'StOAP.INFO'." << endl;
  exit(1);
}

// Read dimensions from the cmd-line specified csv-file.
void AggrEnv::loadDimensions() {
  LOG(INFO) << "Loading dimensions from '" << string(_databasePath) << "'.";

  // check we have parsed the arguments successfully
  if (_databasePath == NULL) {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "database path not set");
  }

  // set the database filename and check if the file can be read
  FileName dbFileName(string(_databasePath), "database", "csv");
  if (!FileUtils::isReadable(dbFileName)) {
    LOG(FATAL) << "database-file '" << dbFileName.fullPath()
                  << "' is not readable.";
  }

  // open the file
  FileReader* file(FileReader::getFileReader(dbFileName));
  file->openFile(true, false);

  // load the database section
  size_t sizeDimensions = 0;
  if (file->isSectionLine() && file->getSection() == "DATABASE") {
    file->nextLine();

    if (file->isDataLine()) {
      sizeDimensions = file->getDataInteger(0);
      file->nextLine();
    }
  }

  // clear the _dimensions vector
  _dimensions.clear();

  try {
    // load the dimension section
    if (file->isSectionLine() && file->getSection() == "DIMENSIONS") {
      file->nextLine();

      while (file->isDataLine()) {
        size_t identifier = file->getDataInteger(0);
        string name = file->getDataString(1);

        if (identifier >= sizeDimensions) {
          LOG(ERROR) << "dimension identifier '" << identifier
                        << "' of dimension '" << name
                        << "' is greater or equal than maximum ("
                        << sizeDimensions << ")";
          throw FileFormatException("wrong identifier for dimension", file);
        }

        size_t type = file->getDataInteger(2);
        if (type == 1) {  // Normal dimension
          LOG(INFO) << "Added dimension '" << name << "' (id " << identifier << ").";

          Dimension* dimension = new Dimension(identifier, name);
          addDimension(dimension);

        } else {
          DLOG(WARNING) << "Skipped '" << name << "' (type " << type << ").";
        }
        file->nextLine();
      }
    } else {
      throw FileFormatException("section 'DIMENSIONS' not found", file);
    }


    // load dimension data into memory and update dimPos and dimMask
    uint32_t dimPos = 0;
    for (auto i = _dimensions.begin(); i != _dimensions.end(); i++) {
      Dimension* dimension = i->second;

      if (dimension != 0) {
        dimension->loadDimension(file);
      }

      // we support only on bin for the path at the moment
      if (dimPos + dimension->dimPos >= 64) {
        throw ErrorException(ErrorException::ERROR_INTERNAL,
             "Sorry, your cube dimension is too big. "
             "Currently StOAP supports only dimension paths which fit into 64bit.");
      }

      // set correct dimPos
      if (dimPos == 0) {
        dimPos = dimension->dimPos;
        dimension->dimPos = 0;
      } else {
        uint32_t tmp = dimension->dimPos;
        dimension->dimPos = dimPos;
        dimPos += tmp;
      }

      // shift the dimension bitmask
      for (uint32_t i = 0; i < dimension->dimPos; ++i)
        dimension->dimMask = dimension->dimMask << 1;
    }
  } catch (const FileFormatException& e) {
    LOG(ERROR) << e.getMessage();
  }

  delete file;
}

void AggrEnv::addDimension(Dimension* dimension) {
  const string& name = dimension->getName();

  if (lookupDimensionByName(name) != 0) {
    throw ParameterException(ErrorException::ERROR_DIMENSION_NAME_IN_USE,
                             "dimension name is already in use", "name", name);
  }

  IdentifierType identifier = dimension->getIdentifier();
  if (lookupDimension(identifier) != 0) {
    throw ParameterException(ErrorException::ERROR_INTERNAL,
                             "dimension identifier is already in use",
                             "identifier", static_cast<int>(identifier));
  }

  // add dimension to mapping
  _dimensions[identifier] = dimension;

  // we have one more dimension
  _numDimensions++;
}

Dimension* AggrEnv::lookupDimensionByName(const string& name) {
  for (auto i = _dimensions.begin(); i != _dimensions.end(); i++) {
    if (i->second->getName() == name)
      return i->second;
  }
  return 0;
}

Dimension* AggrEnv::lookupDimension(IdentifierType identifier) {
  try {
    return _dimensions.at(identifier);
  } catch (const std::out_of_range& oor) {
    return 0;
  }
}

// Read info about available cubes from the database file
void AggrEnv::collectCubeInfo() {
  LOG(INFO) << "Collecting cube information.";

  if (_databasePath == NULL) {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "database path name not set.");
  }

  if (_dimensions.empty() || _numDimensions == 0) {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "dimensions vector is empty.");
  }

  // set the database filename and check if the file can be read
  FileName dbFileName(string(_databasePath), "database", "csv");
  if (!FileUtils::isReadable(dbFileName)) {
    LOG(FATAL) << "Database-file '" << dbFileName.fullPath()
                  << "' is not readable.";
  }

  _cubes.clear();

  // open the file
  FileReader* file(FileReader::getFileReader(dbFileName));
  file->openFile(true, false);

  // find the cube section
  while (!file->isSectionLine() || file->getSection() != "CUBES") {
    file->nextLine();
  }
  file->nextLine();
  while (file->isDataLine()) {
    // check which cubes we can use
    string name = file->getDataString(1);
    int32_t identifier = file->getDataInteger(0);
    const vector<int> dids = file->getDataIntegers(2);
    int type = file->getDataInteger(3);

    if (type == 2 || type == 7) {
      DLOG(WARNING) << "Testing dimensions of cube '" << name << "' (id: "
                    << identifier << "):";
      // retrieve the dimensions used by the cube
      bool cubeIsUsable = true;
      vector<Dimension*> dims;
      for (vector<int>::const_iterator i = dids.begin(); i != dids.end(); i++) {
        // check if the used dimension has been loaded
        if (lookupDimension(*i) == 0) {
          cubeIsUsable = false;
          break;
        }
        dims.push_back(lookupDimension(*i));
        DLOG(WARNING) << "Dimension available: "
                      << lookupDimension(*i)->getName();
      }

      if (cubeIsUsable) {
        const size_t kIdBufferSize = 255;
        char idBuffer[kIdBufferSize];
        snprintf(idBuffer, kIdBufferSize, "%d", identifier);
        string file("database_CUBE_");
        file.append(idBuffer);

        FileName cubeFileName(string(_databasePath), file, "csv");
        LOG(INFO) << "Adding cube with filename '" << cubeFileName.fullPath()
                     << "'.";
        Cube* tmpCube = new Cube(name, cubeFileName, &dims);
        _cubes[identifier] = tmpCube;
      } else {
        LOG(INFO)<< "One or more dimensions have not been found. Proceeding..."
            << endl;
      }
    } else {
      DLOG(WARNING) << "Skipping cube '" << name << "' (id: " << identifier
                    << ") - type: " << type << ".";
    }
    file->nextLine();
  }

  delete file;
}

void AggrEnv::selectAndLoadCube() {
  // special case: no cubes found
  if (_cubes.empty()) {
    LOG(FATAL)
        << "There are no usable cubes available. Please check the database file."
        << endl;
  }

  // special case: only one usable cube exists or server mode is enabled
  if (_cubes.size() == 1 || isServerMode()) {
    if (isServerMode()) {
      // we're in server mode -> select the first usable cube
      LOG(ERROR) << "Server mode is enabled. The first usable cube will be selected.";
    }

    // assign the first cube
    _cubeId = _cubes.begin()->first;
    _cube = _cubes.begin()->second;

  } else {
    LOG(ERROR) << "There are multiple cubes.";
    // there are at least 2 usable cubes, let the user choose
    for (auto it = _cubes.begin(); it != _cubes.end(); it++) {
      cout << "  " << it->first << ") " << it->second->getName() << endl;
    }
    cout << "Please choose a cube and enter it's id: ";
    IdentifierType x;
    cin >> x;
    while (cin.fail() || _cubes.find(x) == _cubes.end()) {
      cin.clear();
      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      cout << "Enter a valid cube id: ";
      cin >> x;
    }
    // assign the cube
    _cubeId = x;
    _cube = _cubes.find(x)->second;
  }
  // load the cube
  LOG(INFO) << "Loading cube '" << _cube->getName() << "'.";
  _cube->loadCube();

  LOG(INFO) << "Loaded " << _cube->sizeFilledCells() << " base cells into '"
               << _cube->getName() << "'.";
}

// Open pipes for input and output
void AggrEnv::openPipe() {
  LOG(INFO) << "Opening FIFO file '" << FIFO_IN << "' for reading.";

  unlink(FIFO_IN);
  unlink(FIFO_OUT);

  FILE *fpin;
  FILE *fpout;
  const int kBufferSize = 100000;
  char readbuf[kBufferSize];

  umask(0);
  mknod(FIFO_IN, S_IFIFO | 0666, 0);
  mknod(FIFO_OUT, S_IFIFO | 0666, 0);

  while (!_exitRequested) {
    // opening the FIFO read-only blocks until some other process opens the FIFO for writing.
    LOG(INFO) << "Waiting for a query...";
    if ((fpin = fopen(FIFO_IN, "r")) == NULL) {
      LOG(FATAL) << "fopen fifo in failed.";
    } else {
      LOG(INFO) << "Client is available.";

      if (fgets(readbuf, kBufferSize, fpin) == NULL) {
        LOG(FATAL) << "fgets fifo in failed.";
      }

      string query(readbuf);
      LOG(INFO) << "Received query '" << query << "'.";

      // opening the FIFO write-only blocks until some other process opens the FIFO for reading.
      if ((fpout = fopen(FIFO_OUT, "w")) == NULL) {
        LOG(FATAL) << "fopen fifo out failed.";
      } else {
        LOG(INFO) << "Reader is available.";

        string answer = handleRequest(query);

        // LOG(INFO) << "Answer is:\n" << answer;
        fputs(answer.c_str(), fpout);
        if (ferror(fpout)) {
          LOG(FATAL) << "fputs fifo out failed.";
        }
        // signal EOF to the reader
        fclose(fpout);

        LOG(INFO) << "Reader has picked up the answer.";
      }
      fclose(fpin);
    }
  }
}

string AggrEnv::handleRequest(const string& request) {
  string result;

  vector<string> reqWords;
  StringUtils::splitString(request, &reqWords, '?');

  // there should always be a request and parameters
  if (reqWords.size() != 2) {
    result =
        "Error: wrong request format. Should be of the form '/request?param1=value&param2=value&..'.";
    return result;
  }

  // the first part of the query is the request
  string req = reqWords[0];

  // there are only two supported requests: /cell/values and /cell/area
  if (req != "/cell/values" && req != "/cell/area") {
    stringstream ss;
    ss << req
       << ": Unsupported request. Only /cell/values and /cell/area are supported."
       << endl;
    return ss.str();
  }

  // the second part are the parameters (param=value&param2=value2&...)
  vector<string> reqKeyValuePairs;
  StringUtils::splitString(reqWords[1], &reqKeyValuePairs, '&');

  // now literally "map" every key and value
  map<string, string> params;
  for (auto kvp = reqKeyValuePairs.begin(); kvp != reqKeyValuePairs.end();
      ++kvp) {
    vector<string> keyValuePair;
    StringUtils::splitString(*kvp, &keyValuePair, '=');
    if (keyValuePair.size() != 2) {
      // wrong format -> skip
      continue;
    }
    params[keyValuePair[0]] = keyValuePair[1];
  }

  if (params.find("cube") != params.end()) {
    IdentifierType cubeId = static_cast<IdentifierType>(stol(params.at("cube")));
    if (_cubeId != cubeId) {
      stringstream ss;
      ss << "Error: the parameter 'cubeId' (" << cubeId
         << ") does not point to the currently loaded cube (Id " << _cubeId
         << ").";
      return ss.str();
    }
  }

  if (req == "/cell/values") {
    if (params.find("paths") == params.end()) {
      result = "Error: the parameter 'paths' is required for /cell/values.";
      return result;
    } else if (params.at("paths").empty()) {
      result = "Error: the parameter 'paths' is empty.";
      return result;
    }

    try {
      stringstream ss;
      vector<IdentifiersType> cellPaths = getCellsFromUrl(params["paths"]);

      vector<IdentifiersType> areaPaths(cellPaths.at(0).size(), IdentifiersType());
      for (size_t path = 0; path < cellPaths.size(); ++path) {
        for (size_t dim = 0; dim < cellPaths.at(path).size(); ++dim) {
          IdentifierType el = cellPaths.at(path).at(dim);
          if (find(areaPaths.at(dim).begin(), areaPaths.at(dim).end(), el) != areaPaths.at(dim).end())
             continue;
          areaPaths.at(dim).push_back(el);
        }
      }

      CubeArea queryArea(this, &(*_cube), Area(areaPaths));
      AggregationProcessor aggrProc(&queryArea, AggregationProcessor::SUM);
      aggrProc.aggregate();
      ss << aggrProc.result(cellPaths, false, true);

      return ss.str();

    } catch (const ErrorException& e) {
      stringstream ss;
      ss << "Error in cell path: " << e.getMessage() << endl;
      return ss.str();
    }

  } else if (req == "/cell/area") {
    if (params.find("area") == params.end()) {
      result = "Error: the parameter 'area' is required for /cell/area.";
      return result;
    } else if (params.at("area").empty()) {
      result = "Error: the parameter 'area' is empty.";
      return result;
    }

    try {
      stringstream ss;
      vector<IdentifiersType> cellArea = getAreaPathFromUrl(params["area"]);
      vector<IdentifiersType> paths = dotPaths(cellArea);

      CubeArea queryArea(this, &(*_cube), cellArea);
      AggregationProcessor aggrProc(&queryArea, AggregationProcessor::SUM);
      aggrProc.aggregate();
      ss << aggrProc.result(paths, true, true);
      return ss.str();

    } catch (const ErrorException& e) {
      stringstream ss;
      ss << "Error: " << e.getMessage() << endl;
      return ss.str();
    }
  }

  return result;
}

void AggrEnv::askQuery() {
  cout << "Enter 'help' for a list of commands. Enter 'exit' to exit the program." << endl << endl;
  const size_t querySize = 2000;
  // char query[querySize];
  string query;
  bool jump = false;

  cin.clear();
  //cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  while (!_exitRequested) {
    if (!jump) {
      cout << "Please enter your query: " << endl << "> ";
    } else {
      cout << "> ";
      jump = false;
    }

    getline(cin, query);  // , '\n'
    if (query.empty()) {
      jump = true;
      continue;
    }
    if (cin.fail() || query.size() > querySize) {
      cout << "Error: the input can only consist of less than " << querySize
           << " characters." << endl;
      cin.clear();
      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if (!processQuery(query))
      break;

    cout << endl;
  }
}

bool AggrEnv::processQuery(const string& query) {
  // ideas for more commands
  // - save cube data to file

  vector<string> queryWords;
  StringUtils::splitString(query, &queryWords, ' ');

  // there should be at least one query word!
  assert(!queryWords.empty());

  // the first word of the query is the command
  string cmd = queryWords[0];
  int numArgs = queryWords.size() - 1;
  if (cmd == "exit") {
    return false;
  } else if (cmd == "info") {
    if (!checkNumArguments(cmd, numArgs, 1))
      return true;
    if (queryWords[1] == "dimensions") {
      printDimensionInfo();
    } else if (queryWords[1] == "cube") {
      printCubeInfo();
    } else if (queryWords[1] == "storage") {
      printStorageInfo();
    } else {
      cout << "error: unkown option " << queryWords[1] << " for info" << endl;
    }
  } else if (cmd == "getCell") {
    if (!checkNumArguments(cmd, numArgs, 1))
      return true;
    printCellValue(queryWords[1]);
  } else if (cmd == "getArea") {
    if (!checkNumArguments(cmd, numArgs, 1))
      return true;
    printAreaValues(queryWords[1]);
  } else if (cmd == "help") {
    cout << "Available commands:" << endl;
    cout << "\texit" << endl;
    cout << "\tgetCell e1,e2,e3,e4,e5,...,en with <en> being element ids."
         << endl;
    cout << "\t\texample: getCell 2,0,2,6,32" << endl;
    cout
        << "\tgetArea {(r1)x(r2)x...x(rn)} with <rn> being ranges of element ids."
        << endl;
    cout << "\t\texamples:" << endl;
    cout << "\t\t- getArea 2x0x2-4x5,8-12x7-11" << endl;
    cout << "\t\t- getArea 10x14x5x13x0-18,20-63" << endl;
    cout << "\t\t- getArea 10-14x14x5x13-24x62-64" << endl;
    cout << "\t\t- getArea 13x14x5x19x33-64" << endl;
    cout << "\tinfo <cube|dimensions|storage>" << endl;
    cout << "\thelp" << endl;
  } else {
    cout << cmd << ": unknown command" << endl;
  }

  return true;
}

bool AggrEnv::checkNumArguments(const string& cmd, int given, int expected) {
  if (given < expected) {
    if (expected == 1) {
      cout << "Error: please provide an argument for " << cmd << endl;
    } else {
      cout << "Error: please provide " << expected << " arguments for " << cmd
           << endl;
    }
    return false;
  } else if (given > expected) {
    cout << "Error: too many arguments for " << cmd << endl;
    return false;
  }
  return true;
}

void AggrEnv::printCubeInfo() {
  cout << "===================================================================="
       << endl;
  cout << "Name:\t\t" << _cube->getName() << endl;
  cout << "Filled cells:\t" << _cube->sizeFilledCells() << endl;
  cout << "Max cells:\t" << _cube->sizeMaxCells() << endl;
  cout << "Used dimensions (ordered by position in key):" << endl;
  cout << "" << endl;
  vector<Dimension*> cubeDimensions = *_cube->getDimensions();
  for (auto i = cubeDimensions.begin(); i != cubeDimensions.end(); i++) {
    Dimension* dim = *i;
    cout << "\tDimension '" << dim->getName() << "' with Id "
         << dim->getIdentifier() << ":" << endl;
    size_t numEl = dim->sizeElements();
    size_t numBase = dim->sizeElements(true);
    size_t numCon = numEl - numBase;
    cout << "\t\tElements: " << numEl << " (" << numBase << " base, " << numCon
         << " consolidated)" << endl;
    cout << "" << endl;
  }
  cout << "===================================================================="
       << endl;
}

void AggrEnv::printDimensionInfo() {
  cout << "===================================================================="
       << endl;
  cout << "There are " << _dimensions.size() << " dimensions:" << endl;
  for (auto i = _dimensions.begin(); i != _dimensions.end(); i++) {
    Dimension* dim = i->second;
    cout << "\tDimension '" << dim->getName() << "' with Id "
         << dim->getIdentifier() << ":" << endl;
    size_t numEl = dim->sizeElements();
    size_t numBase = dim->sizeElements(true);
    size_t numCon = numEl - numBase;
    cout << "\t\tElements: " << numEl << " (" << numBase << " base, " << numCon
         << " consolidated)" << endl;
    cout << "\t\tMax-Depth: " << dim->getDepth() << endl;
    // Why would the following be relevant?
    // cout << "\t\tMemory usage: " << dim->getMemoryUsageStorage() << " bytes" << endl;
    cout << "" << endl;
  }
  cout << "===================================================================="
       << endl;
}

void AggrEnv::printStorageInfo() {
  DoubleStorage* storage = _cube->getStorage();
  cout << "===================================================================="
       << endl;
  cout << "Size:\t\t\t" << storage->m.size() << endl;
  cout << "Maximum size:\t\t" << storage->m.max_size() << endl;

  size_t collisions = 0;
  for (unsigned i = 0; i < storage->m.bucket_count(); ++i) {
    if (storage->m.bucket_size(i) > 1)
      collisions += (storage->m.bucket_size(i) - 1);
  }

  cout << "Bucket count:\t\t" << storage->m.bucket_count() << " (" << collisions
       << " collisions)" << endl;
  cout << "Bucket count (max):\t" << storage->m.max_bucket_count() << endl;
  cout << "Load factor:\t\t" << storage->m.load_factor() << endl;
  cout << "Load factor (max):\t" << storage->m.max_load_factor() << endl;

  cpu_timer t;
  size_t sizeCount = 0;
  for (auto srcIt = storage->m.begin(); srcIt != storage->m.end(); ++srcIt) {
    ++sizeCount;
  }
  t.stop();
  cout << "Full storage iteration time: " << t.format() << endl;
  cout << "===================================================================="
       << endl;
}

void AggrEnv::printCellValue(const string& path) {
  cout << "===================================================================="
       << endl;

  IdentifiersType ids;
  // TODO(jmeinke): fix segmentation fault if an element does not exist
  // split the path by commas and add the ids
  vector<string> strIds;
  StringUtils::splitString(path, &strIds, ',');
  for (auto it = strIds.begin(); it < strIds.end(); ++it) {
    try {
      ids.push_back(static_cast<IdentifierType>(stol(*it)));
    } catch (const std::exception& e) {
      cout << "Error in cell path '" << path << "'" << endl;
      return;
    }
  }

  try {
    CellPath cp(&ids);
    cout << "CellPath is: " << cp.toString() << endl;

    /* cout << "Elements are: " << endl;

    const vector<Dimension*> cubeDimensions = *(_cube->getDimensions());
    const PathType* pathElements = cp.getPathElements();

    for (size_t cDim = 0; cDim < cubeDimensions.size(); ++cDim) {
      Element* pEl = (*pathElements)[cDim];
      cout << "\tIn '" << cubeDimensions[cDim]->getName() << "':" << endl;
      cout << "\t -> Element '" << pEl->getName() << "' (Id "
           << pEl->getIdentifier() << ", type " << pEl->getElementType() << ")."
           << endl;
      ElementsWeightType children = cubeDimensions[cDim]->getChildren(pEl);
      if (children.size() > 0) {
        cout << "\t -> Children:" << endl;
        for (auto chIt = children.begin(); chIt != children.end(); ++chIt) {
          Element* chEl = (*chIt).first;
          double weight = (*chIt).second;
          cout << "\t\t -> Element '" << chEl->getName() << "' (Id "
               << chEl->getIdentifier() << ", type " << chEl->getElementType()
               << ", weight " << weight << ")." << endl;
        }
      }
    } */

    CubeArea queryCell(this, _cube, ids);
    AggregationProcessor aggrProc(&queryCell, AggregationProcessor::SUM);
    aggrProc.aggregate();
    aggrProc.print();
  } catch (const ErrorException& e) {
    cout << "Error: " << e.getMessage() << endl;
    return;
  }

  cout << "===================================================================="
       << endl;
}

void AggrEnv::printAreaValues(const string& path) {
  vector<IdentifiersType> areaPath;

  try {
    areaPath = getAreaPathFromString(path);
  } catch (const ErrorException& e) {
    cout << "Error in cell path:" << endl;
    cout << "\t" << e.getMessage() << endl;
    return;
  }

  CubeArea queryArea(this, _cube, areaPath);
  cout << "===================================================================="
       << endl;
  cout << "Printing values for area " << queryArea.toString() << endl;
  cout << "Size: " << queryArea.getSize() << endl;
  cout << "===================================================================="
       << endl;

  auto_cpu_timer timer(
      "Aggregation time: %ws wall, %us user + %ss system = %ts CPU (%p%)\n");

  AggregationProcessor aggrProc(&queryArea, AggregationProcessor::SUM);
  aggrProc.aggregate();
  aggrProc.print();
}

// Computes the areaPath from a given string
vector<IdentifiersType> AggrEnv::getAreaPathFromString(const string& nPath) {
  vector<IdentifiersType> result;

  string path(nPath);

  // strip some chars to allow different input forms
  char charsToStrip[] = "(){}";
  for (unsigned int i = 0; i < strlen(charsToStrip); ++i) {
    path.erase(std::remove(path.begin(), path.end(), charsToStrip[i]),
               path.end());
  }

  // split the path by x
  vector<string> strElements;
  StringUtils::splitString(path, &strElements, 'x');

  uint32_t dimNum = 0;
  const vector<Dimension*> cubeDimensions = *(_cube->getDimensions());
  if (cubeDimensions.size() != strElements.size()) {
    throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                         "wrong number of area elements");
  }
  for (auto el = strElements.begin(); el < strElements.end(); ++el, ++dimNum) {
    Dimension* dim = cubeDimensions[dimNum];
    IdentifiersType ids;

    // split the path by comma
    vector<string> strRanges;
    StringUtils::splitString(*el, &strRanges, ',');
    for (auto rg = strRanges.begin(); rg < strRanges.end(); ++rg) {
      // split the rest
      vector<string> strLowHi;
      StringUtils::splitString(*rg, &strLowHi, '-');
      if (strLowHi.size() == 1) {
        // this should be a single element id, not a range
        if (strLowHi[0].empty()) {
          throw ErrorException(
              ErrorException::ERROR_INVALID_COORDINATES,
              "wrong range formatting (no value before or after separator)");
        }
        IdentifierType elemId = static_cast<IdentifierType>(stol(strLowHi[0]));
        if (dim->lookupElement(elemId) == 0)
          continue;
        ids.push_back(elemId);
      } else if (strLowHi.size() == 2) {
        // this should be a range
        if (strLowHi[0].empty() || strLowHi[1].empty()) {
          throw ErrorException(
              ErrorException::ERROR_INVALID_COORDINATES,
              "wrong range formatting (no value before or after separator)");
        }
        IdentifierType low = stol(strLowHi[0]);
        IdentifierType high = stol(strLowHi[1]);
        if (low < high) {
          for (IdentifierType elId = low; elId <= high; ++elId) {
            if (dim->lookupElement(elId) == 0)
              continue;
            ids.push_back(elId);
          }
        } else if (low == high) {
          if (dim->lookupElement(low) == 0)
            continue;
          ids.push_back(low);
        } else {
          // low is greater then high
          throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                               "wrong range formatting (low > high)");
        }

      } else {
        // everything else is wrong
        throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                             "wrong range formatting (too many separators)");
      }
    }

    // now add the range of element ids to our areaPath
    result.push_back(ids);
  }

  return result;
}

// Computes the areaPath from a cell request
vector<IdentifiersType> AggrEnv::getCellsFromUrl(const string& path) {
  // one or more paths can be given like the following:
  // 0,5,0,2,8,2:0,5,0,3,8,2:...:...

  vector<IdentifiersType> result;

  // split the url parameter into cell paths by :
  vector<string> cellPaths;
  StringUtils::splitString(path, &cellPaths, ':');

  const vector<Dimension*> cubeDimensions = *(_cube->getDimensions());

  for (auto path = cellPaths.begin(); path < cellPaths.end(); ++path) {
    vector<string> elemIds;
    StringUtils::splitString(*path, &elemIds, ',');

    if (cubeDimensions.size() != elemIds.size()) {
      throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                           "wrong number of elements in requested cellpath");
    }

    IdentifiersType ids;
    uint32_t dimNum = 0;
    for (auto el = elemIds.begin(); el < elemIds.end(); ++el, ++dimNum) {
      Dimension* dim = cubeDimensions[dimNum];

      IdentifierType elemId = static_cast<IdentifierType>(stol(*el));
      if (dim->lookupElement(elemId) == 0) {
        std::ostringstream stringStream;
        stringStream << "requested element " << elemId <<
            " does not exist in dimension " << (dimNum + 1);
        throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                             stringStream.str());
      }
      ids.push_back(elemId);
    }

    result.push_back(ids);
  }

  return result;
}

// Computes the areaPath from a cell request
vector<IdentifiersType> AggrEnv::getAreaPathFromUrl(const string& path) {
  // an area can be given like the following:
  // el1:el2:...:el3 from dim1,el1:el2:...:eln from dim2,... 
  // example: 0,0,0,0:1:2:35:79:325,0:1:2:20:43:65:433,0,8,0

  vector<IdentifiersType> result;

  // split the url parameter into dimension-elements by ,
  vector<string> elemPaths;
  StringUtils::splitString(path, &elemPaths, ',');

  const vector<Dimension*> cubeDimensions = *(_cube->getDimensions());
  if (cubeDimensions.size() != elemPaths.size()) {
    throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                         "wrong number of dimensions in requested areapath");
  }

  uint32_t dimNum = 0;
  for (auto dimElements = elemPaths.begin(); dimElements < elemPaths.end();
      ++dimElements, ++dimNum) {
    if ((*dimElements).empty()) {
      throw ErrorException(
          ErrorException::ERROR_INVALID_COORDINATES,
          "no element ids for at least one dimension in requested areapath");
    }

    Dimension* dim = cubeDimensions[dimNum];
    vector<string> elemIds;
    StringUtils::splitString(*dimElements, &elemIds, ':');

    IdentifiersType ids;
    for (auto el = elemIds.begin(); el < elemIds.end(); ++el) {
      IdentifierType elemId = static_cast<IdentifierType>(stol(*el));
      if (dim->lookupElement(elemId) == 0) {
        std::ostringstream stringStream;
        stringStream << "requested element " << elemId << " does not exist";
        throw ErrorException(ErrorException::ERROR_INVALID_COORDINATES,
                             stringStream.str());
        // continue;
      }
      ids.push_back(elemId);
    }

    result.push_back(ids);
  }

  return result;
}

// get the cell path combinations from the area elements
vector<IdentifiersType> AggrEnv::dotPaths(const vector<IdentifiersType>& in) {
  vector<IdentifiersType> result;

  vector<size_t> counter(in.size(), 0);
  vector<size_t> sizes;

  for(auto it = in.begin(); it != in.end(); ++it) {
    assert(it->size() > 0);
    sizes.push_back(it->size()-1);
  }

  while (counter != sizes) {
    // this is the block adding the IdentifiersType with counter offsets
    IdentifiersType tmp;
    for(size_t dimN = 0; dimN < in.size(); ++dimN) {
      tmp.push_back(in.at(dimN).at(counter[dimN]));
    }
    result.push_back(tmp);

    for(size_t dimN = 0; dimN < counter.size(); ++dimN) {
      ++counter[dimN];
      if (counter[dimN] > sizes[dimN]) {
        counter[dimN] = 0;
      } else break;
    }
  }

  IdentifiersType tmp;
  for(size_t dimN = 0; dimN < in.size(); ++dimN) {
    tmp.push_back(in.at(dimN).at(counter[dimN]));
  }
  result.push_back(tmp);

  return result;
}

// Destructor
AggrEnv::~AggrEnv() {
  for (auto i = _dimensions.begin(); i != _dimensions.end(); i++) {
    delete i->second;
  }
  _dimensions.clear();
  _cubes.clear();
  delete _cube;
}
