

// std
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <iomanip>

// cv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>

// my
#include "my_basics/io.h"
#include "my_basics/config.h"
// #include "my_geometry/feature_match.h"
// #include "my_geometry/epipolar_geometry.h"
#include "my_geometry/motion_estimation.h"
#include "my_slam/frame.h"

// display
#include "my_display/pcl_viewer.h"
using namespace my_display;
const bool ENABLE_PCL_DISPLAY=true;

using namespace std;
using namespace cv;
using namespace my_geometry;

bool checkInputArguments(int argc, char **argv);
int main(int argc, char **argv)
{
    // Read in image filenames and camera prameters.
    assert(checkInputArguments(argc, argv));
    const string config_file = argv[1];
    bool print_res = false;
    vector<string> image_paths = my_basics::readImagePaths(config_file, 150, print_res);
    cv::Mat K = my_basics::readCameraIntrinsics(config_file); // camera intrinsics
    my_geometry::Camera::Ptr camera(                          // init a camera class with common transformations
        new my_geometry::Camera(K));
    my_basics::Config::setParameterFile( // just to remind to set this config file.
        config_file);                    // Following algorithms will read from it for setting params.

    // Prepare pcl display
    double dis_scale=3;
    double  x = 0.5*dis_scale,
            y = -1.0*dis_scale,
            z = -1.0*dis_scale;
    double ea_x = -0.5, ea_y = 0, ea_z = 0;
    string viewer_name = "my pcl viewer";
    my_display::PclViewer::Ptr pcl_displayer(
        new my_display::PclViewer(
            viewer_name, x, y, z, ea_x, ea_y, ea_z));


    // Read in image
    deque<my_slam::Frame::Ptr> frames;
    enum VO_STATE
    {
        INITIALIZATION,
        OK,
        LOST
    };
    VO_STATE vo_state = INITIALIZATION;
    // double mean_depth = 0; // This is the scale factor. Will be estiamted in 1st frame.
    for (int img_id = 0; img_id < (int)image_paths.size(); img_id++)
    {

        // Read image.
        cv::Mat rgb_img = cv::imread(image_paths[img_id]);
        if (rgb_img.data == nullptr){
            cout << "The image file <<"<<image_paths[img_id]<<"<< is empty. Finished."<<endl;
            break;
        }
        printf("\n\n=============================================\n");
        printf("=============================================\n");
        printf("=============================================\n");
        printf("Start processing the %dth image.\n", img_id);

        // Init a frame. Extract keypoints and descriptors.
        my_slam::Frame::Ptr frame = my_slam::Frame::createFrame(rgb_img, camera);
        frame->extractKeyPoints();
        frame->computeDescriptors();
        cout << "Number of keypoints: "<<frame->keypoints_.size()<<endl;

        // Add a new frame into vo.
        vector<KeyPoint> keypoints;
        vector<DMatch> matches;
        Mat descriptors;
        if (img_id == 0)
        {
            Mat T_w_to_c_0 = Mat::eye(4, 4, CV_64F);
            frame->T_w_c_ = T_w_to_c_0;
            vo_state = INITIALIZATION;
        }
        else
        {
            my_slam::Frame::Ptr prev_frame = frames.back();
            frame->matchFeatures(prev_frame);

            printf("\nInerst the image %d:\n", img_id);
            if (vo_state == INITIALIZATION)
            { // initiliazation stage

                // -- Estimation motion
                vector<Mat> list_R, list_t, list_normal;
                vector<vector<DMatch>> list_matches; // these are the inliers matches
                vector<vector<Point3f>> sols_pts3d_in_cam1_by_triang;
                helperEstimatePossibleRelativePosesByEpipolarGeometry(
                    /*Input*/
                    prev_frame->keypoints_, frame->keypoints_,
                    frame->matches_,
                    K,
                    /*Output*/
                    list_R, list_t, list_matches, list_normal, sols_pts3d_in_cam1_by_triang,
                    false); // print result
                
                // -- Compute [epipolar error] and [trigulation error on norm plane] for the 3 solutions (E, H1, H2)
                vector<double> list_error_epipolar;
                vector<double> list_error_triangulation;
                helperEvaluateEstimationsError(
                    prev_frame->keypoints_, frame->keypoints_,list_matches,
                    sols_pts3d_in_cam1_by_triang, list_R, list_t, list_normal, K,
                    /*output*/
                    list_error_epipolar, list_error_triangulation,
                    false); // print result

                // -- Choose 1 solution from the 3 solutions.
                // Currently, I'll just simply choose the result from essential matrix.
                //      Need to read papers such as ORB-SLAM2.
                const int sol_idx = 0;
                Mat R = list_R[sol_idx], t = list_t[sol_idx];
                vector<Point3f> pts3d = sols_pts3d_in_cam1_by_triang[sol_idx];
                const int num_inlier_pts = pts3d.size();

                // -- Normalize the mean depth of points to be 1m
                double mean_depth = 0;
                for (const Point3f &p : pts3d)
                    mean_depth += p.z;
                mean_depth /= num_inlier_pts;
                mean_depth = mean_depth*100;
                t /= mean_depth;
                for (Point3f &p : pts3d)
                {
                    p.x /= mean_depth;
                    p.y /= mean_depth;
                    p.z /= mean_depth;
                }
                
                // -- Update current camera pos
                Mat T_curr_to_prev = transRt2T(R, t);
                frame->T_w_c_ = prev_frame->T_w_c_ * T_curr_to_prev.inv();
                frame->R_curr_to_prev_ = R;
                frame->t_curr_to_prev_ = t;
                // --Update other values
                // frame->inliers_matches_ = list_matches[sol_idx]; // I don't think this is needed
                vo_state = OK;
            }
            else if (vo_state == OK)
            {
                // Now, we have the pose of prev two images and their keypoints matches.

                // Only the 2nd image has computed the inliers,
                //      so for all other images, we still need to find the inliers again.)

                const int frames_size = frames.size();
                my_slam::Frame::Ptr frame1 = frames[frames_size - 2];
                my_slam::Frame::Ptr frame2 = frames[frames_size - 1];

                if(1){  // use helperEstimatePossibleRelativePosesByEpipolarGeometry (slow)
                    vector<Mat> list_R, list_t, list_normal;
                    vector<vector<DMatch>> list_matches; // these are the inliers matches
                    vector<vector<Point3f>> sols_pts3d_in_cam1_by_triang;
                    helperEstimatePossibleRelativePosesByEpipolarGeometry(
                        /*Input*/
                        frame1->keypoints_, frame2->keypoints_,
                        frame2->matches_,
                        K,
                        /*Output*/
                        list_R, list_t, list_matches, list_normal, sols_pts3d_in_cam1_by_triang,
                    false); // print result

                    printf("DEBUG: Printing prev frame1-frame2 inliers number by E/H:\n");
                    for(int i=0;i<list_matches.size();i++){
                        cout<<list_matches[i].size()<<", ";
                    } // RESULT: 只有10几个内点？
                    cout << endl;

                    // -- update values
                    int sol_idx_for_inlier=0;
                    vector<int> inliers_in_kpts2; // inliers index with respect to all the points
                    for (const DMatch &m : list_matches[sol_idx_for_inlier])
                        inliers_in_kpts2.push_back(m.trainIdx);
                    frame2->inliers_of_all_pts_ = inliers_in_kpts2;

                    vector<Point3f> &pts3d_in_cam1 = sols_pts3d_in_cam1_by_triang[sol_idx_for_inlier];
                    vector<Point3f> pts3d_in_cam2;
                    for (const Point3f &pt3d : pts3d_in_cam1)
                        pts3d_in_cam2.push_back(transCoord(pt3d, frame2->R_curr_to_prev_, frame2->t_curr_to_prev_));
                    frame2->inliers_pts3d_ = pts3d_in_cam2;

                }else if(1){  // get inliers of prev two frames, and do triangulation
   
                    // -- Extract some commonly used points
                    vector<Point2f> pts_img1, pts_img2; // matched points
                    extractPtsFromMatches(frame1->keypoints_, frame2->keypoints_, frame2->matches_, pts_img1, pts_img2);
                    vector<Point2f> pts_on_np1, pts_on_np2; // matched points on camera normalized plane
                    for (const Point2f &pt : pts_img1)
                        pts_on_np1.push_back(pixel2camNormPlane(pt, K));
                    for (const Point2f &pt : pts_img2)
                        pts_on_np2.push_back(pixel2camNormPlane(pt, K));
                    Mat R = frame2->R_curr_to_prev_, t = frame2->t_curr_to_prev_;

                    
                    // -- Here cam1 is the prev2 image, cam2 is the prev1 image.
                    vector<int> inliers;// inliers index with respect to the matched points
                    if(0){ // -- Find their inliers by checking epipolar constraints.
                        // -- Due to bad estimation of R and t, this method is very unrealiable
                        const double ERR_EPPI_TRESH = 0.01;
                        const int num_matched_pts = pts_img1.size();
                        for (int i = 0; i < num_matched_pts; i++)
                        {
                            const Point2f &p1 = pts_img1[i], &p2 = pts_img2[i];
                            double err = abs(computeEpipolarConsError(p1, p2, R, t, K));
                            if (err < ERR_EPPI_TRESH)
                            {
                                inliers.push_back(i);
                            }
                        }
                    }else{
                        // Estimate Essential matrix to find the inliers
                        Mat Rtmp, ttmp;
                        helperEstiMotionByEssential(
                            frame1->keypoints_, frame2->keypoints_,
                            frame2->matches_,
                            K, Rtmp, ttmp, inliers);
                    }
                    // -- Use triangulation to find these points pos in prev image's frame
                    vector<Point3f> pts3d_in_cam1, pts3d_in_cam2;
                    printf("In prev frame: num pts = %d, num matches = %d\n",
                        (int)frame2->keypoints_.size(), (int)frame2->matches_.size());
                    printf("   num inliers after checking epipolar error: %d \n",(int)inliers.size());
                    doTriangulation(pts_on_np1, pts_on_np2, R, t, inliers, pts3d_in_cam1);
                    for (const Point3f &pt3d : pts3d_in_cam1)
                        pts3d_in_cam2.push_back(transCoord(pt3d, R, t));

                    // -- update values
                    vector<int> inliers_in_kpts2; // inliers index with respect to all the points
                    for (int idx : inliers)
                        inliers_in_kpts2.push_back(frame2->matches_[idx].trainIdx);
                    frame2->inliers_of_all_pts_ = inliers_in_kpts2;
                    frame2->inliers_pts3d_ = pts3d_in_cam2;
                }

                // --  Find the intersection between [DMatches_curr] and [DMatches_prev],
                // --  and 3d-2d correspondance
                vector<Point3f> pts_3d; // a point's 3d pos in cam1 frame
                vector<Point2f> pts_2d; // a point's 2d pos in image2 pixel frame
                helperFind3Dto2DCorrespondences(frame->matches_, frame->keypoints_,
                                                frame2->inliers_of_all_pts_, frame2->inliers_pts3d_,
                                                pts_3d, pts_2d);
                cout << "Number of 3d-2d pairs: " << pts_3d.size() << endl;

                // -- Solve PnP, get T_cam1_to_cam2
                Mat R_vec, R, t;
                solvePnPRansac(pts_3d, pts_2d, K, Mat(), R_vec, t, false);
                Rodrigues(R_vec, R);

                // -- Update current camera pos
                Mat T_curr_to_prev = transRt2T(R, t);
                frame->T_w_c_ = prev_frame->T_w_c_ * T_curr_to_prev.inv();
                frame->R_curr_to_prev_ = R;
                frame->t_curr_to_prev_ = t;

                // -- Update
                vo_state = OK;
            }
        }
        
        // ------------------------Complete-------------------------------

        printf("\n\n-----Printing frame %d results:---------\n", img_id);
        if(1){// Display image by opencv
            cv::destroyAllWindows();
            cv::Mat img_show=rgb_img.clone();
            std::stringstream ss;
            ss << std::setw(4) << std::setfill('0') << img_id;
            string str_img_id=ss.str();

            if(img_id==0){
                cv::Scalar color(0,255,0);
                cv::Scalar color2= cv::Scalar::all(-1);
                cv::drawKeypoints(img_show, frame->keypoints_, img_show, color);
                cv::imshow ( "rgb_img", img_show );
            }else{
                my_slam::Frame::Ptr prev_frame = frames.back();
                string window_name = "Image "+str_img_id  + ", matched keypoints";
                drawMatches(frame->rgb_img_, frame->keypoints_, 
                    prev_frame->rgb_img_, prev_frame->keypoints_, frame->matches_, img_show);
                cv::namedWindow(window_name, WINDOW_AUTOSIZE);
                cv::imshow(window_name, img_show);
            }
            waitKey(1);
            imwrite("result/"+ str_img_id + ".png", img_show);
        }
        if(ENABLE_PCL_DISPLAY){
            Mat R, R_vec, t;
            getRtFromT(frame->T_w_c_, R, t);
            Rodrigues(R, R_vec);

            cout << endl;
            cout <<"R_world_to_camera:\n"<<R<<endl;
            cout <<"t_world_to_camera:\n"<<t.t()<<endl;
            
            pcl_displayer->updateCameraPose(R_vec, t);
            // pcl_displayer->addPoint(kpt_3d_pos_in_world, r, g, b);
            
            pcl_displayer->update();
            pcl_displayer->spinOnce(100);
            if (pcl_displayer->wasStopped())
                break;
        }
        // Save to buff.
        frames.push_back(frame);
        if (frames.size() > 10)
            frames.pop_front();

        // Print
        cout << "R_curr_to_prev_: " << frame->R_curr_to_prev_ << endl;
        cout << "t_curr_to_prev_: " << frame->t_curr_to_prev_ << endl;
        // Return
        if (img_id == 5)
            break;
        cout<<"Finished an image"<<endl;
    }
}

bool checkInputArguments(int argc, char **argv)
{
    // The only argument is Path to the configuration file, which stores the dataset_dir and camera_info
    const int NUM_ARGUMENTS = 1;
    if (argc - 1 != NUM_ARGUMENTS)
    {
        cout << "Lack arguments: Please input the path to the .yaml config file" << endl;
        return false;
    }
    return true;
}