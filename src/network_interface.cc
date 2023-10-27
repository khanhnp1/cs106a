#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t dst_ip = next_hop.ipv4_numeric();
  auto record = arp_table.find(dst_ip);

  if ( record != arp_table.end()) {
    /* Found a record - Contruct a packet to send */
    EthernetFrame pkt;
    pkt.header.type = EthernetHeader::TYPE_IPv4;
    pkt.header.src  = ethernet_address_;
    pkt.header.dst  = record->second;
    pkt.payload = serialize(dgram);

    packet.push(pkt);
    return;
  }

  /* If dst Mac is unknow - Contruct ARP packet */
  /* Already sent */
  if (arp_timer[next_hop.ipv4_numeric()] < ARP_SENT_TIMEOUT && arp_timer[next_hop.ipv4_numeric()] > 0)
    return;

  ARPMessage arp;
  arp.opcode = ARPMessage::OPCODE_REQUEST;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.sender_ethernet_address = ethernet_address_;  
  arp.target_ip_address = next_hop.ipv4_numeric();
  arp.target_ethernet_address = {};

  EthernetFrame pkt;
  pkt.header.type = EthernetHeader::TYPE_ARP;
  pkt.header.src  = ethernet_address_;
  pkt.header.dst  = ETHERNET_BROADCAST;
  pkt.payload = serialize(arp);
  packet.push(pkt);

  datagram[next_hop.ipv4_numeric()].push(dgram);
  arp_timer[next_hop.ipv4_numeric()] = 0;
  return;
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  /* Packet not for us */
  if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST)
    return {};
  /* Got IPv4 packet */
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram;
    if ( parse(dgram, frame.payload) )
      return dgram;
    
    return {};
  }
  /* Got ARP packet */
  if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp;
    if ( !parse(arp, frame.payload) )
      return {};

    if (arp.opcode == ARPMessage::OPCODE_REQUEST) {
      /* Remember sender IP - MAC*/
      arp_table[arp.sender_ip_address] = arp.sender_ethernet_address;
      arp_timer[arp.sender_ip_address] = 0;
      /* Not for us */
      if (arp.target_ip_address != ip_address_.ipv4_numeric())
        return {};
      /* Construct a Reply */
      arp.target_ip_address = arp.sender_ip_address;
      arp.target_ethernet_address = arp.sender_ethernet_address;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.sender_ethernet_address = ethernet_address_;
      arp.opcode = ARPMessage::OPCODE_REPLY;

      EthernetFrame pkt;
      pkt.header.type = EthernetHeader::TYPE_ARP;
      pkt.header.src  = ethernet_address_;
      pkt.header.dst  = arp.target_ethernet_address;
      pkt.payload = serialize(arp);
      packet.push(pkt);
      return {};
    }

    if (arp.opcode == ARPMessage::OPCODE_REPLY) {
      /* Remember sender IP - MAC*/
      arp_table[arp.sender_ip_address] = arp.sender_ethernet_address;
      arp_timer[arp.sender_ip_address] = 0;
      /* Send the waiting datagram */
      if (datagram.find(arp.sender_ip_address) == datagram.end())
        return {};

      std::queue<InternetDatagram> wait_dgram = datagram[arp.sender_ip_address];
      while ( !wait_dgram.empty() ) {
        EthernetFrame pkt;
        pkt.header.type = EthernetHeader::TYPE_IPv4;
        pkt.header.src  = ethernet_address_;
        pkt.header.dst  = arp.sender_ethernet_address;
        pkt.payload = serialize(wait_dgram.front());

        wait_dgram.pop();
        packet.push(pkt);
      }

      datagram.erase(arp.sender_ip_address);
      return {};
    }
  }

  return{};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for( auto itr = arp_timer.begin(); itr != arp_timer.end();) {
    itr->second += ms_since_last_tick;
    /* Not get ARP Reply */
    if (itr->second > ARP_SENT_TIMEOUT && arp_table.find(itr->first) == arp_table.end()) {
      arp_timer.erase(itr++);
      continue;
    }

    if (itr->second > ARP_RECORD_TIMEOUT && arp_table.find(itr->first) != arp_table.end()) {
      arp_table.erase(itr->first);
      arp_timer.erase(itr++);
      continue;
    }
    itr++;
  }

  return;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if (!packet.empty()) {
    EthernetFrame pkt = packet.front();
    packet.pop();
    return pkt;
  }
  return {};
}
