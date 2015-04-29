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

#ifndef STOAP_OLAP_DOUBLESTORAGE_H_
#define STOAP_OLAP_DOUBLESTORAGE_H_ 1

// #include <sparsehash/sparse_hash_map>
#include <sparsehash/dense_hash_map>

//  #include <ext/pb_ds/assoc_container.hpp>
//  #include <ext/pb_ds/trie_policy.hpp>
//  namespace __gnu_pbds;

#include <Olap.h>
#include "Exceptions/ErrorException.h"

/*
// template<size_t num>
struct mapops {
  // this comes from boost::hash_combine and stackoverflow
  // http://stackoverflow.com/questions/4948780/magic-number-in-boosthash-combine
  inline size_t operator()(const IdentifiersType& ids) const {
    size_t seed = 0;
    for(IdentifiersType::const_iterator i = ids.begin(); i != ids.end(); ++i) {
      seed ^= *i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
  }
};
*/

struct mapops {
  inline uint64_t operator()(const uint64_t& binPath) const {
    return binPath;
  }
};

class DoubleStorage  {
 public:
  DoubleStorage();
  ~DoubleStorage();

  // this was a red-black tree attempt
  // tree<uint64_t, double, std::less<uint64_t>, rb_tree_tag, tree_order_statistics_node_update> m;

  // map holding the double values
  // google::sparse_hash_map<const IdentifiersType, double, mapops> m;
  // google::dense_hash_map<const IdentifiersType, double, mapops> m;
  google::dense_hash_map<const uint64_t, double, mapops> m;

  // double* getValue(const IdentifiersType* ids);
  // void setValue(const IdentifiersType* ids, double value);

  double* getValue(const uint64_t* binPath);
  void addValue(const uint64_t* binPath, double value);
  void setValue(const uint64_t* binPath, double value);
};

#endif  // STOAP_OLAP_DOUBLESTORAGE_H_
