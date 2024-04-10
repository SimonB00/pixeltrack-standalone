
#ifndef HITSCOORDSSOA_H
#define HITSCOORDSSOA_H

#include <cstdint>
#include <memory>
#include <vector>

struct HitsCoordsSoA {
  std::vector<float> x;
  std::vector<float> y;
  std::vector<float> z;
  std::vector<float> r;
  std::vector<int16_t> phi;
  std::vector<uint32_t> global_indexes;

  struct HitsCoordsSoAView {
    float* x;
    float* y;
    float* z;
    float* r;
    int16_t* phi;
    uint32_t* global_indexes;
  };

  std::unique_ptr<HitsCoordsSoAView> m_view;

  void reset() {
    m_view->x = x.data();
    m_view->y = y.data();
    m_view->z = z.data();
    m_view->r = r.data();
    m_view->phi = phi.data();
    m_view->global_indexes = global_indexes.data();
  }

  HitsCoordsSoAView* view() { return m_view.get(); };
  const HitsCoordsSoAView* view() const { return m_view.get(); };
};

#endif  // HITSCOORDSSOA_H
