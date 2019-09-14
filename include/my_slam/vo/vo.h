

#ifndef MY_SLAM_VO_H
#define VO_H

// cv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>

// my

#include "my_slam/basics/basics.h"
#include "my_slam/basics/io.h"
#include "my_slam/basics/config.h"
#include "my_slam/basics/opencv_funcs.h"

#include "my_slam/geometry/camera.h"
#include "my_slam/geometry/feature_match.h"
#include "my_slam/geometry/motion_estimation.h"

#include "my_slam/common_include.h"
#include "my_slam/vo/frame.h"
#include "my_slam/vo/map.h"
#include "my_slam/vo/mappoint.h"
#include "my_slam/vo/vo_commons.h"

namespace my_slam
{
namespace vo
{
using namespace std;
using namespace cv;
using namespace geometry;

class VisualOdometry
{

public: // ------------------------------- Member variables -------------------------------
  typedef std::shared_ptr<VisualOdometry> Ptr;

  enum VOState
  {
    BLANK,
    INITIALIZATION,
    OK,
    LOST
  };
  VOState vo_state_;

  // Frame
  Frame::Ptr curr_;
  Frame::Ptr ref_;
  Frame::Ptr newest_frame_; // temporarily store the newest frame
  cv::Mat prev_T_w_c_;          // pos of previous frame
  deque<Frame::Ptr> frames_buff_;

  // Map
  Map::Ptr map_;

  // Map features
  vector<cv::KeyPoint> keypoints_curr_;
  cv::Mat descriptors_curr_;

  vector<cv::Point3f> matched_pts_3d_in_map_;
  vector<int> matched_pts_2d_idx_;

  // ================================ Functions ================================
public: // basics
  VisualOdometry();
  void addFrame(vo::Frame::Ptr frame);
  void pushFrameToBuff(Frame::Ptr frame)
  {
    const int BUFF_SIZE = 20;
    frames_buff_.push_back(frame);
    if (frames_buff_.size() > BUFF_SIZE)
      frames_buff_.pop_front();
  }

public: // ------------------------------- Initialization -------------------------------
  void estimateMotionAnd3DPoints();
  bool checkIfVoGoodToInit();
  bool isInitialized();

public: // ------------------------------- Triangulation -------------------------------
  // 1. motion_estimation.h: helperTriangulatePoints
  // 2. Some triangulated points have a small viewing angle between two frames.
  //    Remove these points: change "pts3d_in_curr", return a new "inlier_matches"
  void retainGoodTriangulationResult();

public: // ------------------------------- Tracking -------------------------------
  // void find3Dto2DCorrespondences()
  bool checkLargeMoveForAddKeyFrame(Frame::Ptr curr, Frame::Ptr ref);
  void optimizeMap();
  void poseEstimationPnP();

public: // ------------------------------- Mapping -------------------------------
  void addKeyFrame(Frame::Ptr frame);
  void getMappointsInCurrentView(
      vector<MapPoint::Ptr> &candidate_mappoints_in_map,
      cv::Mat &corresponding_mappoints_descriptors);
  void pushCurrPointsToMap();
  double getViewAngle(Frame::Ptr frame, MapPoint::Ptr point);

public: // ------------------------------- BundleAdjustment -------------------------------
  void callBundleAdjustment();
};

} // namespace vo
} // namespace my_slam
#endif // FRAME_H
