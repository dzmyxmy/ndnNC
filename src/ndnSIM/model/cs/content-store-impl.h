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
 */

#ifndef NDN_CONTENT_STORE_IMPL_H_
#define NDN_CONTENT_STORE_IMPL_H_

#include "ndn-content-store.h"
#include "ns3/packet.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"
#include <boost/foreach.hpp>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

#include "../../utils/trie/trie-with-policy.h"

namespace ns3 {
namespace ndn {
namespace cs {

template<class CS>
class EntryImpl : public Entry
{
public:
  typedef Entry base_type;

public:
  EntryImpl (Ptr<ContentStore> cs, Ptr<const ContentObject> header, Ptr<const Packet> packet)
    : Entry (cs, header, packet)
    , item_ (0)
  {
  }

  void
  SetTrie (typename CS::super::iterator item)
  {
    item_ = item;
  }

  typename CS::super::iterator to_iterator () { return item_; }
  typename CS::super::const_iterator to_iterator () const { return item_; }

private:
  typename CS::super::iterator item_;
};



template<class Policy>
class ContentStoreImpl : public ContentStore,
                         protected ndnSIM::trie_with_policy< Name,
                                                             ndnSIM::smart_pointer_payload_traits< EntryImpl< ContentStoreImpl< Policy > >, Entry >,
                                                             Policy >
{
public:
  typedef ndnSIM::trie_with_policy< Name,
                                    ndnSIM::smart_pointer_payload_traits< EntryImpl< ContentStoreImpl< Policy > >, Entry >,
                                    Policy > super;

  typedef EntryImpl< ContentStoreImpl< Policy > > entry;

  static TypeId
  GetTypeId ();

  ContentStoreImpl () { };
  virtual ~ContentStoreImpl () { };

  // from ContentStore

  virtual inline boost::tuple<Ptr<Packet>, Ptr<const ContentObject>, Ptr<const Packet> >
  Lookup (Ptr<const Interest> interest);

  //added by zfx, search Interest in cs, it does not add miss in trace if missTrue=0; if missTrue=1, it can be  replaced by Lookup();

  virtual inline boost::tuple<Ptr<Packet>, Ptr<const ContentObject>, Ptr<const Packet> >
  Lookup_nc (Ptr<const Interest> interest, bool missTrue, uint64_t coef_int);

  virtual inline bool
  IsCoefSame(uint64_t coef_pit, uint64_t coef_data);

  virtual inline bool
  Add (Ptr<const ContentObject> header, Ptr<const Packet> packet);

  // virtual bool
  // Remove (Ptr<Interest> header);

  virtual inline void
  Print (std::ostream &os) const;

  virtual uint32_t
  GetSize () const;

  virtual Ptr<Entry>
  Begin ();

  virtual Ptr<Entry>
  End ();

  virtual Ptr<Entry>
  Next (Ptr<Entry>);

  bool isFull(){ return (GetMaxSize ()<= GetSize ()); };

private:
  void
  SetMaxSize (uint32_t maxSize);

  uint32_t
  GetMaxSize () const;

private:
  static LogComponent g_log; ///< @brief Logging variable

  /// @brief trace of for entry additions (fired every time entry is successfully added to the cache): first parameter is pointer to the CS entry
  TracedCallback< Ptr<const Entry> > m_didAddEntry;
};

//////////////////////////////////////////
////////// Implementation ////////////////
//////////////////////////////////////////


template<class Policy>
LogComponent ContentStoreImpl< Policy >::g_log = LogComponent (("ndn.cs." + Policy::GetName ()).c_str ());


template<class Policy>
TypeId
ContentStoreImpl< Policy >::GetTypeId ()
{
  static TypeId tid = TypeId (("ns3::ndn::cs::"+Policy::GetName ()).c_str ())
    .SetGroupName ("Ndn")
    .SetParent<ContentStore> ()
    .AddConstructor< ContentStoreImpl< Policy > > ()
    .AddAttribute ("MaxSize",
                   "Set maximum number of entries in ContentStore. If 0, limit is not enforced",
                   StringValue ("100"),
                   MakeUintegerAccessor (&ContentStoreImpl< Policy >::GetMaxSize,
                                         &ContentStoreImpl< Policy >::SetMaxSize),
                   MakeUintegerChecker<uint32_t> ())

    .AddTraceSource ("DidAddEntry", "Trace fired every time entry is successfully added to the cache",
                     MakeTraceSourceAccessor (&ContentStoreImpl< Policy >::m_didAddEntry))
    ;

  return tid;
}

template<class Policy>
boost::tuple<Ptr<Packet>, Ptr<const ContentObject>, Ptr<const Packet> >
ContentStoreImpl<Policy>::Lookup (Ptr<const Interest> interest)
{
  NS_LOG_FUNCTION (this << interest->GetName ());
  //std::cout << "Lookup ContentStore:  "<<interest->GetName()<< std::endl;
  /// @todo Change to search with predicate
  typename super::const_iterator node = this->deepest_prefix_match (interest->GetName ());

  if (node != this->end ())
    {
      this->m_cacheHitsTrace (interest, node->payload ()->GetHeader ());

      // NS_LOG_DEBUG ("cache hit with " << node->payload ()->GetHeader ()->GetName ());
      //std::cout << "cache hit with " << node->payload ()->GetHeader ()->GetName ()<< std::endl;
      return boost::make_tuple (node->payload ()->GetFullyFormedNdnPacket (),
                                node->payload ()->GetHeader (),
                                node->payload ()->GetPacket ());
    }
  else
    {
      // NS_LOG_DEBUG ("cache miss for " << interest->GetName ());
      //std::cout <<" cache miss for " << interest->GetName ()<< std::endl;
      this->m_cacheMissesTrace (interest);
      return boost::tuple<Ptr<Packet>, Ptr<ContentObject>, Ptr<Packet> > (0, 0, 0);
    }
}

//added by zfx
template<class Policy>
boost::tuple<Ptr<Packet>, Ptr<const ContentObject>, Ptr<const Packet> >
ContentStoreImpl<Policy>::Lookup_nc (Ptr<const Interest> interest, bool missTrue, uint64_t coef_int)
{
  NS_LOG_FUNCTION (this << interest->GetName ());
  //std::cout << "Lookup ContentStore:  "<<interest->GetName()<< std::endl;
  /// @todo Change to search with predicate
  // a Interest arrived to a router, and it will call this function several time, but miss need only added by 1, which will be added at the last time;
  typename super::const_iterator node = this->deepest_prefix_match (interest->GetName ());

  if (node != this->end () && !IsCoefSame(coef_int, node->payload ()->GetHeader ()->GetCoef()))
    {
      this->m_cacheHitsTrace (interest, node->payload ()->GetHeader ());

      // NS_LOG_DEBUG ("cache hit with " << node->payload ()->GetHeader ()->GetName ());
      //std::cout << "cache hit with " << node->payload ()->GetHeader ()->GetName ()<< std::endl;
      return boost::make_tuple (node->payload ()->GetFullyFormedNdnPacket (),
                                node->payload ()->GetHeader (),
                                node->payload ()->GetPacket ());
    }
  else if(missTrue)
    {
      // NS_LOG_DEBUG ("cache miss for " << interest->GetName ());
      //std::cout <<" cache miss for " << interest->GetName ()<< std::endl;
      this->m_cacheMissesTrace (interest);
      return boost::tuple<Ptr<Packet>, Ptr<ContentObject>, Ptr<Packet> > (0, 0, 0);
    }
      return boost::tuple<Ptr<Packet>, Ptr<ContentObject>, Ptr<Packet> > (0, 0, 0);
}

//added by zfx
template<class Policy>
bool
ContentStoreImpl<Policy>::IsCoefSame(uint64_t coef_pit, uint64_t coef_data)
{
//std::cout<<"IsCoefSame:"<<coef_pit<<" "<<coef_data<<std::endl;
while(coef_pit>=10)
        {
        if(coef_pit%100==coef_data) return true;
        coef_pit/=100;
        }
return false;
}


template<class Policy>
bool
ContentStoreImpl<Policy>::Add (Ptr<const ContentObject> header, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << header->GetName ());

  Ptr< entry > newEntry = Create< entry > (this, header, packet);
  std::pair< typename super::iterator, bool > result = super::insert (header->GetName (), newEntry);

  if (result.first != super::end ())
    {
      if (result.second)
        {
          newEntry->SetTrie (result.first);

          m_didAddEntry (newEntry);
          return true;
        }
      else
        {
          // should we do anything?
          // update payload? add new payload?
          return false;
        }
    }
  else
    return false; // cannot insert entry
}

template<class Policy>
void
ContentStoreImpl<Policy>::Print (std::ostream &os) const
{
  for (typename super::policy_container::const_iterator item = this->getPolicy ().begin ();
       item != this->getPolicy ().end ();
       item++)
    {
      os << item->payload ()->GetName () << std::endl;
    }
}

template<class Policy>
void
ContentStoreImpl<Policy>::SetMaxSize (uint32_t maxSize)
{
  this->getPolicy ().set_max_size (maxSize);
}

template<class Policy>
uint32_t
ContentStoreImpl<Policy>::GetMaxSize () const
{
  return this->getPolicy ().get_max_size ();
}

template<class Policy>
uint32_t
ContentStoreImpl<Policy>::GetSize () const
{
  return this->getPolicy ().size ();
}

template<class Policy>
Ptr<Entry>
ContentStoreImpl<Policy>::Begin ()
{
  typename super::parent_trie::recursive_iterator item (super::getTrie ()), end (0);
  for (; item != end; item++)
    {
      if (item->payload () == 0) continue;
      break;
    }

  if (item == end)
    return End ();
  else
    return item->payload ();
}

template<class Policy>
Ptr<Entry>
ContentStoreImpl<Policy>::End ()
{
  return 0;
}

template<class Policy>
Ptr<Entry>
ContentStoreImpl<Policy>::Next (Ptr<Entry> from)
{
  if (from == 0) return 0;

  typename super::parent_trie::recursive_iterator
    item (*StaticCast< entry > (from)->to_iterator ()),
    end (0);

  for (item++; item != end; item++)
    {
      if (item->payload () == 0) continue;
      break;
    }

  if (item == end)
    return End ();
  else
    return item->payload ();
}


} // namespace cs
} // namespace ndn
} // namespace ns3

#endif // NDN_CONTENT_STORE_IMPL_H_
