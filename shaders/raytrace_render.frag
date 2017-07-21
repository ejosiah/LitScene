#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout(binding=0) uniform sampler2D scene_img;

out vec4 fragColor;

void main(){
	fragColor = texture(scene_img, vec2(gl_FragCoord.xy)/ vec2(textureSize(scene_img, 0)));
}