#include "router.hh"

#include <iostream>
#include <limits>
#include <algorithm>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  _routes_map.insert({interface_num,{ route_prefix,prefix_length, next_hop}});    
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  size_t id = 0;
  for(auto interface : _interfaces){
    while(!interface->datagrams_received().empty()){
      auto datagram = interface->datagrams_received().front();
      uint8_t longest_prefix = 0;
      optional<Address> next_hop;
      size_t interface_num = 0;
      bool is_matched = false;
      for(auto it =_routes_map.begin();it!=_routes_map.end();it++){
      // for(auto range = _routes_map.equal_range(id);range.first!=range.second;range.first++){
        auto route = it->second;
        uint32_t route_prefix = get<0>(route);
        uint8_t prefix_length = get<1>(route);
        uint32_t datagram_dest_ip_address = datagram.header.dst;
        uint32_t matchprefix = route_prefix  ^ datagram_dest_ip_address; 
        uint8_t prelen = std::min((uint8_t)countl_zero(matchprefix), prefix_length);
        if(prelen == prefix_length){
          if(prelen >= longest_prefix){
            longest_prefix = prelen;
            next_hop = get<2>(route);
            interface_num = it->first;
            is_matched = true;
          }
        }
      }
      interface->datagrams_received().pop();
      if(is_matched == 0){
        //do nothing
      }else
       if(next_hop.has_value()){
        if(datagram.header.ttl > 1){
          datagram.header.ttl--;
          datagram.header.compute_checksum();
          _interfaces[interface_num]->send_datagram(datagram, next_hop.value());
        }
      }else{
        //directly connected
        if(datagram.header.ttl > 1){
          datagram.header.ttl--;
          datagram.header.compute_checksum();
          _interfaces[interface_num]->send_datagram(datagram, Address::from_ipv4_numeric(datagram.header.dst));
        }
      }
    }
    id++;
  }
}