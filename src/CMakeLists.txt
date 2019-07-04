file(GLOB_RECURSE axn_SRCS ${PROJECT_SOURCE_DIR}/src/*.cc)

add_library(axnet ${axn_SRCS})
target_link_libraries(axnet ${Boost_LIBRARIES})
