#ifndef GI_INCLUDED
#define GI_INCLUDED

uniform vec3 _AmbientLight = vec3(0.1, 0.1, 0.1);

struct GI{
    vec3 diffuse;
};

GI GetGI(){
    GI gi;
    gi.diffuse = _AmbientLight;
    return gi;
}

#endif