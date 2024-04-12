#include "reassembler.hh"
#include <algorithm>
#include <iostream>
#include <fstream>  // for debugging

using namespace std;
void Reassembler::attempt_write()
{
  // capacity_ = output_.writer().getmaxcapacity();
  capacity_ = output_.writer().available_capacity();
  // uint64_t available_capacity = output_.writer().available_capacity();
  uint64_t available_capacity = capacity_;
  while(available_capacity != 0 && !segs.empty()){
    if(segs.begin()->l != start_) break;
    std::string& nowstring = stringpool[segs.begin()->id];
    auto nowstringlength = nowstring.size();
    auto nowstringlengthleft = nowstringlength - skip_;
    if(available_capacity >= nowstringlengthleft){
      output_.writer().push(nowstring.substr(skip_));
      stringpool.erase(segs.begin()->id);
      segs.erase(segs.begin());
      skip_=0;
      start_ += nowstringlengthleft;
      storage_count -= nowstringlengthleft;
      available_capacity -= nowstringlengthleft;
    }else{
      output_.writer().push(nowstring.substr(skip_, available_capacity));
      skip_ += available_capacity;
      start_ += available_capacity;
      storage_count -= available_capacity;
      available_capacity = 0;
    }
  }
  //if(seen_end && (start_ > last_index || (start_ == last_index && start_ == 0))){
  if(seen_end && start_ >= last_index){
    output_.writer().close();
  }
  return;
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  attempt_write();
  //because all the status variables are unsigned, so we need to handle data = "" case carefully
  if(data.empty()){
    if(is_last_substring) {
      last_index = first_index;
      seen_end = true;
    }
    attempt_write();
    return;
  }

  //normal case
  if(is_last_substring && start_ + capacity_ - 1 >= first_index + data.size() - 1){
    last_index = first_index + data.size() - 1;
    seen_end = true;
  }
  uint64_t indexl=std::max(first_index,start_), indexr=std::min(first_index+data.size()-1, start_+capacity_-1);
  if(last_index != 0) indexr = std::min(indexr, last_index); // last_index is vaild when not 0
  if(indexl > indexr) return; // insert string at least a null
  uint64_t already_inserted = 0;

  if(!segs.empty()){
    // delete segments must start from right to left, because the right iterator will be invalid after erase left
    auto maybe_overlap_seg = segs.upper_bound(seg(indexr, -1, -1));
    // get the last segment that has l <= indexr , when upper_bound failed it will at least return segs.end()
    // which is type-safed
    // if there exist such segment then iter through
    if(maybe_overlap_seg != segs.begin())
    do{
      maybe_overlap_seg--;
      //side case 1: maybe_overlap_seg do not satisfy l <= indexr
      if(maybe_overlap_seg->l >indexr){
        break;
      }
      // handle right bound case first, then left bound case
      // not intersect
      if(maybe_overlap_seg->r < indexl){
        break;
      }
      //string is a substring of maybe_overlap_seg
      else if( maybe_overlap_seg->r >= indexr){
        if(indexl <= maybe_overlap_seg->l) indexr = maybe_overlap_seg->l-1;
        else{
          indexl = maybe_overlap_seg->r+1;
          break;
        }
      }
      //overlap
      else if(indexl <= maybe_overlap_seg->l){
        stringpool.erase(maybe_overlap_seg->id);
        already_inserted += maybe_overlap_seg->r - maybe_overlap_seg->l + 1;
        maybe_overlap_seg = segs.erase(maybe_overlap_seg);
      }
      //intersect
      else{
        indexl = maybe_overlap_seg->r+1;
        break;
      }
    }while(!segs.empty() && maybe_overlap_seg != segs.begin());
  }
  if(indexl > indexr) return; // insert string at least a null

  pool_seq_id++;
  stringpool[pool_seq_id] = data.substr(indexl-first_index, indexr-indexl+1);
  segs.insert(seg(indexl, indexr, pool_seq_id));
  storage_count += indexr-indexl+1 - already_inserted;
  attempt_write();
  return;
}

uint64_t Reassembler::bytes_pending() const
{
  return storage_count;
}
