# Collect all example source files
set(EXAMPLE_SOURCES
    main.cpp
    SimpleExample.cpp
    FutureExample.cpp
    PreparedStatementExample.cpp
    AppenderExample.cpp
    TransactionExample.cpp
    ../DuckDB/async_duckdb.cpp
    ../DuckDB/async_duckdb.hpp
    ${DMQ_LIB_SOURCES}
    ${DMQ_PREDEF_SOURCES}
)

# Create a single executable for all examples
add_executable(ExamplesApp ${EXAMPLE_SOURCES})

# Include directories
target_include_directories(ExamplesApp PRIVATE
    .
    ..
    ../DuckDB
    ${DMQ_ROOT_DIR}
)

# Link libraries
target_link_libraries(ExamplesApp PRIVATE 
    ${DUCKDB_LIB}
    ws2_32
    rstrtmgr
    bcrypt
    winmm
)

# MSVC-specific options
if(MSVC)
    target_compile_options(ExamplesApp PRIVATE
        /MP
        /wd4005
        /wd4146
    )
endif()

# Post-build: Copy DLL
add_custom_command(TARGET ExamplesApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/DuckDB/duckdb.dll"
    "$<TARGET_FILE_DIR:ExamplesApp>"
)
