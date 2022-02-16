#ifndef ECAL_PHISYM_RECHIT_H
#define ECAL_PHISYM_RECHIT_H

/** \class EcalPhiSymRecHit
 * 
 * Dataformat dedicated to Phi Symmetry ecal calibration
 * 
 * Note: SumEt array ordering:
 *       0         - central value
 *       1<->N/2   - misCalib<1
 *       N/2+1<->N - misCalib>1
 *
 * Original Author: Simone Pigazzini (2022)
 */

#include <vector>
#include <cassert>

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"

class EcalPhiSymRecHit
{
public:
    //---ctors---
    EcalPhiSymRecHit();
    EcalPhiSymRecHit(uint32_t id, unsigned int nMisCalibV, unsigned int status=0);
    EcalPhiSymRecHit(uint32_t id, std::vector<float>& etValues, unsigned int status=0);
    
    //---dtor---
    ~EcalPhiSymRecHit();

    //---getters---
    inline uint32_t       GetRawId()        const {return id_;};
    inline unsigned int   GetStatusCode()   const {return chStatus_;};
    inline uint32_t       GetNhits()        const {return nHits_;};
    inline unsigned int   GetNSumEt()       const {return etSum_.size();};
    inline float          GetSumEt(int i=0) const {return etSum_[i];};
    inline float          GetSumEt2()       const {return et2Sum_;};
    inline float          GetLCSum()        const {return lcSum_;};
    inline float          GetLC2Sum()       const {return lc2Sum_;};

    //---utils---
    void         AddHit(float* etValues, float laserCorr=0);
    void         AddHit(std::vector<float>& etValues, float laserCorr=0);
    void         Reset();

    //---operators---
    EcalPhiSymRecHit&    operator+=(const EcalPhiSymRecHit& rhs);

private:

    uint32_t           id_;
    unsigned int       chStatus_;
    uint32_t           nHits_;
    std::vector<float> etSum_;
    float              et2Sum_;
    float              lcSum_;
    float              lc2Sum_;    
};

typedef std::vector<EcalPhiSymRecHit> EcalPhiSymRecHitCollection;

#endif
