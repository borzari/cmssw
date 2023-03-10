#include <alpaka/alpaka.hpp>
#include "HeterogeneousCore/AlpakaInterface/interface/devices.h"
#include "HeterogeneousCore/AlpakaInterface/interface/host.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"

#include "DataFormats/Vertex/interface/ZVertexDefinitions.h"
#include "DataFormats/Vertex/interface/ZVertexSoAHost.h"
#include "DataFormats/Vertex/interface/alpaka/ZVertexUtilities.h"
#include "DataFormats/Vertex/interface/alpaka/ZVertexSoADevice.h"

#include "RecoPixelVertexing/PixelVertexFinding/plugins/PixelVertexWorkSpaceLayout.h"
#include "RecoPixelVertexing/PixelVertexFinding/plugins/PixelVertexWorkSpaceSoAHost.h"
#include "RecoPixelVertexing/PixelVertexFinding/plugins/alpaka/PixelVertexWorkSpaceSoADevice.h"

using namespace std;
using namespace ALPAKA_ACCELERATOR_NAMESPACE;

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  namespace vertexfinder_t {
    void runKernels(Queue& queue);
  }

};  // namespace ALPAKA_ACCELERATOR_NAMESPACE

int main() {
  const auto host = cms::alpakatools::host();
  const auto device = cms::alpakatools::devices<Platform>()[0];
  Queue queue(device);

  vertexfinder_t::runKernels(queue);
  return 0;
}
