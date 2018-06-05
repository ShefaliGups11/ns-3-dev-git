#include <iostream>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/simulator.h"


using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceTrace");

//static Ptr<OutputStreamWrapper> cWndStream;
std::string dir = "Plots/";

static void
tracer (uint32_t oldval, uint32_t newval)
{
  /*std::cout << "Oldvalue: " << oldval << " Newvalue: "<< newval << std::endl;
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << "," << oldval << "," << newval << std::endl;*/

  std::ofstream fPlotQueue (dir + "cwndTraces/A.plotme", std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << newval/536.0 << std::endl;
  fPlotQueue.close ();
}


static void
cwnd ()
{
  std::cout << "cwnd" << std::endl;
  //AsciiTraceHelper ascii;
 // cWndStream = ascii.CreateFileStream (file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&tracer));
}

void
qlen (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();
  Simulator::Schedule (Seconds (0.1), &qlen, queue);
 std::ofstream fPlotQueue (std::stringstream (dir +"/queueTraces/queue-size.plotme").str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();
 }


int main (int argc, char *argv[])
{
  bool trace = true;
  bool pcap = true;
  double simulation_time = 10.0;
  std::string stack = "";
  std::string sock_factory = "ns3::TcpSocketFactory";

  CommandLine cmd;
  cmd.AddValue ("trace", "Enable trace", trace);
  cmd.AddValue ("pcap", "Enable PCAP", pcap);
  cmd.AddValue ("time", "Simulation Duration", simulation_time);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (3);

  PointToPointHelper linkincoming;
  linkincoming.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  linkincoming.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper linkoutgoing;
  linkoutgoing.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  linkoutgoing.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer rightToRouterDevices, routerToLeftDevices;
  rightToRouterDevices = linkincoming.Install (nodes.Get (0), nodes.Get (1));
  routerToLeftDevices = linkoutgoing.Install (nodes.Get (1), nodes.Get (2));

  InternetStackHelper internetStack;
  internetStack.InstallAll ();

  
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer sink_interfaces;  

  Ipv4InterfaceContainer interfaces;
  //address.NewNetwork ();
  interfaces = address.Assign (rightToRouterDevices);
  address.NewNetwork ();
  interfaces = address.Assign (routerToLeftDevices);
  sink_interfaces.Add (interfaces.Get (1)); 

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::string queue_disc_type = "ns3::FifoQueueDisc";

  TypeId qdTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (queue_disc_type, &qdTid), "TypeId " << queue_disc_type << " not found");
  Config::SetDefault (queue_disc_type + "::MaxSize", QueueSizeValue (QueueSize ("38p")));

  TrafficControlHelper tch;
  tch.SetRootQueueDisc (queue_disc_type);
  QueueDiscContainer qd;
  tch.Uninstall (nodes.Get (1)->GetDevice (1));
  qd.Add (tch.Install (nodes.Get (1)->GetDevice (1)).Get (0));
  Simulator::ScheduleNow (&qlen, qd.Get (0));

  uint16_t port = 2000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));

  PacketSinkHelper sinkHelper (sock_factory, sinkLocalAddress);
  
  AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (0, 0), port));
  BulkSendHelper sender (sock_factory, Address ());
  sender.SetAttribute ("MaxBytes", UintegerValue (0));
    

  sender.SetAttribute ("Remote", remoteAddress);
  ApplicationContainer sendApp = sender.Install (nodes.Get (0));
  sendApp.Start (Seconds (0.0));
  Simulator::Schedule (Seconds (0.1), &cwnd);
  sendApp.Stop (Seconds (10.0));

  //sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (2));
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (10.0));
  std::cout << "qlen" << std::endl;
  std::cout << trace << std::endl;
  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  system (dirToSave.c_str ());
  system ((dirToSave + "/cwndTraces/").c_str ());
  system ((dirToSave + "/queueTraces/").c_str ());

  std::cout << "trace" << std::endl;
 
  std::cout << "qlen" << std::endl;

  Simulator::Stop (Seconds (11));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
