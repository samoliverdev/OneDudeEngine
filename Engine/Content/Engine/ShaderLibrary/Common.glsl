#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

float lerp(float a, float b, float t){ return mix(a, b, t); }
vec2 lerp(vec2 a, vec2 b, float t){ return mix(a, b, t); }
vec3 lerp(vec3 a, vec3 b, float t){ return mix(a, b, t); }
vec4 lerp(vec4 a, vec4 b, float t){ return mix(a, b, t); }

float Square(float v){ return v * v; }

vec3 SafeNormalize(vec3 v){ return normalize(v); }

vec3 saturate(vec3 v){ return clamp(v, 0.0, 1.0); }
float saturate(float v){ return clamp(v, 0.0, 1.0); }

float Pow4(float x){ return (x * x) * (x * x); }

/*

Source: https://discourse.panda3d.org/t/glsl-octahedral-normal-packing/15233

Normal packing as described in:
A Survey of Efficient Representations for Independent Unit Vectors
Source: http://jcgt.org/published/0003/02/01/paper.pdf

*/

// For each component of v, returns -1 if the component is < 0, else 1
vec2 sign_not_zero(vec2 v){
    return step(vec2(0.0), v) * 2.0 - 1.0;

    /*#if 0
        // Branch-Less version, but can not work on some drive/opengl version
        return fma(step(vec2(0.0), v), vec2(2.0), vec2(-1.0));
    #else
        // Version with branches (for GLSL < 4.00)
        return vec2(
            v.x >= 0 ? 1.0 : -1.0,
            v.y >= 0 ? 1.0 : -1.0
        );
    #endif*/
}

// Packs a 3-component normal to 2 channels using octahedron normals
vec2 pack_normal_octahedron(vec3 v) {
    #if 0
        // Version as proposed by the paper
        // Project the sphere onto the octahedron, and then onto the xy plane
        vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
        // Reflect the folds of the lower hemisphere over the diagonals
        return (v.z <= 0.0) ? ((1.0 - abs(p.yx))  * sign_not_zero(p)) : p;
    #else
        // Faster version using newer GLSL capatibilities
        v.xy /= dot(abs(v), vec3(1));
        
        #if 0
            // Version with branches
            if (v.z <= 0) v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
            return v.xy;
        #else
            // Branch-Less version
            return mix(v.xy, (1.0 - abs(v.yx)) * sign_not_zero(v.xy), step(v.z, 0.0));
        #endif
    #endif
}

// Unpacking from octahedron normals, input is the output from pack_normal_octahedron
vec3 unpack_normal_octahedron(vec2 packed_nrm) {
    #if 1
        // Version using newer GLSL capatibilities
        vec3 v = vec3(packed_nrm.xy, 1.0 - abs(packed_nrm.x) - abs(packed_nrm.y));
        #if 1
            // Version with branches, seems to take less cycles than the
            // branch-less version
            if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
        #else
            // Branch-Less version
            v.xy = mix(v.xy, (1.0 - abs(v.yx)) * sign_not_zero(v.xy), step(v.z, 0));
        #endif

        return normalize(v);
    #else
        // Version as proposed in the paper. 
        vec3 v = vec3(packed_nrm, 1.0 - dot(vec2(1), abs(packed_nrm)));
        if (v.z < 0)
            v.xy = (vec2(1) - abs(v.yx)) * sign_not_zero(v.xy);
        return normalize(v);
    #endif
}

vec3 reconstructWorldPos(vec2 uv, float depth, mat4 invProj, mat4 invView){
    // Convert [0,1] UV to Normalized Device Coordinates [-1,1]
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // View space position
    vec4 viewPos = invProj * ndc;
    viewPos /= viewPos.w;

    // World space
    vec4 worldPos = invView * viewPos;
    return worldPos.xyz;
}

#endif