#version 130

#define SkipAlpha (.95f)
#define PI (3.14159f)

uniform sampler3D volTex0;
uniform sampler2D tfTexPreInt0;
uniform sampler3D volTex1;
uniform sampler2D tfTexPreInt1;
uniform vec3 eyePos;
uniform vec2 longtitudeRange;
uniform vec2 latitudeRange;
uniform vec2 heightRange;
uniform float dt;
uniform int maxStepCnt;
uniform bool volStartFromZeroLon;

varying vec3 vertex;

struct Hit {
	int isHit;
	float tEntry;
	float tExit;
};
/*
* 函数: intersectSphere
* 功能: 返回视线与球相交的位置
* 参数:
* -- d: 视点出发的方向
* -- r: 球半径
*/
Hit intersectSphere(vec3 d, float r) {
	Hit hit = Hit(0, 0.f, 0.f);

	float tVert = -dot(eyePos, d);
	vec3 pVert = eyePos + tVert * d;

	float r2 = r * r;
	float pVert2 = pVert.x * pVert.x + pVert.y * pVert.y + pVert.z * pVert.z;
	if (pVert2 >= r2) return hit;
	float l = sqrt(r2 - pVert2);

	hit.isHit = 1;
	hit.tEntry = tVert - l;
	hit.tExit = tVert + l;
	return hit;
}

void main() {
	vec3 d = normalize(vertex - eyePos);
	Hit hit = intersectSphere(d, heightRange[1]);
	if (hit.isHit == 0)
		discard;
	float tEntry = hit.tEntry;
	vec3 outerX = eyePos + tEntry * d;

	vec3 pos = outerX;
	float r = sqrt(pos.x * pos.x + pos.y * pos.y);
	float lat = atan(pos.z / r);
	r = length(pos);
	float lon = atan(pos.y, pos.x);
	// 判断视线与外球第一个交点（即进入体的位置）所在象限
	int entryOutOfRng = 0;
	if (lat < latitudeRange[0])
		entryOutOfRng |= 1;
	if (lat > latitudeRange[1])
		entryOutOfRng |= 2;
	if (lon < longtitudeRange[0])
		entryOutOfRng |= 4;
	if (lon > longtitudeRange[1])
		entryOutOfRng |= 8;
	float tExit = hit.tExit;
	hit = intersectSphere(d, heightRange[0]);
	if (hit.isHit != 0)
		tExit = hit.tEntry;
	// 判断视线离开体的位置所在象限
	pos = eyePos + tExit * d;
	r = sqrt(pos.x * pos.x + pos.y * pos.y);
	lat = atan(pos.z / r);
	lon = atan(pos.y, pos.x);
	// 若两个位置均不在范围内，且所在象限相同，则不需要计算该视线
	if ((entryOutOfRng & 1) != 0 && lat < latitudeRange[0])
		discard;
	if ((entryOutOfRng & 2) != 0 && lat > latitudeRange[1])
		discard;
	if ((entryOutOfRng & 4) != 0 && lon < longtitudeRange[0])
		discard;
	if ((entryOutOfRng & 8) != 0 && lon > longtitudeRange[1])
		discard;

	// 执行光线传播算法
	float hDlt = heightRange[1] - heightRange[0];
	float latDlt = latitudeRange[1] - latitudeRange[0];
	float lonDlt = longtitudeRange[1] - longtitudeRange[0];
	vec4 color = vec4(0, 0, 0, 0);
	float tAcc = 0.f;
	int stepCnt = 0;
	float prevScalar0 = -1.f;
	float prevScalar1 = -1.f;
	pos = outerX;
	tExit -= tEntry;

	do {
		r = sqrt(pos.x * pos.x + pos.y * pos.y);
		lat = atan(pos.z / r);
		r = length(pos);
		lon = atan(pos.y, pos.x);

		if (lat < latitudeRange[0] || lat > latitudeRange[1] || lon < longtitudeRange[0] || lon > longtitudeRange[1]) {}
		else {
			r = (r - heightRange[0]) / hDlt;
			lat = (lat - latitudeRange[0]) / latDlt;
			lon = (lon - longtitudeRange[0]) / lonDlt;
			if (volStartFromZeroLon)
				if (lon < .5f) lon += .5f;
				else lon -= .5f;

			vec3 samplePos = vec3(lon, lat, r);
			float scalar = texture(volTex0, samplePos).r;
			if (prevScalar0 < 0.f) prevScalar0 = scalar;
			vec4 tfCol0 = texture(tfTexPreInt0, vec2(prevScalar0, scalar));
			prevScalar0 = scalar;

			scalar = texture(volTex1, samplePos).r;
			if (prevScalar1 < 0.f) prevScalar1 = scalar;
			vec4 tfCol1 = texture(tfTexPreInt1, vec2(prevScalar1, scalar));
			prevScalar1 = scalar;

			float omega = tfCol0.a / (tfCol0.a + tfCol1.a);
			tfCol0.rgb = omega * tfCol0.rgb + (1.f - omega) * tfCol1.rgb;
			tfCol0.a = max(tfCol0.a, tfCol1.a);

			if (tfCol0.a > 0.f) {
				color.rgb = color.rgb + (1.f - color.a) * tfCol0.rgb;
				color.a = color.a + (1.f - color.a) * tfCol0.a;
				if (color.a > SkipAlpha)
					break;
			}
		}

		pos += dt * d;
		tAcc += dt;
		++stepCnt;
	} while (tAcc < tExit && stepCnt <= maxStepCnt);

	gl_FragColor = color;
}
