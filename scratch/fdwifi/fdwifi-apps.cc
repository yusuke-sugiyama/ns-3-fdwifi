/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 *
 * These applications are used in the WiFi Distance Test experiment,
 * described and implemented in test02.cc.  That file should be in the
 * same place as this file.  The applications have two very simple
 * jobs, they just generate and receive packets.  We could use the
 * standard Application classes included in the NS-3 distribution.
 * These have been written just to change the behavior a little, and
 * provide more examples.
 *
 */

#include <ostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

#include "ns3/stats-module.h"

#include "fdwifi-apps.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FdWifiApps");

TypeId
Sender::GetTypeId (void)
{
  static TypeId tid = TypeId ("Sender")
    .SetParent<Application> ()
    .AddConstructor<Sender> ()
    .AddAttribute ("PacketSize", "The size of packets transmitted.",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&Sender::m_pktSize),
                   MakeUintegerChecker<uint32_t>(1))
    .AddAttribute ("Destination", "Target host address.",
                   Ipv4AddressValue ("255.255.255.255"),
                   MakeIpv4AddressAccessor (&Sender::m_destAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Port", "Destination app port.",
                   UintegerValue (1603),
                   MakeUintegerAccessor (&Sender::m_destPort),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Interval", "Delay between transmissions.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&Sender::m_interval),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("Stream", "Random Stream.",
                   StringValue ("ns3::UniformRandomVariable[Stream=-1]"),
                   MakePointerAccessor (&Sender::m_random),
                   MakePointerChecker <RandomVariableStream>())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&Sender::m_txTrace))
  ;
  return tid;
}


Sender::Sender()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_interval = CreateObject<ConstantRandomVariable> ();
  m_socket = 0;
  m_random = CreateObject<UniformRandomVariable> ();
}

Sender::~Sender()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Sender::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void Sender::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0) {
      Ptr<SocketFactory> socketFactory = GetNode ()->GetObject<SocketFactory>
          (UdpSocketFactory::GetTypeId ());
      m_socket = socketFactory->CreateSocket ();
      m_socket->Bind ();
    }

  Simulator::Cancel (m_sendEvent);
  m_sendEvent = Simulator::ScheduleNow (&Sender::SendPacket, this);

  // end Sender::StartApplication
}

void Sender::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
  // end Sender::StopApplication
}

void Sender::SendPacket ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("Sending packet at " << Simulator::Now () << " to " <<
               m_destAddr);

  Ptr<Packet> packet = Create<Packet>(m_pktSize);
  

  TimestampTag timestamp;
  timestamp.SetTimestamp (Simulator::Now ());
  packet->AddByteTag (timestamp);

  // Could connect the socket since the address never changes; using SendTo
  // here simply because all of the standard apps do not.
  m_socket->SendTo (packet, 0, InetSocketAddress (m_destAddr, m_destPort));

  // Report the event to the trace.
  m_txTrace (packet);
  double interval = m_interval->GetValue ();
  double logval = -log(m_random->GetValue());
  Time nextTxTime = Seconds (interval * logval);
  NS_LOG_INFO("nextTime:" << nextTxTime << " in:" << interval << " log:" << logval);
  m_sendEvent = Simulator::Schedule (nextTxTime, &Sender::SendPacket, this);

}




//----------------------------------------------------------------------
//-- Receiver
//------------------------------------------------------
TypeId
Receiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("Receiver")
    .SetParent<Application> ()
    .AddConstructor<Receiver> ()
    .AddAttribute ("Port", "Listening port.",
                   UintegerValue (1603),
                   MakeUintegerAccessor (&Receiver::m_port),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("NumPackets", "Total number of packets to recv.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Receiver::m_numPkts),
                   MakeUintegerChecker<uint32_t>(0))
    .AddTraceSource ("Rx", "Receive data packet",
                     MakeTraceSourceAccessor (&Receiver::m_rxTrace))
  ;
  return tid;
}

Receiver::Receiver() :
  m_calc (0),
  m_delay (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

Receiver::~Receiver()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Receiver::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void
Receiver::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0) {
      Ptr<SocketFactory> socketFactory = GetNode ()->GetObject<SocketFactory>
          (UdpSocketFactory::GetTypeId ());
      m_socket = socketFactory->CreateSocket ();
      InetSocketAddress local = 
        InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
    }

  m_socket->SetRecvCallback (MakeCallback (&Receiver::Receive, this));
  m_count = 0;

  // end Receiver::StartApplication
}

void
Receiver::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0) {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  // end Receiver::StopApplication
}

void
Receiver::SetCounter (Ptr<CounterCalculator<> > calc)
{
  m_calc = calc;
  // end Receiver::SetCounter
}
void
Receiver::SetDelayTracker (Ptr<TimeMinMaxAvgTotalCalculator> delay)
{
  m_delay = delay;
  // end Receiver::SetDelayTracker
}

void
Receiver::Receive (Ptr<Socket> socket)
{
  // NS_LOG_FUNCTION (this << socket << packet << from);
  
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from))) {
    if (InetSocketAddress::IsMatchingType (from)) {
      NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " <<
		   InetSocketAddress::ConvertFrom (from).GetIpv4 ());
    }
    TimestampTag timestamp;
    // Should never not be found since the sender is adding it, but
    // you never know.
    if (packet->FindFirstMatchingByteTag (timestamp) && m_numPkts != 0) {
      Time tx = timestamp.GetTimestamp ();
      
      if (m_delay != 0) {
	m_delay->Update (Simulator::Now () - tx);
      }
      /* [add] 20140618 sugiyama */
      m_rxTrace (++m_count, m_numPkts);
      /* [end]*/
    }
    
    if (m_calc != 0) {
      m_calc->Update ();
    }
    
    // end receiving packets
  }
  
  // end Receiver::Receive
}




//----------------------------------------------------------------------
//-- TimestampTag
//------------------------------------------------------
TypeId 
TimestampTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("TimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<TimestampTag> ()
    .AddAttribute ("Timestamp",
                   "Some momentous point in time!",
                   EmptyAttributeValue (),
                   MakeTimeAccessor (&TimestampTag::GetTimestamp),
                   MakeTimeChecker ())
  ;
  return tid;
}
TypeId 
TimestampTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
TimestampTag::GetSerializedSize (void) const
{
  return 8;
}
void 
TimestampTag::Serialize (TagBuffer i) const
{
  int64_t t = m_timestamp.GetNanoSeconds ();
  i.Write ((const uint8_t *)&t, 8);
}
void 
TimestampTag::Deserialize (TagBuffer i)
{
  int64_t t;
  i.Read ((uint8_t *)&t, 8);
  m_timestamp = NanoSeconds (t);
}

void
TimestampTag::SetTimestamp (Time time)
{
  m_timestamp = time;
}
Time
TimestampTag::GetTimestamp (void) const
{
  return m_timestamp;
}

void 
TimestampTag::Print (std::ostream &os) const
{
  os << "t=" << m_timestamp;
}
