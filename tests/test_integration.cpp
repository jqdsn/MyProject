#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/pipeline.h"
#include "ops/canny.h"
#include "ops/morph.h"
#include "ops/threshold.h"
using namespace mp;
TEST(Integration, SimplePipelineKeepsSize){
  cv::Mat img = cv::Mat::zeros(256,256,CV_8UC3);
  cv::circle(img, {128,128}, 50, {255,255,255}, 2);
  Pipeline p;
  p.add(std::make_shared<op::Canny>(50,150,3,true));
  p.add(std::make_shared<op::Morph>(cv::MORPH_OPEN,3,1));
  p.add(std::make_shared<op::Threshold>(128.0, cv::THRESH_BINARY));
  auto out = p.run(Frame{img,"t"});
  ASSERT_EQ(out.mat.size(), img.size());
}
