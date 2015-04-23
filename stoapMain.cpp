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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <vector>
#include "Olap.h"
#include "glog/logging.h"
#include "Stoap/AggregationEnvironment.h"

using std::string;
using std::vector;
using std::endl;


void signalHandler(int signal) {
  LOG(INFO) << "Signaled to stop.";
  exit(0);
}

// Main function.
int main(int argc, char** argv) {
  cout << "====================================================================" << endl;
  cout << "== " << PACKAGE_NAME << endl;
  cout << "== Version " << PACKAGE_VERSION << endl;
  cout << "== Author " << PACKAGE_AUTHOR << endl;
  cout << "== " << PACKAGE_BUGREPORT << endl;
  cout << "====================================================================" << endl;
  cout << "" << endl;

  // prevent memory from being paged to the swap area
  mlockall(MCL_CURRENT | MCL_FUTURE);

  // Initialize the singleton
  AggrEnv& aggrEnv = AggrEnv::instance();

  // parse cmdline arguments
  aggrEnv.parseCommandLineArguments(argc, argv);

  // set up logging - some flags for this have been set by above function
  google::InitGoogleLogging(argv[0]);

  // handle failure signals
  google::InstallFailureSignalHandler();

  // handle exit signals
  signal(SIGINT, &signalHandler);  // SIGINT is produced when pressing Ctrl-C
  signal(SIGQUIT, &signalHandler);  // SIGQUIT is produced when pressing Ctrl-backslash

  FLAGS_logbufsecs = 1;
  FLAGS_logtostderr = 1;
  if(aggrEnv.isServerMode()){
    // write to a file instead of stderr
    google::SetLogSymlink(0, string("StOAP").c_str());
    google::SetLogDestination(0, string("StOAP").c_str());
    FLAGS_logtostderr = 0;
  }

  // read the dimensions and elements
  aggrEnv.loadDimensions();

  // get the list of available cubes
  aggrEnv.collectCubeInfo();

  // select and load the cube
  aggrEnv.selectAndLoadCube();

  // if --server-mode is enabled, a pipe will be opened for listening
  if(aggrEnv.isServerMode()) {
    aggrEnv.openPipe();
  } else {
    aggrEnv.askQuery();
  }

  google::FlushLogFiles(0);
  cout << "" << endl;
  cout << "Good bye!" << endl;
  exit(0);
}
