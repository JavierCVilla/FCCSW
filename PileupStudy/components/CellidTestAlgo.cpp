#include "GaudiAlg/GaudiAlgorithm.h"

#include "FWCore/DataHandle.h"
#include "DetInterface/IGeoSvc.h"

#include "datamodel/MCParticleCollection.h"
#include "datamodel/TrackHitCollection.h"
#include "datamodel/GenVertex.h"

#include "DD4hep/LCDD.h"
#include "DDSegmentation/BitField64.h"

#include "TGeoManager.h"

class IGeoSvc;


void countNodes(TGeoNode* node, const std::string nodeName, int& nodeCount) {
  std::string currentNodeName = node->GetName();
  if (currentNodeName.find(nodeName) != std::string::npos) {
    ++nodeCount;
  }
  auto daughters = node->GetNodes();
  TIter iObj(daughters);
  while (TObject* obj = iObj()) {
    TGeoNode* next_node = dynamic_cast<TGeoNode*>(obj);
    if (next_node) countNodes(next_node, nodeName, nodeCount);
  }
}

class CellidTestAlgo : public GaudiAlgorithm {

public:
  CellidTestAlgo(const std::string& name, ISvcLocator* svcLoc) :
    GaudiAlgorithm(name, svcLoc)
  {
    declareInput("hits", m_trkHits, "hits");
  }

  ~CellidTestAlgo() {};

  StatusCode initialize() {
    m_geoSvc = service ("GeoSvc");
    return GaudiAlgorithm::initialize();
  }

  StatusCode execute() {
    auto lcdd = m_geoSvc->lcdd();
    auto allReadouts = lcdd->readouts();
    auto readoutBarrel = lcdd->readout("TrackerBarrelReadout");
    auto m_decoderBarrel = readoutBarrel.idSpec().decoder();
    auto readoutEndcap = lcdd->readout("TrackerEndcapReadout");
    auto m_decoderEndcap = readoutEndcap.idSpec().decoder();
    const fcc::TrackHitCollection* trkhits = m_trkHits.get();

    info() << "hits size: " << trkhits->size() << endmsg;
    int cntr = 0;
    for (const auto& hit : *trkhits) {

      if (10 < cntr++ || true) {
        auto aCellId = hit.Core().Cellid;

        m_decoderBarrel->setValue(aCellId);
        int system_id = (*m_decoderBarrel)["system"];
        if (system_id == 10 || system_id == 12 ) {
        int layer_id = (*m_decoderBarrel)["layer"];
        int rod_id = (*m_decoderBarrel)["rod"];
        int module_id = (*m_decoderBarrel)["module"];
        //int layer_id = -1;
        info() << "hit cellid: " << aCellId << "in system " << system_id << ", layer " << layer_id << ", rod " << rod_id << ", module" << module_id << endmsg;
        } else if (system_id == 11 || system_id == 13) {
        m_decoderEndcap->setValue(aCellId);
        int layer_id = (*m_decoderEndcap)["layer"];
        int petal_id = (*m_decoderEndcap)["petal"];
        info() << "hit cellid: " << aCellId << "in system " << system_id << ", layer " << layer_id << ", petal " << petal_id << endmsg;
        }

      }
    }
    
    // counting physical volumes
    info() << "Count physical volumes ... " << endmsg;
    auto highest_vol = gGeoManager->GetTopVolume();
    auto  daughters = highest_vol->GetNodes();
    int nodeCount = 0;
    // the sensitive part of the module has the material string in its name
    std::string nodeName = "Silicon";
    TIter iObj(daughters);
    info() << highest_vol->GetName() << endmsg;
    while (TObject* obj = iObj()) {
      // dynamic_cast to a node
      TGeoNode* node = dynamic_cast<TGeoNode*>(obj);
      info() << "\t  ... in " << node->GetName() << endmsg;
      if (node) countNodes(node, nodeName, nodeCount);
    }
    info() << nodeCount << " modules found!" << endmsg;
    return StatusCode::SUCCESS;
  }

  StatusCode finalize() {
    return GaudiAlgorithm::finalize();
  }

private:
  DataHandle<fcc::TrackHitCollection> m_trkHits;
  SmartIF<IGeoSvc> m_geoSvc;
};
DECLARE_COMPONENT(CellidTestAlgo)
