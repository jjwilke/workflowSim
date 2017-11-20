

add_library(tracer traceTest.cpp)
target_include_directories(tracer PUBLIC ${PIN_INCLUDE_DIRS})

foreach(DEF ${PIN_DEFINES})
target_compile_definitions(tracer PUBLIC -D${DEF})
endforeach()

