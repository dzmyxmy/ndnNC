/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
*         Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "multipath.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (Multipath);

LogComponent Multipath::g_log = LogComponent (Multipath::GetLogName ().c_str ());

std::string
Multipath::GetLogName ()
{
  return super::GetLogName ()+".Multipath";
}


TypeId
Multipath::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::Multipath")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <Multipath> ()
    ;
  return tid;
}

Multipath::Multipath ()
{
}

bool
Multipath::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr< Interest> header,
                                Ptr<const Packet> origPacket,
                                Ptr<pit::Entry> pitEntry)
{
  uint32_t path2;
  FwHopCountTag hopCountTag;
      if (origPacket->PeekPacketTag (hopCountTag)) path2 = hopCountTag.Get();
  //just use multipath at the first hop, then use best route forwarding.
  if( path2 > 0)//consumer is hop 0 
	{
	return BestRouteDoPropagateInterest(inFace, header, origPacket, pitEntry);	
	}
  NS_LOG_FUNCTION (this << header->GetName ());

  // No real need to call parent's (green-yellow-red's strategy) method, since it is incorporated
  // in the logic of the Multipath strategy
  //
  // // Try to work out with just green faces
  // bool greenOk = super::DoPropagateInterest (inFace, header, origPacket, pitEntry);
  // if (greenOk)
  //   return true;
  int propagatedCount = 0;

  const fib::FaceMetric *metricFace_before;
  uint32_t Flagm = 0;
  bool Flagc = true;
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
	Flagm++;
      if(Flagm==1)
        {
        metricFace_before=&metricFace;
        continue;
        }
        //just forward to two faces;
      if (Flagm>2  || metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) break;
      if(Flagc &&  metricFace.GetBit()==metricFace_before->GetBit())
        {
        break;
        }
      else
        {
	pitEntry->GetFibEntry()->UpdateBit(metricFace.GetFace(),(metricFace.GetBit()+1)%2);
      if (!TrySendOutInterest (inFace,  metricFace.GetFace (), header, origPacket, pitEntry))
        {
	  Flagc = false;
          continue;
        }

        return 1;
        }
    }
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
	pitEntry->GetFibEntry()->UpdateBit(metricFace.GetFace(),(metricFace.GetBit()+1)%2);
if (!TrySendOutInterest (inFace,  metricFace.GetFace (), header, origPacket, pitEntry)) continue;

      return ++propagatedCount;
    }
  return 1;

}

bool
Multipath::BestRouteDoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> header,
                                Ptr<const Packet> origPacket,
                                Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << header->GetName ());
  int propagatedCount = 0;
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
        break;

      if (!TrySendOutInterest (inFace, metricFace.GetFace (), header, origPacket, pitEntry))
        {
          continue;
        }

      propagatedCount++;
      break; // do only once
    }

  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}


} // namespace fw
} // namespace ndn
} // namespace ns3
