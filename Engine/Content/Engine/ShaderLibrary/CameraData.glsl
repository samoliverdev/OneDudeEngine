#ifndef CAMERA_DATA_INCLUDED
#define CAMERA_DATA_INCLUDED

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
    Uniform mat4 invProjection;
    Uniform mat4 invView;
EndUniform()

#endif