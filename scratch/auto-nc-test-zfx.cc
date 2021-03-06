
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include <ns3/ndnSIM/utils/tracers/ndn-cs-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>
#include <iostream>
using namespace std;
using namespace ns3;
//namespace std{
//namespace ns3{
int main (int argc, char *argv[])
{
//std:int a;  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("100Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("1ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("100"));
  CommandLine cmd;
  cmd.Parse (argc, argv);
  AnnotatedTopologyReader topologyReader ("", 10);
  topologyReader.SetFileName ("topo/topo100-zfx.txt");
  topologyReader.Read ();
  /*PointToPointHelper p2p;
  // Creating nodes
  Ptr<Node> Node1 = CreateObject<Node> ();
  Ptr<Node> Node2 = CreateObject<Node> ();
  Ptr<Node> Node3 = CreateObject<Node> ();
  Ptr<Node> Node4 = CreateObject<Node> ();
  Ptr<Node> Node5 = CreateObject<Node> ();
  Ptr<Node> Node6 = CreateObject<Node> ();
  Ptr<Node> Node7 = CreateObject<Node> ();
  p2p.Install(Node1, Node2);
  p2p.Install(Node1, Node3);
  p2p.Install(Node2, Node4);
  p2p.Install(Node2, Node6);
  p2p.Install(Node3, Node4);
  p2p.Install(Node3, Node7);
  p2p.Install(Node4, Node5);
  p2p.Install(Node5, Node6);
  p2p.Install(Node5, Node7);*/
  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetDefaultRoutes(true);
  //ccnxHelper.InstallAll ();
  // Installing applications
  ccnxHelper.SetPit ("ns3::ndn::pit::Lru","MaxSize", "1000");
ccnxHelper.SetContentStore ("ns3::ndn::cs::Lru","MaxSize","4000");   ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::Pinform");
  //ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  //ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::Multipath");
  ccnxHelper.InstallAll ();
 
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ccnxGlobalRoutingHelper;
  ccnxGlobalRoutingHelper.InstallAll ();
  //Add /prefix origins to ndn::GlobalRouter
Ptr<Node> Node0 = Names::Find<Node>("0");
Ptr<Node> Node10 = Names::Find<Node>("10");
Ptr<Node> Node20 = Names::Find<Node>("20");
Ptr<Node> Node30 = Names::Find<Node>("30");
Ptr<Node> Node40 = Names::Find<Node>("40");
Ptr<Node> Node50 = Names::Find<Node>("50");
Ptr<Node> Node60 = Names::Find<Node>("60");
Ptr<Node> Node70 = Names::Find<Node>("70");
Ptr<Node> Node80 = Names::Find<Node>("80");
Ptr<Node> Node90 = Names::Find<Node>("90");
ccnxGlobalRoutingHelper.AddOrigins ("/ndn/vod/nc", Node30);
ccnxGlobalRoutingHelper.AddOrigins ("/ndn/vod/nc", Node60);
ndn::AppHelper Client("ns3::ndn::ConsumerCbrNC");
//ndn::AppHelper Client("ns3::ndn::ConsumerZipfMandelbrot");
Client.SetPrefix("/ndn/vod/nc");
Client.SetAttribute("Frequency", StringValue("200"));
Client.SetAttribute("Randomize", StringValue ("uniform"));
ApplicationContainer app0 = Client.Install (Node0);
app0.Start(Seconds(0));
ApplicationContainer app10 = Client.Install (Node10);
app10.Start(Seconds(3));
ApplicationContainer app20 = Client.Install (Node20);
app20.Start(Seconds(6));
ApplicationContainer app40 = Client.Install (Node40);
app40.Start(Seconds(12));
ApplicationContainer app50 = Client.Install (Node50);
app50.Start(Seconds(15));
ApplicationContainer app70 = Client.Install (Node70);
app70.Start(Seconds(21));
ApplicationContainer app80 = Client.Install (Node80);
app80.Start(Seconds(24));
ApplicationContainer app90 = Client.Install (Node90);
app90.Start(Seconds(27));
 
  /*ndn::AppHelper producer("ns3::ndn::Producer");
  producer.SetPrefix("/ndn/vod/nc");
  producer.SetAttribute("PayloadSize", StringValue("1024"));
  producer.Install (one);*/
  //ccnxGlobalRoutingHelper.CalculateRoutes ();
  //ApplicationContainer app4 = producer.Install(node4);
  //ApplicationContainer app4 = producer.Install(node4);
  //ApplicationContainer app4 = producer.Install(node4);
  //ApplicationContainer app4 = producer.Install(node4);
  
  ndn::AppHelper consumerHelper ("NetworkCodingApp");
consumerHelper.SetAttribute("Freshness", TimeValue (Seconds (4)));
  //ApplicationContainer app1 = consumerHelper.Install (node1); 
  //ApplicationContainer app2 = consumerHelper.Install (node2);
  //ApplicationContainer app3 = consumerHelper.Install (node3);
  //ApplicationContainer app1 = producer.Install (node1);
  //consumerHelper.Install (Node50);
  consumerHelper.Install (Node30);
  consumerHelper.Install (Node60);
  ccnxGlobalRoutingHelper.CalculateRoutes ();
  //app1.Start(Seconds (0.0)); // will send out Interest, which nobody will receive (Interests generated by an app will not got back to the app)
  //app1.Stop(Seconds(20.0));
  //app6.Start (Seconds (1.0)); // will send out an Interests, which will be received and satisfied by app1
  //app7.Start (Seconds (1.0));
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
  AggregateTracer = ndn::L3AggregateTracer::InstallAll ("data-bestroute/nc100/aggregate-trace.txt", Seconds (0.5));
 
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
  L3RateTracer = ndn::L3RateTracer::InstallAll ("data-bestroute/nc100/rate-trace.txt", Seconds (0.5));
 
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::CsTracer> > >
  aggTracers = ndn::CsTracer::InstallAll ("data-bestroute/nc100/cs-trace.txt", Seconds (1));
 
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
  delayTracer = ndn::AppDelayTracer::InstallAll ("data-bestroute/nc100/app-delays-trace.txt");

Simulator::Stop (Seconds (60)); 
  Simulator::Run ();
  for (NodeList::Iterator node = NodeList::Begin (); node != NodeList::End (); node ++)
  {
    std::cout << "Node #" << (*node)->GetId () << std::endl;
    (*node)->GetObject<ndn::ContentStore> ()->Print (std::cout);
      std::cout << std::endl;
  }
  Simulator::Destroy ();
  return 0;
}
//}
