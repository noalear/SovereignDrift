shader_type canvas_item;

// A ship's shield: a faint elliptical bubble of hexagonal cells around the
// whole hull, the same pale colour for every side so it never reads as
// friend or foe.  It is about a fifth opaque at full charge and has no hard
// edge: it fades out toward its rim.  As it weakens it thins out, cells
// drop away and the rest flicker near the end; hits light it up where they
// land and ripple across it, and it shatters when it fails.
//
// Each shield family has its own look (GameData.SHIELD_FAMILIES):
//  0 Deflector: the hex bubble;
//  1 Barrier: a heavy double shell of broad overlapping plates that
//    breathes slowly, a touch warmer;
//  2 Flicker: no cells, a thin skin of fast interference bands and
//    sparkles that never quite holds still, a touch cooler.
// A shield being raised sweeps in from the hull outward.

uniform float strength = 1.0;       // shield / shield_max
uniform float strength_front = 1.0; // the front half's charge / its size
uniform float strength_rear = 1.0;  // the rear half's
uniform float alpha = 1.0;          // cloak and fades
uniform vec2 aspect = vec2(1.0, 0.5);   // bubble half size, normalised to the longer side
uniform vec4 hits[4];               // xy: point on the bubble (-1..1), z: age (s), w: weight
uniform float collapse = 1.0;       // seconds since the shield went down
uniform float guard = 0.0;          // respawn protection, 0..1
uniform int family = 0;             // 0 Deflector, 1 Barrier, 2 Flicker
uniform float boot = 0.0;           // raising: 1 at the start of the boot, 0 when up

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// distance to the edge of the nearest hex cell, and that cell's id
vec3 hex(vec2 p) {
	vec2 s = vec2(1.0, 1.7320508);
	vec2 a = mod(p, s) - s * 0.5;
	vec2 b = mod(p - s * 0.5, s) - s * 0.5;
	vec2 g = dot(a, a) < dot(b, b) ? a : b;
	vec2 id = p - g;
	vec2 q = abs(g);
	float edge = 0.5 - max(dot(q, s * 0.5), q.x);
	return vec3(edge, id);
}

void fragment() {
	vec2 p = UV * 2.0 - 1.0;            // ellipse is the unit circle here
	// the front and rear halves (the ship points along +x) blend across the middle
	float s_here = mix(strength_rear, strength_front, smoothstep(-0.25, 0.25, p.x));
	float r = length(p);
	if (r > 1.0) {
		discard;
	}
	float z = sqrt(max(1.0 - r * r, 0.0));
	// brighter toward the rim like a bubble seen side-on, but faded out
	// before the edge so there is no outline
	float soft = 1.0 - smoothstep(0.78, 1.0, r);
	float rim = pow(1.0 - z, 1.6) * soft;
	vec2 wp = p * aspect;               // world proportions
	vec3 h;
	float line;
	float body;
	float intact;
	float flick = 1.0;
	float charge = 0.45 + 0.55 * s_here;
	vec3 col = vec3(0.82, 0.9, 1.0);
	if (family == 1) {
		// Barrier: broad plates laid like scales, two shells, a slow breath
		vec2 cp = wp * 5.0 / (0.55 + 0.45 * z);
		float row = floor(cp.y);
		vec2 q = vec2(cp.x + 0.5 * mod(row, 2.0), cp.y);
		vec2 f = fract(q) - 0.5;
		h = vec3(0.5 - max(abs(f.x), abs(f.y)), floor(q));
		line = 1.0 - smoothstep(0.0, 0.05, h.x);
		float bevel = smoothstep(0.0, 0.18, h.x) * (0.6 + 0.4 * f.y + 0.2);
		float cell = hash(h.yz);
		intact = step(cell, s_here * 1.1 - 0.02);
		float breath = 0.85 + 0.15 * sin(TIME * 1.3);
		float shell = exp(-pow((r - 0.82) * 14.0, 2.0)) + 0.7 * exp(-pow((r - 0.68) * 16.0, 2.0));
		col = vec3(0.94, 0.91, 0.8);
		body = ((0.09 * soft + 0.15 * rim) * (0.45 + 0.55 * intact) * (0.7 + 0.3 * bevel)
			+ 0.1 * line * intact * (0.5 + rim) * soft + 0.07 * shell * soft * (0.4 + 0.6 * s_here)) * breath;
		if (s_here < 0.25) {
			flick = 0.6 + 0.4 * step(0.3, hash(h.yz + floor(TIME * 6.0)));
		}
	} else if (family == 2) {
		// Flicker: a thin skin of running bands and sparkles, never still
		float t = TIME;
		float bands = 0.5 + 0.5 * sin(wp.x * 22.0 - t * 14.0 + sin(wp.y * 9.0 + t * 5.0) * 2.0);
		float weave = 0.5 + 0.5 * sin((wp.x + wp.y) * 31.0 + t * 19.0);
		vec2 sp = floor(wp * 40.0);
		float spark = step(0.985, hash(sp + floor(t * 24.0))) * smoothstep(0.5, 0.1, length(fract(wp * 40.0) - 0.5));
		h = vec3(bands, sp);
		line = pow(bands * weave, 3.0);
		// weak spots drift across it as it weakens, soft edged
		vec2 gp = wp * 7.0 + vec2(t * 0.7, 0.0);
		vec2 gi = floor(gp);
		vec2 gf = smoothstep(0.0, 1.0, fract(gp));
		float n = mix(mix(hash(gi), hash(gi + vec2(1.0, 0.0)), gf.x), mix(hash(gi + vec2(0.0, 1.0)), hash(gi + vec2(1.0, 1.0)), gf.x), gf.y);
		intact = smoothstep(s_here * 1.2 + 0.08, s_here * 1.2 - 0.08, n);
		flick = 0.7 + 0.3 * step(0.25, hash(vec2(floor(t * 30.0), 1.7)));
		float skin = exp(-pow((r - 0.9) * 9.0, 2.0));
		col = vec3(0.8, 1.0, 0.96);
		body = (0.04 * soft + 0.1 * rim + 0.1 * skin * soft) * (0.4 + 0.6 * intact)
			+ 0.1 * line * intact * soft + 0.35 * spark * soft * s_here;
		if (s_here < 0.25) {
			flick *= 0.4 + 0.6 * step(0.5, hash(vec2(floor(t * 40.0), 3.3)));
		}
	} else {
		// Deflector: hex cells, a little bent toward the rim
		vec2 cp = wp * 9.0 / (0.6 + 0.4 * z);
		h = hex(cp);
		line = 1.0 - smoothstep(0.0, 0.07, h.x);
		// weaker shields thin out and lose cells; nearly gone, the rest flicker
		float cell = hash(h.yz);
		intact = step(cell, s_here * 1.15 - 0.05);
		flick = s_here < 0.25 ? 0.5 + 0.5 * step(0.4, hash(h.yz + floor(TIME * 18.0))) : 1.0;
		body = (0.07 * soft + 0.13 * rim) * (0.5 + 0.5 * intact) + 0.08 * line * intact * (0.4 + rim) * soft;
	}
	float a = s_here > 0.0 ? body * charge * flick : 0.0;
	// raising: a bright front sweeps out from the hull, the shield forming behind it
	if (boot > 0.0) {
		float front = 1.0 - boot;
		a = 0.0;
		a += exp(-pow((r - front) * 12.0, 2.0)) * 0.22 * soft;
		a += step(r, front) * (0.03 + 0.12 * line) * soft;
	}
	// hits: a flash where it landed and a ring running out across the bubble
	float hit = 0.0;
	for (int i = 0; i < 4; i++) {
		vec4 hv = hits[i];
		if (hv.w <= 0.0 || hv.z > 0.9) {
			continue;
		}
		float d = length((p - hv.xy * 0.85) * aspect);
		float fade = 1.0 - hv.z / 0.9;
		float ring = exp(-pow((d - hv.z * 1.4) * 9.0, 2.0)) * fade;
		float flash = exp(-d * d * 22.0) * max(1.0 - hv.z / 0.22, 0.0);
		float glow = exp(-d * d * 6.0) * fade * fade;   // the cells round the impact light up
		hit += (ring * (0.15 + 0.4 * line) + glow * (0.05 + 0.35 * line) + flash * 0.5) * hv.w;
	}
	a += hit * (0.4 + 0.6 * soft);
	// collapse: the cells shatter one by one and are gone in half a second
	if (collapse < 0.5) {
		float t = collapse / 0.5;
		float gone = step(hash(h.yz * 1.7 + 3.1) * 0.9 + 0.1, t);
		a += (1.0 - gone) * (1.0 - t) * (0.03 + 0.3 * line) * (0.5 + 0.5 * soft);
	}
	// respawn protection: a gentle white shimmer
	if (guard > 0.0) {
		float sh = 0.5 + 0.5 * sin(TIME * 9.0 + (p.x + p.y) * 6.0);
		a += guard * (0.04 + 0.1 * rim) * (0.6 + 0.4 * sh);
	}
	COLOR = vec4(col, clamp(a, 0.0, 0.5) * alpha);
}
