#! /bin/bash
export TBB_INCLUDE=/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/tbb/2019_U9-bcolbf/include
export TBB_DIR=/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/tbb/2019_U9-bcolbf/lib

g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o external_task external_task.cc
g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o external_group_looping external_group_looping.cc
g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o external_group_extra_thread external_group_extra_thread.cc
g++ -std=c++17 -I${TBB_INCLUDE} -L${TBB_DIR} -ltbb -pthread -o external_group_extended external_group_extended.cc
