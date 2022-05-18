// C++ headers
#include <algorithm>
#include <numeric>

// CMSSW headers
#include "CUDACore/cudaCompat.h"

#include "plugin-SiPixelClusterizer/SiPixelRawToClusterGPUKernel.h"  // !
#include "plugin-SiPixelClusterizer/gpuClusteringConstants.h"        // !

#include "PixelRecHitsCustom.h"
#include "plugin-SiPixelRecHits/gpuPixelRecHits.h"

#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>


std::string path = "/home/simonb/documents/thesis/not_sorted/";
int n_events = 1770;
int nLayers = 48;

std::map<int,int> def_hits_map() {
  std::map<int,int> event_nhits = {};
  std::ifstream is_;
  is_.open(path + "hits_per_event.csv");
  std::string a;

  for(int i = 0; is_ >> a; ++i) {
    std::stringstream str(a);
    std::string row;
    std::vector<int> key_values;
  
    while(std::getline(str, row, ',')) {
      key_values.push_back(std::stoi(row));
    }
    event_nhits[key_values[0]] = key_values[1];
  }
  return event_nhits;
}

std::map<int,int> n_hits_map = def_hits_map();

namespace {
  __global__ void setHitsLayerStart(uint32_t const* __restrict__ hitsModuleStart,
                                    pixelCPEforGPU::ParamsOnGPU const* cpeParams,
                                    uint32_t* hitsLayerStart) {
    assert(0 == hitsModuleStart[0]);

    int begin = blockIdx.x * blockDim.x + threadIdx.x;
    constexpr int end = 11;
    for (int i = begin; i < end; i += blockDim.x * gridDim.x) {
      hitsLayerStart[i] = hitsModuleStart[cpeParams->layerGeometry().layerStart[i]];
#ifdef GPU_DEBUG
      printf("LayerStart %d %d: %d\n", i, cpeParams->layerGeometry().layerStart[i], hitsLayerStart[i]);
#endif
    }
  }
}  // namespace

namespace pixelgpudetails {

  // crea un nuovo costruttore con i miei vettori
  TrackingRecHit2DCPU PixelRecHitGPUKernelCustom::makeHits(SiPixelDigisSoA const& digis_d,
                                                     SiPixelClustersSoA const& clusters_d,
                                                     BeamSpotPOD const& bs_d,
                                                     pixelCPEforGPU::ParamsOnGPU const* cpeParams) const {
    auto nHits = clusters_d.nClusters();  // size vettori
    TrackingRecHit2DCPU hits_d(nHits, cpeParams, clusters_d.clusModuleStart(), nullptr);  // costruttore

    if (digis_d.nModules())  // protect from empty events
      gpuPixelRecHits::getHits(cpeParams, &bs_d, digis_d.view(), digis_d.nDigis(), clusters_d.view(), hits_d.view());

    // assuming full warp of threads is better than a smaller number...
    if (nHits) {
      setHitsLayerStart(clusters_d.clusModuleStart(), cpeParams, hits_d.hitsLayerStart());
    }

    if (nHits) {
      cms::cuda::fillManyFromVector(hits_d.phiBinner(), 10, hits_d.iphi(), hits_d.hitsLayerStart(), nHits, 256);
    }

    return hits_d;
  }

  TrackingRecHit2DCPU PixelRecHitGPUKernelCustom::makeHits2(int file_number) const {
    std::vector<float> hits_x_coordinates;
    std::vector<float> hits_y_coordinates;
    std::vector<float> hits_z_coordinates;
    std::vector<float> hits_r_coordinates;
    std::vector<int> global_indexes;
    std::vector<short> phi;

    if(file_number >= 5000 && file_number < 5500) {
      std::cout << "This file is missing" << '\n';
    } else {
    //std::string x_file_name = path + "x_ns" + std::to_string(file_number) + ".dat";
    //std::string y_file_name = path + "y_ns" + std::to_string(file_number) + ".dat";
    //std::string z_file_name = path + "z_ns" + std::to_string(file_number) + ".dat";
    //std::string index_file_name = path + "globalIndexes_ns" + std::to_string(file_number) + ".dat";
    std::string x_file_name = path + "x_blue" + std::to_string(file_number) + ".dat";
    std::string y_file_name = path + "y_blue" + std::to_string(file_number) + ".dat";
    std::string z_file_name = path + "z_blue" + std::to_string(file_number) + ".dat";
    std::string index_file_name = path + "globalIndexes_blue" + std::to_string(file_number) + ".dat";
    std::string phi_file_name = path + "phi_blue" + std::to_string(file_number) + ".dat";

    // Read the x_ns*.dat.dat file
    std::ifstream is_1;
    is_1.open(x_file_name);
    float a;

    // Create the vector containing all the x coordinates of the hits
    for(int i = 0; is_1 >> a; ++i) {
      hits_x_coordinates.push_back(a);
      }
      is_1.close();
      std::cout << "x " << hits_x_coordinates[0] << '\n';

      // Read the y_ns*.dat.dat file
      std::ifstream is_2;
      is_2.open(y_file_name);
      float b;

      // Create the vector containing all the y coordinates of the hits
      for(int i = 0; is_2 >> b; ++i) { 
        hits_y_coordinates.push_back(b); 
        }
      is_2.close();
      std::cout << "y " << hits_y_coordinates[0] << '\n';

      // Read the z_ns*.dat.dat file
      std::ifstream is_3;
      is_3.open(z_file_name);
      float c;

      // Create the vector containing all the z coordinates of the hits
      for(int i = 0; is_3 >> c; ++i) { 
        hits_z_coordinates.push_back(c); }
      is_3.close();
      std::cout << "z " << hits_z_coordinates[0] << '\n';
      for(int i = 0 ; i < static_cast<int>(hits_y_coordinates.size()); ++i) {
        hits_r_coordinates.push_back(sqrt(pow(hits_y_coordinates[i],2) + pow(hits_z_coordinates[i],2)));
      }

      // Fill the hit's global indexes
      std::ifstream is_4;
      is_4.open(index_file_name);
      int d;
      for(int i = 0; is_4 >> d; ++i) { 
        global_indexes.push_back(d); 
      }
      is_4.close();

      // Fill phi
      for(int i = 0; i < (int)(hits_x_coordinates.size()); ++i) {
        float e = atan(hits_y_coordinates[i]/hits_x_coordinates[i]);
        //std::cout << e << '\n';
        //std::cout << phi2short(e) << '\n';
        phi.push_back(phi2short(e));
      }
      std::cout << "phi " << phi[0] << '\n';
    }
    std::vector<uint32_t> layerStart_ = {0};
    for(int j = 1; j < static_cast<int>(global_indexes.size()) - 1; ++j) {
      if(global_indexes[j+1] != global_indexes[j]) {
        layerStart_.push_back(j+1);
      }
    }
    
    TrackingRecHit2DCPU hits_d(hits_x_coordinates, hits_y_coordinates, hits_z_coordinates, hits_r_coordinates, layerStart_, phi, nullptr);
    std::cout << "x " << hits_d.view()->xGlobal(0) << '\n';
    std::cout << "y " << hits_d.view()->yGlobal(0) << '\n';
    std::cout << "z " << hits_d.view()->zGlobal(0) << '\n';
    std::cout << "phi " << hits_d.view()->iphi(0) << '\n';
    hits_d.view()->m_iphi = phi.data();
    //std::cout << hits_d.view()->m_iphi[10] << '\n';
    cms::cuda::fillManyFromVector(hits_d.phiBinner(), 10, hits_d.view()->m_iphi, hits_d.hitsLayerStart(), hits_d.nHits(), 256);
    std::cout << "tutto ok dopo fill" << '\n';
    return hits_d;
  }

}  // namespace pixelgpudetails
