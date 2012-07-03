clang -std=c++11 -o unittests -DUSE_PARALLEL pthread_permit.c unittests.cpp -lrt -ltbb
if [ "$?" != "0" ]; then
  clang -std=c++11 -o unittests pthread_permit.c unittests.cpp -lrt
fi
clang -std=c++11 -o pthread_permit_speedtest pthread_permit.c pthread_permit_speedtest.cpp -lrt
