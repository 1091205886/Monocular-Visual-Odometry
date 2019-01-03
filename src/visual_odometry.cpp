
#include <boost/timer.hpp>

#include "my_slam/visual_odometry.h"
#include "my_common/config.h"

namespace my_slam
{

VisualOdometry::VisualOdometry() : state_(INITIALIZING),
    // map_(new Map),
    ref_(nullptr), curr_(nullptr)
{

    // max_num_lost_       = Config::get<float> ( "max_num_lost" );
    // min_inliers_        = Config::get<int> ( "min_inliers" );
    // key_frame_min_rot   = Config::get<double> ( "keyframe_rotation" );
    // key_frame_min_trans = Config::get<double> ( "keyframe_translation" );
    // map_point_erase_ratio_ = Config::get<double> ( "map_point_erase_ratio" );

    // Set for matching descriptors
    // num_lost_(0), num_inliers_(0)

}

bool VisualOdometry::addFrame(Frame::Ptr frame) // This is the "main" function
{
    static Buff_FramesWithFeatures buff_frames;
    const int BUFFSIZE = my::Config::get<int>("buffsize_of_prev_frames");

    boost::timer timer;
    bool resAddFrameSuccess = true;

    // Compute features in current frame.
    curr_ = frame;
    extractKeyPoints();
    computeDescriptors();
    vector<cv::DMatch> matches;

    switch (state_)
    {
    case INITIALIZING:
    {
        if (buff_frames.size() < BUFFSIZE)
        {
            // do nothing
        }
        else
        {
            // matching features between "buff_frames[0]" and "curr_"
            cv::Mat desciptors_prev=buff_frames.buff_descriptors_[0];
            my_geometry::matchFeatures(desciptors_prev, descriptors_curr_, matches);

            // Estimate camera motion between two frames
            // Mat R,t;
            // pose_estimation_2d2d ( keypoints_1, keypoints_2, matches, R, t );
        }

        ref_ = frame;
        // extract features from first frame and add them into map

        break;
    }
    }

    // Print result
    cout << endl;
    cout << "Add frame " << frame->id_ << endl;
    cout << "time cost = " << timer.elapsed() * 1000 << "ms" << endl;
    cout << "number of keypoints = " << keypoints_curr_.size() << endl;
    cout << "rows of descriptors_curr_ = " << descriptors_curr_.rows << endl;
    cout << "number of matched keypoints = " << matches.size() << endl;

    // Manage the buff
    buff_frames.push_back(curr_, keypoints_curr_, descriptors_curr_);
    if (buff_frames.size() > BUFFSIZE)
    {
        buff_frames.pop_front();
    }

    // Return
    return resAddFrameSuccess;
}


// void VisualOdometry::matchFeatures_withMapPoints()
// {
//     //  Input:  map_->map_points_
//     //          curr_, descriptors_curr_
//     //  Output:
//     //          matched_3d_MapPoints_
//     //          matched_2d_keypts_index_

//     // Find the MapPoints that fall into the current frame
//     Mat candidates_descriptors;
//     vector<MapPoint::Ptr> candidates_MapPoints;
//     for (auto &iter_map_point : map_->map_points_)
//     {
//         MapPoint::Ptr &p = iter_map_point.second;
//         // check if p in curr frame image
//         if (curr_->isInFrame(p->pos_))
//         {
//             // add to candidates_MapPoints
//             p->visible_times_++;
//             candidates_MapPoints.push_back(p);
//             candidates_descriptors.push_back(p->descriptor_);
//         }
//     }

//     // Match keypoints' descriptors
//     vector<cv::DMatch> matches;
//     my_geometry::matchFeatures(candidates_descriptors, descriptors_curr_, matches);

//     // Push points to result
//     matched_3d_MapPoints_.clear();
//     matched_2d_keypts_index_.clear();
//     for (cv::DMatch &m : matches)
//     {
//         matched_3d_MapPoints_.push_back(candidates_MapPoints[m.queryIdx]); // map point in map
//         matched_2d_keypts_index_.push_back(m.trainIdx);                    // key point in current frame
//     }
//     // cout<<"good matches: "<<matched_3d_MapPoints_.size() <<endl;
// }

} // namespace my_slam