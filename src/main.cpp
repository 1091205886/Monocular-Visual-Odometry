
#include <iostream>
#include <opencv2/highgui/highgui.hpp>

#include <string>
#include <boost/format.hpp> // for setting image filename

#include "my_basics/config.h"
#include "my_slam/visual_odometry.h"
#include "my_slam/camera.h"
#include "my_slam/frame.h"

// const int FRAME_RATE=30;

// Read in all image paths
vector<string> getImagePaths()
{
    //  Input: path of the configuration file, which stores the dataset(images) folder
    //  Output: image_paths
    vector<string> image_paths;

    // Set up image_paths
    string dataset_dir = my_basics::Config::get<string>("dataset_dir"); // get dataset_dir from config
    boost::format filename_fmt(dataset_dir + "/rgb_%05d.png");
    for (int i = 0; i < 5; i++)
    {
        image_paths.push_back((filename_fmt % i).str());
    }

    // Print result
    cout << endl;
    cout << "Reading from dataset_dir: " << dataset_dir << endl;
    cout << "Number of images: " << image_paths.size() << endl;
    cout << "Print the first 5 image filenames:" << endl;
    for (auto s : image_paths)
        cout << s << endl;
    cout << endl;

    // Return
    return image_paths;
}

my_slam::Camera::Ptr setupCamera(){
    my_slam::Camera::Ptr camera(
        new my_slam::Camera(
            my_basics::Config::get<double>("camera_info.fx"),
            my_basics::Config::get<double>("camera_info.fy"),
            my_basics::Config::get<double>("camera_info.cx"),
            my_basics::Config::get<double>("camera_info.cy")
        )
    );
    return camera;
}

int main(int argc, char **argv)
{
    
    // -----------------------------------------------------------
    // Check input arguments:
    //      Path to the configuration file, which stores the dataset_dir and camera_info
    const int NUM_ARGUMENTS = 1;
    if (argc - 1 != NUM_ARGUMENTS)
    {
        cout << "Lack arguments: Please input the path to the .yaml config file" << endl;
        return 1;
    }
    const string path_of_config_file = argv[1];
    my_basics::Config::setParameterFile(path_of_config_file);

    // -----------------------------------------------------------
    
    vector<string> image_paths=getImagePaths();

    my_slam::Camera::Ptr camera=setupCamera();
        
    my_slam::VisualOdometry::Ptr vo ( new my_slam::VisualOdometry );

    for (int i = 0; i< image_paths.size(); i++){
        
        // Read image.
        cv::Mat rgb_img = cv::imread ( image_paths[i] );
        if ( rgb_img.data==nullptr )
            break;

        // Add to map        
        // my_slam::Frame::Ptr frame( new my_slam::Frame(rgb_img, camera) );
        my_slam::Frame::Ptr frame = my_slam::Frame::createFrame(rgb_img, camera);
        vo->addFrame(frame);

        // Display
        cv::Mat img_show=rgb_img.clone();
        cv::Scalar color(0,255,0);
        cv::Scalar color2= cv::Scalar::all(-1);
        cv::drawKeypoints(img_show, vo->getCurrentKeypoints(), img_show, color);
        cv::imshow ( "image", img_show );
        cv::waitKey (1000);
    }
}