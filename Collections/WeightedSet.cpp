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

#include "Collections/WeightedSet.h"

#include <set>
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Olap.h"
#include "Exceptions/ErrorException.h"


Set::Iterator::Iterator(const Set::Iterator &it)
    : it(it.it),
      pos(it.pos),
      par(it.par),
      singleElementId(it.singleElementId),
      end(it.end) {
}

Set::Iterator::Iterator(const SetType::iterator &it, const Set &s, bool end)
    : it(it),
      pos(end ? 0 : it->first),
      par(&s),
      singleElementId(NO_IDENTIFIER ),
      end(end) {
}

Set::Iterator::Iterator(const SetType::iterator &it, IdentifierType pos,
                        const Set &s)
    : it(it),
      pos(pos),
      par(&s),
      singleElementId(NO_IDENTIFIER ),
      end(false) {
}

Set::Iterator::Iterator(IdentifierType singleElementId, bool end)
    : pos(0),
      par(0),
      singleElementId(singleElementId),
      end(end) {
  if (singleElementId == NO_IDENTIFIER ) {
    throw ErrorException(ErrorException::ERROR_INTERNAL, "invalid element ID");
  }
}

IdentifierType Set::Iterator::operator*() const {
  return singleElementId == NO_IDENTIFIER ? pos : singleElementId;
}

bool Set::Iterator::operator!=(const Set::Iterator &it) const {
  return !(*this == it);
}

bool Set::Iterator::operator==(const Set::Iterator &it) const {
  if (singleElementId == NO_IDENTIFIER && it.singleElementId == NO_IDENTIFIER ) {
    return this->it == it.it
        && (this->it == par->ranges.end() ? true : pos == it.pos);
  } else if (singleElementId != NO_IDENTIFIER
      && it.singleElementId != NO_IDENTIFIER ) {
    return end == it.end && (end ? true : singleElementId == it.singleElementId);
  } else {
    const SetType::const_iterator &it1 =
        singleElementId == NO_IDENTIFIER ? this->it : it.it;
    const Set *par1 = singleElementId == NO_IDENTIFIER ? par : it.par;
    IdentifierType pos1 = singleElementId == NO_IDENTIFIER ? pos : it.pos;
    IdentifierType elemId2 =
        singleElementId == NO_IDENTIFIER ? it.singleElementId : singleElementId;
    bool end2 = singleElementId == NO_IDENTIFIER ? it.end : end;
    if (end2) {
      return it1 == par1->ranges.end();
    } else {
      return
          it1 == par1->ranges.end() ?
              false : par1->size() == 1 && pos1 == elemId2;
    }
  }
}

Set::Iterator &Set::Iterator::operator--() {
  if (singleElementId == NO_IDENTIFIER ) {
    if (!end && pos > it->first) {
      pos--;
    } else {
      if (it != par->ranges.begin()) {
        --it;
        pos = it->second;
      }
    }
  } else {
    end = false;
  }
  return *this;
}

Set::Iterator &Set::Iterator::operator++() {
  if (singleElementId == NO_IDENTIFIER ) {
    if (!end && pos < it->second) {
      pos++;
    } else {
      if (it != par->ranges.end()) {
        ++it;
        if (it != par->ranges.end()) {
          pos = it->first;
        } else {
          pos = 0;
          end = true;
        }
      }
    }
  } else {
    end = true;
  }
  return *this;
}

Set::Iterator Set::Iterator::operator++(int i) {
  Iterator ret(*this);
  ++(*this);
  return ret;
}

Set::Iterator &Set::Iterator::operator=(const Iterator &it) {
  par = it.par;
  pos = it.pos;
  this->it = it.it;
  singleElementId = it.singleElementId;
  end = it.end;
  return *this;
}

Set::Set()
    : siz(0) {
}

Set::Set(bool full) {
  if (full) {
    ranges.insert(make_pair(0, ALL_IDENTIFIERS ));
    siz = (size_t) -1;
  } else {
    siz = 0;
  }
}

Set::Set(const Set &set)
    : ranges(set.ranges),
      siz(set.siz) {
}

bool Set::insert(IdentifierType id) {
  bool result = false;

  SetType::iterator itp = ranges.lower_bound(make_pair(id, 0));
  if (itp == ranges.end() || itp->first != id) {
    if (itp != ranges.begin()) {
      --itp;
    }
  }
  if (itp == ranges.end()) {
    ranges.insert(make_pair(id, id));
    ++siz;
    result = true;
  } else {
    SetType::iterator tmp;
    if (itp->second < id) {
      if (itp->second == id - 1) {
        tmp = itp;
        --tmp;
        tmp = ranges.insert(tmp, make_pair(itp->first, id));
        ranges.erase(itp);
        itp = tmp;
        result = true;
        siz += 1;
      }
      SetType::iterator itn = itp;
      ++itn;
      if (itn == ranges.end()) {
        if (!result) {
          ranges.insert(itp, make_pair(id, id));
          ++siz;
          result = true;
        }
      } else {
        if (itn->first > id) {
          if (itn->first == id + 1) {
            tmp = ranges.insert(itp, make_pair(id, itn->second));
            ranges.erase(itn);
            itn = tmp;
            if (!result) {
              result = true;
              ++siz;
            }
          } else {
            if (!result) {
              ranges.insert(itp, make_pair(id, id));
              ++siz;
              result = true;
            }
          }
        }
        if (itp->second == itn->first) {
          ranges.insert(itp, make_pair(itp->first, itn->second));
          ranges.erase(itp);
          ranges.erase(itn);
        }
      }
    } else {
      if (itp->first > id) {
        if (id == itp->first - 1) {
          ranges.insert(make_pair(id, itp->second));
          ranges.erase(itp);
        } else {
          ranges.insert(make_pair(id, id));
        }
        ++siz;
        result = true;
      }
    }
  }
  return result;
}

Set::Iterator Set::begin() const {
  return Iterator(ranges.begin(), *this, ranges.begin() == ranges.end());
}

Set::Iterator Set::end() const {
  return Iterator(ranges.end(), *this, true);
}

Set::Iterator Set::find(IdentifierType id) const {
  pair<IdentifierType, IdentifierType> val(id, 0);
  SetType::iterator it = ranges.lower_bound(val);
  if (it != ranges.end() && it->first == id) {
    return Iterator(it, id, *this);
  }
  if (it != ranges.begin()) {
    --it;
    if (it->first <= id && it->second >= id) {
      return Iterator(it, id, *this);
    }
  }
  return end();
}

bool Set::isInSet(const IdentifierType id) const {
  pair<IdentifierType, IdentifierType> val(id, 0);
  SetType::iterator it = ranges.lower_bound(val);
  if (it != ranges.end() && it->first == id) {
    return true;
  }
  if (it != ranges.begin()) {
    --it;
    if (it->first <= id && it->second >= id) {
      return true;
    }
  }
  return false;
}

Set::Iterator Set::lowerBound(IdentifierType id) const {
  pair<IdentifierType, IdentifierType> val(id, 0);
  SetType::iterator it = ranges.lower_bound(val);
  if (it != ranges.end() && it->first == id) {
    return Iterator(it, id, *this);
  }
  if (it != ranges.begin()) {
    SetType::iterator sit = it;
    --sit;
    if (sit->first <= id && sit->second >= id) {
      return Iterator(sit, id, *this);
    }
  }
  if (it != ranges.end()) {
    return Iterator(it, it->first, *this);
  }
  return end();
}

Set::Iterator Set::erase(const Set::Iterator &it) {
  siz--;
  if (it.it->first == it.pos) {
    pair<IdentifierType, IdentifierType> val = *it.it;
    SetType::iterator tmp = it.it;
    ++tmp;
    ranges.erase(it.it);
    if (val.second != it.pos) {
      val.first++;
      return Iterator(ranges.insert(val).first, val.first, *this);
    } else {
      return Iterator(tmp, *this, tmp == ranges.end());
    }
  } else if (it.it->second == it.pos) {
    pair<IdentifierType, IdentifierType> val = *it.it;
    ranges.erase(it.it);
    val.second--;
    return Iterator(ranges.insert(val).first, val.second, *this);
  } else {
    pair<IdentifierType, IdentifierType> val1 = *it.it;
    pair<IdentifierType, IdentifierType> val2 = *it.it;
    ranges.erase(it.it);
    val1.second = it.pos - 1;
    val2.first = it.pos + 1;
    ranges.insert(val1);
    return Iterator(ranges.insert(val2).first, val2.first, *this);
  }
}

size_t Set::size() const {
  return siz;
}

void Set::clear() {
  ranges.clear();
  siz = 0;
}

Set::range_iterator Set::rangeLowerBound(IdentifierType id) const {
  SetType::iterator it = ranges.lower_bound(make_pair(id, 0));
  if (it != ranges.end() && it->first == id) {
    return range_iterator(it);
  }
  if (it != ranges.begin()) {
    --it;
  }
  return range_iterator(it);
}

void Set::insertRange(IdentifierType low, IdentifierType high) {
  siz += high - low + 1;
  SetType::iterator it = ranges.end();
  if (it != ranges.begin()) {
    --it;
  }
  ranges.insert(it, make_pair(low, high));
}

bool Set::empty() const {
  return siz == 0;
}

bool Set::operator!=(const Set &set) const {
  return siz != set.siz || ranges != set.ranges;
}

bool Set::operator==(const Set &set) const {
  return siz == set.siz && ranges == set.ranges;
}

bool Set::intersection_check(const Set* set, Set** intersection,
                             Set** complement) const {
  // TODO(jj): optimize Set::intersection to operate with ranges instead of iterators
  if (complement) {
    (*complement) = new Set();
  }
  if (intersection) {
    (*intersection) = new Set();
  }
  if (!set) {
    if (intersection) {
      (*intersection) = new Set(*this);
    }
    return true;
  }
  Iterator it1 = begin();
  Iterator it2 = set->begin();
  Iterator end1 = end();
  Iterator end2 = set->end();
  bool hasIntersection = false;
  bool hasComplement = false;
  for (; it1 != end1 && it2 != end2;) {
    if (*it1 < *it2) {
      hasComplement = true;
      if (complement) {
        if (!(*complement)) {
          (*complement) = new Set();
        }
        (*complement)->insert(*it1);
      }
      ++it1;
    } else if (*it1 == *it2) {
      if (intersection) {
        if (!(*intersection)) {
          (*intersection) = new Set();
        }
        (*intersection)->insert(*it1);
      }
      ++it1;
      ++it2;
      hasIntersection = true;
      if (!intersection && !complement) {
        return hasIntersection;
      }
    } else {
      ++it2;
    }
  }
  if (it1 != end1) {
    hasComplement = true;
    if (complement) {
      if (!(*complement)) {
        (*complement) = new Set();
      }
      while (it1 != end1) {
        (*complement)->insert(*it1);
        hasComplement = true;
        ++it1;
      }
    }
  }
  if (!hasComplement && intersection) {
    (*intersection) = new Set(*this);
  }
  if (!intersection && complement) {
    (*complement) = new Set(*this);
  }
  return hasIntersection;
}

bool Set::intersection(const Set* set, Set** intersection,
                       Set** complement) const {
  if (complement) {
    (*complement) = new Set();
  }
  if (!set) {
    if (intersection) {
      (*intersection) = new Set(*this);
    }
    return true;
  }
  if (intersection) {
    (*intersection) = new Set();
  }
  bool hasIntersection = false;

  Set::range_iterator filterIt = set->rangeBegin();
  Set::range_iterator setIt = rangeBegin();
  Set::range_iterator setEnd = rangeEnd();
  Set::range_iterator filterEnd = set->rangeEnd();

  for (; setIt != setEnd; ++setIt) {
    IdentifierType setLow = setIt.low();
    IdentifierType setHigh = setIt.high();
    if (filterIt == filterEnd) {
      if (!intersection && !complement) {
        return false;
      }
      if (complement) {
        (*complement)->insertRange(setLow, setHigh);
      } else {
        break;
      }
    } else {
      IdentifierType setCurr = setLow;
      while (filterIt != filterEnd) {
        IdentifierType filterLow = filterIt.low();
        IdentifierType filterHigh = filterIt.high();
        if (filterHigh < setLow) {
          ++filterIt;
        } else if (filterLow > setHigh) {
          break;
        } else {
          hasIntersection = true;
          if (!intersection && !complement) {
            return true;
          }
          if (complement && setCurr < filterLow) {
            (*complement)->insertRange(setCurr, filterLow - 1);
          }
          setCurr = min(setHigh, filterHigh);
          if (intersection) {
            (*intersection)->insertRange(max(filterLow, setLow), setCurr);
          }
          ++setCurr;
          if (filterHigh > setHigh) {
            break;
          } else {
            ++filterIt;
          }
        }
      }
      if (complement && setCurr <= setHigh) {
        (*complement)->insertRange(setCurr, setHigh);
      }
    }
  }

  return hasIntersection;
}

double WeightedSet::const_iterator::second() const {
  return parent->rangeWeight(it->first);
}

double WeightedSet::rangeWeight(IdentifierType rangeStart) const {
  map<IdentifierType, double>::const_iterator found = weights.find(rangeStart);
  if (found == weights.end()) {
    return 1;
  } else {
    return found->second;
  }
}

void WeightedSet::pushSorted(IdentifierType low, IdentifierType high,
                             double weight) {
  if (empty()) {
    ranges.insert(make_pair(low, high));
    if (weight != 1) {
      weights[low] = weight;
    }
  } else {
    SetType::reverse_iterator lastRange = ranges.rbegin();
    if (lastRange->second + 1 == low
        && rangeWeight(lastRange->first) == weight) {
      // append
      pair<IdentifierType, IdentifierType> val = *lastRange;
      val.second = high;
      ranges.erase(--ranges.end());
      ranges.insert(val);
    } else {
      ranges.insert(make_pair(low, high));
      if (weight != 1) {
        weights[low] = weight;
      }
    }
  }
  siz += high - low + 1;
}

void WeightedSet::fastAdd(IdentifierType id, double weight) {
  pair<SetType::iterator, bool> itb = ranges.insert(make_pair(id, id));
  if (itb.second) {
    if (weight != 1) {
      weights[id] = weight;
    }
    siz++;
  } else {
    map<IdentifierType, double>::iterator it = weights.find(id);
    if (it == weights.end()) {
      weights[id] = 1 + weight;
    } else {
      weights[id] += weight;
    }

    if (weights[id] == 1) {
      weights.erase(weights.find(id));
    }
  }
}

void WeightedSet::consolidate() {
// consolidates WeightedSet - joins all one-id ranges into valid ranges used in WeightedSet
  for (SetType::iterator it = ranges.begin(); it != ranges.end(); ++it) {
    double weight = rangeWeight(it->first);

    while (true) {
      SetType::iterator next = it;
      ++next;
      if (next == ranges.end() || it->second + 1 != next->first) {
        // no more ids to process or not continuous
        break;
      }

      double nextWeight = rangeWeight(next->first);
      if (nextWeight != weight) {
        // different weights, cannot be joined
        break;
      }

      // next will be joined into it, delete next
      if (nextWeight != 1) {
        weights.erase(weights.find(next->first));
      }
      ranges.erase(next);

      // join
      pair<IdentifierType, IdentifierType> join = *it;
      join.second++;
      ranges.erase(it);
      pair<SetType::iterator, bool> ins = ranges.insert(join);
      it = ins.first;
    }
  }
}

// optimized insert when push_back is needed
void WeightedSet::pushSorted(IdentifierType id, double weight) {
  pushSorted(id, id, weight);
}

void WeightedSet::clear() {
  ranges.clear();
  siz = 0;
  weights.clear();
}

WeightedSet::range_iterator WeightedSet::lastRange() {
  if (siz > 0) {
    return range_iterator(--ranges.end(), *this);
  } else {
    throw ErrorException(ErrorException::ERROR_INTERNAL, "set is empty");
  }
}

double WeightedSet::weight(IdentifierType id) const {
  Set::range_iterator rit = rangeLowerBound(id);
  if (rit != rangeEnd() && rit.low() <= id && id <= rit.high()) {
    return rangeWeight(rit.low());
  } else {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "element id out of range");
  }
}

