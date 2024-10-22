#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
out vec3 pos;

uniform mat4 projection;
uniform mat4 view;

void main(){
    pos = _pos;
    gl_Position = projection * view * vec4(pos, 1.0); 
}
#endif

#if defined(FRAGMENT)
out vec4 FragColor;
in vec3 pos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main(){		
	// The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    vec3 N = normalize(pos);
    vec3 irradiance = vec3(0.0);   
    
    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta){
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta){
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            vec3 environment = texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            //environment = environment / (environment + vec3(1.0)); //Tone Mapping
            environment = min(environment, vec3(10)); //500 eliminate fireflies Source: https://github.com/furkansarihan/enigine/blob/master/src/assets/shaders/irradiance.fs

            irradiance += environment;
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    FragColor = vec4(irradiance, 1.0);
}
#endif