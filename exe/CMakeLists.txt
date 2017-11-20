
add_executable(mmapPinSim mmapPinSim.cpp)
target_include_directories(mmapPinSim PUBLIC ${SST_CORE}/include)

install(TARGETS mmapPinSim  
      RUNTIME DESTINATION bin)

