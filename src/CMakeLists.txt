set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


add_library(core
  core/pipeline.cpp
  core/registry.cpp
  backend/specs_store.cpp
  measure/calibration.cpp
  measure/geometry.cpp
  measure/caliper.cpp
  measure/gauges.cpp
  measure/perspective.cpp
  measure/report.cpp
  ops/threshold.cpp
  ops/canny.cpp
  ops/morph.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(core PUBLIC Qt6::Core ${OpenCV_LIBS})

add_executable(myproject_gui
  gui/main.cpp
  gui/MainWindow.h
  gui/MainWindow.cpp
  gui/MainWindow.ui
  gui/RoiView.cpp
)
target_link_libraries(myproject_gui PRIVATE core  Qt6::Widgets ${OpenCV_LIBS})

add_executable(myproject_backend
  backend/server.cpp
)
target_link_libraries(myproject_backend PRIVATE  core Qt6::Network ${OpenCV_LIBS})
