shader_type canvas_item;

// A ship's shield: a faint elliptical bubble of hexagonal cells around the
// whole hull, the same pale colour for every side so it never reads as
// friend or foe.  It is about a fifth opaque at full charge and has no hard
// edge: it fades out toward its rim.  As it weakens it thins out, cells
// drop away and the rest flicker near the end; hits light it up where they
// land and ripple across it, and it shatters when it fails.

uniform float strength = 1.0;       // shield / shield_max
uniform float strength_front = 1.0; // the front half's charge / its size
uniform float strength_rear = 1.0;  // the rear half's
uniform float alpha = 1.0;          // cloak and fades
uniform vec2 aspect = vec2(1.0, 0.5);   // bubble half size, normalised to the longer side
uniform vec4 hits[4];               // xy: point on the bubble (-1..1), z: age (s), w: weight
uniform float collapse = 1.0;       // seconds since the shield went down
uniform float guard = 0.0;          // respawn protection, 0..1

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
	// cells in world proportions, a little bent toward the rim
	vec2 cp = p * aspect * 9.0 / (0.6 + 0.4 * z);
	vec3 h = hex(cp);
	float line = 1.0 - smoothstep(0.0, 0.07, h.x);
	// weaker shields thin out and lose cells; nearly gone, the rest flicker
	float cell = hash(h.yz);
	float intact = step(cell, s_here * 1.15 - 0.05);
	float flick = s_here < 0.25 ? 0.5 + 0.5 * step(0.4, hash(h.yz + floor(TIME * 18.0))) : 1.0;
	float charge = 0.45 + 0.55 * s_here;
	vec3 col = vec3(0.82, 0.9, 1.0);
	float body = (0.07 * soft + 0.13 * rim) * (0.5 + 0.5 * intact) + 0.08 * line * intact * (0.4 + rim) * soft;
	float a = s_here > 0.0 ? body * charge * flick : 0.0;
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
