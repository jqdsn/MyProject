#include "measure/geometry.h"
#include <opencv2/imgproc.hpp>
namespace mp {
Line2D fitLineLSQ(const std::vector<cv::Point2f>& pts){
  CV_Assert(!pts.empty());
  cv::Vec4f l; cv::fitLine(pts, l, cv::DIST_L2, 0, 1e-2, 1e-2);
  return Line2D{{l[2],l[3]}, {l[0],l[1]}};
}
Circle fitCircleKasa(const std::vector<cv::Point2f>& pts){
  CV_Assert(pts.size()>=3);
  double Sx=0,Sy=0,Sxx=0,Syy=0,Sxy=0,Sx3=0,Sy3=0,Sx2y=0,Sxy2=0;
  for (auto&p:pts){ double x=p.x,y=p.y,x2=x*x,y2=y*y;
    Sx+=x;Sy+=y;Sxx+=x2;Syy+=y2;Sxy+=x*y;Sx3+=x2*x;Sy3+=y2*y;Sx2y+=x2*y;Sxy2+=x*y2;}
  double N=pts.size();
  double C=N*Sxx - Sx*Sx, D=N*Sxy - Sx*Sy, E=N*Syy - Sy*Sy;
  double G=.5*(N*(Sx3+Sxy2) - Sx*(Sxx+Syy)), H=.5*(N*(Sy3+Sx2y) - Sy*(Sxx+Syy));
  double denom=C*E - D*D;
  cv::Point2f c((E*G-D*H)/denom, (C*H-D*G)/denom);
  float r = std::sqrt((Sxx+Syy + N*(c.x*c.x + c.y*c.y) - 2*(c.x*Sx + c.y*Sy))/N);
  return Circle{c,r};
}
}
