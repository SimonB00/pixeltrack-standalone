// C++ headers
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>

// CMSSW headers
#include "CUDACore/cudaCompat.h"

#include "plugin-SiPixelClusterizer/SiPixelRawToClusterGPUKernel.h"  // !
#include "plugin-SiPixelClusterizer/gpuClusteringConstants.h"        // !

#include "PixelRecHits.h"
#include "gpuPixelRecHits.h"

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

namespace {
  void setHitsLayerStart(uint32_t const* __restrict__ hitsModuleStart,
                         pixelCPEforGPU::ParamsOnGPU const* cpeParams,
                         uint32_t* hitsLayerStart) {
    assert(0 == hitsModuleStart[0]);

    int begin = 0;
    constexpr int end = 11;
    for (int i = begin; i < end; i += 1) {
      hitsLayerStart[i] = hitsModuleStart[cpeParams->layerGeometry().layerStart[i]];
#ifdef GPU_DEBUG
      printf("LayerStart %d %d: %d\n",
             i,
             cpeParams->layerGeometry().layerStart[i],
             hitsLayerStart[i]);
#endif
    }
  }
}  // namespace

namespace pixelgpudetails {

  HitsCoordsSoAView view() {
    return HitsCoordsSoAView{
        x.data(), y.data(), z.data(), r.data(), phi.data(), global_indexes.data()};
  };

  TrackingRecHit2DCPU PixelRecHitGPUKernel::makeHits(
      SiPixelDigisSoA const& digis_d,
      SiPixelClustersSoA const& clusters_d,
      BeamSpotPOD const& bs_d,
      pixelCPEforGPU::ParamsOnGPU const* cpeParams) const {
    auto nHits = clusters_d.nClusters();
    TrackingRecHit2DCPU hits_d(nHits, cpeParams, clusters_d.clusModuleStart(), nullptr);

    if (digis_d.nModules())  // protect from empty events
      gpuPixelRecHits::getHits(
          cpeParams, &bs_d, digis_d.view(), digis_d.nDigis(), clusters_d.view(), hits_d.view());

    // assuming full warp of threads is better than a smaller number...
    if (nHits) {
      setHitsLayerStart(clusters_d.clusModuleStart(), cpeParams, hits_d.hitsLayerStart());
    }

    if (nHits) {
      cms::cuda::fillManyFromVector(
          hits_d.phiBinner(), 10, hits_d.iphi(), hits_d.hitsLayerStart(), nHits);
    }

    return hits_d;
  }

  TrackingRecHit2DCPU::makeHits(const std::string& fileName) {
    HitsCoordsSoA hits;
    uint32_t nHits{};

    std::fstream iFile(fileName);
    if (!iFile.is_open()) {
      std::cerr << "Error opening file" << std::endl;
    }

    std::string line;
    // TODO: maybe add a bit of error handling in the header
    getline(iFile, line);
    while (getline(iFile, line)) {
      std::stringstream fileStream(line);
      std::string temp;

      getline(fileStream, temp, ',');
      float x{std::stof(temp)};
      hits.x.push_back(x);
      getline(fileStream, temp, ',');
      float y{std::stof(temp)};
      hits.y.push_back(y);
      getline(fileStream, temp, ',') hits.z.push_back(std::stof(temp));

      hits.r.push_back(std::sqrt(x * x + y * y));

      getline(fileStream, temp, ',');
      hits.global_indexes.push_back(std::stoi(temp));
      getline(fileStream, temp);
      hits.phi.push_back(std::stof(temp));

      ++nHits;
    }

    std::vector<uint32_t> layerStart_ = {0};
    for (size_t j{1}; j < hits.global_indexes.size() - 1; ++j) {
      if (hits.global_indexes[j + 1] != hits.global_indexes[j]) {
        layerStart_.push_back(j + 1);
      }
    }

    TrackingRecHit2DCPU hits_d(nHits, hits.view(), layerStart_, nullptr);
    return hits_d;
  }

}  // namespace pixelgpudetails
