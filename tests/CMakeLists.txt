add_executable(skinning_test skinning_test.cpp)
target_link_libraries(skinning_test PRIVATE acm_skin)
add_test(NAME skinning_test COMMAND skinning_test)
