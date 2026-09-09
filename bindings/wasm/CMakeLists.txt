if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Wasm Bindings require an Emscripten (emcmake) build.")
endif()

add_executable(stards_wasm stards_bind.cpp)

target_link_libraries(stards_wasm PRIVATE StarDS)

target_link_options(stards_wasm PRIVATE
    -lembind
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME=initStarDS
    -sFETCH=1
    -sALLOW_MEMORY_GROWTH=1
)