#version 450 core
#pragma debug(on)
#pragma optimize(off)

layout(binding=11) uniform sampler2D diffuseMap;

in VERTEX{
  smooth vec3 lightDir;
  smooth vec3 normal;
  smooth vec4 color;
  smooth vec2 uv;
} vertex_in;

out vec4 fragColor;

void main(){
	vec3 L = normalize(vertex_in.lightDir);
	vec3 N = normalize(vertex_in.normal);

	float diffuse = max(0, dot(L, N));

	fragColor = diffuse * texture(diffuseMap, vertex_in.uv);
}
