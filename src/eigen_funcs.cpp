
#include "my_common/eigen_funcs.h"

namespace my
{
    
Eigen::Affine3d transCVMatRt2Affine3d(const Mat &R0, const Mat &t)
{
    // check input: whether R0 is a SO3 or xyz-Euler-Angles
    Mat R = R0.clone();
    if (R.rows == 3 && R.cols == 1)
        cv::Rodrigues(R, R);
    assert(R.rows == 3 && R.cols == 3);

    // compute
    Eigen::Affine3d T;
    T.linear() = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>::Map(reinterpret_cast<const double *>(R.data));
    T.translation() = Eigen::Vector3d::Map(reinterpret_cast<const double *>(t.data));
    return T;
}

} // namespace my