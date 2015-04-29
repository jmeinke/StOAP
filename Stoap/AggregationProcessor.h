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

#ifndef STOAP_STOAP_AGGREGATIONPROCESSOR_H_
#define STOAP_STOAP_AGGREGATIONPROCESSOR_H_

#include "Olap.h"
#include "Engine/AggregationMap.h"
#include "Olap/DoubleStorage.h"

class AggregationProcessor {
 public:
  enum AggregationType {
    SUM = 0,
    AVG,
    COUNT,
    MAX,
    MIN
  };

  AggregationProcessor(CubeArea* cArea, AggregationType cType);
  ~AggregationProcessor();

  void aggregate();
  void print();
  string result(const vector<IdentifiersType>& request, bool addPath = false, bool addZero = false);

 protected:
  void aggregateCell(const IdentifiersType &key, const double value);
  size_t getNumTargets(const IdentifiersType &key);
  void initParentKey(const IdentifiersType &key, size_t &multiDimCount,
                     double *fixedWeight);
  void nextParentKey(size_t multiDimCount, size_t &changeMultiDim);

  // the area of relevant source cells
  CubeArea* srcArea;

  // the area to be calculated
  CubeArea* calcArea;

  // the type of the aggregation
  AggregationType calcType;

  // the storage which will hold the computed values
  DoubleStorage* resultStorage;
  vector<size_t> resultSize;

  IdentifiersType prevSourceKey;
  IdentifiersType lastKeyParent;
  vector<AggregationMap::TargetReader> lastTargets;
  vector<AggregationMap::TargetReader> currentTarget;
  IdentifiersType parentKey;
  IdentifiersType multiDims;
  AggregationMaps parentMaps;
};

#endif /* AGGREGATIONPROCESSOR_H_ */
