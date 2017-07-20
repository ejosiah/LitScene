#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout(location=0) in vec2 position;
layout(location=5) in vec2 texCoord;
smooth out vec2 uv;

void main(){
	uv = position;
	gl_Position = vec4(position, 0, 1);
}