set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(fmt-star-viz-demo
               configobjdialog.h
               configobjdialog.cpp
               mainwindow.h
               mainwindow.cpp
               fmt_star_viz_demo.cpp
               )

include_directories(
                   )

target_link_libraries(fmt-star-viz-demo
                      fmt-star
                      fmt-star-viz
                      ${QT_LIBRARIES}
                     )
