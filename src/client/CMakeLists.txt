add_executable(bbc ClientMain.cpp)
add_library(client Client.cpp)
target_link_libraries(bbc client Authenticator communication Message configfiles commandLineInterface)
set_target_properties(bbc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
install (TARGETS bbc Net RUNTIME DESTINATION bin LIBRARY DESTINATION lib)