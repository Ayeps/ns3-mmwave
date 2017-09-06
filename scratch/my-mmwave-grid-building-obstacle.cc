/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-helper.h"
#include <ns3/buildings-helper.h>
#include "ns3/log.h"
#include <ns3/buildings-module.h>


using namespace ns3;

int 
main (int argc, char *argv[])
{
  std::string traceFile = "mobility/cityGrid.ns_movements";

  CommandLine cmd;
  cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);
  cmd.Parse (argc, argv);

  LogComponentEnable("LteRlcAm", LOG_LEVEL_LOGIC);

  /* Information regarding the traces generated:
   *
   * 1. UE_1_SINR.txt : Gives the SINR for each sub-band
   * 	Subframe no.  | Slot No. | Sub-band  | SINR (db)
   *
   * 2. UE_1_Tb_size.txt : Allocated transport block size
   * 	Time (micro-sec)  |  Tb-size in bytes
   * */

  Ptr<MmWaveHelper> ptr_mmWave = CreateObject<MmWaveHelper> ();
  ptr_mmWave->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));
  ptr_mmWave->Initialize();

  NodeContainer enbXNodes, enbYNodes;
  NodeContainer ueNodes;
  enbXNodes.Create (150);
  enbYNodes.Create(50);
  ueNodes.Create (1);
/*
  Ptr < Building > building;
  building = Create<Building> ();
  building->SetBoundaries (Box (20.0, 40.0,
                                0.0, 20.0,
                                0.0, 20.0));
  building->SetBuildingType (Building::Office);
  building->SetExtWallsType (Building::ConcreteWithWindows);
  building->SetNFloors (1);
  building->SetNRoomsX (1);
  building->SetNRoomsY (1);
  */
  Ptr< GridBuildingAllocator > cityGrid;
  cityGrid = CreateObject< GridBuildingAllocator > ();
  cityGrid->SetAttribute ("GridWidth", UintegerValue (10));  // number of buildings in the width
  cityGrid->SetAttribute ("LengthX", DoubleValue (50));      // side X of a single building
  cityGrid->SetAttribute ("LengthY", DoubleValue (50));      // side Y of a single building
  cityGrid->SetAttribute ("DeltaX", DoubleValue (10));       // size of the street between buildings
  cityGrid->SetAttribute ("DeltaY", DoubleValue (10));       // size of the street between buildings
  cityGrid->SetAttribute ("Height", DoubleValue (10));       // side Z of a single building
  cityGrid->SetBuildingAttribute ("NRoomsX", UintegerValue (1)); // number of rooms in X axis
  cityGrid->SetBuildingAttribute ("NRoomsY", UintegerValue (1)); // number of rooms in Y axis
  cityGrid->SetBuildingAttribute ("NFloors", UintegerValue (1)); // number of floors in Z axis
  cityGrid->SetAttribute ("MinX", DoubleValue (0));          // the X coordinate where the grid starts
  cityGrid->SetAttribute ("MinY", DoubleValue (0));          // the Y coordinate where the grid starts
  cityGrid->Create (50);                                     // number of buildings in the grid
  
  // Position of enb
  MobilityHelper enbXmobility, enbYmobility;
  enbXmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbXmobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0),
                                "MinY", DoubleValue (0),
                                "DeltaX", DoubleValue (20),
                                "DeltaY", DoubleValue (60),
                                "GridWidth", UintegerValue (30),
                                "LayoutType", StringValue("RowFirst"));
  enbXmobility.Install (enbXNodes);
  BuildingsHelper::Install (enbXNodes);
  enbYmobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0),
                                "MinY", DoubleValue (30),
                                "DeltaX", DoubleValue (60),
                                "DeltaY", DoubleValue (60),
                                "GridWidth", UintegerValue (30),
                                "LayoutType", StringValue("RowFirst"));
  enbYmobility.Install (enbYNodes);
  BuildingsHelper::Install (enbYNodes);


  // Create Ns2MobilityHelper with the specified trace log file as parameter
//   Ns2MobilityHelper ns2mob = Ns2MobilityHelper (traceFile);
//   ns2mob.Install ();
  
  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.Install (ueNodes);
  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (-5, 55, 0));
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));
  
//   MobilityHelper uemobility;
//   Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
//   uePositionAlloc->Add (Vector (80.0, -80.0, 0.0));
//   uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//   uemobility.SetPositionAllocator(uePositionAlloc);
  uemobility.Install (ueNodes);
  
  BuildingsHelper::Install (ueNodes);

  NetDeviceContainer enbNetDev = ptr_mmWave->InstallEnbDevice (NodeContainer (enbXNodes,enbYNodes));
  NetDeviceContainer ueNetDev = ptr_mmWave->InstallUeDevice (ueNodes);


  ptr_mmWave->AttachToClosestEnb (ueNetDev, enbNetDev);
  ptr_mmWave->EnableTraces();

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  ptr_mmWave->ActivateDataRadioBearer (ueNetDev, bearer);
  BuildingsHelper::MakeMobilityModelConsistent ();

  Simulator::Stop (Seconds (1));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


