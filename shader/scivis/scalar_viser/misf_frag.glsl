#version 130

#define SkipAlpha (.95f)
#define MaxIsoValNum (16)
#define PI (3.14159f)

uniform sampler3D volTex;
uniform vec3 eyePos;
uniform vec3 lightPos;
uniform vec3 dSamplePos;
uniform mat3 rotMat;
uniform float dt;
uniform float minLatitute;
uniform float maxLatitute;
uniform float minLongtitute;
uniform float maxLongtitute;
uniform float minHeight;
uniform float maxHeight;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float shininess;
uniform float sortedIsoVals[MaxIsoValNum];
uniform vec4 isosurfCols[MaxIsoValNum];
uniform int maxStepCnt;
uniform int volStartFromZeroLon;
uniform int useShading;

varying vec3 vertex;

struct Hit {
	int isHit;
	float tEntry;
};

/*
* 函数: intersectInnerSphere
* 功能: 返回视线与内球相交的位置
* 参数:
* -- p: 视线与外球相交的位置
* -- e2pDir: 视点指向p的方向
*/
Hit intersectInnerSphere(vec3 p, vec3 e2pDir) {
	Hit hit = Hit(0, 0.f);
	vec3 p2c = -p;
	float l = dot(e2pDir, p2c);
	if (l <= 0.f)
		return hit;

	float m2 = maxHeight * maxHeight - l * l;
	float innerR2 = minHeight * minHeight;
	if (m2 >= innerR2)
		return hit;

	float q = sqrt(innerR2 - m2);
	hit.isHit = 1;
	hit.tEntry = l - q;
	return hit;
}

/*
* 函数: anotherIntersectionOuterSphere
* 功能: 返回视线与外球相交的另一个位置
* 参数:
* -- p: 视线与外球相交的第一个位置
* -- e2pDir: 视点指向p的方向
*/
float anotherIntersectionOuterSphere(vec3 p, vec3 e2pDir) {
	vec3 p2c = -p;
	float l = dot(e2pDir, p2c);
	return 2.f * l;
}

void main() {
	vec3 d = normalize(vertex - eyePos);
	vec3 pos = vertex;
	float r = sqrt(pos.x * pos.x + pos.y * pos.y);
	float lat = atan(pos.z / r);
	r = length(pos);
	float lon = atan(pos.y, pos.x);
	// 判断视线与外球第一个交点（即进入体的位置）所在象限
	int entryOutOfRng = 0;
	if (lat < minLatitute)
		entryOutOfRng |= 1;
	if (lat > maxLatitute)
		entryOutOfRng |= 2;
	if (lon < minLongtitute)
		entryOutOfRng |= 4;
	if (lon > maxLongtitute)
		entryOutOfRng |= 8;
	float tExit = anotherIntersectionOuterSphere(vertex, d);
	Hit hit = intersectInnerSphere(vertex, d);
	if (hit.isHit != 0)
		tExit = hit.tEntry;
	// 判断视线离开体的位置所在象限
	pos = vertex + tExit * d;
	r = sqrt(pos.x * pos.x + pos.y * pos.y);
	lat = atan(pos.z / r);
	lon = atan(pos.y, pos.x);
	// 若两个位置均不在范围内，且所在象限相同，则不需要计算该视线
	if ((entryOutOfRng & 1) != 0 && lat < minLatitute)
		discard;
	if ((entryOutOfRng & 2) != 0 && lat > maxLatitute)
		discard;
	if ((entryOutOfRng & 4) != 0 && lon < minLongtitute)
		discard;
	if ((entryOutOfRng & 8) != 0 && lon > maxLongtitute)
		discard;
	float hDlt = maxHeight - minHeight;
	float latDlt = maxLatitute - minLatitute;
	float lonDlt = maxLongtitute - minLongtitute;
	vec4 color = vec4(0, 0, 0, 0);
	vec3 entry2Exit = pos - vertex;
	float tMax = length(entry2Exit);
	float tAcc = 0.f;
	pos = vertex;
	int stepCnt = 0;
	// 执行光线传播算法
	int realMaxStepCnt = maxStepCnt;
	vec3 prevSamplePos;
	float prevScalar = 100.f; // 初始为大值，使得第一次采样无法匹配等值面
	do {
		r = sqrt(pos.x * pos.x + pos.y * pos.y);
		lat = atan(pos.z / r);
		r = length(pos);
		lon = atan(pos.y, pos.x);

		if (lat < minLatitute || lat > maxLatitute || lon < minLongtitute || lon > maxLongtitute) {}
		else {
			r = (r - minHeight) / hDlt;
			lat = (lat - minLatitute) / latDlt;
			lon = (lon - minLongtitute) / lonDlt;
			if (volStartFromZeroLon != 0)
				if (lon < .5f) lon += .5f;
				else lon -= .5f;

			vec3 samplePos = vec3(lon, lat, r);
			float scalar = texture(volTex, samplePos).r;
			int find = -1;
			for (int i = 0; i < MaxIsoValNum; ++i) {
				if (sortedIsoVals[i] < 0.f) break;
				if (prevScalar != scalar && prevScalar <= sortedIsoVals[i] && scalar >= sortedIsoVals[i]) {
					find = i;
					break;
				}
				if (scalar <= sortedIsoVals[i]) break;
			}
			if (find != -1 && useShading != 0) {
				float scalarDlt = scalar - prevScalar;
				vec3 cmptSamplePos =
					(sortedIsoVals[find] - prevScalar) / scalarDlt * samplePos +
					(scalar - sortedIsoVals[find]) / scalarDlt * prevSamplePos;

				vec3 N;
				N.x = texture(volTex, cmptSamplePos + vec3(dSamplePos.x, 0, 0)).r - texture(volTex, cmptSamplePos - vec3(dSamplePos.x, 0, 0)).r;
				N.y = texture(volTex, cmptSamplePos + vec3(0, dSamplePos.y, 0)).r - texture(volTex, cmptSamplePos - vec3(0, dSamplePos.y, 0)).r;
				N.z = texture(volTex, cmptSamplePos + vec3(0, 0, dSamplePos.z)).r - texture(volTex, cmptSamplePos - vec3(0, 0, dSamplePos.z)).r;
				N = rotMat * normalize(N);
				if (dot(N, d) > 0) N = -N;

				vec3 p2l = normalize(lightPos - pos);
				vec3 hfDir = normalize(-d + p2l);

				float ambient = ka;
				float diffuse = kd * max(0, dot(N, p2l));
				float specular = ks * pow(max(0, dot(N, hfDir)), shininess);

				vec4 isosurfCol = isosurfCols[find];
				isosurfCol.rgb = (ambient + diffuse + specular) * isosurfCol.rgb;

				color.rgb = color.rgb + (1.f - color.a) * isosurfCol.a * isosurfCol.rgb;
				color.a = color.a + (1.f - color.a) * isosurfCol.a;
				if (color.a > SkipAlpha)
					break;
			}
			else if (find != -1) {
				vec4 isosurfCol = isosurfCols[find];

				color.rgb = color.rgb + (1.f - color.a) * isosurfCol.a * isosurfCol.rgb;
				color.a = color.a + (1.f - color.a) * isosurfCol.a;
				if (color.a > SkipAlpha)
					break;
			}

			prevScalar = scalar;
			prevSamplePos = samplePos;
		}

		pos += dt * d;
		tAcc += dt;
		++stepCnt;
		if (stepCnt > realMaxStepCnt) break;
	} while (tAcc < tMax);

	gl_FragColor = color;
}
