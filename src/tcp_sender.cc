#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>  

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_count_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  
  // if(last_window_size_ == 0) {
  //   //send a single byte
  //   //save the single byte in retransmission buffer
  //   // last_window_size_ = 1;
  //   TCPSenderMessage tmp{};
  //   tmp.seqno = Wrap32::wrap(next_send_absolute_seqno,isn_);
  //   tmp.payload = reader().peek().substr(0,1);
  //   transmit(tmp);
  //   buffer_.push({tmp,{next_send_absolute_seqno,next_send_absolute_seqno}});
  //   if(timer_running_ == false){
  //     timer_running_ = true;
  //     last_alarm_ticks_ = totalalive_ticks_;
  //   }
  //   next_send_absolute_seqno++;
  //   sequence_numbers_in_flight_++;
  //   return;
  // }
  // Your code here.
  // send a segment with the maximum size create from the reader
  // save the segment in retransmission
  uint64_t space_remaining = 0;
  // if(last_window_size_ > sequence_numbers_in_flight_)
  space_remaining = (last_window_size_ == 0?1:last_window_size_) - sequence_numbers_in_flight_;
  while(space_remaining > 0){

    TCPSenderMessage tmp{};
    if(addition_fin){
      tmp.seqno = Wrap32::wrap(next_send_absolute_seqno,isn_);
      tmp.FIN = true;
      addition_fin = false;
      transmit(tmp);
      buffer_.push({tmp,{next_send_absolute_seqno,next_send_absolute_seqno}});
      if(timer_running_ == false){
        timer_running_ = true;
        last_alarm_ticks_ = totalalive_ticks_;
      }
      timer_running_ = true;
      next_send_absolute_seqno++;
      sequence_numbers_in_flight_++;
      addition_fin = false;
      is_actually_fin = true;
      return;
    }

    uint64_t now_payload_size = 0;
    if(send_beginning_){
      tmp.SYN = true;
      send_beginning_ = false;
      now_payload_size = 1;
      space_remaining--;
    }

    tmp.seqno = Wrap32::wrap(next_send_absolute_seqno,isn_);
    string_view peek {};
    for(peek= input_.reader().peek();!peek.empty()&&tmp.payload.size()<=TCPConfig::MAX_PAYLOAD_SIZE;peek= input_.reader().peek()){
      uint64_t max_peek_size = min(min(peek.size(),TCPConfig::MAX_PAYLOAD_SIZE - tmp.payload.size()),space_remaining);
      if(max_peek_size == 0){
        break;
      }
      tmp.payload += peek.substr(0,max_peek_size);
      now_payload_size += max_peek_size;
      input_.reader().pop(max_peek_size);
      space_remaining -= max_peek_size;
    }
    if(!reached_fin&&reader().is_finished()&&input_.reader().peek().empty()){
      if(space_remaining == 0){
        addition_fin = true; 
      }else{
        tmp.FIN = true;
        now_payload_size++;
      }
      reached_fin = true;
    } 
    if(tmp.sequence_length() == 0){
      break;
    }
    is_rst |= reader().has_error();
    tmp.RST = is_rst;
    transmit(tmp);
    buffer_.push({tmp,{next_send_absolute_seqno,next_send_absolute_seqno + tmp.sequence_length() - 1}});
    if(timer_running_ == false){
      timer_running_ = true;
      last_alarm_ticks_ = totalalive_ticks_;
    }
    timer_running_ = true;
    next_send_absolute_seqno += tmp.sequence_length();
    sequence_numbers_in_flight_ += tmp.sequence_length();

    if(input_.reader().peek().empty()){
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage tmp{};
  tmp.seqno = Wrap32::wrap(next_send_absolute_seqno,isn_);
  tmp.RST = is_rst | reader().has_error();;
  return tmp;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  //dunno what to do with RST`
  if(msg.RST){
    //do something
    is_rst = true;
    writer().set_error();
    return;
  }
  // before receiver get SYN, the receivermessage does not have ackno
  if(!msg.ackno.has_value()){
    last_window_size_ = msg.window_size;
    return;
  }
  

  Wrap32 peer_send_ackno= msg.ackno.value();
  uint64_t absolute_ackno = peer_send_ackno.unwrap(isn_, last_ackno_absolute);
  bool ackno_success = false;
  if(last_ackno_absolute + sequence_numbers_in_flight_ < absolute_ackno){
    //impossible ackno
    return;
  }
  if(absolute_ackno < last_ackno_absolute){
    //duplicate ackno
    return;
  }
  sequence_numbers_in_flight_ -= absolute_ackno - last_ackno_absolute;
  while(!buffer_.empty() && buffer_.front().second.second < absolute_ackno){
    //sequence_numbers_in_flight_ -= buffer_.front().first.sequence_length();
    buffer_.pop();
    ackno_success = true;
  }
  if(ackno_success){
    //reset the RTO
    now_RTO_ms_ = initial_RTO_ms_;
    retransmissions_count_ = 0;
    if(!buffer_.empty()){
      last_alarm_ticks_ = totalalive_ticks_;
    }else timer_running_ = false;
  }
  last_window_size_ = msg.window_size;
  //maybe cause problem
  last_ackno_absolute = absolute_ackno;
  return;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  totalalive_ticks_ += ms_since_last_tick;
  if(timer_running_&&totalalive_ticks_ - last_alarm_ticks_ >= now_RTO_ms_){
    if(buffer_.empty()) return;
    //resend the first segment in the retransmission buffer
    transmit(buffer_.front().first);
    if(last_window_size_ !=0){
      //keep track of consective retransmissions
      retransmissions_count_++;
      //double the RTO
      now_RTO_ms_ *= 2;
    }
    last_alarm_ticks_ = totalalive_ticks_;
  }
  return;
}
