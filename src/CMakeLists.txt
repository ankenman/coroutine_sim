add_library(csim_infra STATIC
        csim/core/module.cpp
        csim/core/system.cpp
        csim/utilities/work_queue.cpp
        csim/core/payload.cpp
        csim/utilities/cache.cpp
        csim/utilities/delay_channel.cpp
        csim/utilities/scheduled_event.cpp
        csim/tracing/tracer.cpp
        csim/config/config.cpp
        csim/config/knob_list.cpp
        csim/config/topology_loader.cpp
)
target_include_directories(csim_infra PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(csim_infra PUBLIC fschuetz04::simcpp20 magic_enum::magic_enum nlohmann_json::nlohmann_json)
target_compile_features(csim_infra PUBLIC cxx_std_20)

add_library(csim_models STATIC
        models/initiator.cpp
        models/target.cpp
        models/interconnect.cpp
        models/home_agent.cpp
        models/transaction_inbox.cpp
        models/transaction_queue.cpp
)
target_link_libraries(csim_models PUBLIC csim_infra)
target_include_directories(csim_models PUBLIC ../include)

add_executable(coroutine_sim main.cpp)
target_link_libraries(coroutine_sim PRIVATE csim_models)