
#ifndef HITSCOORDSSOA_H
#define HITSCOORDSSOA_H

#include <vector>

struct HitsCoordsSoA {
  std::vector<float> x;
  std::vector<float> y;
  std::vector<float> z;
  std::vector<float> r;
  std::vector<float> phi;
  std::vector<int> global_indexes;

  struct HitsCoordsSoAView {
    float* x;
    float* y;
    float* z;
    float* r;
    float* phi;
    int* global_indexes;
  };
};

#endif	// HITSCOORDSSOA_H
