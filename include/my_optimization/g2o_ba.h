
#ifndef G2O_BA_H
#define G2O_BA_H

#include "my_basics/opencv_funcs.h"

#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>


namespace my_optimization
{
using namespace std;
using namespace cv;

void bundleAdjustment(
    const vector<Point3f> points_3d,
    const vector<Point2f> points_2d,
    const Mat &K,
    Mat &T_world_to_cam_cv);
} // namespace my_optimization
#endif