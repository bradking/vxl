// This is brl/bpro/bprb/bprb_process.h
#ifndef bprb_process_ext_h_
#define bprb_process_ext_h_
//------------------------------------------------------------------------------
//:
// \file
// \brief The bprb process base class
//
//  An extension class to the bprb_process to modify some behaviour. bprb_process 
//  assumes the input and output types are defined in the constructor and the 
//  input and output array are allocated early on. This extension does not expect 
//  to get the input-output types or arrays initialized.
// 
// \author
//   G.D. Tunali
//
// \verbatim
//  Modifications:
// \endverbatim
//------------------------------------------------------------------------------
#include "bprb_process.h"

//#include <vcl_vector.h>
//#include <vcl_string.h>
//#include <bprb/bprb_parameters_sptr.h>
//#include <brdb/brdb_value_sptr.h>

class bprb_process_ext : public bprb_process
{
 public:
  bprb_process_ext();
  //: Clone the process
  virtual bprb_process_ext* clone() const = 0;

  bool set_input(unsigned i, brdb_value_sptr const& value);

  bool set_input_data(vcl_vector<brdb_value_sptr> const& inputs);

  bool set_output_types(vcl_vector<vcl_string> const& types);
   
  bool set_output(unsigned i, brdb_value_sptr val);

 protected:
  //: Copy Constructor
 // bprb_process_ext(const bprb_process_ext& other);

  virtual ~bprb_process_ext() {}

};

#endif // bprb_process_ext_h_
