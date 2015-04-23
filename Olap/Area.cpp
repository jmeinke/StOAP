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

#include "Olap/Area.h"

#include <set>
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Olap.h"
#include "Collections/WeightedSet.h"
#include "Exceptions/ErrorException.h"

Area::PathIterator::PathIterator(const Area &area, bool end,
                                 const vector<ConstElemIter> &path)
    : path(path),
      end(end),
      area(&area),
      singlePath(false) {
  if (!end) {
    for (size_t i = 0; i < area.dimCount(); i++) {
      ids.push_back(*path[i]);
    }
  }
}

Area::PathIterator::PathIterator(const Area &area, bool end,
                                 const IdentifiersType &path)
    : ids(path),
      end(end),
      area(&area),
      singlePath(true) {
}

Area::PathIterator &Area::PathIterator::operator++() {
  if (!end) {
    if (singlePath) {
      end = true;
    } else {
      for (size_t j = path.size(); j > 0; j--) {
        size_t i = j - 1;
        ++(path[i]);
        if (path[i] != area->elemEnd(i)) {
          ids[i] = *path[i];
          break;
        } else {
          if (!i) {
            end = true;
          } else {
            path[i] = area->elemBegin(i);
            ids[i] = *path[i];
          }
        }
      }
    }
  }
  return *this;
}

const IdentifiersType &Area::PathIterator::operator*() const {
  return ids;
}

bool Area::PathIterator::operator==(const PathIterator &it) const {
  if (area != it.area) {
    return false;
  }
  if (end != it.end) {
    return false;
  } else {
    if (!end) {
      if (singlePath && it.singlePath) {
        return ids == it.ids;
      } else if (!singlePath && !it.singlePath) {
        for (size_t i = 0; i < path.size(); i++) {
          if (path[i] != it.path[i]) {
            return false;
          }
        }
      } else {
        const IdentifiersType &p1 = singlePath ? ids : it.ids;
        const vector<ConstElemIter> &p2 = singlePath ? it.path : path;
        for (size_t i = 0; i < p1.size(); i++) {
          if (p1[i] != *p2[i]) {
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool Area::PathIterator::operator!=(const PathIterator &it) const {
  return !(it == *this);
}

Area::Area(size_t dimCount)
    : area(dimCount),
      areaSize(0),
      stepSizes(dimCount, 0) {
}

Area::Area(const Area &area)
    : area(area.area),
      areaSize(area.areaSize),
      stepSizes(area.stepSizes) {
}

Area::Area(const IdentifiersType &path, bool useSet)
    : area(useSet ? path.size() : 0),
      areaSize(1),
      stepSizes(path.size(), 0) {
  if (useSet) {
    size_t size = path.size();
    for (size_t i = 0; i < size; i++) {
      Set* s(new Set);
      s->insert(path[i]);
      insert((IdentifierType) i, s, false);
    }
  } else {
    vpath = path;
  }
  // calcSize();
}

Area::Area(const vector<IdentifiersType> &idsArea)
    : area(idsArea.size()),
      areaSize(0),
      stepSizes(idsArea.size(), 0) {
  size_t size = idsArea.size();
  for (size_t i = 0; i < size; i++) {
    Set* s(new Set());
    if (idsArea[i].size() == 1 && idsArea[i][0] == ALL_IDENTIFIERS ) {
      s = new Set(true);
    } else {
      for (size_t j = 0; j < idsArea[i].size(); j++) {
        s->insert(idsArea[i][j]);
      }
    }
    insert((IdentifierType) i, s, false);
  }
  calcSize();
}

Area::ConstElemIter Area::elemBegin(size_t dimOrdinal) const {
  if (vpath.size()) {
    return Set::Iterator(vpath[dimOrdinal], false);
  } else {
    return area[dimOrdinal]->begin();
  }
}

Area::ConstElemIter Area::elemEnd(size_t dimOrdinal) const {
  if (vpath.size()) {
    return Set::Iterator(vpath[dimOrdinal], true);
  } else {
    return area[dimOrdinal]->end();
  }
}

Area::PathIterator Area::pathBegin() const {
  if (vpath.size()) {
    return PathIterator(*this, false, vpath);
  } else {
    vector<ConstElemIter> path;
    if (areaSize) {
      for (size_t i = 0; i < area.size(); i++) {
        path.push_back((area[i])->begin());
      }
    }
    return PathIterator(*this, !areaSize, path);
  }
}

Area::PathIterator Area::pathEnd() const {
  vector<ConstElemIter> path;
  return PathIterator(*this, true, path);
}

void Area::insert(size_t dimOrdinal, const Set* elems, bool calc) {
  if (dimOrdinal >= area.size()) {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "Area::insert invalid dimOrdinal");
  }
  area[dimOrdinal] = elems;
  if (calc) {
    calcSize();
  }
}

Area::ConstElemIter Area::find(size_t dimOrdinal, IdentifierType elemId) const {
  if (vpath.size()) {
    return Set::Iterator(vpath[dimOrdinal], vpath[dimOrdinal] != elemId);
  } else {
    return area[dimOrdinal]->find(elemId);
  }
}

Area::PathIterator Area::find(const IdentifiersType &path) const {
  if (path.size() != area.size()) {
    return pathEnd();
  }
  if (vpath.size()) {
    return vpath == path ? pathBegin() : pathEnd();
  } else {
    vector<ConstElemIter> p;
    for (size_t i = 0; i < area.size(); i++) {
      ConstElemIter it = area[i]->find(path[i]);
      if (it == area[i]->end()) {
        return pathEnd();
      }
      p.push_back(it);
    }
    return PathIterator(*this, false, p);
  }
}

size_t Area::dimCount() const {
  return vpath.size() ? vpath.size() : area.size();
}

size_t Area::elemCount(size_t dimOrdinal) const {
  if (vpath.size()) {
    return 1;
  } else {
    return
        (dimOrdinal < area.size() && area[dimOrdinal]) ?
            area[dimOrdinal]->size() : 0;
  }
}

bool Area::intersection(const Area &area) const {
  size_t dims = dimCount();
  vector<Set*> intersectionSets(dims);
  vector<Set*> complementSets(dims);
  vector<Set*>::iterator interIt = intersectionSets.begin();
  vector<Set*>::iterator complIt = complementSets.begin();
  for (size_t dim = 0; dim < dimCount(); ++interIt, ++complIt, dim++) {
    const Set* s1 = getDim(dim);
    const Set* s2 = area.getDim(dim);
    if (!s1->intersection(s2, 0, 0)) {
      return false;
    }
  }
  return true;
}

// TODO(jmeinke): speed this up, this is a very slow intersection, O(n*m)
Area* Area::intersect(const CubeArea &area) const {
  vector<IdentifiersType> resultPaths;
  for(auto selfIt = this->pathBegin(); selfIt != this->pathEnd(); ++selfIt) {
    for(auto areaIt = area.pathBegin(); areaIt != area.pathEnd(); ++areaIt) {
      if (*selfIt == *areaIt) resultPaths.push_back(*selfIt);
    }
  }

  return new Area(resultPaths);
}

const Set* Area::getDim(size_t dimOrdinal) const {
  if (isSingleCell()) {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "invalid call of Area::getDim() method");
  }
  return area[dimOrdinal];
}

string Area::toString() const {
  stringstream ss;
  ss << *this;
  return ss.str();
}

void Area::calcSize() {
  areaSize = 1;
  if (!vpath.size()) {
    for (size_t i = area.size(); i > 0; i--) {
      if (area[i - 1]) {
        stepSizes[i - 1] = areaSize;
        areaSize *= area[i - 1]->size();
      } else {
        areaSize = 0;
        break;
      }
    }
  }
}

CubeArea::CubeArea(const AggrEnv* env, Cube* cube, size_t dimCount)
    : Area(dimCount),
      env(env),
      cube(cube) {
}

CubeArea::CubeArea(const AggrEnv* env, Cube* cube,
                   const Area &area)
    : Area(area),
      env(env),
      cube(cube) {
}
CubeArea::CubeArea(const AggrEnv* env, Cube* cube, const IdentifiersType &path,
                   bool useSet)
    : Area(path, useSet),
      env(env),
      cube(cube) {
}

CubeArea::CubeArea(const AggrEnv* env, Cube* cube,
                   const vector<IdentifiersType> &area)
    : Area(area),
      env(env),
      cube(cube) {
}

CubeArea* CubeArea::expandBase(AggregationMaps *aggregationMaps) const {
  const vector<Dimension*> &dimensions = *cube->getDimensions();
  CubeArea* result = new CubeArea(env, cube, dimensions.size());

  aggregationMaps->resize(dimensions.size());

  if (dimCount() != dimensions.size()) {
    throw ErrorException(
        ErrorException::ERROR_INTERNAL,
        "CubeArea::expandBase area and dimension size differ.");
  }

  auto didit = dimensions.begin();
  for (size_t dim = 0; dim < dimCount(); dim++, ++didit) {
    Dimension* dimension = *didit;

    Set* s = new Set();
    for (ConstElemIter eit = elemBegin(dim); eit != elemEnd(dim); ++eit) {
      IdentifierType eId = *eit;
      Element* element = dimension->lookupElement(eId);
      if (!element) {
        /* LOG(ERROR) << "CubeArea::expandBase element id: " << *eit
         << " not found in dimension: " << dimension->getName()
         ; */
        continue;  // possible corrupted journal
      }

      /*
      DLOG(WARNING) << "CubeArea::expandBase element id: " << eId
                    << " added in dimension: " << dimension->getName() ;
      */

      try {
        const WeightedSet* baseE = dimension->getBaseElements(element);
        for (auto baseIt = baseE->begin(); baseIt != baseE->end(); ++baseIt) {
          // it seems there is a bug in Set::insert which adds the same id multiple times.
          // therefore check if the element already exists
          if (s->find(baseIt.first()) == s->end()) {
            s->insert(baseIt.first());
          }
        }
        /*
        DLOG(WARNING) << "CubeArea::expandBase adding " << baseE->size()
                      << " source elements to aggregation map." ;
        */
        aggregationMaps->at(dim).buildBaseToParentMap(eId, baseE);
      } catch (const ErrorException& e) {
        LOG(ERROR) << "CubeArea::expandBase exception: " << e.getMessage()
                      ;
      }
    }
    result->insert(dim, s);
    aggregationMaps->at(dim).compactSourceToTarget();
  }
  return result;
}

// TODO(jmeinke): Implement query supporting star as wildcard?
/* Area* CubeArea::expandStar(ExpandStarType type) const {
 const vector<Dimension*>& dimensions = *cube->getDimensions();
 size_t dimCount = dimensions.size();
 Area* result(new Area(dimCount));

 for (size_t i = 0; i < dimCount; i++) {
 if (elemCount(i)) {
 result->insert(i, getDim(i));
 } else {
 // const Dimension* dimension = dimensions[i];
 // TODO(jmeinke): means getting all element ids
 // result->insert(i, dimension->getElemIds(type));
 }
 }
 return result;
 } */

CubeArea::CellType CubeArea::getType(const PathIterator &cellPath) const {
  const vector<Dimension*> &dimensions = *cube->getDimensions();
  CubeArea::CellType pt = CubeArea::BASE_NUMERIC;
  size_t i = 0;
  for (auto dim = dimensions.begin(); dim != dimensions.end(); ++dim, ++i) {
    ElementType et;

    Element *element = (*dim)->lookupElement((*cellPath)[i]);
    if (!element) {
      throw ParameterException(
          ErrorException::ERROR_INVALID_COORDINATES,
          "element with id '" + StringUtils::convertToString((*cellPath)[i])
              + "' not found in dimension '" + (*dim)->getName() + "'",
          "id", (*cellPath)[i]);
    }
    et = element->getElementType();

    if (et == ElementType::CONSOLIDATED) {
      pt = CubeArea::CONSOLIDATED;
    }
  }
  return pt;
}

bool CubeArea::isBase(const PathIterator &cellPath) const {
  return getType(cellPath) != CONSOLIDATED;
}

ostream& operator<<(ostream& ostr, const Area &cubeArea) {
  ostr << '{';
  for (size_t dim = 0; dim != cubeArea.dimCount(); dim++) {
    if (dim) {
      ostr << 'x';
    }

    ostr << '(';
    const Set* s = cubeArea.getDim(dim);
    if (!s) {
      ostr << '*';
    } else {
      ostr << *s;
    }
    ostr << ')';
  }
  ostr << '}';
  return ostr;
}

ostream& operator<<(ostream& ostr, const Set &set) {
  for (std::set<std::pair<IdentifierType, IdentifierType> >::iterator range =
      set.ranges.begin(); range != set.ranges.end(); ++range) {
    if (range != set.ranges.begin()) {
      ostr << ',';
    }
    if (range->first == range->second) {
      ostr << range->first;
    } else {
      ostr << range->first << '-' << range->second;
    }
  }
  return ostr;

}
