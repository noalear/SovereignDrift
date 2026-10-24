shader_type canvas_item;

// A ship's shield: an elliptical bubble of hexagonal cells around the whole
// hull.  Its colour runs blue -> amber -> red with strength, cells drop out
// as it weakens, a gauge round the rim shows the charge left, it flickers
// when nearly gone, hits ripple across it from where they land, and it
// flashes white as it collapses.

uniform float strength = 1.0;       // shield / shield_max
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
	float r = length(p);
	if (r > 1.0) {
		discard;
	}
	float z = sqrt(max(1.0 - r * r, 0.0));
	// rim light, strongest at the edge like a real bubble seen side-on
	float rim = pow(1.0 - z, 2.2);
	float edge_aa = 1.0 - smoothstep(0.965, 1.0, r);
	// cells in world proportions, a little bent toward the rim
	vec2 cp = p * aspect * 9.0 / (0.6 + 0.4 * z);
	vec3 h = hex(cp);
	float line = 1.0 - smoothstep(0.0, 0.07, h.x);
	// weaker shields lose cells; nearly gone, the rest flicker
	float cell = hash(h.yz);
	float intact = step(cell, strength * 1.15 - 0.05);
	float flick = strength < 0.25 ? 0.55 + 0.45 * step(0.4, hash(h.yz + floor(TIME * 18.0))) : 1.0;
	vec3 blue = vec3(0.35, 0.72, 1.0);
	vec3 amber = vec3(1.0, 0.78, 0.3);
	vec3 red = vec3(1.0, 0.3, 0.22);
	vec3 col = mix(red, mix(amber, blue, smoothstep(0.35, 0.7, strength)), smoothstep(0.1, 0.35, strength));
	// fill and rim stay faintly through lost cells so the bubble reads as
	// worn rather than holed; the cell lines go with the cells
	float fill = (0.04 + 0.34 * rim) * (0.4 + 0.6 * intact);
	float lines = 0.16 * line * intact * (0.4 + rim);
	// a gauge along the rim: lit from the bow back round both sides in
	// proportion to the charge, so a glance tells how much is left
	float band = smoothstep(0.885, 0.915, r) * (1.0 - smoothstep(0.945, 0.97, r));
	float around = abs(atan(p.y, p.x)) / 3.14159265;
	float gauge = band * (step(around, strength) * 0.55 + 0.08);
	float a = strength > 0.0 ? (fill + lines + gauge) * flick : 0.0;
	// hits: a flash where it landed and a ring running out across the bubble
	for (int i = 0; i < 4; i++) {
		vec4 hv = hits[i];
		if (hv.w <= 0.0 || hv.z > 0.9) {
			continue;
		}
		float d = length((p - hv.xy * 0.92) * aspect);
		float fade = 1.0 - hv.z / 0.9;
		float ring = exp(-pow((d - hv.z * 1.4) * 9.0, 2.0)) * fade;
		float flash = exp(-d * d * 22.0) * max(1.0 - hv.z / 0.22, 0.0);
		float glow = exp(-d * d * 6.0) * fade * fade;   // the cells round the impact light up
		float k = (ring * (0.5 + 1.4 * line) + glow * (0.15 + 1.2 * line) + flash * 2.0) * hv.w;
		a += k;
		col = mix(col, vec3(0.92, 0.97, 1.0), clamp((flash + glow * line * 0.6) * hv.w, 0.0, 1.0));
	}
	// collapse: a white flash, then the cells shatter one by one, red, and
	// the last of them go about half a second after the shield fell
	if (collapse < 0.5) {
		float t = collapse / 0.5;
		float gone = step(hash(h.yz * 1.7 + 3.1) * 0.9 + 0.1, t);
		float burst = (1.0 - gone) * (1.0 - t * 0.6) * (0.03 + 0.8 * line + 0.35 * rim);
		a += burst + (1.0 - smoothstep(0.0, 0.15, t)) * 0.3 * rim;
		col = mix(vec3(1.0, 0.35, 0.25), vec3(1.0, 0.95, 0.9), 1.0 - smoothstep(0.0, 0.3, t));
	}
	// respawn protection: a white shimmer
	if (guard > 0.0) {
		float sh = 0.5 + 0.5 * sin(TIME * 9.0 + (p.x + p.y) * 6.0);
		a += guard * (0.12 + 0.35 * rim) * (0.6 + 0.4 * sh);
		col = mix(col, vec3(1.0), guard * 0.7);
	}
	COLOR = vec4(col, clamp(a, 0.0, 0.85) * edge_aa * alpha);
}
