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
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 *
 */


#ifndef STOAP_COLLECTIONS_ASSOCIATIVEARRAY_H_
#define STOAP_COLLECTIONS_ASSOCIATIVEARRAY_H_ 1

#include <string.h>
#include <iostream>
#include "Olap.h"


////////////////////////////////////////////////////////////////////////////////
/// @brief associative array for POD data
///
/// An associative array for POD data. You must use map or hash_map if you
/// want to store real objects. The associative array stores elements of a given
/// type. An element must contain its key. There is no seperate buffer for
/// keys. The description describes how to generate the hash value for keys
/// and elements, how to compare keys and elements, and how to check for empty
/// elements.
////////////////////////////////////////////////////////////////////////////////

template<typename KEY, typename ELEMENT, typename DESC>
class AssociativeArray {
 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief constructs a new associative array for POD data
  ////////////////////////////////////////////////////////////////////////////////

  explicit AssociativeArray(size_t size)
      : desc() {
    initialize(size);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief constructs a new associative array for POD data
  ////////////////////////////////////////////////////////////////////////////////

  AssociativeArray(size_t size, const DESC& desc)
      : desc(desc) {
    initialize(size);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief deletes a associative array for POD data
  ////////////////////////////////////////////////////////////////////////////////

  ~AssociativeArray() {
    delete[] table;
  }

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief returns number of elements
  ////////////////////////////////////////////////////////////////////////////////

  size_t size() const {
    return nrUsed;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief returns the capacity
  ////////////////////////////////////////////////////////////////////////////////

  size_t capacity() const {
    return nrAlloc;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief returns element table
  ////////////////////////////////////////////////////////////////////////////////

  const ELEMENT * getTable() const {
    return table;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief clears the array
  ////////////////////////////////////////////////////////////////////////////////

  void clear() {
    delete[] table;
    initialize(nrAlloc);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief clears the array and deletes the elements
  ////////////////////////////////////////////////////////////////////////////////

  void clearAndDelete() {
    for (size_t i = 0; i < nrAlloc; i++) {
      desc.deleteElement(table[i]);
    }

    delete[] table;
    initialize(nrAlloc);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief finds an element with a given key
  ////////////////////////////////////////////////////////////////////////////////

  const ELEMENT& findKey(const KEY& key) {
    // update statistics
    nrFinds++;

    // compute the hash
    uint32_t hash = desc.hashKey(key);

    // search the table
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])
        && !desc.isEqualKeyElement(key, table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesF++;
    }

    // return whatever we found
    return table[i];
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief finds a given element
  ////////////////////////////////////////////////////////////////////////////////

  const ELEMENT& findElement(const ELEMENT& element) {
    // update statistics
    nrFinds++;

    // compute the hash
    uint32_t hash = desc.hashElement(element);

    // search the table
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])
        && !desc.isEqualElementElement(element, table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesF++;
    }

    // return whatever we found
    return table[i];
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief adds a new element
  ////////////////////////////////////////////////////////////////////////////////

  bool addElement(const ELEMENT& element, bool overwrite = true) {
    // update statistics
    nrAdds++;

    // search the table
    size_t hash = desc.hashElement(element);
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])
        && !desc.isEqualElementElement(element, table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesA++;
    }

    // if we found an element, return
    if (!desc.isEmptyElement(table[i])) {
      if (overwrite) {
        memcpy(&table[i], &element, sizeof(ELEMENT));
      }

      return false;
    }

    // add a new element to the associative array
    memcpy(&table[i], &element, sizeof(ELEMENT));
    nrUsed++;

    // if we were adding and the table is more than half full, extend it
    if (nrAlloc < 2 * nrUsed) {
      ELEMENT * oldTable = table;
      size_t oldAlloc = nrAlloc;

      nrAlloc = 2 * nrAlloc + 1;
      nrUsed = 0;
      nrResizes++;

      table = new ELEMENT[nrAlloc];

      for (size_t i = 0; i < nrAlloc; i++) {
        desc.clearElement(table[i]);
      }

      for (size_t i = 0; i < oldAlloc; i++) {
        if (!desc.isEmptyElement(oldTable[i])) {
          addNewElement(oldTable[i]);
        }
      }

      delete[] oldTable;
    }

    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief adds a new element with key
  ////////////////////////////////////////////////////////////////////////////////

  bool addElement(const KEY& key, const ELEMENT& element,
                  bool overwrite = true) {
    // update statistics
    nrAdds++;

    // search the table
    size_t hash = desc.hashKey(key);
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])
        && !desc.isEqualKeyElement(key, table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesA++;
    }

    // if we found an element, return
    if (!desc.isEmptyElement(table[i])) {
      if (overwrite) {
        memcpy(&table[i], &element, sizeof(ELEMENT));
      }

      return false;
    }

    // if we were adding and the table is more than half full, extend it
    if (nrAlloc < 2 * (nrUsed + 1)) {
      ELEMENT * oldTable = table;
      size_t oldAlloc = nrAlloc;

      nrAlloc = 2 * nrAlloc + 1;
      nrUsed = 0;
      nrResizes++;

      table = new ELEMENT[nrAlloc];

      for (size_t i = 0; i < nrAlloc; i++) {
        desc.clearElement(table[i]);
      }

      for (size_t i = 0; i < oldAlloc; i++) {
        if (!desc.isEmptyElement(oldTable[i])) {
          addNewElement(oldTable[i]);
        }
      }

      delete[] oldTable;
    }

    // add a new element to the associative array
    addNewElement(key, element);

    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief removes a key
  ////////////////////////////////////////////////////////////////////////////////

  ELEMENT removeKey(const KEY& key) {
    // update statistics
    nrRems++;

    // search the table
    size_t hash = desc.hashKey(key);
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])
        && !desc.isEqualKeyElement(key, table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesD++;
    }

    // if we did not find such an item
    if (desc.isEmptyElement(table[i])) {
      return table[i];
    }

    // return found element
    ELEMENT element = table[i];

    // remove item
    desc.clearElement(table[i]);
    nrUsed--;

    // and now check the following places for items to move here
    size_t k = (i + 1) % nrAlloc;

    while (!desc.isEmptyElement(table[k])) {
      size_t j = desc.hashElement(table[k]) % nrAlloc;

      if ((i < k && !(i < j && j <= k)) || (k < i && !(i < j || j <= k))) {
        table[i] = table[k];
        desc.clearElement(table[k]);
        i = k;
      }

      k = (k + 1) % nrAlloc;
    }

    // return success
    return element;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief removes an element
  ////////////////////////////////////////////////////////////////////////////////

  bool removeElement(const ELEMENT& element) {
    // update statistics
    nrRems++;

    // search the table
    size_t hash = desc.hashElement(element);
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])
        && !desc.isEqualElementElement(element, table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesD++;
    }

    // if we did not find such an item return false
    if (desc.isEmptyElement(table[i])) {
      return false;
    }

    // remove item
    desc.clearElement(table[i]);
    nrUsed--;

    // and now check the following places for items to move here
    size_t k = (i + 1) % nrAlloc;

    while (!desc.isEmptyElement(table[k])) {
      size_t j = desc.hashElement(table[k]) % nrAlloc;

      if ((i < k && !(i < j && j <= k)) || (k < i && !(i < j || j <= k))) {
        table[i] = table[k];
        desc.clearElement(table[k]);
        i = k;
      }

      k = (k + 1) % nrAlloc;
    }

    // return success
    return true;
  }

 private:
  void initialize(size_t size) {
    table = new ELEMENT[size];

    for (size_t i = 0; i < size; i++) {
      desc.clearElement(table[i]);
    }

    nrAlloc = size;
    nrUsed = 0;
    nrFinds = 0;
    nrAdds = 0;
    nrRems = 0;
    nrResizes = 0;
    nrProbesF = 0;
    nrProbesA = 0;
    nrProbesD = 0;
    nrProbesR = 0;
  }

  void addNewElement(const ELEMENT& element) {
    // compute the hash
    size_t hash = desc.hashElement(element);

    // search the table
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesR++;
    }

    // add a new element to the associative array
    memcpy(&table[i], &element, sizeof(ELEMENT));
    nrUsed++;
  }

  void addNewElement(const KEY& key, const ELEMENT& element) {
    // compute the hash
    size_t hash = desc.hashKey(key);

    // search the table
    size_t i = hash % nrAlloc;

    while (!desc.isEmptyElement(table[i])) {
      i = (i + 1) % nrAlloc;
      nrProbesR++;
    }

    // add a new element to the associative array
    memcpy(&table[i], &element, sizeof(ELEMENT));
    nrUsed++;
  }

 private:
  DESC desc;

  size_t nrAlloc;  // the size of the table
  size_t nrUsed;  // the number of used entries
  ELEMENT * table;  // the table itself

  size_t nrFinds;  // statistics
  size_t nrAdds;  // statistics
  size_t nrRems;  // statistics
  size_t nrResizes;  // statistics

  size_t nrProbesF;  // statistics
  size_t nrProbesA;  // statistics
  size_t nrProbesD;  // statistics
  size_t nrProbesR;  // statistics
};

#endif  // STOAP_COLLECTIONS_ASSOCIATIVEARRAY_H_
