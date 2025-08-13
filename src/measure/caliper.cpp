#include "measure/caliper.h"
#include <opencv2/imgproc.hpp>
#include <vector>
#include <algorithm>
namespace mp {
CaliperResult caliper1D(const cv::Mat& grayIn, cv::Point2f a, cv::Point2f b, int samples){
  CV_Assert(!grayIn.empty());
  cv::Mat gray = grayIn.channels()==1? grayIn : ([&]{cv::Mat g; cv::cvtColor(grayIn,g,cv::COLOR_BGR2GRAY); return g;})();
  std::vector<float> prof(samples);
  for (int i=0;i<samples;++i){
    float t=float(i)/(samples-1);
    cv::Point2f p=a*(1.f-t)+b*t;
    int x=std::clamp((int)std::round(p.x),0,gray.cols-1);
    int y=std::clamp((int)std::round(p.y),0,gray.rows-1);
    prof[i]=gray.at<uchar>(y,x);
  }
  float best=-1e9f; int bi=-1;
  for (int i=1;i<samples;++i){ float g = prof[i]-prof[i-1]; if (g>best){ best=g; bi=i; } }
  CaliperResult r; r.index=bi; r.response=best;
  if (bi>=0){ float t=float(bi)/(samples-1); r.position=a*(1.f-t)+b*t; }
  return r;
}
}
