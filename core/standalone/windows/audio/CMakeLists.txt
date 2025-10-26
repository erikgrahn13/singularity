add_library(SingularityASIO STATIC SingularityASIO.cpp)
target_link_libraries(SingularityASIO PUBLIC asio)

# target_include_directories(SingularityASIO PUBLIC ${CMA})