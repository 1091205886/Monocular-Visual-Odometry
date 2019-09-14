
#ifndef MY_SLAM_FEATURE_MATCH_H
#define MY_SLAM_FEATURE_MATCH_H

#include "my_slam/common_include.h"

namespace my_slam
{
namespace geometry
{

void extractKeyPoints(cv::Mat &image, vector<cv::KeyPoint> &keypoints,
                      bool SET_PARAM_BY_YAML = true);

void computeDescriptors(cv::Mat &image, vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors,
                        bool SET_PARAM_BY_YAML = true);

void matchFeatures(
    const cv::Mat &descriptors_1, const cv::Mat &descriptors_2,
    vector<cv::DMatch> &matches,
    const bool PRINT_RES = false,
    const bool SET_PARAM_BY_YAML = true);

// Remove duplicate matches.
// After cv's match func, many kpts in I1 might matched to a same kpt in I2.
// Sorting the trainIdx(I2), and make the match unique.
void removeDuplicatedMatches(vector<cv::DMatch> &matches);

// Use a grid to remove the keypoints that are too close to each other.
void selectUniformByGrid(vector<cv::KeyPoint> &keypoints,
                         const int image_rows, const int image_cols,
                         const bool SET_PARAM_BY_YAML = true);

// --------------------- Other assistant functions ---------------------
double computeMeanDistBetweenkeypoints(
    const vector<cv::KeyPoint> &kpts1, const vector<cv::KeyPoint> &kpts2, const vector<cv::DMatch> &matches);

// --------------------- datatype transform ---------------------
vector<cv::DMatch> inliers2DMatches(const vector<int> inliers);
vector<cv::KeyPoint> pts2keypts(const vector<cv::Point2f> pts);

} // namespace geometry
} // namespace my_slam

#endif
