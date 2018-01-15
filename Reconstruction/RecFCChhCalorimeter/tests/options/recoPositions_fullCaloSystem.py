from Gaudi.Configuration import *

from Configurables import ApplicationMgr, FCCDataSvc, PodioOutput

# ECAL readouts
ecalBarrelReadoutName = "ECalBarrelEta"
ecalBarrelReadoutNamePhiEta = "ECalBarrelPhiEta"
ecalEndcapReadoutName = "EMECPhiEta"
ecalFwdReadoutName = "EMFwdPhiEta"
# HCAL readouts
hcalBarrelReadoutName = "BarHCal_Readout"
hcalBarrelReadoutNamePhiEta = hcalBarrelReadoutName + "_phieta"
hcalExtBarrelReadoutName = "ExtBarHCal_Readout"
hcalExtBarrelReadoutNamePhiEta = hcalExtBarrelReadoutName + "_phieta"
hcalEndcapReadoutName = "HECPhiEta"
hcalFwdReadoutName = "HFwdPhiEta"
# Tail Catcher readout
tailCatcherReadoutName = "Muons_Readout"
# layer radii needed for cell positions after re-segmentation 
ecalBarrelLayerRadii = [193.0] + [198.5] + [207.5] + [216.5] + [225.5] + [234.5] + [243.5] + [252.5];
# layers to be merged in endcaps, field name of the readout
ecalEndcapNumberOfLayersToMerge = [2] + [2] + [4]*38
hcalEndcapNumberOfLayersToMerge = [2] + [4]*20

podioevent = FCCDataSvc("EventDataSvc", input="/eos/experiment/fcc/hh/simulation/samples/v01/singlePart/pim/bFieldOn/eta0/100GeV/simu/output_condor_novaj_201801061004386054.root")

# reads HepMC text file and write the HepMC::GenEvent to the data service
from Configurables import PodioInput
podioinput = PodioInput("PodioReader", collections = ["ECalBarrelCells", "HCalBarrelCells", "HCalExtBarrelCells", "ECalEndcapCells", "HCalEndcapCells", "ECalFwdCells", "HCalFwdCells", "TailCatcherCells", "GenParticles", "GenVertices"], OutputLevel = DEBUG)

from Configurables import GeoSvc
detectors_to_use=['file:Detector/DetFCChhBaseline1/compact/FCChh_DectEmptyMaster.xml',
                  'file:Detector/DetFCChhTrackerTkLayout/compact/Tracker.xml',
                  'file:Detector/DetFCChhECalInclined/compact/FCChh_ECalBarrel_withCryostat.xml',
                  'file:Detector/DetFCChhHCalTile/compact/FCChh_HCalBarrel_TileCal.xml',
                  'file:Detector/DetFCChhHCalTile/compact/FCChh_HCalExtendedBarrel_TileCal.xml',
                  'file:Detector/DetFCChhCalDiscs/compact/Endcaps_coneCryo.xml',
                  'file:Detector/DetFCChhCalDiscs/compact/Forward_coneCryo.xml',
                  'file:Detector/DetFCChhTailCatcher/compact/FCChh_TailCatcher.xml',
                  'file:Detector/DetFCChhBaseline1/compact/FCChh_Solenoids.xml',
                  'file:Detector/DetFCChhBaseline1/compact/FCChh_Shielding.xml']

geoservice = GeoSvc("GeoSvc", detectors = detectors_to_use, OutputLevel = WARNING)

#Configure tools for calo cell positions
from Configurables import CellPositionsECalBTool, CellPositionsHCalBTool, CellPositionsCaloDiscsTool, CellPositionsTailCatcherTool 
ECalBcells = CellPositionsECalBTool("CellPositionsECalB", 
                                    readoutName = ecalBarrelReadoutNamePhiEta, 
                                    layerRadii = ecalBarrelLayerRadii, 
                                    OutputLevel = INFO)
EMECcells = CellPositionsCaloDiscsTool("CellPositionsEMEC", 
                                    readoutName = ecalEndcapReadoutName, 
                                    mergedLayers = ecalEndcapNumberOfLayersToMerge, 
                                    OutputLevel = INFO)
ECalFwdcells = CellPositionsCaloDiscsTool("CellPositionsECalFwd", 
                                        readoutName = ecalFwdReadoutName, 
                                        OutputLevel = INFO)
HCalBcells = CellPositionsHCalBTool("CellPositionsHCalB", 
                                    readoutName = hcalBarrelReadoutName, 
                                    OutputLevel = INFO)
HCalExtBcells = CellPositionsHCalBTool("CellPositionsHCalExtB", 
                                       readoutName = hcalExtBarrelReadoutName, 
                                       OutputLevel = INFO)
HECcells = CellPositionsCaloDiscsTool("CellPositionsHEC", 
                                   readoutName = hcalEndcapReadoutName, 
                                   mergedLayers = hcalEndcapNumberOfLayersToMerge, 
                                   OutputLevel = INFO)
HCalFwdcells = CellPositionsCaloDiscsTool("CellPositionsHCalFwd", 
                                        readoutName = hcalFwdReadoutName, 
                                        OutputLevel = INFO)
TailCatchercells = CellPositionsTailCatcherTool("CellPositionsTailCatcher", 
                                                readoutName = tailCatcherReadoutName, 
                                                centralRadius = 901.5,
                                                OutputLevel = INFO)

# cell positions
from Configurables import CreateCellPositions
positionsEcalBarrel = CreateCellPositions("positionsEcalBarrel", 
                                          positionsTool=ECalBcells, 
                                          hits = "ECalBarrelCells", 
                                          positionedHits = "ECalBarrelCellPositions", 
                                          OutputLevel = INFO)
positionsHcalBarrel = CreateCellPositions("positionsHcalBarrel", 
                                          positionsTool=HCalBcells, 
                                          hits = "HCalBarrelCells", 
                                          positionedHits = "HCalBarrelCellPositions", 
                                          OutputLevel = INFO)
positionsHcalExtBarrel = CreateCellPositions("positionsHcalExtBarrel", 
                                          positionsTool=HCalExtBcells, 
                                          hits = "HCalExtBarrelCells", 
                                          positionedHits = "HCalExtBarrelCellPositions", 
                                          OutputLevel = INFO)
positionsEcalEndcap = CreateCellPositions("positionsEcalEndcap", 
                                          positionsTool=EMECcells, 
                                          hits = "ECalEndcapCells", 
                                          positionedHits = "ECalEndcapCellPositions", 
                                          OutputLevel = INFO)
positionsHcalEndcap = CreateCellPositions("positionsHcalEndcap", 
                                          positionsTool=HECcells, 
                                          hits = "HCalEndcapCells", 
                                          positionedHits = "HCalEndcapCellPositions", 
                                          OutputLevel = INFO)
positionsEcalFwd = CreateCellPositions("positionsEcalFwd", 
                                          positionsTool=ECalFwdcells, 
                                          hits = "ECalFwdCells", 
                                          positionedHits = "ECalFwdCellPositions", 
                                          OutputLevel = INFO)
positionsHcalFwd = CreateCellPositions("positionsHcalFwd", 
                                          positionsTool=HCalFwdcells, 
                                          hits = "HCalFwdCells", 
                                          positionedHits = "HCalFwdCellPositions", 
                                          OutputLevel = INFO)
positionsTailCatcher = CreateCellPositions("positionsTailCatcher", 
                                          positionsTool=TailCatchercells, 
                                          hits = "TailCatcherCells", 
                                          positionedHits = "TailCatcherCellPositions", 
                                          OutputLevel = INFO)

out = PodioOutput("out", OutputLevel=DEBUG)
out.filename = "~/FCCSW/digi_cellPostions_pim_100GeV.root"
out.outputCommands = ["keep *","drop ECalBarrelCells","drop ECalEndcapCells","drop ECalFwdCells","drop HCalBarrelCells", "drop HCalExtBarrelCells", "drop HCalEndcapCells", "drop HCalFwdCells", "drop TailCatcherCells"]

#CPU information
from Configurables import AuditorSvc, ChronoAuditor
chra = ChronoAuditor()
audsvc = AuditorSvc()
audsvc.Auditors = [chra]
podioinput.AuditExecute = True
positionsEcalBarrel.AuditExecute = True
positionsEcalEndcap.AuditExecute = True
positionsEcalFwd.AuditExecute = True
positionsHcalBarrel.AuditExecute = True
positionsHcalExtBarrel.AuditExecute = True
positionsHcalEndcap.AuditExecute = True
positionsHcalFwd.AuditExecute = True
positionsTailCatcher.AuditExecute = True
out.AuditExecute = True

ApplicationMgr(
TopAlg = [    podioinput,
              positionsEcalBarrel,
              positionsEcalEndcap,
              positionsEcalFwd, 
              positionsHcalBarrel, 
              positionsHcalExtBarrel, 
              positionsHcalEndcap, 
              positionsHcalFwd,
              positionsTailCatcher,
              out
              ],
    EvtSel = 'NONE',
    EvtMax = 1,
    ExtSvc = [geoservice, podioevent, audsvc],
)
