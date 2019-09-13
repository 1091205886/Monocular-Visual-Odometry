
#ifndef MAP_H
#define MAP_H

#include "my_slam/vo/common_include.h"
#include "my_slam/vo/frame.h"
#include "my_slam/vo/mappoint.h"

namespace vo
{
using namespace std; 
using namespace cv;

class Map
{
public:
    typedef shared_ptr<Map> Ptr;
    unordered_map<int, Frame::Ptr > keyframes_; 
    unordered_map<int, MapPoint::Ptr >  map_points_; 

    Map() {}
    
    void insertKeyFrame( Frame::Ptr frame );
    void insertMapPoint( MapPoint::Ptr map_point );
    Frame::Ptr findKeyFrame(int frame_id);
    bool checkKeyFrame(int frame_id);
    
};
}

#endif // MAP_H
