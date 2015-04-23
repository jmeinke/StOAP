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

#ifndef STOAP_COLLECTIONS_WEIGHTEDSET_H_
#define STOAP_COLLECTIONS_WEIGHTEDSET_H_ 1

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Olap.h"

class Set {
 public:
  typedef set<pair<IdentifierType, IdentifierType> > SetType;

  class Iterator : public std::iterator<std::forward_iterator_tag,
      IdentifierType, ptrdiff_t, IdentifierType *, IdentifierType &> {
   public:
    Iterator(const Iterator &it);
    Iterator();
    Iterator(const SetType::iterator &it, const Set &s, bool end);
    Iterator(const SetType::iterator &it, IdentifierType pos, const Set &s);
    Iterator(IdentifierType elemId, bool end);
    IdentifierType operator*() const;
    bool operator!=(const Iterator &it) const;
    bool operator==(const Iterator &it) const;
    Iterator &operator--();
    Iterator &operator++();
    Iterator operator++(int i);
    Iterator &operator=(const Iterator &it);
   protected:
    SetType::const_iterator it;
   private:
    //general set
    IdentifierType pos;
    const Set *par;
    friend class Set;
    //single element
    IdentifierType singleElementId;
    bool end;
  };
  class range_iterator {
   public:
    explicit range_iterator(const SetType::const_iterator &it)
        : it(it) {
    }

    bool operator!=(const range_iterator &it2) const {
      return it2.it != it;
    }
    bool operator==(const range_iterator &it2) const {
      return it2.it == it;
    }
    range_iterator &operator++() {
      ++it;
      return *this;
    }
    range_iterator &operator--() {
      --it;
      return *this;
    }
    range_iterator operator++(int) {
      range_iterator ret(*this);
      ++(*this);
      return ret;
    }

    IdentifierType low() const {
      return it->first;
    }
    IdentifierType high() const {
      return it->second;
    }

   private:
    SetType::const_iterator it;
  };

  Set();
  explicit Set(bool full);
  virtual ~Set() {
  }
  Set(const Set &set);
  bool insert(IdentifierType id);
  template<typename T> void insert(T first, T last) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }
  Iterator begin() const;
  Iterator end() const;
  Iterator find(IdentifierType id) const;
  Iterator lowerBound(IdentifierType id) const;
  // Use 'it = set.erase(it)' not 'set.erase(it++)' !!!
  Iterator erase(const Iterator &it);
  size_t size() const;
  size_t getRangesCount() const {
    return ranges.size();
  }
  void clear();

  range_iterator rangeBegin() const {
    return range_iterator(ranges.begin());
  }
  range_iterator rangeEnd() const {
    return range_iterator(ranges.end());
  }
  range_iterator rangeLowerBound(IdentifierType id) const;
  void insertRange(IdentifierType low, IdentifierType high);

  bool empty() const;
  bool operator!=(const Set &set) const;
  bool operator==(const Set &set) const;
  bool intersection_check(const Set* set, Set** intersection,
                          Set** complement) const;
  bool intersection(const Set* set, Set** intersection, Set** complement) const;
  const Set* limit(IdentifierType start, IdentifierType end) const;
  bool validate() const;

  static Set* addAncestors(const Set* set, Dimension* dim);

 protected:
  static void addAncestor(Set* set, Dimension* dim, IdentifierType elemId);

  SetType ranges;
  size_t siz;
  friend ostream& operator<<(ostream& ostr, const Set &set);
};

class WeightedSet : public Set {
 public:

  class const_iterator : public Set::Iterator {
   public:
    const_iterator(const SetType::iterator &it, const WeightedSet &s, bool end)
        : Iterator(it, s, end),
          parent(&s) {
    }

    IdentifierType first() const {
      return this->operator *();
    }
    double second() const;
   private:
    const WeightedSet *parent;
  };

  class range_iterator : public Set::range_iterator {
   public:
    range_iterator(const SetType::iterator &it, const WeightedSet &s)
        : Set::range_iterator(it),
          it(it),
          parent(&s) {
    }

    bool operator==(const range_iterator &it2) const {
      return it2.it == it;
    }
    bool operator!=(const range_iterator &it2) const {
      return it2.it != it;
    }
    range_iterator &operator++() {
      ++it;
      return *this;
    }
    range_iterator operator++(int) {
      range_iterator ret(*this);
      ++(*this);
      return ret;
    }

    IdentifierType low() const {
      return it->first;
    }
    IdentifierType high() const {
      return it->second;
    }
    double weight() const;

   private:
    SetType::const_iterator it;
    const WeightedSet *parent;
  };

  WeightedSet()
      : Set() {
  }
  virtual ~WeightedSet() {
  }
  explicit WeightedSet(const Set &s)
      : Set(s) {
  }
  WeightedSet(const WeightedSet &ws)
      : Set(ws),
        weights(ws.weights) {
  }
  WeightedSet(const WeightedSet &ws, double factor)
      : Set(ws),
        weights(ws.weights) {
    multiply(factor);
  }

  WeightedSet* add(const WeightedSet &ws2, double factor) const;
  WeightedSet* subtract(const WeightedSet &ws2, double factor) const;
  void multiply(double factor);

  void pushSorted(IdentifierType low, IdentifierType high, double weight);
  void pushSorted(IdentifierType id, double weight);

  void fastAdd(IdentifierType id, double weight);  // add ids in random order, only one by one
  void consolidate();  // optimize set after fastAdd sequence

  void clear();

  const_iterator begin() const {
    return const_iterator(ranges.begin(), *this, ranges.begin() == ranges.end());
  }
  const_iterator end() const {
    return const_iterator(ranges.end(), *this, true);
  }

  range_iterator rangeBegin() const {
    return range_iterator(ranges.begin(), *this);
  }
  range_iterator rangeEnd() const {
    return range_iterator(ranges.end(), *this);
  }
  range_iterator lastRange();

  size_t rangesCount() const {
    return ranges.size();
  }

  bool hasWeights() const {
    return !weights.empty();
  }
  double weight(IdentifierType id) const;

  friend class const_iterator;
  friend class range_iterator;

 private:
  double rangeWeight(IdentifierType rangeStart) const;

  map<IdentifierType, double> weights;  // weight = 1 if not found
};

#endif  // STOAP_COLLECTIONS_WEIGHTEDSET_H_
