cmake_minimum_required(VERSION 3.16.3)
project(Uniform_sample_generator)

set(CMAKE_CXX_STANDARD 17)

# Find ROOT
find_package(ROOT REQUIRED)

# Include ROOT headers
include_directories(${ROOT_INCLUDE_DIRS})

add_executable(Uniform_sample_generator main.cpp
        Uniform_sample_generator.C
        AddParticle.C
        Generate_uniform_event.C
        CodeRun.cpp
        BoolToString.C
        DisplyText.C
        ConfigBeamE.cpp)

# Link against ROOT libraries
target_link_libraries(Uniform_sample_generator ${ROOT_LIBRARIES})