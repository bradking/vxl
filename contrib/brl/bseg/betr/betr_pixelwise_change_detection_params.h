 #ifndef betr_pixelwise_change_detection_params_h_
#define betr_pixelwise_change_detection_params_h_
//:
// \file
// \brief Parameters for the edgel change_detection algorithm
// \author Thomas Pollard
// \date November 18, 2016
//
// method - an integer specifying the algorithm used, defined below and in
//          baml_detect_change.h
//
//  0 Census       
//  1 Birchfield-Tomasi      
//  2 Gradient difference
//  3 Non-parametric transfer function learning
//                                          
#include <string>
#include <iostream>
#include <vcl_compiler.h>
#include "betr_params.h"
#include <baml/baml_detect_change.h>
//#include <sdet/sdet_detector_params.h>

class betr_pixelwise_change_detection_params : public betr_params
{
 public:

  betr_pixelwise_change_detection_params(){
    method_list_.push_back(BIRCHFIELD_TOMASI); // 0
    method_list_.push_back(CENSUS); // 1
    method_list_.push_back(DIFFERENCE); // 2
    method_list_.push_back(GRADIENT_DIFF); // 3
    method_list_.push_back(NON_PARAMETRIC); // 4
    method_list_.push_back(HIST_CMP); // 5
  }

  //: check values of parameters to flag illegal values
  virtual bool sanity_check(std::string& errors) const;

  virtual void serialize( Json::Value& root ) const{
    int method_idx = 0;
    for( int m = 0; m < method_list_.size(); m++ )
      if( pw_params_.method == method_list_[m] ) method_idx = m;
    root["method"] = method_idx;
    root["correct_gain_offset"] = pw_params_.correct_gain_offset;
    root["num_tiles"] = pw_params_.num_tiles;
    root["registration_rad"] = pw_params_.registration_refinement_rad;
    root["pChange"] = pw_params_.pChange;
    root["event_width"] = pw_params_.event_width;
    root["event_height"] = pw_params_.event_height;
    //root["bt_std"] = pw_params_.bt_std;
    root["bt_rad"] = pw_params_.bt_rad;
    //root["census_std"] = pw_params_.census_std;
    root["census_tol"] = pw_params_.census_tol;
    root["census_rad"] = pw_params_.census_rad;
    //root["grad_std"] = pw_params_.grad_std;
    root["img_bit_depth"] = pw_params_.img_bit_depth;
    root["hist_bit_depth"] = pw_params_.hist_bit_depth;
    root["neighborhood_size"] = pw_params_.neighborhood_size;
    root["num_bins"] = pw_params_.num_bins;
    root["grad_mag_on"] = pw_params_.grad_mag_on;
    root["hist_method"] = pw_params_.hist_method;
    root["multiple_ref"] = pw_params_.multiple_ref; 
    root["multi_method"] = pw_params_.multi_method;
    root["pGoodness"] = pw_params_.pGoodness;
  }

  virtual void deserialize( Json::Value& root){
    int method_idx = std::min( 
      (int)method_list_.size(), std::max( 0, (int)root["method"].asInt() ) );
    pw_params_.method = method_list_[method_idx]; 
    pw_params_.correct_gain_offset = root["correct_gain_offset"].asBool(); 
    pw_params_.num_tiles = root["num_tiles"].asInt();
    pw_params_.registration_refinement_rad = root["registration_rad"].asInt();
    pw_params_.pChange = root["pChange"].asFloat();
    pw_params_.event_width = root["event_width"].asInt();
    pw_params_.event_height = root["event_height"].asInt();
    //pw_params_.bt_std = root["bt_std"].asFloat();
    pw_params_.bt_rad = root["bt_rad"].asInt();
    //pw_params_.census_std = root["census_std"].asFloat();
    pw_params_.census_tol = root["census_tol"].asInt();
    pw_params_.census_rad = root["census_rad"].asInt();
   // pw_params_.grad_std = root["grad_std"].asFloat();
    pw_params_.img_bit_depth = root["img_bit_depth"].asInt();
    pw_params_.hist_bit_depth = root["hist_bit_depth"].asInt();
    pw_params_.neighborhood_size = root["neighborhood_size"].asInt();
    pw_params_.num_bins = root["num_bins"].asInt();
    pw_params_.grad_mag_on = root["grad_mag_on"].asBool ();
    pw_params_.hist_method = root["hist_method"].asString();
    pw_params_.multiple_ref = root["multiple_ref"].asBool(); 
    pw_params_.multi_method = root["multi_method"].asString();
    pw_params_.pGoodness = root["pGoodness"].asFloat();
  }

  //: parameter block
  baml_change_detection_params pw_params_;

protected:

  std::vector< baml_change_detection_method > method_list_;
};

std::ostream&  operator<<(std::ostream& s, betr_pixelwise_change_detection_params const& ecdp);
std::istream&  operator>>(std::istream& s, betr_pixelwise_change_detection_params& ecdp);

#endif   // DO NOT ADD CODE AFTER THIS LINE! END OF DEFINITION FOR CLASS betr_pixelwise_change_detection_params.

