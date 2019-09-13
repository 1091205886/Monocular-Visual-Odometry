
#ifndef MAPPOINT_H
#define MAPPOINT_H

#include "my_slam/vo/common_include.h"
#include "my_slam/vo/frame.h"

namespace vo
{
using namespace std; 
using namespace cv;

// class Frame;
class MapPoint
{
public: // Basics Properties
    typedef shared_ptr<MapPoint> Ptr;

    static int factory_id_; 
    int id_;
    Point3f pos_;
    Mat norm_; // Vector pointing from camera center to the point
    vector<unsigned char> color_; // r,g,b
    Mat descriptor_; // Descriptor for matching 

public: // Properties for constructing local mapping

    bool        good_;      // TODO: determine wheter a good point 
    int         matched_times_;     // being an inliner in pose estimation
    int         visible_times_;     // being visible in current frame 
    
public: // Functions
    MapPoint (const Point3f& pos,  const Mat& descriptor, const Mat& norm,
        unsigned char r=0, unsigned char g=0, unsigned char b=0);
    // void resetPos(Point3f pos);

};
}

#endif // MAPPOINT_H
