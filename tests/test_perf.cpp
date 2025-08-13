#include <gtest/gtest.h>
#include <chrono>
#include "core/pipeline.h"
#include "ops/canny.h"
using namespace mp;
TEST(Perf, Canny1080pOver1fps){
  cv::Mat img = cv::Mat::ones(1080,1920,CV_8UC3)*127;
  Pipeline p; p.add(std::make_shared<op::Canny>(50,150,3,true));
  Frame f{img,"p"};
  auto t0 = std::chrono::high_resolution_clock::now();
  const int N=5; for (int i=0;i<N;++i) (void)p.run(f);
  double dt = std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-t0).count();
  double fps = N/dt;
  EXPECT_GT(fps, 1.0);
}
