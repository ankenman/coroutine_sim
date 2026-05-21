set(tests
        scheduled_event_test.cpp
        work_queue_test.cpp
        tracer_test.cpp
        transaction_inbox_test.cpp
        transaction_queue_test.cpp
        config_test.cpp
)

foreach(test ${tests})
    string(REPLACE _test.cpp _test target ${test})
    add_executable(${target} ${test})
    add_test(NAME ${target} COMMAND ${target})
    target_link_libraries(${target} PRIVATE
            fschuetz04::simcpp20
            GTest::gtest_main
            magic_enum::magic_enum
            csim_models
    )
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/include)
endforeach()

include(GoogleTest)
