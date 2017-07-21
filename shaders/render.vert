#version 450 core
#pragma debug(on)
#pragma optimize(off)


layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 tangent;
layout(location=3) in vec3 bitangent;
layout(location=4) in vec4 color;
layout(location=5) in vec2 uv;

uniform vec3 lightPos;
uniform mat4 V;
uniform mat4 MV;
uniform mat4 MVP;
uniform mat3 normalMatrix;

out VERTEX{
  smooth vec3 lightDir;
  smooth vec3 normal;
  smooth vec4 color;
  smooth vec2 uv;
} vertex_out;

void main(){
	vertex_out.color = color;
	vertex_out.normal = normalMatrix * normal;
	vertex_out.uv = uv;
	vertex_out.lightDir = (V * vec4(lightPos, 1)).xyz - (MV * vec4(position, 1)).xyz;
	gl_Position = MVP * vec4(position, 1);
}
