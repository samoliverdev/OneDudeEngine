#ifndef BASE_INCLUDED
#define BASE_INCLUDED

/*
#define BeginAttribute() \
    #if defined(VERTEX)
    
#define EndAttribute() \
    #endif
*/

#if defined(OpenGL_API)
    precision highp float;
    precision highp int;

    #define In(loc) in
    #define Out(loc) out
    #define Attribute(loc) layout(location = loc) in

    #define OutPosition gl_Position

#endif

#if defined(WebGPU_API)
    #define In(loc) layout(location = loc) in 
    #define Out(loc) layout(location = loc) out 
    #define Attribute(loc) layout(location = loc) in

    #define OutPosition gl_Position
#endif

#endif