# Single-threaded OLAP Aggregation Processor (StOAP)

StOAP is the C++ implementation of a single-threaded in-memory multidimensional OLAP (MOLAP) aggregation processor. It is based on a
heavily stripped-down mix of open-source code from versions 3.1 and [5.1](http://sourceforge.net/p/palo/code/HEAD/tree/molap/server/5.1/) of the in-memory MOLAP
server *[Palo](http://en.wikipedia.org/wiki/Palo_%28OLAP_database%29)* by Jedox AG. All features not related to the loading and processing of cube data, such as user management, HTTP request handling, caching, and many more have been
removed. Since StOAP was created solely to compare single-threaded aggregation with its parallel GPU aggregation counterpart in my bachelor thesis, the focus is the aggregation of values along multiple dimensional hierarchies of the cube. There is no functionality for modifying and saving cube values.

## Features

* single-threaded
* weighted, source-based aggregation
* in-memory data processing
* ability to load MOLAP data cubes built by the Jedox OLAP server (only numerical values)
* command-line interface for loading a cube and retrieving cell values
* interface for inter-process communication using named pipes

## Usage

After the program was successfully compiled (see below for instructions), it can be executed like the following:
```
Usage: ./stoapMain [options] <database-path>
<database-path> path to a folder containing database and cube files.
Options:
 -v, --log-level: log-level can be [0|1|2|3|4].
 -s, --server-mode: if specified, an input and output FIFO file will be created
                    in /tmp/stoap-in and /tmp/stoap-out. All logger output goes to
                    a log file called 'StOAP.INFO'.
```

The *Data* directory provides an example cube with approximately 1.3M filled base cells.

### Server Mode

If the argument `-s` was given, one can send request to the named pipe /tmp/stoap-in.
Requests can be of the following forms (same syntax as the HTTP requests for the Jedox OLAP server):

```
/cell/values?paths=(path_1):(path_2):...:(path_n)
```
where `(path)` is a spatial key with element indices separated by commas, e.g. `0,5,0,2,8,2`.

```
/cell/area?area=(elset_1),(elset_2),...,(elset_n)
```
where `(elset)` is a set of element indices separated by colons, e.g. `0:1:2:3:4:5`.

After the request was sent to /tmp/stoap-in, one can fetch the answer from /tmp/stoap-out:

```
$ cat /tmp/stoap-out
```

### Command-line Interface (CLI)

If the argument `-s` was not given, one will be dropped to a CLI. The following commands are available:

* help
* getCell `(path)`
* getArea `{(r1)x(r2)x...x(rn)}`, with `(r)` being comma-separated ranges of element indices
  * Example: getArea `2x0x2-4x5,8-12x7-11` (for a cube with 5 dimensions)
* info cube
* info dimensions
* info storage
* exit

## Aggregation method

After a cell value request was received, the first step is the construction of an aggregation map. Furthermore, a hash table for holding the results is allocated. Only then the actual aggregation process starts: One filled source cell after another is iterated. For each of them the value is added to all assigned target cells.
The following pseudo-code illustrates how the source-based aggregation works:

```c++
for(cellIterator srcCell = cube.begin(); srcCell != cube.end(); ++srcCell) {
	if (!isPartOfAggregation(srcCell->path)) continue;
	while(targetExistsFor(srcCell->path)) {
		int[dimCount] targetPath;
		for(dimId in dimensions) {
			targetPath[dimId] = retrieveTargetId(dimId);
		}
		aggregate(targetPath, srcCell->value);
	}
}
```

## Open Challenge: Efficient Cube Data Structure

The major bottleneck for the performance of the aggregation in StOAP is the implementation of the cube. Since all filled cube cells are iterated in the aggregation algorithm, an efficient data structure for the cube’s in-memory fact table storage is a key requirement to obtain fast run-times. Hence, this data structure must provide various features:

* support for keys of arbitrary length, as a cube may consist of a large number of dimensions
* iteration over all stored keys and values
* support for insertion, update and lookup
* space-efficiency, because all data is kept in the RAM

Multiple publications, such as [AH13](http://ojs.academypublisher.com/index.php/jcp/article/view/jcp080511361144), [Böh+11](https://wwwdb.inf.tu-dresden.de/misc/team/boehm/pubs/btw2011.pdf) and [ZZN14](http://doi.acm.org/10.1145/2588555.2588564), suggest using extensible multidimensional arrays, kD-, prefix-, B+ -trees and other sophisticated data structures for the implementation of a multidimensional storage. Despite this, I could not find a documented, open-source and ready-to-use implementation satisfying all requirements listed above. As a temporary solution the cube structure was implemented using Google’s dense hash map, which is a part of [SHP12](https://code.google.com/p/sparsehash/). In the code a spatial key is implemented as  `vector<size_t>`, but stored as `uint64_t` in the map. The key of `vector<size_t>` is converted using bin packing (each dimension has its own bitmask).

## Building StOAP for Linux

Install the required build tools and libraries on your system:

* CMake >= 2.8
* gcc version >= 4.4.7
* Boost libraries 1.56 with the thread, system, regex, and timer components 
* Google glog
* Google sparsehash

Then execute the following steps:

```
$ git clone git://github.com/jmeinke/StOAP.git && cd StOAP
$ git checkout master
$ mkdir build
$ cd build
$ cmake ../
$ make
```

The *stoapMain* executable should now be available in build/bin.
Building StOAP on Windows has not yet been tested.

### Documentation using Doxygen

After using cmake to create makefiles in the `build` directory, documentation can be generated using Doxygen like the following:

```
$ cd doc
$ doxygen
```
