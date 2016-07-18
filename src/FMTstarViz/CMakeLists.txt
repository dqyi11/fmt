
set(LIB fmt-star-viz)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(${LIB} SHARED
            path_planning_info.h
            path_planning_info.cpp
            fmt_star_viz.h
            fmt_star_viz.cpp
           )

include_directories(
                   )

target_link_libraries(${LIB} 
                      fmt-star
                      ${LIBXML2_LIBRARIES}
                      ${QT_LIBRARIES}
                     )
