shader_type canvas_item;

// An asteroid lit by the Sun: a lumpy, cratered surface on a rough
// potato-shaped body.  The mesh is a fan from the rock's centre whose UV
// runs from 0 there to 1 on the outline, which gives the body its bulge;
// the surface detail comes from noise and a few craters, and the light
// falls from the Sun's direction (in the rock's own space) with hard
// shadows, as it does in vacuum.

uniform vec2 light_dir = vec2(1.0, 0.0);   // toward the Sun, rock space
uniform float seed = 1.0;
uniform float radius = 10.0;
uniform vec3 albedo : source_color = vec3(0.3, 0.29, 0.27);
uniform float metal = 0.0;                  // 1 for nickel-iron rocks
uniform float flash = 0.0;                  // a hit lights it up briefly

varying vec2 lp;

void vertex() {
	lp = VERTEX / radius;
}

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 hash2(float n) {
	return fract(sin(vec2(n, n + 1.7) * vec2(43758.5453, 22578.1459)));
}

float noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
			mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
	float v = 0.0;
	float a = 0.5;
	for (int i = 0; i < 5; i++) {
		v += a * noise(p);
		p = p * 2.03 + vec2(1.7, 9.2);
		a *= 0.5;
	}
	return v;
}

// surface height: lumps, grit and a handful of craters with raised rims
float height(vec2 p) {
	vec2 o = vec2(seed * 3.1, seed * 1.7);
	float h = fbm(p * 1.8 + o) * 0.6 + fbm(p * 6.0 + o * 1.3) * 0.08;
	for (int i = 0; i < 6; i++) {
		vec2 c = (hash2(seed + float(i) * 7.3) - 0.5) * 1.5;
		float cr = 0.1 + 0.26 * hash(vec2(seed, float(i)));
		float d = length(p - c) / cr;
		float bowl = d < 1.0 ? (d * d - 1.0) * 0.5 : 0.0;
		float rim = exp(-pow((d - 1.0) * 3.0, 2.0)) * 0.16;
		h += (bowl + rim) * cr * 2.2;
	}
	return h;
}

void fragment() {
	float edge = COLOR.a;                // 0 on a thin outer skirt: a soft outline
	vec2 q = UV;                         // 0 at the centre, ~1 on the outline
	float rr = clamp(length(q), 0.0, 1.0);
	// the body's bulge: steep near the outline, flat on top
	float z = sqrt(max(1.0 - rr * rr * 0.94, 0.0));
	vec3 n = normalize(vec3(q * 0.95, z + 0.12));
	// bumps from the height field
	float e = 0.02;
	float h0 = height(lp);
	float hx = height(lp + vec2(e, 0.0));
	float hy = height(lp + vec2(0.0, e));
	vec2 g = vec2(hx - h0, hy - h0) / e;
	n = normalize(n - vec3(g * 0.55, 0.0) * (0.4 + 0.6 * z));
	vec3 l = normalize(vec3(light_dir, 0.38));
	float lambert = dot(n, l);
	// hard terminator, a little scattered light, nearly black in shadow
	float diff = smoothstep(-0.05, 0.25, lambert) * max(lambert, 0.0) * 1.3 + 0.07;
	// crater floors are darker and smoother, fresh rims a touch brighter
	float tone = 0.8 + 0.35 * fbm(lp * 3.3 + vec2(seed * 2.0, 4.0)) + clamp(h0 - 0.3, -0.2, 0.2) * 0.6;
	vec3 base = albedo * tone;
	vec3 c = base * diff;
	// nickel-iron glints
	vec3 hv = normalize(l + vec3(0.0, 0.0, 1.0));
	c += vec3(0.9, 0.88, 0.85) * pow(max(dot(n, hv), 0.0), 30.0) * metal * 0.3 * step(0.0, lambert);
	// a faint rim of light where the Sun grazes the outline
	c += albedo * pow(rr, 6.0) * max(dot(normalize(q + 1e-4), light_dir), 0.0) * 0.35;
	c = mix(c, vec3(1.0, 0.9, 0.75), flash);
	COLOR = vec4(c, edge);
}
