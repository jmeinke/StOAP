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
 * \author Frank Celler, triagens GmbH, Cologne, Germany
 * \author Achim Brandt, triagens GmbH, Cologne, Germany
 * \author Zurab Khadikov, Jedox AG, Freiburg, Germany
 * \author Christoffer Anselm, Jedox AG, Freiburg, Germany
 * \author Jiri Junek, qBicon s.r.o., Prague, Czech Republic
 * \author Ladislav Morva, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Jakl, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Dolezal, qBicon s.r.o., Hradec Kralove, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#ifndef STOAP_OLAP_H_
#define STOAP_OLAP_H_ 1

#include "./config.h"
#include "System/system-unix.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <boost/timer/timer.hpp>
#include <glog/logging.h>

#include <algorithm>
#include <deque>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>
#include <sstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <limits>
#include <iomanip>
#include <iterator>

using std::string;
using std::vector;
using std::endl;
using std::pair;
using std::map;
using std::unordered_map;
using std::cerr;
using std::cout;
using std::cin;
using std::ios;
using std::streamsize;
using std::streampos;
using std::stringstream;
using std::ostream;
using std::ofstream;
using std::set;
using std::map;
using std::deque;
using std::mem_fun;
using std::make_pair;
using std::max;
using std::min;
using std::stol;
using std::numeric_limits;
using std::setprecision;

using boost::timer::cpu_timer;
using boost::timer::auto_cpu_timer;

class Element;

////////////////////////////////////////////////////////////////////////////////
// defines
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief the FIFO files for test queries
////////////////////////////////////////////////////////////////////////////////

#define FIFO_IN "/tmp/stoap-in"
#define FIFO_OUT "/tmp/stoap-out"

////////////////////////////////////////////////////////////////////////////////
/// @brief no-identifier marker
////////////////////////////////////////////////////////////////////////////////

#define NO_IDENTIFIER ((IdentifierType) ~(uint32_t)0)

////////////////////////////////////////////////////////////////////////////////
/// @brief all-identifiers marker
////////////////////////////////////////////////////////////////////////////////

#define ALL_IDENTIFIERS ((IdentifierType)(NO_IDENTIFIER-1))

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief identifier
////////////////////////////////////////////////////////////////////////////////

typedef uint32_t IdentifierType;

////////////////////////////////////////////////////////////////////////////////
/// @brief depth
////////////////////////////////////////////////////////////////////////////////

typedef uint32_t DepthType;

////////////////////////////////////////////////////////////////////////////////
/// @brief indent
////////////////////////////////////////////////////////////////////////////////

typedef uint32_t IndentType;

////////////////////////////////////////////////////////////////////////////////
/// @brief level
////////////////////////////////////////////////////////////////////////////////

typedef uint32_t LevelType;

////////////////////////////////////////////////////////////////////////////////
/// @brief position
////////////////////////////////////////////////////////////////////////////////

typedef uint32_t PositionType;

////////////////////////////////////////////////////////////////////////////////
/// @brief path
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
/// @brief type for one element and weight
////////////////////////////////////////////////////////////////////////////////

typedef pair<Element*, double> ElementWeightType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for elements and weights
////////////////////////////////////////////////////////////////////////////////

typedef vector<ElementWeightType> ElementsWeightType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for elements and weights
////////////////////////////////////////////////////////////////////////////////

typedef map<Element*, double> ElementsWeightMap;

////////////////////////////////////////////////////////////////////////////////
/// @brief list of parents
////////////////////////////////////////////////////////////////////////////////

typedef ElementsType ParentsType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for identifiers and weights
////////////////////////////////////////////////////////////////////////////////

typedef map<IdentifierType, double> IdentifiersWeightMap;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for identifier and weight
////////////////////////////////////////////////////////////////////////////////

typedef pair<IdentifierType, double> IdentifierWeightType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for identifiers and weights
////////////////////////////////////////////////////////////////////////////////

typedef vector<IdentifierWeightType> IdentifiersWeightType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for path and weight
////////////////////////////////////////////////////////////////////////////////

typedef pair<vector<Element*>, double> PathWeightType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for optimized vector of bits
////////////////////////////////////////////////////////////////////////////////
typedef vector<bool> BitVector;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for filenames
////////////////////////////////////////////////////////////////////////////////

struct FileName {
  FileName()
      : path("."),
        name("ERROR"),
        extension("") {
  }

  FileName(const string& path, const string& name, const string& extension)
      : path(path),
        name(name),
        extension(extension) {
  }

  FileName(const FileName& old)
      : path(old.path),
        name(old.name),
        extension(old.extension) {
  }

  FileName(const FileName& old, const string& extension)
      : path(old.path),
        name(old.name),
        extension(extension) {
  }

  string fullPath() const {
    if (path.empty()) {
      if (extension.empty()) {
        return name;
      } else {
        return name + "." + extension;
      }
    } else if (extension.empty()) {
      return path + "/" + name;
    } else {
      return path + "/" + name + "." + extension;
    }
  }

  string path;
  string name;
  string extension;
};

// /////////////////////////////////////////////////////////////////////////////
// enumerations
// /////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief Element types
////////////////////////////////////////////////////////////////////////////////

enum ElementType {
  UNDEFINED = 0,
  NUMERIC = 1,
  STRING = 2,
  CONSOLIDATED = 4,
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
/// @brief type for filter base range
////////////////////////////////////////////////////////////////////////////////

struct BaseRangeType {
  IdentifierType low;
  IdentifierType high;

  struct BaseRangeFlagsType {
    BaseRangeFlagsType()
        : mergeNext(false),
          mergeNextWeight(false) {
    }

    uint8_t mergeNext :1;  // merges with next range if ignoring weight
    uint8_t mergeNextWeight :1;  // merges with next range even if weight matters
  } flags;

  BaseRangeType(IdentifierType low, IdentifierType high)
      : low(low),
        high(high),
        flags() {
  }

  BaseRangeType()
      : low(0),
        high(0),
        flags() {
  }

  // Attention! if this.low == y.low then the bigger area (greater .high) is returned as "less"
  bool operator<(const BaseRangeType& _yVal) const {
    return (this->low < _yVal.low
        || (this->low == _yVal.low && this->high > _yVal.high));
  }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief type for filter base ranges
////////////////////////////////////////////////////////////////////////////////

typedef vector<BaseRangeType> BaseRangesType;

////////////////////////////////////////////////////////////////////////////////
/// @brief type for base range
////////////////////////////////////////////////////////////////////////////////

struct BaseRangeWeightType : public BaseRangeType {
  double weight;

  BaseRangeWeightType()
      : BaseRangeType(),
        weight(0) {
  }

  BaseRangeWeightType(IdentifierType low, IdentifierType high, double weight)
      : BaseRangeType(low, high),
        weight(weight) {
  }
};

class IdHolder {
 public:
  IdHolder()
      : id(0) {
  }
  IdentifierType getNewId() {
    IdentifierType ret = id;
    id++;
    return ret;
  }
  void setStart(IdentifierType newid) {
    id = newid;
  }
  IdentifierType getLastId() {
    return id;
  }

 private:
  IdentifierType id;
};

////////////////////////////////////////////////////////////////////////////////
/// @brief type for base ranges
/// element = range
////////////////////////////////////////////////////////////////////////////////

typedef vector<BaseRangeWeightType> BaseRangesWeightType;

const IdentifierType NO_RULE = ~0;

class Cube;
class DimensionList;
class Dimension;
class Relations;
class WeightedSet;
class ElementPage;
class Lock;
class LockList;
class PaloSharedMutex;
struct CellValueContext;
struct ElementsContext;
struct FileName;
class CellValueStream;
class Area;
class CubeArea;
class Set;
class StorageCpu;
class StringStorageCpu;
class PageCollection;
class PathTranslator;
class AggregationMap;
typedef vector<AggregationMap> AggregationMaps;

/*
template<typename ValueType> class ICellMap;

typedef boost::shared_ptr<Cube> PCube;
typedef boost::shared_ptr<const Cube> CPCube;

typedef boost::shared_ptr<DimensionList> PDimensionList;
typedef boost::shared_ptr<const DimensionList> CPDimensionList;

typedef boost::shared_ptr<Dimension> PDimension;
typedef boost::shared_ptr<const Dimension> CPDimension;

typedef boost::shared_ptr<Relations> PRelations;
typedef boost::shared_ptr<const Relations> CPRelations;

typedef boost::shared_ptr<WeightedSet> PWeightedSet;
typedef boost::shared_ptr<const WeightedSet> CPWeightedSet;

typedef boost::shared_ptr<ElementPage> PElementPage;
typedef boost::shared_ptr<const ElementPage> CPElementPage;

typedef boost::shared_ptr<IdHolder> PIdHolder;

typedef boost::shared_ptr<Lock> PLock;
typedef boost::shared_ptr<const Lock> CPLock;

typedef boost::shared_ptr<LockList> PLockList;
typedef boost::shared_ptr<const LockList> CPLockList;

typedef boost::shared_ptr<PaloSharedMutex> PSharedMutex;
typedef boost::shared_ptr<const PaloSharedMutex> CPSharedMutex;

typedef boost::shared_ptr<CellValueContext> PCellValueContext;
typedef boost::shared_ptr<const CellValueContext> CPCellValueContext;

typedef boost::shared_ptr<ElementsContext> PElementsContext;
typedef boost::shared_ptr<const ElementsContext> CPElementsContext;

typedef boost::shared_ptr<FileName> PFileName;
typedef boost::shared_ptr<const FileName> CPFileName;

typedef boost::shared_ptr<CellValueStream> PCellStream;
typedef boost::shared_ptr<const CellValueStream> CPCellStream;

typedef boost::shared_ptr<Area> PArea;
typedef boost::shared_ptr<const Area> CPArea;

typedef boost::shared_ptr<vector<IdentifiersType> > PPaths;
typedef boost::shared_ptr<const vector<IdentifiersType> > CPPaths;

typedef boost::shared_ptr<CubeArea> PCubeArea;
typedef boost::shared_ptr<const CubeArea> CPCubeArea;

typedef boost::shared_ptr<Set> PSet;
typedef boost::shared_ptr<const Set> CPSet;

typedef boost::shared_ptr<StorageCpu> PStorageCpu;
typedef boost::shared_ptr<const StorageCpu> CPStorageCpu;

typedef boost::shared_ptr<StringStorageCpu> PStringStorageCpu;
typedef boost::shared_ptr<const StringStorageCpu> CPStringStorageCpu;

typedef boost::shared_ptr<PageCollection> PPageCollection;
typedef boost::shared_ptr<const PageCollection> CPPageCollection;

typedef boost::shared_ptr<ICellMap<double> > PDoubleCellMap;
typedef boost::shared_ptr<const ICellMap<double> > CPDoubleCellMap;

typedef boost::shared_ptr<PathTranslator> PPathTranslator;
typedef boost::shared_ptr<const PathTranslator> CPPathTranslator;

typedef boost::shared_ptr<AggregationMaps> PAggregationMaps;
typedef boost::shared_ptr<const AggregationMaps> CPAggregationMaps;
*/

#endif  // STOAP_OLAP_H_
