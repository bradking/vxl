// This is core/vil/vil_block_cache.h
#ifndef vil_block_cache_h_
#define vil_block_cache_h_
#ifdef VCL_NEEDS_PRAGMA_INTERFACE
#pragma interface
#endif
//:
// \file
// \brief A block cache with block population prioitized by age
// \author J. L. Mundy
//
#include <vcl_queue.h>
#include <vcl_vector.h>
#include <vil/vil_image_view.h>

//container for blocks to maintain a timestamp and mutation state
struct bcell
{
  bcell(const unsigned bindex_i, const unsigned bindex_j,
        vil_image_view_base_sptr const& blk):
    bindex_i_(bindex_i), bindex_j_(bindex_j), birthdate_(time_++), blk_(blk)
  {}

  //:block indices
  unsigned bindex_i_;  unsigned bindex_j_;
  //:the time of insertion into the queue
  unsigned long birthdate_;
  //:the block itself
  vil_image_view_base_sptr blk_;
  //:update the age of a block
  void touch(){birthdate_=time_++;}
  //: for debug
  void print()const{vcl_cout << "[" << bindex_i_ << ' ' << bindex_j_ 
                        << "](" << birthdate_ << ")\n";}
  private:
  static unsigned long time_; //static timekeeper
};

class vil_block_cache 
{
 public:

  vil_block_cache(const unsigned block_capacity) : nblocks_(block_capacity){}
  ~vil_block_cache();

  //:add a block to the buffer
  bool add_block(const unsigned& block_index_i, const unsigned& block_index_j,
                 vil_image_view_base_sptr const& blk); 

  //:retrieve a block from the buffer
  bool get_block(const unsigned& block_index_i, const unsigned& block_index_j,
                 vil_image_view_base_sptr& blk) const;
    
  //:block capacity
  unsigned block_size() const{return nblocks_;}
 private:
  struct compare
  {
    bool operator()(bcell* const& c1, bcell* const& c2) const
    {return c1->birthdate_ > c2->birthdate_;}
  };
  //members
  //:block queue
  vcl_priority_queue<bcell*, vcl_vector<bcell*>, compare> queue_;
  //:block index
  vcl_vector<bcell*> blocks_;
  //:capacity in blocks
  unsigned nblocks_; 
  //private functions
  //:remove the lowest priority block 
  bool remove_block();
};

#endif // vil_block_cache_h_
