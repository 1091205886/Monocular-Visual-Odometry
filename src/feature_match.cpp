
#include "mygeometry/feature_match.h"
#include "myslam/config.h"

namespace mygeometry
{

// set default ORB params
#define _num_keypoints 5000
#define _scale_factor 1.2
#define _level_pyramid 4

// set default Grid params for non-maximum suppression
#define _max_num_keypoints 1000
#define _grid_size 8
#define _max_pts_in_grid 2 /*x=8, y=2, (640/x)*(480/x)*y=4800*2=9600*/

// set default feature matching params
#define _match_ratio 2.0

void extractKeyPoints(cv::Mat &image, vector<cv::KeyPoint> &keypoints,
    const bool SET_PARAM_BY_YAML)
{
    static int num_keypoints=_num_keypoints, level_pyramid=_level_pyramid;
    static double scale_factor=_scale_factor;
    static bool FIRST_IN=true;
   if (FIRST_IN){
        FIRST_IN=false;
        if(SET_PARAM_BY_YAML){
            num_keypoints = myslam::Config::get<int>("number_of_keypoints_to_extract");
            scale_factor = myslam::Config::get<double>("scale_factor");
            level_pyramid = myslam::Config::get<int>("level_pyramid");
        }
    }
    static cv::Ptr<cv::ORB> orb = cv::ORB::create(num_keypoints, scale_factor, level_pyramid);

    // compute
    orb->detect(image, keypoints);
    _remove_tooclose_keypoints_by_grid(keypoints, image.rows, image.cols, SET_PARAM_BY_YAML);
}

void computeDescriptors(cv::Mat &image, vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors,
    const bool SET_PARAM_BY_YAML)
{
    static int num_keypoints=_num_keypoints, level_pyramid=_level_pyramid;
    static double scale_factor=_scale_factor;
    static bool FIRST_IN=true;
    if (FIRST_IN){
        FIRST_IN=false;
        if(SET_PARAM_BY_YAML){
            num_keypoints = myslam::Config::get<int>("number_of_keypoints_to_extract");
            scale_factor = myslam::Config::get<double>("scale_factor");
            level_pyramid = myslam::Config::get<int>("level_pyramid");
        }
    }
    static cv::Ptr<cv::ORB> orb = cv::ORB::create(num_keypoints, scale_factor, level_pyramid);

    // compute
    orb->compute(image, keypoints, descriptors);
}


void _remove_tooclose_keypoints_by_grid(vector<cv::KeyPoint>& keypoints,
    const int image_rows, const int image_cols,
    const bool SET_PARAM_BY_YAML)
{
    static int GRID_SIZE=_grid_size, MAX_PTS_IN_GRID=_max_pts_in_grid;
    static int MAX_NUM_KEYPOINTS = _max_num_keypoints;
    static bool FIRST_IN=true;
    if(FIRST_IN){
        FIRST_IN=false;
        if(SET_PARAM_BY_YAML){
            MAX_NUM_KEYPOINTS = myslam::Config::get<int>("max_number_of_keypoints");
        }
    }
    static vector<vector<int>> grid(image_rows/GRID_SIZE,
        vector<int>(image_cols/GRID_SIZE,0));

    // clear grid
    for (auto row: grid) //clear grid
        std::fill(row.begin(),row.end(),0);
    
    // Insert keypoints to grid. If not full, insert this keypoint to result
    vector<cv::KeyPoint> tmp_keypoints;
    int cnt=0;
    for (auto &kpt: keypoints){
        int row=((int)kpt.pt.y)/GRID_SIZE, col=((int)kpt.pt.x)/GRID_SIZE;
        if (++grid[row][col]<=MAX_PTS_IN_GRID){
            tmp_keypoints.push_back(kpt);
            cnt++;
            if (cnt>MAX_NUM_KEYPOINTS)break;
        }
    }

    // return
    keypoints=tmp_keypoints;
}



void matchFeatures(
    const cv::Mat &descriptors_1, const cv::Mat &descriptors_2,
    vector<cv::DMatch> &matches,
    const bool PRINT_RES, const bool SET_PARAM_BY_YAML)
{
    static cv::FlannBasedMatcher matcher_flann(new cv::flann::LshIndexParams(5, 10, 2));
    static double match_ratio = _match_ratio;
    static bool FIRST_IN=true;
    if(FIRST_IN){
        FIRST_IN=false;
        if(SET_PARAM_BY_YAML){
            match_ratio = myslam::Config::get<int>("match_ratio");
        }
    }

    // Match keypoints with similar descriptors.
    // For kpt_i, if kpt_j's descriptor if most similar to kpt_i's, then they are matched.
    vector<cv::DMatch> all_matches;
    matcher_flann.match(descriptors_1, descriptors_2, all_matches);

    // Find a min-distance threshold for selecting good matches
    double min_dis = 9999999, max_dis = 0;
    for (int i = 0; i < all_matches.size(); i++){
        double dist = all_matches[i].distance;
        if (dist < min_dis) min_dis = dist;
        if (dist > max_dis) max_dis = dist;
    }
    double distance_threshold = max<float>(min_dis * match_ratio, 30.0);
    // Another way of getting the minimum:
    // min_dis = std::min_element(all_matches.begin(), all_matches.end(),
    //     [](const cv::DMatch &m1, const cv::DMatch &m2) {return m1.distance < m2.distance;})->distance;

    // Select good matches and push to the result vector.
    matches.clear();
    for (cv::DMatch &m : all_matches)
        if (m.distance < distance_threshold)
            matches.push_back(m);

    // Sort res by "trainIdx", and then
    // remove duplicated "trainIdx" to obtain unique matches.
    _remove_duplicate_matches(matches);

    if (PRINT_RES){
        printf("Matching features: threshold = %f\n",distance_threshold);
        printf("Number of matches: %d\n", (int)matches.size());
        printf("-- Max dist : %f \n", max_dis);
        printf("-- Min dist : %f \n", min_dis);
    }
}

void _remove_duplicate_matches(vector<cv::DMatch> &matches)
{
    // Sort res by "trainIdx".
    sort(matches.begin(), matches.end(),
         [](const cv::DMatch &m1, const cv::DMatch &m2) {
             return m1.trainIdx < m2.trainIdx;
         });
    // Remove duplicated "trainIdx", so that the matches will be unique.
    vector<cv::DMatch> res;
    if (!matches.empty()) res.push_back(matches[0]);
    for (int i = 1; i < matches.size(); i++)
    {
        if (matches[i].trainIdx != matches[i - 1].trainIdx)
        {
            res.push_back(matches[i]);
        }
    }
    res.swap(matches);
}

} // namespace mygeometry