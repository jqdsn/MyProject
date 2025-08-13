#pragma once
namespace mp {
struct Calibration {
  double scale_mm_per_px = 0.02;
  double offset_mm = 0.0;
  double toMM(double px) const { return scale_mm_per_px * px + offset_mm; }
  double toPX(double mm) const { return (mm - offset_mm) / scale_mm_per_px; }
};
}
