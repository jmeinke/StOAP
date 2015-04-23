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
 * \author Jiri Junek, qBicon s.r.o., Prague, Czech Republic
 * \author Ladislav Morva, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Jakl, qBicon s.r.o., Prague, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#ifndef STOAP_OLAP_AREA_H_
#define STOAP_OLAP_AREA_H_ 1

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Olap.h"
#include "Collections/WeightedSet.h"
#include "Stoap/AggregationEnvironment.h"
#include "Engine/AggregationMap.h"

class AggrEnv;

class Area {
 public:
  typedef Set::Iterator ConstElemIter;

  class PathIterator {
   public:
    PathIterator(const Area &area, bool end, const vector<ConstElemIter> &path);
    PathIterator(const Area &area, bool end, const IdentifiersType &path);
    PathIterator();
    PathIterator(const PathIterator &it);
    PathIterator &operator++();
    const IdentifiersType &operator*() const;
    bool operator==(const PathIterator &it) const;
    bool operator!=(const PathIterator &it) const;
    bool operator!=(const IdentifiersType &key) const;
    PathIterator operator+(double count) const;
    double operator-(const PathIterator &it) const;
    string toString() const;
    IdentifierType at(size_t index) const;
   private:
    double getPosition() const;
    vector<ConstElemIter> path;
    IdentifiersType ids;
    bool end;
    const Area *area;
    bool singlePath;
  };

  explicit Area(size_t dimCount);
  Area(const Area &area);
  Area(const IdentifiersType &path, bool useSet);
  explicit Area(const vector<IdentifiersType> &area);

  ConstElemIter elemBegin(size_t dimOrdinal) const;
  ConstElemIter elemEnd(size_t dimOrdinal) const;

  PathIterator pathBegin() const;
  PathIterator pathEnd() const;
  PathIterator getIterator(const Area* path) const;

  void insert(size_t dimOrdinal, const Set* elems, bool calc = true);
  ConstElemIter find(size_t dimOrdinal, IdentifierType elemId) const;
  PathIterator find(const IdentifiersType &path) const;
  PathIterator lowerBound(const IdentifiersType &path) const;
  size_t dimCount() const;
  size_t elemCount(size_t dimOrdinal) const;
  size_t elemMax(size_t dimOrdinal) const;
  const Set* getDim(size_t dimOrdinal) const;
  bool isOverlapping(const Area &area) const;
  double getSize() const {
    return areaSize;
  }
  string toString() const;
  bool operator==(const Area &area) const;
  bool validate() const;
  bool intersection(const Area &area) const;
  Area* intersect(const CubeArea &area) const;
  Area* reduce(uint32_t dimOrdinal, const set<IdentifierType>& subset) const;

 private:
  void calcSize();
  vector<const Set*> area;  //general area
  double areaSize;
  vector<double> stepSizes;

 protected:
  IdentifiersType vpath;  //single cell
  bool isSingleCell() const {
    return !vpath.empty();
  }
};

class CubeArea : public Area {
 public:
  enum CellType {
    NONE = 0,
    BASE_NUMERIC = 1,
    BASE_STRING = 2,
    BASE = 3,
    CONSOLIDATED = 4,
    NUMERIC = 5,
    ALL = 7
  };
  enum ExpandAggrType {
    SELF = 1,
    CHILDREN = 2,
    LEAVES = 4
  };
  enum ExpandStarType {
    BASE_ELEMENTS = 1,
    TOP_ELEMENTS = 2,
    ALL_ELEMENTS = 3,
    NUMERIC_ELEMENTS /*base and cons*/= 4
  };

  CubeArea(const AggrEnv* env, Cube* cube, size_t dimCount);
  CubeArea(const AggrEnv* env, Cube* cube, const Area &area);
  CubeArea(const AggrEnv* env, Cube* cube,
           const IdentifiersType &path, bool useSet = true);
  CubeArea(const AggrEnv* env, Cube* cube,
           const vector<IdentifiersType> &area);
  /*
   list<CubeArea*> split(PathIterator begin, size_t cellCount,
   bool &toTheEnd) const;
   void split(const IdentifiersType &start, const IdentifiersType &stop,
   list<CubeArea*> &areaList, size_t dim) const;
   void splitbyTypes(SubCubeList &stringAreas, SubCubeList &numericAreas,
   SubCubeList &consolidatedAreas) const;
   */

  CubeArea* expandBase(AggregationMaps *aggregationMaps) const;

  Area* expandStar(ExpandStarType type) const;
  Area* expandStarOptim(Set* fullSet) const;

  CellType getType(const PathIterator &cellPath) const;
  bool isBase(const PathIterator &cellPath) const;

  const AggrEnv* getEnv() const {
    return env;
  }
  Cube* getCube() const {
    return cube;
  }
  /* bool intersection(const Area& area, CubeArea** intersection,
   SubCubeList* complementList) const; */
  CubeArea* copy() const;
  bool operator==(const CubeArea& area) const;

 private:
  const AggrEnv* env;
  Cube* cube;
};

ostream& operator<<(ostream& ostr, const Area &cubeArea);
ostream& operator<<(ostream& ostr, const vector<IdentifiersType> &cubePaths);
ostream& operator<<(ostream& ostr, const Set &set);

#endif  // STOAP_OLAP_AREA_H_
