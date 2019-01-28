// A class "PclViewer" for displaying 3D points and camera using PCL library.

/*
The reason I made this class:
PCL compiles really really slow. 
If I include pcl libraries in my "main.cpp", it will be tooooo slow to compile and debug.
So I encapsulate the essential functions, and include pcl libraries only in "pcl_display.cpp".
In this way, there is no compiling but only linking to pcl when compiling "main.cpp".
Instead of 15+ seconds, the linking only takes like a 3 seconds.
*/


#ifndef pcl_display_H
#define pcl_display_H

#include <memory>
#include <string>
#include <opencv2/core/core.hpp>

using namespace std;
typedef unsigned char uchar;

namespace my_display
{
class PclViewer
{

  public:
    // ----------------- variables -----------------
    typedef shared_ptr<PclViewer> Ptr;

    string viewer_name_;
    string camera_frame_name_;

    cv::Mat cam_R_vec_;
    cv::Mat cam_t_;

  public:
    // ----------------- constructor -----------------
    PclViewer( 
      double x = 1.0, double y = -1.0, double z = -1.0, 
      double rotaxis_x = -0.5, double rotaxis_y = 0, double rotaxis_z = 0
    );

  public:
    void updateMapPoints(const vector<cv::Point3f> &vec_pos, const vector<vector<unsigned char>> &vec_color);
    void updateCurrPoints(const vector<cv::Point3f> &vec_pos, const vector<vector<unsigned char>> &vec_color);
    void updatePointsInView(const vector<cv::Point3f> &vec_pos, const vector<vector<unsigned char>> &vec_color);

    void updateCameraPose(const cv::Mat &R_vec, const cv::Mat &t);
    void update();
    void spinOnce(unsigned int millisecond);
    bool wasStopped();
};
}

#endif