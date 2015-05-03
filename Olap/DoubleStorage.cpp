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

#include "Olap/DoubleStorage.h"

// #include <sparsehash/sparse_hash_map>
#include <sparsehash/dense_hash_map>

#include <Olap.h>
#include "Exceptions/ErrorException.h"

DoubleStorage::DoubleStorage() {
  const uint64_t empty = (0xFFFFFFFFFFFFFFFFULL);

  m.set_empty_key(empty);
  // set deleted key must only be used if we want to use erase
  // but we don't want to use erase. If we want to use it,
  // the key must differ from empty key.
  // m.set_deleted_key(empty);
  m.min_load_factor(0.9);
  m.max_load_factor(1.0);
}

DoubleStorage::~DoubleStorage() {
  m.clear();
}

double* DoubleStorage::getValue(const uint64_t* binPath) {
  auto it = m.find(*binPath);
  if (it == m.end()) {
    return NULL;
  }
  return &it->second;
}

void DoubleStorage::addValue(const uint64_t* binPath, double value) {
  // auto it = m.find(*binPath);
  // if (it == m.end()) {
  //   m[*binPath] = value;
  // } else {
    m[*binPath] += value;
  // }
}

void DoubleStorage::setValue(const uint64_t* binPath, double value) {
  m[*binPath] = value;
}

/*
double* DoubleStorage::getValue(const IdentifiersType* ids) {
  auto it = m.find(*ids);
  if (it == m.end()) {
    return NULL;
  }
  return &it->second;
}

void DoubleStorage::setValue(const IdentifiersType* ids, double value) {
  m[*ids] = value;
}
*/
