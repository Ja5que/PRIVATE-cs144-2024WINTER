#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  //translate datagram to ethernet frame
  uint32_t datagram_dest_ip_address = dgram.header.dst;
  uint32_t next_hop_ip_address = next_hop.ipv4_numeric();
  Serializer serializer;
  if(arp_table_.count(next_hop_ip_address) == 0){
    //unknown ethernet address
    //send an ARP request
    if(arp_request_pool_.count(next_hop_ip_address)==0 || last_tick_-arp_request_pool_[datagram_dest_ip_address]>5000){
      ARPMessage arp;
      arp.opcode = ARPMessage::OPCODE_REQUEST;
      arp.sender_ethernet_address = ethernet_address_;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.target_ip_address = next_hop_ip_address;
      // arp.target_ethernet_address = 0;
      arp.serialize(serializer);
      EthernetFrame frame;
      frame.header.dst = ETHERNET_BROADCAST;
      frame.header.src = ethernet_address_;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.payload = serializer.output();
      arp_request_pool_[next_hop_ip_address]=last_tick_;
      datagrams_to_send_.insert({next_hop_ip_address,{dgram,next_hop}});
      transmit(frame);
    }
  }else{
    EthernetFrame frame;
    //already know the ethernet address
    frame.header.dst = arp_table_[next_hop_ip_address];
    frame.header.src = ethernet_address_;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    dgram.serialize(serializer);
    frame.payload = serializer.output();
    transmit(frame);
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST){
  //   return;
  // }
  //if frame is IPV4
  if(frame.header.type == EthernetHeader::TYPE_IPv4){
    if(frame.header.dst != ethernet_address_ ){
      return;
    }
    Parser parser(frame.payload);
    InternetDatagram dgram;
    dgram.parse(parser);
    datagrams_received_.push(dgram);    
  }else if(frame.header.type == EthernetHeader::TYPE_ARP){
    Parser parser(frame.payload);
    ARPMessage arp;
    arp.parse(parser);
    if(arp.target_ip_address != ip_address_.ipv4_numeric()){
      return;
    }
    arp_table_[arp.sender_ip_address] = arp.sender_ethernet_address;
    arp_table_ttl_.push({arp.sender_ip_address,last_tick_});; 
    if(datagrams_to_send_.count(arp.sender_ip_address)>0){
      for(auto range = datagrams_to_send_.equal_range(arp.sender_ip_address);range.first!=range.second;++range.first){
        InternetDatagram dgram = range.first->second.first;
        Address next_hop = range.first->second.second;
        send_datagram(dgram,next_hop);
      }
      datagrams_to_send_.erase(arp.sender_ip_address);
    }
    if(arp.opcode == ARPMessage::OPCODE_REQUEST){
      ARPMessage arp_reply;
      arp_reply.opcode = ARPMessage::OPCODE_REPLY;
      arp_reply.sender_ethernet_address = ethernet_address_;
      arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
      arp_reply.target_ethernet_address = arp.sender_ethernet_address;
      arp_reply.target_ip_address = arp.sender_ip_address;
      Serializer serializer;
      arp_reply.serialize(serializer);
      EthernetFrame sendframe;
      sendframe.header.dst = arp.sender_ethernet_address;
      sendframe.header.src = ethernet_address_;
      sendframe.header.type = EthernetHeader::TYPE_ARP;
      sendframe.payload = serializer.output();
      transmit(sendframe);
    }else if(arp.opcode == ARPMessage::OPCODE_REPLY){
      // do nothing
    }else{
      throw runtime_error("Unknown ARP opcode");
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  last_tick_ += ms_since_last_tick;
  while(!arp_table_ttl_.empty()&&last_tick_-arp_table_ttl_.front().second>30000){
    arp_table_.erase(arp_table_ttl_.front().first);
    arp_table_ttl_.pop();
  }
}
