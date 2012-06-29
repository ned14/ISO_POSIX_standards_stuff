g++ -std=c++0x -o unittests -DUSE_PARALLEL -I../intel_tbb/include unittests.cpp -lpthread -L ../intel_tbb/lib -ltbb_debug
if ERRORLEVEL 1 g++ -std=c++0x -o unittests unittests.cpp -lpthread
g++ -std=c++0x -o pthread_permit_speedtest pthread_permit_speedtest.cpp -lpthread
