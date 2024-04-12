#include "reassembler.hh"
#include <algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t indexl=first_index, indexr=std::min(first_index+data.size()-1, start_+capacity_-1);
  if(last_index != -1) indexr = std::min(indexr, last_index);
  if(indexl >= indexr) return; // insert string at least a null

  if(!segs.empty()){
    // delete segments must start from right to left, becasue the right iterator will be invalid after erase left
    auto maybe_overlap_seg = segs.upper_bound(seg(indexr, -1, -1));
    // get the last segment that has indexr < l, when upper_bound failed it will at least return segs.end()
    // which is type-safed
    do{
      maybe_overlap_seg--;
      //overlap
      if(maybe_overlap_seg->l <= indexl){
        stringpool.erase(maybe_overlap_seg->id);
        segs.erase(maybe_overlap_seg);
      }
      // not intersect
      else if(maybe_overlap_seg->r <= indexr){
        break;
      }
      //intersect
      else{
        indexl = maybe_overlap_seg->r;
        break;
      }
    }while(maybe_overlap_seg != segs.begin());
  }
  if(indexl >= indexr) return; // insert string at least a null

  pool_seq_id++;
  stringpool[pool_seq_id] = data.substr(indexl-first_index, indexr-indexl+1);
  segs.emplace(indexl, indexr, pool_seq_id);
  storage_count += indexr-indexl+1;

  return;
}

uint64_t Reassembler::bytes_pending() const
{
  return storage_count;
}
