
target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(${NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lzw.c
)