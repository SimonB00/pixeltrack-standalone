//
// Original Author: Felice Pantaleo, CERN
//

#include <array>
#include <cassert>
#include <functional>
#include <vector>
#include <fstream>

#include "Framework/Event.h"

#include "CAHitNtupletGeneratorOnGPU.h"


namespace {

  template <typename T>
  T sqr(T x) {
    return x * x;
  }

  cAHitNtupletGenerator::QualityCuts makeQualityCuts() {
    auto coeff = std::vector<double>{0.68177776, 0.74609577, -0.08035491, 0.00315399};  // chi2Coeff
    return cAHitNtupletGenerator::QualityCuts{// polynomial coefficients for the pT-dependent chi2 cut
                                              {(float)coeff[0], (float)coeff[1], (float)coeff[2], (float)coeff[3]},
                                              // max pT used to determine the chi2 cut
                                              10.f,  // chi2MaxPt
                                                     // chi2 scale factor: 30 for broken line fit, 45 for Riemann fit
                                              30.f,  // chi2Scale
                                                     // regional cuts for triplets
                                              {
                                                  0.3f,  //tripletMaxTip
                                                  0.5f,  // tripletMinPt
                                                  12.f   // tripletMaxZip
                                              },
                                              // regional cuts for quadruplets
                                              {
                                                  0.5f,  // quadrupletMaxTip
                                                  0.3f,  // quadrupletMinPt
                                                  12.f   // quadrupletMaxZip
                                              }};
  }
}  // namespace

using namespace std;
CAHitNtupletGeneratorOnGPU::CAHitNtupletGeneratorOnGPU(edm::ProductRegistry& reg)
    : m_params(false,             // onGPU
               3,                 // minHitsPerNtuplet,
               458752,            // maxNumberOfDoublets
               false,             //useRiemannFit
               true,              // fit5as4,
               true,              //includeJumpingForwardDoublets
               true,              // earlyFishbone
               false,             // lateFishbone
               true,              // idealConditions
               false,             //fillStatistics
               true,              // doClusterCut
               true,              // doZ0Cut
               true,              // doPtCut
               0.899999976158,    // ptmin
               0.00200000009499,  // CAThetaCutBarrel
               0.00300000002608,  // CAThetaCutForward
               0.0328407224959,   // hardCurvCut
               0.15000000596,     // dcaCutInnerTriplet
               0.25,              // dcaCutOuterTriplet
               makeQualityCuts()) {
#ifdef DUMP_GPU_TK_TUPLES
  printf("TK: %s %s % %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
         "tid",
         "qual",
         "nh",
         "charge",
         "pt",
         "eta",
         "phi",
         "tip",
         "zip",
         "chi2",
         "h1",
         "h2",
         "h3",
         "h4",
         "h5");
#endif

  m_counters = new Counters();
  memset(m_counters, 0, sizeof(Counters));
}

CAHitNtupletGeneratorOnGPU::~CAHitNtupletGeneratorOnGPU() {
  if (m_params.doStats_) {
    CAHitNtupletGeneratorKernelsCPU::printCounters(m_counters);
  }
  delete m_counters;
}

PixelTrackHeterogeneous CAHitNtupletGeneratorOnGPU::makeTuples(TrackingRecHit2DCPU const& hits_d, float bfield) const {
  PixelTrackHeterogeneous tracks(std::make_unique<pixelTrack::TrackSoA>());
  auto* soa = tracks.get();   // Oggetto che punta alle tracks
  assert(soa);
  std::cout << " BEFORE KERNELS INITI" << std::endl;
  CAHitNtupletGeneratorKernelsCPU kernels(m_params);
  std::cout << " AFTER KERNELS INITI" << std::endl;
  kernels.counters_ = m_counters;
  kernels.allocateOnGPU(nullptr);
  std::cout << " AFTER allocateGPU" << std::endl;

  kernels.buildDoublets(hits_d, nullptr);
  std::cout << " AFTER buildDoublets" << std::endl;
  kernels.launchKernels(hits_d, soa, nullptr);
  std::cout << " AFTER launchKernels" << std::endl;
  // std::cout << "NUMBER OF TRACKS " << soa->m_nTracks << std::endl;
  kernels.fillHitDetIndices(hits_d.view(), soa, nullptr);  // in principle needed only if Hits not "available"
  std::cout << " AFTER fillHitDetIndices" << std::endl;
  if (0 == hits_d.nHits())
    return tracks;

  // now fit
  HelixFitOnGPU fitter(bfield, m_params.fit5as4_);
  fitter.allocateOnGPU(&(soa->hitIndices), kernels.tupleMultiplicity(), soa);
  std::cout << " AFTER allocateOnGPUfitter" << std::endl;
  
  if (m_params.useRiemannFit_) {
    fitter.launchRiemannKernelsOnCPU(hits_d.view(), hits_d.nHits(), CAConstants::maxNumberOfQuadruplets());
  } else {
    fitter.launchBrokenLineKernelsOnCPU(hits_d.view(), hits_d.nHits(), CAConstants::maxNumberOfQuadruplets());
  }
  std::cout << " berfore classifytuple " << std::endl;
  kernels.classifyTuples(hits_d, soa, nullptr);
  std::cout << " after classifyTuples " << std::endl;
  
  // Write the tracks on a file
  std::string trackPath = "/home/simonb/documents/thesis/tracksData/tracks6000.dat";
  std::ofstream trackFile;
  trackFile.open(trackPath);
  for(auto & hi : soa->hitIndices){
    std::cout << " makeTuples HI " << hi << std::endl;
    trackFile << soa->hitIndices->capacity() << '\n';
    trackFile << hi << '\n';
  }
  trackFile.close();
  return tracks;
}