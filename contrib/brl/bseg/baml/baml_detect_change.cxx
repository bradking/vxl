// This is brl/baml/baml_detect_change.cxx

#include <iomanip>
#include <algorithm>

#include <vnl/vnl_inverse.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/vil_crop.h>
#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_sobel_3x3.h>
#include <vil/algo/vil_median.h>
#include <vil/algo/vil_gauss_filter.h>

#include "baml_birchfield_tomasi.h"
#include "baml_detect_change.h"
#include "baml_census.h"
#include "baml_utilities.h"
#include "bsta/bsta_histogram.h"

#include <vul/vul_file_iterator.h>
#include <vil/vil_load.h>

//--------------------------SINGLE IMAGE CHANGE DETECTION----------------------------------
bool baml_change_detection::detect(
  const vil_image_view<vxl_uint_16>& img_target,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid,
  vil_image_view<float>& change_prob_target)
{
  float fg = 0; 
  int width = img_target.ni();
  int height = img_target.nj();
  vil_image_view<float> score;
  bool cd_success = baml_change_detection::detect_internal(img_target, img_ref, valid, score, fg);
  if (!cd_success) return false;
  // Covert score into probability
  float sigma = baml_sigma(score);
  vil_image_view<float> prob;
  baml_gaussian(score, prob, sigma);

  change_prob_target.set_size(width, height);
  change_prob_target.fill(0.0);
  // Convert probability to log likelihood ratio
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid(x, y) == false) continue;
      change_prob_target(x, y) = log(fg) - log( prob(x, y));
    }
  }
  // Convert likelihood into probability
  baml_sigmoid(change_prob_target, change_prob_target, params_.pChange);
  return true;
}

//--------------------------MULTI IMAGE CHANGE DETECTION----------------------------------
bool 
baml_change_detection::multi_image_detect(
  const vil_image_view<vxl_uint_16>& img_target,
  const std::vector<vil_image_view<vxl_uint_16> > img_ref,
  const std::vector<vil_image_view<bool> > valid,
  vil_image_view<float>& change_prob_target) {
  
  // use internal change detection function to align/obtain scores
  std::vector<vil_image_view<float> > scores;
  vil_image_view<vxl_uint_16> img_tar_crop;
  std::vector<float> foreground_dist;
  int max_x_off = 0;
  int crop_width = 0;
  int max_y_off = 0;
  int crop_height = 0;
  detect_mutli_internal(img_target, img_ref, valid, scores, img_tar_crop, foreground_dist, max_x_off, crop_width, max_y_off, crop_height);

  vil_image_view<float> change_prob_target_big; // a change probability map that is the same size as the input target image
  change_prob_target_big.set_size(img_target.ni(), img_target.nj());
  change_prob_target_big.fill(0.0);
  vil_image_view<float> change_prob_target_crop = vil_crop(change_prob_target_big, max_x_off, crop_width, max_y_off, crop_height); // cropped view of the probability map that only looks at pixels within the area cropped for image alignment

  if (strcmp((params_.multi_method).c_str(), "product") == 0) { // product method
    baml_change_detection::multi_product(scores, foreground_dist, change_prob_target_crop);
  }
  else if (strcmp((params_.multi_method).c_str(), "sum") == 0) { // sum method
    baml_change_detection::multi_sum(scores, foreground_dist, change_prob_target_crop);
  }
  else if (strcmp((params_.multi_method).c_str(), "maximum") == 0) { // probability maximization method
    baml_change_detection::multi_max_prob(scores, foreground_dist, change_prob_target_crop);
  }
  else {
    std::cerr << "Multi-image fusion method not recognized";
    return false;
  }

  // return a probability map that is the same size as the target input. Any pixels that were cropped
  // for image alignment refinement are 0 probability
  change_prob_target.deep_copy(change_prob_target_big);
  return true;
}


//--------------------------EXPECTED TIME OF CHANGE----------------------------------
bool baml_change_detection::expected_time_change(
  const vil_image_view<vxl_uint_16>& img_target,
  const std::vector<vil_image_view<vxl_uint_16> > img_ref,
  const std::vector<vil_image_view<bool> > valid,
  vil_image_view<float>& change_time) {

  // use internal change detection function to align/obtain scores
  std::vector<vil_image_view<float> > scores;
  vil_image_view<vxl_uint_16> img_tar_crop;
  std::vector<float> foreground_dist;
  int max_x_off = 0;
  int crop_width = 0;
  int max_y_off = 0;
  int crop_height = 0;
  detect_mutli_internal(img_target, img_ref, valid, scores, img_tar_crop, foreground_dist, max_x_off, crop_width, max_y_off, crop_height);

  // covert scores to probs
  std::vector<vil_image_view<float> > probs;
  vil_image_view<float> score;
  for (int t = 0; t < scores.size(); t++) {
    vil_image_view<float> prob;
    score = scores[t];
    float sigma = baml_sigma(score);
    baml_gaussian(score, prob, sigma);
    probs.push_back(prob);
  }

  float alpha = 1;

  // create expect time of change map (time is based on index of reference image NOT on actual time)
  int width = img_tar_crop.ni(); int height = img_tar_crop.nj();
  change_time.set_size(width, height);
  change_time.fill(0.0);
  vil_image_view<float> sum_num_im, sum_denom_im; // temp visualizations to look at for debugging purposes
  sum_num_im.set_size(width, height); // temp
  sum_num_im.fill(0.0); // temp
  sum_denom_im.set_size(width, height); // temp
  sum_denom_im.fill(0.0); // temp
  float product, sum_numerator, sum_denominator;
  for (int x = 0; x < width; x++) { // loop over each pixel
    for (int y = 0; y < height; y++) {
      sum_numerator = 0;
      sum_denominator = 0;
      for (int t = -1; t < (int)scores.size(); t++) { // loop over each reference image
        vil_image_view<float> prob_s;
        product = 0;
        for (int s = t + 1; s < scores.size(); s++) {
          prob_s = probs[s]; // get score image at time s
          product += log(prob_s(x, y)*alpha + foreground_dist[0] * (1 - alpha));
        }
        product += (t + 1)*log(foreground_dist[0]);
        sum_numerator += t*exp(product);
        sum_denominator += exp(product);
      }
      change_time(x, y) = sum_numerator / sum_denominator;
      sum_denom_im(x, y) = sum_denominator;
      sum_num_im(x, y) = sum_numerator;
    }
  }

  // method 2: arg max
  vil_image_view<float> change_time_arg_max;
  change_time_arg_max.set_size(width, height);
  change_time_arg_max.fill(0.0);
  float arg_max_prob;
  float best_t;
  for (int x = 0; x < width; x++) { // loop over each pixel
    for (int y = 0; y < height; y++) {
      arg_max_prob = -FLT_MAX;
      for (int t = -1; t < (int)scores.size(); t++) { // loop over each reference image
        vil_image_view<float> prob_s;
        product = 0;
        for (int s = t + 1; s < (int)scores.size(); s++) {
          prob_s = probs[s]; // get score image at time s
          product += log(prob_s(x, y)*alpha + foreground_dist[0] * (1 - alpha));
        }
        product += (t + 1)*log(foreground_dist[0]);
        if (arg_max_prob < product) {
          arg_max_prob = product;
          best_t = t;
        }
      }
      change_time(x, y) = best_t;
    }
  }
  return true;
}
//-------SINGLE IMAGE CHANGE DETECTION INTERNAL, WHICH DOES THE ACTUAL CHANGE SCORES------------
bool baml_change_detection::detect_internal(
  const vil_image_view<vxl_uint_16>& img_target,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid,
  vil_image_view<float>& change_prob_target,
  float& foreground_dist)
{

  // Correct gain/offset
  vil_image_view<vxl_uint_16> corr_ref;
  if (params_.correct_gain_offset)
    baml_correct_gain_offset_tiled(img_target, img_ref, valid, params_.num_tiles, corr_ref);
  else
    corr_ref.deep_copy(img_ref);

  // Bound the registration search
  int reg_rad = std::min(20, std::max(0,
    params_.registration_refinement_rad));

  bool dc_success = false;

  // Find appropriate translational offsets
  vil_image_view<vxl_uint_16> img_tar_crop;
  vil_image_view<vxl_uint_16> img_ref_crop;
  vil_image_view <bool> valid_crop;
  vil_image_view<float> score;
  vil_image_view<float> score_crop;
  vil_image_view<float> score_best;
  score.set_size(img_ref.ni(), img_ref.nj());
  score_best.set_size(img_ref.ni(), img_ref.nj());
  int tar_x_off; // target x (width) offset 
  int tar_y_off; // target y (height) offset 
  int ref_x_off; // reference x (width) offset
  int ref_y_off; // reference x (height) offset
  int crop_n_i; // cropped width
  int crop_n_j; // cropped height
  float max_mean = 0;
  float mean_score = 0;

  // try all offsets within the selected translational radius
  for (int x_off = -reg_rad; x_off <= reg_rad; x_off++) {
    for (int y_off = -reg_rad; y_off <= reg_rad; y_off++) {

      // determine cropping offsets and size
      if (x_off < 0) {
        ref_x_off = -x_off;
        tar_x_off = 0;
        crop_n_i = img_ref.ni() + x_off;
      }
      else {
        ref_x_off = 0;
        tar_x_off = x_off;
        crop_n_i = img_ref.ni() - x_off;
      }
      if (y_off < 0) {
        ref_y_off = -y_off;
        tar_y_off = 0;
        crop_n_j = img_ref.nj() + y_off;
      }
      else {
        ref_y_off = 0;
        tar_y_off = y_off;
        crop_n_j = img_ref.nj() - y_off;
      }
      score.fill(0.0);
      img_tar_crop = vil_crop(img_target, tar_x_off, crop_n_i, tar_y_off, crop_n_j);
      img_ref_crop = vil_crop(corr_ref, ref_x_off, crop_n_i, ref_y_off, crop_n_j);
      valid_crop = vil_crop(valid, tar_x_off, crop_n_i, tar_y_off, crop_n_j);
      score_crop = vil_crop(score, tar_x_off, crop_n_i, tar_y_off, crop_n_j);

      // Detect change using specified method
      if (params_.method == BIRCHFIELD_TOMASI)
        dc_success = detect_bt(
          img_tar_crop, img_ref_crop, valid_crop, score_crop, foreground_dist);
      else if (params_.method == CENSUS)
        dc_success = detect_census(
          img_tar_crop, img_ref_crop, valid_crop, score_crop, foreground_dist);
      else if (params_.method == DIFFERENCE)
        dc_success = detect_difference(
          img_tar_crop, img_ref_crop, valid_crop, score_crop, foreground_dist);
      else if (params_.method == GRADIENT_DIFF)
        dc_success = detect_gradient(
          img_tar_crop, img_ref_crop, valid_crop, score_crop, foreground_dist);
      else if (params_.method == NON_PARAMETRIC)
        dc_success = detect_nonparam(
          img_tar_crop, img_ref_crop, valid_crop, score_crop, foreground_dist);
      else if (params_.method == HIST_CMP)
        dc_success = detect_histcmp(
          img_tar_crop, img_ref_crop, valid_crop, score_crop, foreground_dist);

      // save the score image if it improved the results 
      // NOTE: score and score_crop point to the same area of memory
      vil_math_mean(mean_score, score_crop, 0);
      if (mean_score > max_mean) {
        change_prob_target.deep_copy(score);
        max_mean = mean_score;
      }
    }
  }
  if (!dc_success) return false;
  return true;
}

//-------MULTI IMAGE CHANGE DETECTION INTERNAL, WHICH DOES ALIGNMENT AND SCORE COMPUTATION------------
bool baml_change_detection::detect_mutli_internal(
  const vil_image_view<vxl_uint_16>& img_target,
  const std::vector<vil_image_view<vxl_uint_16> >& img_ref,
  const std::vector<vil_image_view<bool> >& valid,
  std::vector<vil_image_view<float> >& probabilities,
  vil_image_view<vxl_uint_16>& img_tar_crop,
  std::vector<float>& foreground_dist,
  int& max_x_off,
  int& crop_width,
  int& max_y_off,
  int& crop_height)
{
  // Align all the images together by finding pairwise alignments
  // and cropping everything to the same size based on the minimun
  // and maximum pairwise x and y offsets
  std::vector<int> x_offsets, y_offsets; // store best offset for each reference/target pair
  int reg_rad = std::min(20, std::max(0, // Bound the registration search
    params_.registration_refinement_rad));
  bool dc_success = false;
  // Find appropriate translational offsets
  vil_image_view<vxl_uint_16> img_ref_crop;
  vil_image_view <bool> valid_crop;
  vil_image_view<float> score;
  vil_image_view<float> score_crop;
  vil_image_view<float> score_best;
  score.set_size(img_target.ni(), img_target.nj());
  int tar_x_off; // target x (width) offset 
  int tar_y_off; // target y (height) offset 
  int ref_x_off; // reference x (width) offset
  int ref_y_off; // reference x (height) offset
  int crop_n_i; // cropped width
  int crop_n_j; // cropped height
  float max_mean;
  float mean_score = 0;
  int best_x, best_y;
  vil_image_view<vxl_uint_16> corr_ref;

  // loop over all of our reference images
  for (int img_num = 0; img_num < img_ref.size(); img_num++) {
    best_x = 100; best_y = 100;
    max_mean = 0;
    baml_correct_gain_offset_tiled(img_target, img_ref[img_num], valid[img_num], params_.num_tiles, corr_ref);
    // try all offsets within the selected translational radius for this image pair
    for (int x_off = -reg_rad; x_off <= reg_rad; x_off++) {
      for (int y_off = -reg_rad; y_off <= reg_rad; y_off++) {

        // determine cropping offsets and size
        if (x_off < 0) {
          ref_x_off = -x_off;
          tar_x_off = 0;
          crop_n_i = img_target.ni() + x_off;
        }
        else {
          ref_x_off = 0;
          tar_x_off = x_off;
          crop_n_i = img_target.ni() - x_off;
        }
        if (y_off < 0) {
          ref_y_off = -y_off;
          tar_y_off = 0;
          crop_n_j = img_target.nj() + y_off;
        }
        else {
          ref_y_off = 0;
          tar_y_off = y_off;
          crop_n_j = img_target.nj() - y_off;
        }
        score.fill(0.0);
        img_tar_crop = vil_crop(img_target, tar_x_off, crop_n_i, tar_y_off, crop_n_j);
        img_ref_crop = vil_crop(corr_ref, ref_x_off, crop_n_i, ref_y_off, crop_n_j);
        valid_crop = vil_crop(valid[img_num], tar_x_off, crop_n_i, tar_y_off, crop_n_j);
        score_crop = vil_crop(score, tar_x_off, crop_n_i, tar_y_off, crop_n_j);

        // Detect change using difference because it is the simplest/fastest method so we use it for alignment
        float fg = 0;//don't actual need here
        dc_success = detect_difference(img_tar_crop, img_ref_crop, valid_crop, score_crop, fg);

        // save the score image if it improved the results 
        // NOTE: score and score_crop point to the same area of memory
        vil_math_mean(mean_score, score_crop, 0);
        if (mean_score > max_mean) {
          best_x = x_off; best_y = y_off;
          max_mean = mean_score;
        }
      }
    }
    if (best_x > reg_rad || best_y > reg_rad) {
      std::cerr << "No appropriate registration offset was found";
      return false;
    }
    x_offsets.push_back(best_x); // save best x for this pair
    y_offsets.push_back(best_y); // save best y for this pair
  }

  // find min/max translational offsets in x and y directions
  max_x_off = 0;
  int min_x_off = 0;
  max_y_off = 0;
  int min_y_off = 0;
  for (int i = 0; i < img_ref.size(); i++) {
    if (x_offsets[i] > max_x_off) max_x_off = x_offsets[i];
    if (x_offsets[i] < min_x_off) min_x_off = x_offsets[i];
    if (y_offsets[i] > max_y_off) max_y_off = y_offsets[i];
    if (y_offsets[i] < min_y_off) min_y_off = y_offsets[i];
  }

  // crop all images based on their pairwise best offsets and the overall min/max offsets
  crop_width = img_target.ni() - max_x_off + min_x_off;
  crop_height = img_target.nj() - max_y_off + min_y_off;
  img_tar_crop = vil_crop(img_target, max_x_off, crop_width, max_y_off, crop_height);
  std::vector<vil_image_view<vxl_uint_16> > img_ref_crop_vec;
  std::vector<vil_image_view<bool> > valid_crop_vec;
  for (int i = 0; i < img_ref.size(); i++) {
    vil_image_view<vxl_uint_16> cur_crop = vil_crop(img_ref[i], max_x_off - x_offsets[i], crop_width, max_y_off - y_offsets[i], crop_height);
    vil_image_view<bool> cur_valid_crop = vil_crop(valid[i], max_x_off - x_offsets[i], crop_width, max_y_off - y_offsets[i], crop_height);
    img_ref_crop_vec.push_back(cur_crop);
    valid_crop_vec.push_back(cur_valid_crop);
  }
  params_.registration_refinement_rad = 0; // We've already aligned our images

                                           // perform pairwise change detect on each reference image
  vil_image_view<vxl_byte> change_vis;
  for (int i = 0; i < img_ref_crop_vec.size(); i++) {
    vil_image_view<float> prob;
    float fg = 0;
    detect_internal(img_tar_crop, img_ref_crop_vec[i], valid_crop_vec[i], prob, fg);
    probabilities.push_back(prob);
    foreground_dist.push_back(fg);
  }
  return true;
}

//=================CHANGE DETECTION METHODS===========================================
//-----------------BIRCHFIELD TOMASI--------------------------------------------------
bool
baml_change_detection::detect_bt(
  const vil_image_view<vxl_uint_16>& img_tar,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid_ref,
  vil_image_view<float>& probability,
  float& foreground_dist)
{
  int width = img_tar.ni(), height = img_tar.nj();

  if (img_ref.ni() != width || img_ref.nj() != height ||
    valid_ref.ni() != width || valid_ref.nj() != height)
    return false;

  // Compute min and max observed intensities in target image
  vxl_uint_16 min_int = (vxl_uint_16)(pow(2, 16) - 1);
  vxl_uint_16 max_int = (vxl_uint_16)0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid_ref(x, y) == false) continue;
      min_int = std::min(min_int, img_tar(x, y));
      max_int = std::max(max_int, img_tar(x, y));
    }
  }

  // Compute foreground likelihood assuming uniform distribution on foreground
  foreground_dist = 1.0 / (max_int - min_int);

  // create score image
  vil_image_view<float> score;
  score.set_size(width, height);
  score.fill(0.0);

  // Compute Birchfield-Tomasi score
  vil_image_view<vxl_uint_16> score_uint16;
  if (!baml_compute_birchfield_tomasi(
    img_tar, img_ref, score_uint16, params_.bt_rad))
    return false;
  //convert score to float
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      score(x, y) = (float)score_uint16(x, y);
    }
  }

  //covert to probability
  float sigma = baml_sigma(score);
  baml_gaussian(score, probability, sigma);

  return true;
}


//-----------------CENSUS----------------------------------------------
bool
baml_change_detection::detect_census(
  const vil_image_view<vxl_uint_16>& img_tar,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid_ref,
  vil_image_view<float>& probability,
  float& foreground_dist)
{
  int width = img_tar.ni(), height = img_tar.nj();

  if (img_ref.ni() != width || img_ref.nj() != height ||
    valid_ref.ni() != width || valid_ref.nj() != height)
    return false;
  // Bound-check census_rad
  if (params_.census_rad <= 0) params_.census_rad = 1;
  if (params_.census_rad > 3) params_.census_rad = 3;
  int census_diam = params_.census_rad * 2 + 1;

  // Pre-build a census lookup table
  unsigned char lut[256];
  baml_generate_bit_set_lut(lut);
  bool only_32_bits = (census_diam <= 5);


  // Compute foreground likelihood assuming uniform distribution on foreground
  foreground_dist = 1.0 / (census_diam*census_diam);

  // Compute both census images
  vil_image_view<vxl_uint_64> census_tar, census_ref;
  vil_image_view<vxl_uint_64> salience_tar, salience_ref;
  baml_compute_census_img(
    img_tar, census_diam, census_tar, salience_tar, params_.census_tol);
  baml_compute_census_img(
    img_ref, census_diam, census_ref, salience_ref, params_.census_tol);

  // create score image
  vil_image_view<float> score;
  score.set_size(width, height);
  score.fill(0.0);

  // Compute hamming distance between images and create score image
  score.set_size(width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid_ref(x, y) == false) continue;

      unsigned long long int cen_diff = baml_compute_diff_string(
        census_tar(x, y), census_ref(x, y),
        salience_tar(x, y), salience_ref(x, y));

      score(x,y) = (float)baml_compute_hamming_lut(cen_diff, lut, only_32_bits);
    }
  }

  //covert to probability
  float sigma = baml_sigma(score);
  baml_gaussian(score, probability, sigma);
  return true;
}

//---------------DIFFERENCE------------------------------------------------
bool baml_change_detection::detect_difference(const vil_image_view<vxl_uint_16>& img_tar,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid_ref,
  vil_image_view<float>& probability,
  float& foreground_dist)
{
  int width = img_tar.ni(), height = img_tar.nj();

  if (img_ref.ni() != width || img_ref.nj() != height ||
    valid_ref.ni() != width || valid_ref.nj() != height)
    return false;

  // Compute min and max observed intensities in target image fore foreground distribution
  vxl_uint_16 min_int = (vxl_uint_16)(pow(2, 16) - 1);
  vxl_uint_16 max_int = (vxl_uint_16)0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid_ref(x, y) == false) continue;
      min_int = std::min(min_int, img_tar(x, y));
      max_int = std::max(max_int, img_tar(x, y));
    }
  }

  // Compute foreground likelihood assuming uniform distribution on foreground
  foreground_dist = 1.0 / (float) (max_int - min_int);

  // Calculate scores
  probability.set_size(width, height);
  probability.fill(0);

  // create score image
  vil_image_view<float> score;
  score.set_size(width, height);
  score.fill(0.0);

  // Calculate scores
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (img_ref(x, y) > img_tar(x, y))
        score(x, y) = (float) img_ref(x, y) - (float) img_tar(x, y);
      else
        score(x, y) = (float) img_tar(x, y) - (float) img_ref(x, y);
    }
  }

  //covert to probability
  float sigma = baml_sigma(score);
  baml_gaussian(score, probability, sigma);

  return true;
}

//-------------GRADIENT--------------------------------------------------
bool
baml_change_detection::detect_gradient(
  const vil_image_view<vxl_uint_16>& img_tar,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid_ref,
  vil_image_view<float>& probability,
  float& foreground_dist)
{
  float mag_tol = 0.00001f;
  int width = img_tar.ni(), height = img_tar.nj();

  if (img_ref.ni() != width || img_ref.nj() != height ||
    valid_ref.ni() != width || valid_ref.nj() != height)
    return false;

  // Compute gradient images
  vil_image_view<float> grad_x_tar, grad_y_tar, grad_x_ref, grad_y_ref;
  vil_sobel_3x3<vxl_uint_16, float>(img_tar, grad_x_tar, grad_y_tar);
  vil_sobel_3x3<vxl_uint_16, float>(img_ref, grad_x_ref, grad_y_ref);

  // Compute min and max observed intensities in target image
  vxl_uint_16 min_int = (vxl_uint_16)(pow(2, 16) - 1);
  vxl_uint_16 max_int = (vxl_uint_16)0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid_ref(x, y) == false) continue;
      min_int = std::min(min_int, img_tar(x, y));
      max_int = std::max(max_int, img_tar(x, y));
    }
  }

  // Compute foreground likelihood assuming uniform distribution on foreground
  foreground_dist = 1.0 / (max_int - min_int);

  // create score image
  vil_image_view<float> score;
  score.set_size(width, height);
  score.fill(0.0);
  // Compute distance between images
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid_ref(x, y) == false) continue;

      float grad_mag_tar = sqrt(
        grad_x_tar(x, y)*grad_x_tar(x, y) + grad_y_tar(x, y)*grad_y_tar(x, y));
      float grad_mag_ref = sqrt(
        grad_x_ref(x, y)*grad_x_ref(x, y) + grad_y_ref(x, y)*grad_y_ref(x, y));
      float grad_ip = grad_x_tar(x, y)*grad_x_ref(x, y) + grad_y_tar(x, y)*grad_y_ref(x, y);
      // if the magnitudes are large enough compute their angle 
      // difference, otherwise leave at default 90 degrees
      float angle_diff = 3.14159 / 2.0;
      if (grad_mag_tar > mag_tol && grad_mag_ref > mag_tol)
        angle_diff = acos(grad_ip / (grad_mag_tar*grad_mag_ref));

      //score(x,y) = pow( grad_mag_tar - grad_mag_ref, 2 );

      score(x,y) = pow( grad_x_tar(x,y)-grad_x_ref(x,y), 2 ) + pow( grad_y_tar(x,y)-grad_y_ref(x,y), 2 );

      //score(x, y) = grad_mag_tar*grad_mag_ref - grad_ip;

      //score(x,y) = pow( std::max( grad_mag_tar, grad_mag_ref )*sin(angle_diff), 2 );
      //score(x,y) = pow( 0.5f*( grad_mag_tar+grad_mag_ref )*fabs( sin(angle_diff) ), 2 );
    }
  }
  ////tar_lh.deep_copy(score);
  //// Covert score into probability
  //float sigma = baml_sigma(score);
  //vil_image_view<float> prob;
  //baml_gaussian(score, prob, sigma);

  //// Get parameters for background Gaussian distribution
  //float gauss_var = sigma*sigma;
  //float gauss_norm = log(1.0f / (sigma*sqrt(2 * 3.14159f)));

  //// Compute foreground likelihood assuming uniform distribution on foreground
  //float lfg = log(1.0f / (4 * sigma));
  //// Convert probability to log likelihood ratio
  //for (int y = 0; y < height; y++) {
  //  for (int x = 0; x < width; x++) {
  //    if (valid_ref(x, y) == false) continue;
  //    // Convert to likelihood ratio
  //    float lbg = gauss_norm - score(x, y) / gauss_var;
  //    tar_lh(x, y) = lfg - lbg;
  //  }
  //}

  //covert to probability
  float sigma = baml_sigma(score);
  baml_gaussian(score, probability, sigma);

  return true;
}

//-------------NON-PARAMETRIC-----------------------------------------------------------
bool
baml_change_detection::detect_nonparam(
  const vil_image_view<vxl_uint_16>& img_tar,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid_ref,
  vil_image_view<float>& probability,
  float& foreground_dist)
{
  // Hardcoded params
  float gauss_rad_percent = 0.01f;
  double double_tol = 0.000000001;

  int img_bit_ds = pow(2, params_.img_bit_depth - params_.hist_bit_depth);

  int hist_range = pow(2, params_.hist_bit_depth);
  int gauss_rad = (int)(gauss_rad_percent*hist_range);
  double gauss_sd = gauss_rad / 3.0;

  int width = img_tar.ni(), height = img_tar.nj();

  if (img_ref.ni() != width || img_ref.nj() != height ||
    valid_ref.ni() != width || valid_ref.nj() != height)
    return false;

  // Initialize score image
  probability.set_size(width, height);
  probability.fill(0.0f);

  int min_bin = hist_range, max_bin = 0;

  // Setup a transfer function array
  vil_image_view<double> tf_raw((int)hist_range, (int)hist_range);
  tf_raw.fill(0.0);

  // Populate the transfer function
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (!valid_ref(x, y)) continue;

      int tx = (int)(img_tar(x, y) / img_bit_ds);
      int ty = (int)(img_ref(x, y) / img_bit_ds);
      if (tx >= hist_range || ty >= hist_range) {
        std::cerr << "ERROR: baml_detect_change_nonparam, observed intensity "
          << img_tar(x, y) << ' ' << img_ref(x, y) << " larger than expected bit range, aborting\n";
        return false;
      }
      tf_raw(tx, ty) += 1.0;

      // Record min and max bin
      min_bin = std::min(min_bin, tx);
      max_bin = std::max(max_bin, tx);
    }
  }

  // Pre-compute probability of foreground
  foreground_dist = 1.0 / (max_bin - min_bin);

  // Blur the transfer array to account for sampling issues
  vil_image_view<double> tf_blur;
  vil_gauss_filter_2d(tf_raw, tf_blur, gauss_sd, gauss_rad);

  vil_image_view<float> bg(hist_range, hist_range);
  bg.fill(0.0f);

  // Normalize transfer function to compute probability of background
  for (int r = 1; r < hist_range; r++) {

    double tar_sum = 0.0;
    for (int t = 1; t < hist_range; t++)
      tar_sum = tar_sum + tf_blur(t, r);

    if (tar_sum < double_tol) continue;

    for (int t = 1; t < hist_range; t++) {
      if (tf_blur(t, r) < double_tol) bg(t, r) = double_tol;
      else bg(t, r) = tf_blur(t, r) / tar_sum;
    }
  }

  // One more pass to look up appearance prob
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (!valid_ref(x, y)) continue;
      int tx = (int)(img_tar(x, y) / img_bit_ds);
      int ty = (int)(img_ref(x, y) / img_bit_ds);
      probability(x, y) = bg(tx, ty);
    } //x
  } //y
  
  return true;
}

//--------------HISTOGRAM COMPARE-------------------------------------------------
bool
baml_change_detection::detect_histcmp(
  const vil_image_view<vxl_uint_16>& img_tar,
  const vil_image_view<vxl_uint_16>& img_ref,
  const vil_image_view<bool>& valid_ref,
  vil_image_view<float>& probability,
  float& foreground_dist)
{
  int width = img_tar.ni(), height = img_tar.nj();

  if (img_ref.ni() != width || img_ref.nj() != height ||
    valid_ref.ni() != width || valid_ref.nj() != height)
    return false;
  
  // create float target and reference images to be compared
  vil_image_view<float> target, ref;
  target.set_size(width, height);
  ref.set_size(width, height);
  if (params_.grad_mag_on) { // if using the gradient magnitude compute gradient images
    vil_image_view<float> grad_x_tar, grad_y_tar, grad_x_ref, grad_y_ref;
    vil_sobel_3x3<vxl_uint_16, float>(img_tar, grad_x_tar, grad_y_tar);
    vil_sobel_3x3<vxl_uint_16, float>(img_ref, grad_x_ref, grad_y_ref);
    vil_image_view<float> grad_mag_tar, grad_mag_ref;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        target(x, y) = sqrt(pow(grad_x_tar(x, y), 2) + pow(grad_y_tar(x, y), 2));
        ref(x, y) = sqrt(pow(grad_x_ref(x, y), 2) + pow(grad_y_tar(x, y), 2));
      }
    }
  }
  else { // otherwise make float versions of the input target and reference images
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        target(x, y) = (float)img_tar(x, y);
        ref(x, y) = (float)img_ref(x, y);
      }
    }
  }

  // Compute min and max observed intensities in either image and in just target image
  vxl_uint_16 min_int_tar = (vxl_uint_16)(pow(2, 16) - 1);
  vxl_uint_16 max_int_tar = (vxl_uint_16)0;
  float max_int = 0;
  float min_int = (pow(2, 16) - 1);
  // find min/max intensities
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (valid_ref(x, y) == false) continue;
      min_int = std::min(min_int, target(x, y));
      max_int = std::max(max_int, target(x, y));
      min_int = std::min(min_int, ref(x, y));
      max_int = std::max(max_int, ref(x, y));
      min_int_tar = std::min(min_int_tar, img_tar(x, y));
      max_int_tar = std::max(max_int_tar, img_tar(x, y));
    }
  }

  // Compute foreground likelihood assuming uniform distribution on foreground
  foreground_dist = 1.0f / (max_int_tar - min_int_tar);

  // calculate bin edges (params_.num_bins evenly spaced bins)
  std::vector<double> edges;
  double step_size = (double)(max_int - min_int) / params_.num_bins;
  for (int n = 0; n <= params_.num_bins; n++) {
    edges.push_back((double)min_int + n*step_size);
  }

  // find the bin number at each pixel
  bool found_bin_ref, found_bin_tar;
  vil_image_view<int> bin_tar;
  bin_tar.set_size(width, height);
  bin_tar.fill(0);
  vil_image_view<int> bin_ref;
  bin_ref.set_size(width, height);
  bin_ref.fill(0);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      found_bin_ref = false;
      found_bin_tar = false;
      for (int n = 0; n < params_.num_bins; n++) {
        if (ref(x, y) < edges[n + 1] && !found_bin_ref) {
          bin_ref(x, y) = n;
          found_bin_ref = true;
        }
        if (target(x, y) < edges[n + 1] && !found_bin_tar) {
          bin_tar(x, y) = n;
          found_bin_tar = true;
        }
        if (found_bin_ref && found_bin_tar) break;
      }
      if (!found_bin_ref) {
        bin_ref(x, y) = params_.num_bins - 1;
      }
      if (!found_bin_tar) {
        bin_tar(x, y) = params_.num_bins - 1;
      }
    }
  }

  // create the score image
  vil_image_view<float> score;
  score.set_size(width, height);
  score.fill(0.0f);
  std::vector<float> hist_ref(params_.num_bins);
  std::vector<float> hist_tar(params_.num_bins);
  float a, b, c;
  for (int y = (params_.neighborhood_size - 1) / 2; y < height - (params_.neighborhood_size - 1) / 2; y++) {   // loop over rows
    for (int x = (params_.neighborhood_size - 1) / 2; x < width - (params_.neighborhood_size - 1) / 2; x++) {   // loop over columns
        if ((x- (params_.neighborhood_size - 1) / 2) % 10 == 0) { // reset histogram every 10 columns to prevent floating point error from becoming too high
          for (int n = 0; n < params_.num_bins; n++) {
            hist_ref[n] = 0;
            hist_tar[n] = 0;
          }
          // fill in histogram by counting every pixel in a params_.neighborhood_size neighborhood
          for (int y1 = y - (params_.neighborhood_size - 1) / 2; y1 < y + (params_.neighborhood_size - 1) / 2 + 1; y1++) {
            for (int x1 = x - (params_.neighborhood_size - 1) / 2; x1 < x + (params_.neighborhood_size - 1) / 2 + 1; x1++) {
              build_hist(ref, bin_ref, edges, x1, y1, step_size, true, hist_ref);
              build_hist(target, bin_tar, edges, x1, y1, step_size, true, hist_tar);
            }
          }
        }
        else { // fill in histogram by subtracting the first column of the previous tile and adding the last column of the current tile
          for (int y1 = y - (params_.neighborhood_size - 1) / 2; y1 < y + (params_.neighborhood_size - 1) / 2 + 1; y1++) {
            build_hist(ref, bin_ref, edges, x + (params_.neighborhood_size - 1) / 2, y1, step_size, true, hist_ref); // add to reference
            build_hist(target, bin_tar, edges, x + (params_.neighborhood_size - 1) / 2, y1, step_size, true, hist_tar); // add to target
            build_hist(ref, bin_ref, edges, x - (params_.neighborhood_size - 1) / 2 - 1, y1, step_size, false, hist_ref); // subtract from reference
            build_hist(target, bin_tar, edges, x - (params_.neighborhood_size - 1) / 2 - 1, y1, step_size, false, hist_tar); // subtract from target
          }
        }
        // create histogram objects
        bsta_histogram<float> hist_t(min_int, max_int, hist_tar, 0);
        bsta_histogram<float> hist_r(min_int, max_int, hist_ref, 0);
        // select appropriate histogram comparison method
        if (strcmp((params_.hist_method).c_str(), "intersection") == 0) {
          score(x, y) = -log(hist_intersect(hist_r, hist_t));
        }
        else if (strcmp((params_.hist_method).c_str(), "jensen shannon") == 0) {
          score(x, y) = js_divergence(hist_r, hist_t);
        }
        else if (strcmp((params_.hist_method).c_str(), "bhattacharyya") == 0) {
          score(x, y) = bhatt_distance(hist_r, hist_t);
        }
        else {
          std::cerr << "histogram comparison technique not recognized\n";
          return false;
        }
        if (std::isinf(score(x, y))) {
          score(x, y) = 100;
        }
    }
  }
  //covert to probability
  float sigma = baml_sigma(score);
  baml_gaussian(score, probability, sigma);

  return true;
}

//--------------------------------HISTOGRAM COMPARE HELPER----------------------------------------------
bool
baml_change_detection::build_hist( // adds or removes pixel specified in by (x1, y1) to or from weighted histogram, hist
  const vil_image_view<float>& img, // image of interest
  const vil_image_view<int>& bin_img, // bin number of corresponding pixel in img
  const std::vector<double>& edges, // bin edges
  const int x1, // column position of pixel
  const int y1, // row postition of pixel
  const float step_size, // width of bins specified in edges
  const bool adding, // true if adding, false if subtracting
  std::vector <float>& hist) // histogram that is being altered
{
  float mid_point, dist;
  if (bin_img(x1, y1) == 0) { // special case of first bin
    mid_point = (edges[1] + edges[0]) / 2;
    dist = fabs(img(x1, y1) - mid_point);
    if (img(x1, y1) <= mid_point) { // fully in first bin
      if (adding) {
        hist[0] = hist[0] + 1;
      }
      else {
        hist[0] = hist[0] - 1;
      }
    }
    else { // split between first and second bin
      if (adding) {
        hist[0] = hist[0] + (1 - dist / step_size);
        hist[1] = hist[1] + dist / step_size;
      }
      else {
        hist[0] = hist[0] - (1 - dist / step_size);
        hist[1] = hist[1] - dist / step_size;
      }
    }
    // make sure there are no floating point precision errors
    if (hist[0] < 0) hist[0] = 0;
    if (hist[1] < 0) hist[1] = 0;
  }
  else if (bin_img(x1, y1) == (params_.num_bins - 1)) { // special case of last bin
    mid_point = (edges[params_.num_bins] + edges[params_.num_bins - 1]) / 2;
    dist = fabs(img(x1, y1) - mid_point);
    if (img(x1, y1) >= mid_point) { // fully in the last bin
      if (adding) {
        hist[params_.num_bins - 1] += 1;
      }
      else {
        hist[params_.num_bins - 1] -= 1;
      }
    }
    else { // split between last and second to last bin
      if (adding) {
        hist[params_.num_bins - 1] += (1 - dist / step_size);
        hist[params_.num_bins - 2] += dist / step_size;
      }
      else {
        hist[params_.num_bins - 1] -= (1 - dist / step_size);
        hist[params_.num_bins - 2] -= dist / step_size;
      }
    }
    // make sure there are no floating point precision errors
    if (hist[params_.num_bins - 1] < 0) hist[params_.num_bins - 1] = 0;
    if (hist[params_.num_bins - 2] < 0) hist[params_.num_bins - 2] = 0;
  }
  else { // not in either the first or last bin
    mid_point = (edges[bin_img(x1, y1)] + edges[bin_img(x1, y1) + 1]) / 2;
    dist = fabs(img(x1, y1) - mid_point);
    // always alter our own bin
    if (adding) {
      hist[bin_img(x1, y1)] += (1 - dist / step_size); // add to our bin
    }
    else {
      hist[bin_img(x1, y1)] -= (1 - dist / step_size); // subtract from our bin
    }
    // find which neighboring bin should be altered
    if ((img(x1, y1) - edges[bin_img(x1, y1)]) < step_size / 2) { // closer to bin_ref(x1,y1)-1
      if (adding) {
        hist[bin_img(x1, y1) - 1] += dist / step_size;
      }
      else {
        hist[bin_img(x1, y1) - 1] -= dist / step_size;
      }
    }
    else { // closer to bin_ref(x1,y1)+1
      if (adding) {
        hist[bin_img(x1, y1) + 1] += dist / step_size;
      }
      else {
        hist[bin_img(x1, y1) + 1] -= dist / step_size;
      }
    }
    // make sure there are no floating point precision errors
    if (hist[bin_img(x1, y1)] < 0) hist[bin_img(x1, y1)] = 0;
    if (hist[bin_img(x1, y1) - 1] < 0) hist[bin_img(x1, y1) - 1] = 0;
    if (hist[bin_img(x1, y1) + 1] < 0) hist[bin_img(x1, y1) + 1] = 0;
  }
  return true;
}

//=================MULTI IMAGE CHANGE DETECTION METHODS==============================
//---------------------------PRODUCT---------------------------------------------------
bool baml_change_detection::multi_product(
  const std::vector<vil_image_view<float> >& pw_probabilities,
  const std::vector<float>& foreground_dist,
  vil_image_view<float>& probability
) {
  int num_ref = pw_probabilities.size();
  vil_image_view<float> prob = pw_probabilities[0];
  int width = prob.ni();
  int height = prob.nj();

  // Initialize output image
  probability.set_size(width, height);
  probability.fill(0.0);
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      float prod = 1;
      for (int t = 0; t < num_ref; t++) {
        prob = pw_probabilities[t]; // get probability image at time t
        prod *= prob(x, y) / foreground_dist[t]*params_.pGoodness + (1 - params_.pGoodness);
      }
      probability(x, y) = 1 / (1 + (1 - params_.pChange) / params_.pChange*prod);
    }
  }
  return true;
}

//---------------------------SUM---------------------------------------------------
bool baml_change_detection::multi_sum(
  const std::vector<vil_image_view<float> >& pw_probabilities,
  const std::vector<float>& foreground_dist,
  vil_image_view<float>& probability
) {
  int num_ref = pw_probabilities.size();
  vil_image_view<float> s = pw_probabilities[0];
  int width = s.ni();
  int height = s.nj();

  // Initialize output image
  probability.set_size(width, height);
  probability.fill(0.0);
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      float sum = 0;
      for (int t = 0; t < num_ref; t++) {
        s = pw_probabilities[t]; // get probability image at time t
        sum += s(x, y) / foreground_dist[t]*(1 / (float)num_ref);
      }
      probability(x, y) = 1 / (1 + (1 - params_.pChange) / params_.pChange*sum);
    }
  }
  return true;
}

//---------------------------PROBABILITY MAXIMIZATION---------------------------------------
bool baml_change_detection::multi_max_prob(
  const std::vector<vil_image_view<float> >& pw_probabilities,
  const std::vector<float>& foreground_dist,
  vil_image_view<float>& probability
) {
  int num_ref = pw_probabilities.size();
  vil_image_view<float> prob = pw_probabilities[0];
  int width = prob.ni();
  int height = prob.nj();

  // Initialize output image
  probability.set_size(width, height);
  probability.fill(0.0);
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      float max = FLT_MIN;
      for (int t = 0; t < num_ref; t++) {
        prob = pw_probabilities[t]; // get score image at time t
        max = std::max(max, prob(x, y)/foreground_dist[t]);
      }
      probability(x, y) = 1 / (1 + (1 - params_.pChange) / params_.pChange*max);
    }
  }

  return true;
}
