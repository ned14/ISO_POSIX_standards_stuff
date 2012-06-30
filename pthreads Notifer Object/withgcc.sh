g++ -std=c++0x -o unittests -DUSE_PARALLEL unittests.cpp -lrt -ltbb
if [ "$?" != "0" ]; then
  g++ -std=c++0x -o unittests unittests.cpp -lrt
fi
g++ -std=c++0x -o pthread_permit_speedtest pthread_permit_speedtest.cpp -lrt
