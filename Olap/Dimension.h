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
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#ifndef STOAP_OLAP_DIMENSION_H_
#define STOAP_OLAP_DIMENSION_H_ 1

#include <set>
#include <utility>
#include <deque>
#include <string>
#include <vector>

#include "Olap.h"
#include "Collections/StringUtils.h"
#include "Collections/AssociativeArray.h"
#include "Olap/Element.h"
#include "Exceptions/ParameterException.h"

class FileReader;
class FileWriter;
class Cube;

////////////////////////////////////////////////////////////////////////////////
/// @brief class for OLAP dimension
////////////////////////////////////////////////////////////////////////////////

class Dimension {
 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief list of parents
  ////////////////////////////////////////////////////////////////////////////////

  typedef vector<Element*> ParentsType;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief creates new dimension with given identifier
  ////////////////////////////////////////////////////////////////////////////////

  Dimension(IdentifierType identifier, const string& name);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief deletes dimension
  ////////////////////////////////////////////////////////////////////////////////

  ~Dimension();

  ////////////////////////////////////////////////////////////////////////////////
  /// @{
  /// @name functions to save and load the dimension
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief reads data from file
  ////////////////////////////////////////////////////////////////////////////////

  void loadDimension(FileReader* file);

 private:
  uint32_t loadOverview(FileReader* file);

  void loadElementParents(FileReader* file, Element* element,
                          IdentifiersType* parents, uint32_t numElements);

  void loadElementChildren(FileReader* file, Element* element,
                           IdentifiersType* children, vector<double>* weights,
                           uint32_t numElements);

  void loadElement(FileReader* file, uint32_t numElements);

  void loadElements(FileReader* file, uint32_t numElements);

  // void checkElementName(const string& name);

  void updateLevel();

  void addParentsToSortedList(Element* child, set<Element*>* knownElements);

  // bool isCycle(const ParentsType*, const ElementsWeightType*);

 public:

  ////////////////////////////////////////////////////////////////////////////////
  /// @{
  /// @name getter and setter
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets identifier of dimension
  ////////////////////////////////////////////////////////////////////////////////

  IdentifierType getIdentifier() {
    return identifier;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets or rename dimension
  ////////////////////////////////////////////////////////////////////////////////

  void setName(const string& name) {
    this->name = name;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets dimension name
  ////////////////////////////////////////////////////////////////////////////////

  const string& getName() {
    return name;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief returns dimension type
  ////////////////////////////////////////////////////////////////////////////////

  ItemType getType();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets highest consolidation level
  ////////////////////////////////////////////////////////////////////////////////

  // LevelType getLevel();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets highest indentation
  ////////////////////////////////////////////////////////////////////////////////

  // IndentType getIndent();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets highest consolidation depth
  ////////////////////////////////////////////////////////////////////////////////

  DepthType getDepth();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief calculate memory usage of the elements vector
  ////////////////////////////////////////////////////////////////////////////////

  size_t getMemoryUsageStorage();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets list of elements (sorted by position)
  ////////////////////////////////////////////////////////////////////////////////

  vector<Element *> getElements(IdentifierType level = NO_IDENTIFIER );

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets list of base elements (unsorted)
  ////////////////////////////////////////////////////////////////////////////////

  vector<Element *> getBaseElements();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets maximal used identifier
  ////////////////////////////////////////////////////////////////////////////////

  IdentifierType getMaximalIdentifier();

  ////////////////////////////////////////////////////////////////////////////////
  /// @}
  ////////////////////////////////////////////////////////////////////////////////

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @{
  /// @name functions to update internal structures
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief updates level, indent, and depth information
  ////////////////////////////////////////////////////////////////////////////////

  void updateLevelIndentDepth();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief updates base element list of each element
  ////////////////////////////////////////////////////////////////////////////////

  // void updateElementBaseElements();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief updates the list of topological sorted elements
  ////////////////////////////////////////////////////////////////////////////////

  void updateTopologicalSortedElements();

  ////////////////////////////////////////////////////////////////////////////////
  /// @}
  ////////////////////////////////////////////////////////////////////////////////

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @{
  /// @name element functions
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief deletes all elements
  ////////////////////////////////////////////////////////////////////////////////

  void clearElements();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets number of elements
  ////////////////////////////////////////////////////////////////////////////////

  size_t sizeElements(bool onlyBase = false);

  /**
   * @brief gets all parent elements of a child element
   * @param child is the element whose parents we want to return
   * @return parent elements ordered by position
   */
  const ParentsType* getParents(Element* child) {
    ChildParentsPair * cpp = childToParents.findKey(child);
    return cpp == 0 ? &emptyParents : &cpp->parents;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets child elements
  ////////////////////////////////////////////////////////////////////////////////

  const ElementsWeightType getChildren(Element* parent) {
    ElementsWeightType result;
    if (parent) {
      ParentChildrenPair *pcp = parentToChildren.findKey(parent);
      if (pcp) {
        result = pcp->children;
      }
    } else {
      // root elements
      ElementsType elems = getElements(NO_IDENTIFIER );
      for (ElementsType::const_iterator it = elems.begin(); it != elems.end();
          it++) {
        if (getParents(*it)->size() == 0) {
          result.push_back(pair<Element*, double>(*it, 1.0));  // weight not needed for roots
        }
      }
    }
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets child identifiers for an element
  ////////////////////////////////////////////////////////////////////////////////
  const IdentifiersWeightType getChildrenIds(Element* parent) {
    IdentifiersWeightType result;
    if (parent) {
      ParentChildrenPair *pcp = parentToChildren.findKey(parent);
      if (pcp) {
        //if (!pcp->children.empty()) {
        for (auto it = pcp->children.begin(); it != pcp->children.end(); ++it) {
          result.push_back(
              pair<IdentifierType, double>((*it).first->getIdentifier(),
                                           (*it).second));
        }
        //}
      }
    } else {
      // root elements
      ElementsType elems = getElements(NO_IDENTIFIER);
      for (ElementsType::const_iterator it = elems.begin(); it != elems.end();
          it++) {
        if (getParents(*it)->size() == 0) {
          result.push_back(
              pair<IdentifierType, double>((*it)->getIdentifier(), 1.0));  // weight not needed for roots
        }
      }
    }
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets the base elements
  ////////////////////////////////////////////////////////////////////////////////

  const IdentifiersWeightType getBaseIWs(Element* parent);
  const WeightedSet* getBaseElements(Element* parent);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief adds childs to (consolidated) element
  ////////////////////////////////////////////////////////////////////////////////

  void addChildren(Element * parent, const ElementsWeightType * children);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets element by identifier
  ////////////////////////////////////////////////////////////////////////////////

  Element* lookupElement(IdentifierType elementIdentifier) {
    return elementIdentifier < elements.size() ? elements[elementIdentifier] : 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets element by name
  ////////////////////////////////////////////////////////////////////////////////

  Element* lookupElementByName(const string& name) {
    return nameToElement.findKey(name);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets element by position
  ////////////////////////////////////////////////////////////////////////////////

  Element* lookupElementByPosition(PositionType position) {
    return positionToElement.findKey(position);
  }

  void checkLevelIndentDepth();
  // void checkBaseElements();

  // dimension position in a uint64_t binpath
  IdentifierType dimPos;
  uint64_t dimMask;

  ////////////////////////////////////////////////////////////////////////////////
  /// @}
  ////////////////////////////////////////////////////////////////////////////////

 protected:
  IdentifierType identifier;  // identifier of the dimension
  string name;  // name of the dimension

 private:
  static ElementsWeightType emptyChildren;
  static ParentsType emptyParents;

  // true if maxLevel, maxDepth, and maxIndent are valid
  bool isValidLevel;
  // max level of elements, 0 = base element, levelParent = max(levelChild) + 1
  LevelType maxLevel;
  // max indents of elements, 1 = element has no father, indentChild = indent of first father + 1
  IndentType maxIndent;
  // max depth of elements, 0 = element has no father, depthChild = max(depthFather) + 1
  DepthType maxDepth;

  bool isValidBaseElements;  // true if the list of base elements off all elements is valid

  bool isValidSortedElements;  // true if the list of topological elements is valid
  deque<Element*> sortedElements;  // list of topological sorted elements

  struct Name2ElementDesc {
    uint32_t hash(const string& name) {
      return StringUtils::hashValueLower(name.c_str(), name.size());
    }

    bool isEmptyElement(Element * const & element) {
      return element == 0;
    }

    uint32_t hashElement(Element * const & element) {
      return hash(element->getName());
    }

    uint32_t hashKey(const string& key) {
      return hash(key);
    }

    bool isEqualElementElement(Element * const & left,
                               Element * const & right) {
      return left->getName() == right->getName();
    }

    bool isEqualKeyElement(const string& key, Element * const & element) {
      const string& name = element->getName();

      return key.size() == name.size()
          && strncasecmp(key.c_str(), name.c_str(), key.size()) == 0;
    }

    void clearElement(Element * & element) {
      element = 0;
    }
  };

  struct Position2ElementDesc {
    bool isEmptyElement(Element * const & element) {
      return element == 0;
    }

    uint32_t hashElement(Element * const & element) {
      return element->getPosition();
    }

    uint32_t hashKey(const PositionType& key) {
      return key;
    }

    bool isEqualElementElement(Element * const & left,
                               Element * const & right) {
      return left->getPosition() == right->getPosition();
    }

    bool isEqualKeyElement(const PositionType& key, Element * const & element) {
      return key == element->getPosition();
    }

    void clearElement(Element * & element) {
      element = 0;
    }
  };

  struct ParentChildrenPair {
    explicit ParentChildrenPair(Element * parent)
        : parent(parent) {
    }

    Element * parent;
    ElementsWeightType children;
  };

  struct Parent2ChildrenDesc {
    bool isEmptyElement(ParentChildrenPair * const & element) {
      return element == 0;
    }

    uint32_t hashElement(ParentChildrenPair * const & element) {
#if SIZEOF_VOIDP == 8
      uint64_t p = reinterpret_cast<uint64_t>(element->parent);
      return (uint32_t) ((p & 0xFFFFFF) ^ (p >> 32));
#else
      return reinterpret_cast<uint32_t> (element->parent);
#endif
    }

    uint32_t hashKey(Element * const & key) {
#if SIZEOF_VOIDP == 8
      uint64_t p = reinterpret_cast<uint64_t>(key);
      return (uint32_t) ((p & 0xFFFFFF) ^ (p >> 32));
#else
      return reinterpret_cast<uint32_t> (key);
#endif
    }

    bool isEqualElementElement(ParentChildrenPair * const & left,
                               ParentChildrenPair * const & right) {
      return left->parent == right->parent;
    }

    bool isEqualKeyElement(Element * const & key,
                           ParentChildrenPair * const & element) {
      return key == element->parent;
    }

    void clearElement(ParentChildrenPair * & element) {
      element = 0;
    }

    void deleteElement(ParentChildrenPair * & element) {
      delete element;
    }
  };

  struct ChildParentsPair {
    explicit ChildParentsPair(Element * child)
        : child(child) {
    }

    Element * child;
    ParentsType parents;
  };

  struct Child2ParentsDesc {
    bool isEmptyElement(ChildParentsPair * const & element) {
      return element == 0;
    }

    uint32_t hashElement(ChildParentsPair * const & element) {
#if SIZEOF_VOIDP == 8
      uint64_t p = reinterpret_cast<uint64_t>(element->child);
      return (uint32_t) ((p & 0xFFFFFF) ^ (p >> 32));
#else
      return reinterpret_cast<uint32_t> (element->child);
#endif
    }

    uint32_t hashKey(Element * const & key) {
#if SIZEOF_VOIDP == 8
      uint64_t p = reinterpret_cast<uint64_t>(key);
      return (uint32_t) ((p & 0xFFFFFF) ^ (p >> 32));
#else
      return reinterpret_cast<uint32_t> (key);
#endif
    }

    bool isEqualElementElement(ChildParentsPair * const & left,
                               ChildParentsPair * const & right) {
      return left->child == right->child;
    }

    bool isEqualKeyElement(Element * const & key,
                           ChildParentsPair * const & element) {
      return key == element->child;
    }

    void clearElement(ChildParentsPair * & element) {
      element = 0;
    }

    void deleteElement(ChildParentsPair * & element) {
      delete element;
    }
  };

  struct ElementSetDesc {
    bool isEmptyElement(Element * const & element) {
      return element == 0;
    }

    uint32_t hashElement(Element * const & element) {
#if SIZEOF_VOIDP == 8
      uint64_t p = reinterpret_cast<uint64_t>(element);
      return (uint32_t) ((p & 0xFFFFFF) ^ (p >> 32));
#else
      return reinterpret_cast<uint32_t> (element);
#endif
    }

    uint32_t hashKey(Element * const & key) {
#if SIZEOF_VOIDP == 8
      uint64_t p = reinterpret_cast<uint64_t>(key);
      return (uint32_t) ((p & 0xFFFFFF) ^ (p >> 32));
#else
      return reinterpret_cast<uint32_t> (key);
#endif
    }

    bool isEqualElementElement(Element * const & left,
                               Element * const & right) {
      return left == right;
    }

    bool isEqualKeyElement(Element * const & key, Element * const & element) {
      return key == element;
    }

    void clearElement(Element * & element) {
      element = 0;
    }
  };

  vector<Element*> elements;  // list of elements
  size_t numElements;  // number of used elements in the list of elements

  AssociativeArray<Element*, ParentChildrenPair*, Parent2ChildrenDesc> parentToChildren;
  AssociativeArray<Element*, ChildParentsPair*, Child2ParentsDesc> childToParents;
  AssociativeArray<string, Element*, Name2ElementDesc> nameToElement;
  AssociativeArray<PositionType, Element*, Position2ElementDesc> positionToElement;
};

#endif  // STOAP_OLAP_DIMENSION_H_
