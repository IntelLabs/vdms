#include "vcl/CustomVCL.h"

void custom_vcl_function(VCL::Image& img, const Json::Value& ops)
{
    cv::Mat tmp = img.get_cvmat(true);
    img.deep_copy_cv(tmp);
}