set(ImguiSources 
    imgui.cpp
    imgui_draw.cpp
    imgui_demo.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
)
if(SupportOpenGL)
    set(ImguiSources ${ImguiSources} 
        backends/imgui_impl_opengl3.cpp
    )
endif()
if(SupportWebGPU)
    set(ImguiSources ${ImguiSources} 
        backends/imgui_impl_wgpu.cpp
    )
endif()

#add_library(imgui STATIC ${ImguiSources})
#target_include_directories(imgui PUBLIC .)
#target_link_libraries(imgui PUBLIC webgpu glfw)