# =============================================================================
# practice/CMakeLists.txt
#
# 练习区独立编译成 exercises.exe。
# 这里写崩了只会让 exercises.exe 失败，主程序 PointCloudPractice 照常能编译。
#
# 为什么练习区要单独一个可执行文件而不是塞进主程序：
#   练习是你随手改的地方，主程序是"能跑的参考实现"。
#   两者混在一起，一次练习写错就再也编译不出主程序，你就没法对照参考实现了。
# =============================================================================

add_executable(exercises
    exercises.cpp
    # 复用主项目的两个基础模块（不复制代码）：
    #   console.cpp 提供 printKV / readInt 等
    #   cloud_io.cpp 提供 loadCloud / originalScenePath
    ${CMAKE_SOURCE_DIR}/src/core/console.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cloud_io.cpp
)

target_include_directories(exercises PRIVATE
    ${CMAKE_SOURCE_DIR}/src          # 复用主项目的 core/ 头文件
    ${PCL_INCLUDE_DIRS}
)

target_link_directories(exercises PRIVATE ${PCL_LIBRARY_DIRS})
target_link_libraries(exercises PRIVATE ${PCL_LIBRARIES})

if(MSVC)
    target_compile_options(exercises PRIVATE /utf-8 /bigobj /wd4996 /wd4819)
    target_compile_definitions(exercises PRIVATE
        NOMINMAX
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
        BOOST_ALL_NO_LIB
    )
endif()

# 练习区也把 DLL 拷到输出目录，否则跑到一半 0xC0000135
add_custom_command(TARGET exercises POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:exercises> "$<TARGET_FILE_DIR:exercises>"
    COMMAND_EXPAND_LISTS
    VERBATIM)
