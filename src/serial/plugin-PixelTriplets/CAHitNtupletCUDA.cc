#include "Framework/EventSetup.h"
#include "Framework/Event.h"
#include "Framework/PluginFactory.h"
#include "Framework/EDProducer.h"
#include "Framework/RunningAverage.h"

#include "CAHitNtupletGeneratorOnGPU.h"
#include "CUDADataFormats/PixelTrackHeterogeneous.h"
#include "CUDADataFormats/TrackingRecHit2DHeterogeneous.h"

class CAHitNtupletCUDA : public edm::EDProducer {
public:
  explicit CAHitNtupletCUDA(edm::ProductRegistry& reg);
  ~CAHitNtupletCUDA() override = default;

private:
  void produce(edm::Event& iEvent, const edm::EventSetup& iSetup) override;

  edm::EDGetTokenT<TrackingRecHit2DCPU> tokenHitCPU_;
  edm::EDPutTokenT<PixelTrackHeterogeneous> tokenTrackCPU_;

  CAHitNtupletGeneratorOnGPU gpuAlgo_;
};

CAHitNtupletCUDA::CAHitNtupletCUDA(edm::ProductRegistry& reg)
    : tokenHitCPU_{reg.consumes<TrackingRecHit2DCPU>()},
      tokenTrackCPU_{reg.produces<PixelTrackHeterogeneous>()},
      gpuAlgo_(reg) {}

void CAHitNtupletCUDA::produce(edm::Event& iEvent, const edm::EventSetup& es) {
  auto bf = 0.0114256972711507*2;  // 1/fieldInGeV
  std::cout << "RITORNATO QUALCOSA?-1 " << std::endl;
  auto const& hits = iEvent.get(tokenHitCPU_);
  std::cout << "RITORNATO QUALCOSA?0 " << std::endl;
  PixelTrackHeterogeneous tuples_ = gpuAlgo_.makeTuples(hits, bf);
  std::cout << "RITORNATO QUALCOSA? " << std::endl;
  // iEvent.emplace(tokenTrackCPU_, gpuAlgo_.makeTuples(hits, bf));
  std::cout << "nHITS" << tuples_->hitIndices.size() << '\n';
  for(auto& hi : tuples_->hitIndices){
    std::cout << "HI " << hi << std::endl;
  }

}

DEFINE_FWK_MODULE(CAHitNtupletCUDA);
