#version 410 core

#if defined(VERTEX)
layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main(){
    gl_Position = model * vec4(aPos, 1.0);
}
#endif

#if defined(GEOMETRY)
layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 lightSpaceMatrices[4];

void main(){          
	for (int i = 0; i < 3; ++i){
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}  
#endif

#if defined(FRAGMENT)
void main(){            
    // gl_FragDepth = gl_FragCoord.z;
} 
#endif