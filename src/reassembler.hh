#pragma once

#include "byte_stream.hh"
#include <unordered_map>
#include <set>
struct seg;
class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

  // Access starting index
  uint64_t getstart() const { return start_; }

  // Access closing status
  bool getclosed() const { return is_closed; }

  // set error state
  void set_error() { output_.set_error(); }

private:
  ByteStream output_; // the Reassembler writes to this ByteStream
  std::unordered_map<uint64_t,std::string> stringpool{};
  std::set<seg> segs{};
  uint64_t start_{0};
  uint64_t capacity_{0}; // trace bytestream capacity
  uint64_t last_index{0};
  uint64_t pool_seq_id{0};
  uint64_t storage_count{0};
  uint64_t skip_{0};
  bool seen_end{false};
  bool is_closed{false};
  void attempt_write();
};

struct seg{
  uint64_t l,r,id;
  seg(uint64_t _l, uint64_t _r, uint64_t _id): l(_l), r(_r), id(_id) {}
  bool operator < (const seg &rhs) const{
    return l < rhs.l;
  }
};