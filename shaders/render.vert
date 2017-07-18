#version 450 core
#pragma debug(on)
#pragma optimize(off)

#pragma include("lighting.vert.glsl")


layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 tangent;
layout(location=3) in vec3 bitangent;
layout(location=4) in vec4 color;
layout(location=5) in vec2 uv;

uniform mat4 V;
uniform mat4 MV;
uniform mat4 MVP;

void main(){
	vertex_out.texCoord = uv;
	vertex_out.color = color;
	applyLight(MV, V, position, normal, tangent, bitangent);
	gl_Position = MVP * vec4(position, 1);
}
