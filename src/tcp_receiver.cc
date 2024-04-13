#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // TCPSenderMessage contains sequence number in warp32, payload, SYN/FIN/RST flag
 
  // get the absolute sequence number
  // if SYN flag is set, set the zero point to the sequence number
  // if SYN flag is not set, unwrap the sequence number
  SYN_received_ |= message.SYN;
  RST_received_ |= message.RST;
  if(message.RST)  reassembler_.set_error();
  if(message.SYN)  zero_point_ = message.seqno;
  uint64_t absolute_seqno = message.seqno.unwrap(zero_point_, reassembler_.getstart());
  reassembler_.insert(absolute_seqno+(message.SYN?0:-1), message.payload, message.FIN);
  // Your code here.
  // (void)message;
  return;
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message;
  if(SYN_received_){
    if(reassembler_.getclosed()) message.ackno = zero_point_  + 1 + reassembler_.getstart() + 1;
    else  message.ackno = zero_point_  + 1 + reassembler_.getstart();
  }
  message.window_size = std::min((uint64_t)UINT16_MAX,reassembler_.writer().available_capacity());
  // if(reassembler_.writer().has_error()) RST_received_ = true;
  message.RST = RST_received_ | reassembler_.writer().has_error();
  return message;
}
