#include "byte_stream.hh"
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return {closed_ == true};
}

void Writer::push( string data )
{
  if(closed_){
    return;
  }
  if(data =="") return;
  auto len = min(data.size(), available_capacity());
  if(len == 0) return;
  size_ += len;
  write_count_ += len;
  buffer_.push_back(data.substr(0, len));
  // (void)data;
  return;
}

void Writer::close()
{
  closed_ = 1;
  return;
}

uint64_t Writer::available_capacity() const
{
  return {capacity_ - size_ + skip_};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {write_count_};
}

bool Reader::is_finished() const
{
  // Your code here.
  return {closed_ == 1 && size_ == 0};
}

uint64_t Reader::bytes_popped() const
{
  return {read_count_};
}

string_view Reader::peek() const
{
  string_view fuckview = buffer_.front();
  fuckview.remove_prefix(skip_);
  //  if (buffer_.front().size()==0 || buffer_.front().size() == skip_ ) {
  //     std::cout<<"fuck";
  //   }
  return fuckview;
}

void Reader::pop( uint64_t len )
{
  while(len){
    if(len >= buffer_.front().size() - skip_){
      len -= buffer_.front().size() - skip_;
      size_ -= buffer_.front().size();
      read_count_ += buffer_.front().size() - skip_;
      skip_ = 0;
      buffer_.pop_front();
    } else {
      skip_ += len;
      read_count_ += len;
      len = 0;
    }
  }
    return;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {size_ - skip_};
}
