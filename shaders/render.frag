#version 450 core
#pragma debug(on)
#pragma optimize(off)

#pragma include("lighting.frag.glsl")
#pragma include("ads_diffuse.glsl")

uniform mat4 V;
out vec4 fragColor;

void main(){
	fragColor = phongLightModel(V);
}
