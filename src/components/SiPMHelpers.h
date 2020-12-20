#include <iostream>
#include <chrono>

using namespace std;

struct Timer{
  Timer();
  ~Timer();
  chrono::time_point<chrono::high_resolution_clock> start;
};

Timer::Timer(){
  start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer(){
  auto end = std::chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  std::cout << "Timer took: " << duration.count() << " s" <<'\n';
  std::cout << "Timer took: " << duration.count()*1000 <<" ms" <<'\n';
  std::cout << "Timer took: " << duration.count()*1e6 <<" us" <<'\n';
  std::cout << endl;
}
