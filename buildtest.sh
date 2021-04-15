#!/bin/bash

rm -rf build/
mkdir build
cd build
cmake ..
make
# make install
#echo "**************** Test for cluster_api ****************"
#./test/cluster_api_test
#echo "*************** Test for cluster_server **************"
#./server_test/cluster_server_test
