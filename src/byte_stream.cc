#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return {closed_ == true};
}

void Writer::push( string data )
{
  auto len = min(data.size(), available_capacity());
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
  return {capacity_ - size_};
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
  // Your code here.
  return {read_count_};
}

string_view Reader::peek() const
{
  // bool flag = skip_ < buffer_.front().size();
  // if(flag){
  //   return {buffer_.front().substr(skip_,skip_+1)};
  // }else{
  // }
  // Your code here.
  return {buffer_.front().substr(skip_,skip_+1)};
}

void Reader::pop( uint64_t len )
{
  while(len){
    if(len +skip_ >= buffer_.front().size()){
      len -= buffer_.front().size();
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
  return {size_};
}
