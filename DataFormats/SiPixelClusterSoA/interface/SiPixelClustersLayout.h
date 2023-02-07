#ifndef DataFormats_SiPixelClusterSoA_interface_SiPixelClustersLayout_h
#define DataFormats_SiPixelClusterSoA_interface_SiPixelClustersLayout_h

#include "DataFormats/SoATemplate/interface/SoALayout.h"

GENERATE_SOA_LAYOUT(SiPixelClustersLayout,
                    SOA_COLUMN(uint32_t, moduleStart),
                    SOA_COLUMN(uint32_t, clusInModule),
                    SOA_COLUMN(uint32_t, moduleId),
                    SOA_COLUMN(uint32_t, clusModuleStart))

using SiPixelClustersLayoutSoA = SiPixelClustersLayout<>;
using SiPixelClustersLayoutSOAView = SiPixelClustersLayout<>::View;
using SiPixelClustersLayoutSOAConstView = SiPixelClustersLayout<>::ConstView;


#endif  // DataFormats_SiPixelClusterSoA_interface_SiPixelClustersLayout_h
