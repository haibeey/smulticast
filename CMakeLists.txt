cmake_minimum_required(VERSION 3.14)

project(smulticast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    inifile
    GIT_REPOSITORY https://github.com/Rookfighter/inifile-cpp.git
    GIT_TAG master
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

FetchContent_MakeAvailable(inifile spdlog)

file(GLOB SOURCE_FILES "src/*.cpp")

add_executable(
    ${PROJECT_NAME}
    ${SOURCE_FILES}
    main.cpp
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    inicpp::inicpp
)


if(BUILD_TESTS)
  set(GOOGLE_TEST_SHA 063de7e9578f82b369302001269680b4b1553359)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/${GOOGLE_TEST_SHA}.zip
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  file(GLOB TEST_SOURCE_FILES "test/*.cpp")
  add_executable(${PROJECT_NAME}_test ${TEST_SOURCE_FILES} ${SOURCE_FILES})
  target_include_directories(${PROJECT_NAME}_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(${PROJECT_NAME}_test PRIVATE GTest::gtest_main spdlog::spdlog inicpp::inicpp)

  include(GoogleTest)
  gtest_discover_tests(${PROJECT_NAME}_test)
endif()
