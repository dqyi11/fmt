
set(LIB fmt-star)

add_library(${LIB} SHARED
            kdtree++/allocator.hpp
            kdtree++/function.hpp
            kdtree++/iterator.hpp
            kdtree++/kdtree.hpp
            kdtree++/node.hpp
            kdtree++/region.hpp
            fmt_star.h
            fmt_star.cpp
           )

target_link_libraries(${LIB})
