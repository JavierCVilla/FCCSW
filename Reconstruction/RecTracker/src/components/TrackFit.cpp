
#include "DetInterface/ITrackingGeoSvc.h"
#include "RecInterface/ITrkVolumeManagerSvc.h"
#include "DetInterface/IGeoSvc.h"
#include "RecInterface/ITrackSeedingTool.h"

#include "datamodel/TrackHitCollection.h"

#include "ACTS/Utilities/Identifier.hpp"
#include "ACTS/Detector/TrackingGeometry.hpp"
#include "ACTS/EventData/Measurement.hpp"
#include "ACTS/Examples/BuildGenericDetector.hpp"
#include "ACTS/Extrapolation/ExtrapolationCell.hpp"
#include "ACTS/Extrapolation/ExtrapolationEngine.hpp"
#include "ACTS/Extrapolation/IExtrapolationEngine.hpp"
#include "ACTS/Extrapolation/MaterialEffectsEngine.hpp"
#include "ACTS/Extrapolation/RungeKuttaEngine.hpp"
#include "ACTS/Extrapolation/StaticEngine.hpp"
#include "ACTS/Extrapolation/StaticNavigationEngine.hpp"
#include "ACTS/Fitter/KalmanFitter.hpp"
#include "ACTS/Fitter/KalmanUpdator.hpp"
#include "ACTS/MagneticField/ConstantBField.hpp"
#include "ACTS/Surfaces/PerigeeSurface.hpp"
#include "ACTS/Utilities/Definitions.hpp"
#include "ACTS/Utilities/Logger.hpp"

#include "datamodel/PositionedTrackHitCollection.h"
#include "datamodel/MCParticleCollection.h"
#include "datamodel/GenVertexCollection.h"
#include "datamodel/TrackHitCollection.h"



#include "DD4hep/LCDD.h"
#include "DD4hep/Volumes.h"
#include "DDSegmentation/BitField64.h"
#include "DDRec/API/IDDecoder.h"

#include <cmath>

#include "TrackFit.h"
#include "FastHelix.h"
#include "TrackParameterConversions.h"




DECLARE_ALGORITHM_FACTORY(TrackFit)

TrackFit::TrackFit(const std::string& name, ISvcLocator* svcLoc) : GaudiAlgorithm(name, svcLoc) {

  declareInput("positionedTrackHits", m_positionedTrackHits,"hits/TrackerPositionedHits");
  declareInput("genParticles", m_genParticlesHandle, "GenParticles");
  declareInput("genVertices", m_genVerticesHandle, "GenVertices");
  declareProperty("trackSeedingTool", m_trackSeedingTool);
  declarePublicTool(m_trackSeedingTool, "TrackSeedingTool/TruthSeedingTool");

  }

TrackFit::~TrackFit() {}

StatusCode TrackFit::initialize() {

  m_geoSvc = service ("GeoSvc");

  StatusCode sc = GaudiAlgorithm::initialize();
  if (sc.isFailure()) return sc;

  m_trkGeoSvc = service("TrackingGeoSvc");
  if (nullptr == m_trkGeoSvc) {
    error() << "Unable to locate Tracking Geometry Service. " << endmsg;
    return StatusCode::FAILURE;
  }

  m_trkVolMan = service("TrkVolMan");
  if (nullptr == m_trkVolMan) {
    error() << "Unable to locate Tracking Geometry Service. " << endmsg;
    return StatusCode::FAILURE;
  }

  m_trkGeo = m_trkGeoSvc->trackingGeometry();
  return sc;
}

StatusCode TrackFit::execute() {

    auto lcdd = m_geoSvc->lcdd();
    auto allReadouts = lcdd->readouts();
    auto readoutBarrel = lcdd->readout("TrackerBarrelReadout");
    auto m_decoderBarrel = readoutBarrel.idSpec().decoder();
    auto readoutEndcap = lcdd->readout("TrackerEndcapReadout");
    auto m_decoderEndcap = readoutEndcap.idSpec().decoder();

  const fcc::PositionedTrackHitCollection* hits = m_positionedTrackHits.get();
  const fcc::MCParticleCollection* mcparticles = m_genParticlesHandle.get();

  std::vector<FitMeas_t> fccMeasurements;
  std::vector<const Acts::Surface*> surfVec;
  fccMeasurements.reserve(hits->size());

  /// @todo: get local position from segmentation
  double fcc_l1 = 0;
  double fcc_l2 = 0;
  int hitcounter = 0;
  GlobalPoint middlePoint;
  GlobalPoint outerPoint;

  auto seedmap = m_trackSeedingTool->findSeeds(hits);
  for (auto it = seedmap.begin(); it != seedmap.end(); ++it) {
    if (it->first == 1) { // TrackID for primary particle

    auto hit = (*hits)[it->second]; // the TrackID maps to an index for the hit collection

    long long int theCellId = hit.core().cellId;
    debug() << theCellId << endmsg;
    debug() << "position: x: " << hit.position().x << "\t y: " << hit.position().y << "\t z: " << hit.position().z << endmsg; 
    debug() << "phi: " << std::atan2(hit.position().y, hit.position().x) << endmsg;
    m_decoderBarrel->setValue(theCellId);
    int system_id = (*m_decoderBarrel)["system"];
    debug() << " hit in system: " << system_id;
    int layer_id = (*m_decoderBarrel)["layer"];
    debug() << "\t layer " << layer_id;
    int module_id = (*m_decoderBarrel)["module"];
    debug() << "\t module " << module_id;
    debug() << endmsg;
    fcc_l1 = (*m_decoderBarrel)["x"] * 0.005;
    fcc_l2 = (*m_decoderBarrel)["z"] * 0.01;
    (*m_decoderBarrel)["x"] = 0; // workaround for broken `volumeID` method --
    (*m_decoderBarrel)["z"] = 0; // set everything not connected with the VolumeID to zero,
    // so the cellID can be used to look up the tracking surface
    if (hitcounter == 0) {
      middlePoint = GlobalPoint(hit.position().x, hit.position().y, hit.position().z);
    } else if (hitcounter == 1) {

      outerPoint = GlobalPoint(hit.position().x, hit.position().y, hit.position().z);
    }
    // need to use cellID without segmentation bits
    const Acts::Surface* fccSurf = m_trkVolMan->lookUpTrkSurface(Identifier(m_decoderBarrel->getValue()));
    debug() << " found surface pointer: " << fccSurf<< endmsg;
    if (nullptr != fccSurf) {
    double std1 = 0.001;
    double std2 = 0.001;
    ActsSymMatrixD<2> cov;
    cov << std1* std1, 0, 0, std2* std2;
    fccMeasurements.push_back(Meas_t<eLOC_1, eLOC_2>(*fccSurf, hit.core().cellId, std::move(cov), fcc_l1, fcc_l2));
    surfVec.push_back(fccSurf);

  // debug printouts
    
    ++hitcounter;
    }
    }
  }

  
  FastHelix helix(outerPoint, middlePoint, GlobalPoint(0,0,0), 0.006);
  PerigeeTrackParameters res = ParticleProperties2TrackParameters(GlobalPoint(0,0,0), helix.getPt(), 0.006, 1);
  

    double pT = 0;
    double                x  = 0;
    double                y  = 0;
    double                z  = 0;
    double                px = 0;//pT * cos(phi);
    double                py = 0;//pT * sin(phi);
    double                pz = 0;//pT / tan(theta);
    double                q  = 1; //(charge != 0) ? charge : +1;

    for (auto pp: *mcparticles) {
      px = 1000 * pp.core().p4.px;
      py = 1000 * pp.core().p4.py;
      pz = 1000 * pp.core().p4.pz;
    }
    Vector3D              pos(x, y, z);
    Vector3D              mom(px, py, pz);



  ActsVector<ParValue_t, NGlobalPars> pars;
  std::cout << res.d0 << "\t" << res.z0 << "\t" << res.phi0 << "\t" << res.qOverPt << std::endl;
    pars << res.d0, res.z0, res.phi0, 0.705026844, 0.0001959031;
  auto startCov =
      std::make_unique<ActsSymMatrix<ParValue_t, NGlobalPars>>(ActsSymMatrix<ParValue_t, NGlobalPars>::Identity());

  const Surface* pSurf =  m_trkGeo->getBeamline();
  //auto startTP = std::make_unique<BoundParameters>(std::move(startCov), std::move(pars), *pSurf);
      auto           startTP = std::make_unique<BoundParameters>(
          std::move(startCov), std::move(pos), std::move(mom), q, *pSurf);

std::cout << "mine: " << *startTP << std::endl;
  ExtrapolationCell<TrackParameters> exCell(*startTP);
  exCell.addConfigurationMode(ExtrapolationMode::CollectSensitive);
  exCell.addConfigurationMode(ExtrapolationMode::CollectPassive);
  exCell.addConfigurationMode(ExtrapolationMode::StopAtBoundary);

  auto exEngine = initExtrapolator(m_trkGeo);
  exEngine->extrapolate(exCell);

  debug() << "got " << exCell.extrapolationSteps.size() << " extrapolation steps" << endmsg;

  std::vector<FitMeas_t> vMeasurements;
  vMeasurements.reserve(exCell.extrapolationSteps.size());

  // identifier
  long int id = 0;
  // random numbers for smearing measurements
  /// @todo: use random number service
  std::default_random_engine e;
  std::uniform_real_distribution<double> std_loc1(1, 5);
  std::uniform_real_distribution<double> std_loc2(0.1, 2);
  std::normal_distribution<double> g(0, 1);

   double std1, std2, l1, l2;
  for (const auto& step : exCell.extrapolationSteps) {
    const auto& tp = step.parameters;
    std::cout << "epopos: " << tp->position().x() << "\t" << tp->position().y() << "\t" << tp->position().z() << std::endl;
    if (tp->associatedSurface().type() != Surface::Plane) continue;

    std1 = 1;//std_loc1(e);
    std2 = 1; //std_loc2(e);
    l1 = tp->get<eLOC_1>(); // + std1 * g(e);
    l2 = tp->get<eLOC_2>(); // + std2 * g(e);
    ActsSymMatrixD<2> cov;
    cov << std1* std1, 0, 0, std2* std2;
    vMeasurements.push_back(Meas_t<eLOC_1, eLOC_2>(tp->associatedSurface(), id, std::move(cov), l1, l2));
    ++id;
  }

  debug() << "created " << vMeasurements.size() << " pseudo-measurements" << endmsg;
  for (const auto& m : vMeasurements)
    info() << m << endmsg;

  debug() << "created " << fccMeasurements.size() << " fcc-measurements" << endmsg;
  for (const auto& m : fccMeasurements)
    info() << m << endmsg;

  KalmanFitter<MyExtrapolator, CacheGenerator, NoCalibration, GainMatrixUpdator> KF;
  KF.m_oCacheGenerator = CacheGenerator();
  KF.m_oCalibrator = NoCalibration();
  KF.m_oExtrapolator = MyExtrapolator(exEngine);
  KF.m_oUpdator = GainMatrixUpdator();

  if (fccMeasurements.size() > 0) {
  auto track = KF.fit(fccMeasurements, std::make_unique<BoundParameters>(*startTP));
  std::cout << "fit done!" << std::endl;

  // dump track
  int trackCounter = 0;
  for (const auto& p : track) {
  auto smoothedState = *p->getSmoothedState();
  auto cov = *smoothedState.covariance();
    debug() << *p->getCalibratedMeasurement() << endmsg;
    debug() << *p->getSmoothedState() << endmsg;
    double pt  = std::sqrt(smoothedState.momentum().x() * smoothedState.momentum().x() + smoothedState.momentum().y() * smoothedState.momentum().y());
  for (auto pp: *mcparticles) {
    if (trackCounter == fccMeasurements.size() - 1) {
    std::cout <<  "fitresult: " << std::sqrt(pp.core().p4.px * pp.core().p4.px + pp.core().p4.py * pp.core().p4.py) << "\t" << pt << "\t"  << pp.core().p4.pz << "\t" << smoothedState.momentum().z() <<  "\t" <<std::sqrt(cov(4,4)) << std::endl;
    }
  }
  trackCounter++;
  }
  }

  return StatusCode::SUCCESS;
}

StatusCode TrackFit::finalize() {
  StatusCode sc = GaudiAlgorithm::finalize();
  return sc;
}
