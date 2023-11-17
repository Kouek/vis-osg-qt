#version 130

uniform float minLatitute;
uniform float maxLatitute;
uniform float minLongtitute;
uniform float maxLongtitute;
uniform float minHeight;
uniform float maxHeight;
uniform int volStartFromZeroLon;

in vec3 vertex;

void main() {
	const vec3 lowCol = vec3(51.f / 255.f, 128.f / 255.f, 1.f);
	const vec3 highCol = vec3(1.f, 131.f / 255.f, 51.f / 255.f);

	float t = length(vertex);
	t = (t - minHeight) / (maxHeight - minHeight);

	gl_FragColor = vec4(mix(lowCol, highCol, t), 1.f);
}
