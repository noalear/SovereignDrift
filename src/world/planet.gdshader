shader_type canvas_item;

// A lit sphere with a procedural surface.  kind: 0 rocky, 1 gas giant,
// 2 ice world, 3 living world, 4 star, 5 asteroid, 6 moon.
uniform int kind = 0;
uniform vec4 color_a : source_color = vec4(0.6, 0.4, 0.3, 1.0);
uniform vec4 color_b : source_color = vec4(0.3, 0.2, 0.15, 1.0);
uniform vec4 color_c : source_color = vec4(0.2, 0.4, 0.8, 1.0);
uniform vec2 light_dir = vec2(1.0, 0.0);
uniform float seed = 1.0;
uniform float atmosphere = 0.0;
uniform float spin = 0.02;
// half the drawn quad, in body radii
uniform float extent = 1.25;

float hash(vec3 p) {
	p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3) + seed);
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash(i + vec3(0, 0, 0)), hash(i + vec3(1, 0, 0)), f.x),
	               mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
	               mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float fbm(vec3 p) {
	float v = 0.0;
	float a = 0.5;
	for (int i = 0; i < 5; i++) {
		v += a * noise(p);
		p = p * 2.03 + vec3(1.7, 9.2, 3.1);
		a *= 0.5;
	}
	return v;
}

void fragment() {
	vec2 uv = UV * 2.0 - 1.0;
	float r2 = dot(uv, uv);
	float r = sqrt(r2);
	vec4 col = vec4(0.0);
	if (kind == 4) {
		// star: limb-darkened photosphere, corona and flares that fade out
		// well inside the quad, so its square edge never shows
		float rr = r * extent;          // distance in body radii
		float t = TIME * 0.15;
		float n = fbm(vec3(uv * extent * 2.4, t));
		float core = smoothstep(1.0, 0.9, rr);
		float limb = sqrt(max(1.0 - rr * rr, 0.0));
		vec3 disc = mix(color_b.rgb, color_a.rgb, pow(limb, 0.45)) + vec3(1.0, 0.9, 0.7) * limb * 0.4 * n;
		float fade = smoothstep(extent, extent * 0.5, rr);
		float glow = exp(-max(rr - 0.95, 0.0) * 3.2) * (0.75 + 0.35 * n) * fade;
		vec3 c = mix(color_a.rgb * 0.9 + color_b.rgb * 0.1, disc, core);
		float a = max(core, glow);
		float flare = fbm(vec3(atan(uv.y, uv.x) * 2.0, rr * 4.8 - t * 2.0, seed)) * smoothstep(1.7, 0.95, rr) * (1.0 - core);
		c += vec3(1.0, 0.8, 0.5) * flare * 0.5;
		a = clamp(a + flare * 0.5 * fade, 0.0, 1.0);
		COLOR = vec4(c, a);
	} else {
		float body = 1.0 - smoothstep(0.94, 0.97, r);
		float halo = atmosphere * exp(-max(r - 0.94, 0.0) * 14.0) * (1.0 - body);
		if (body <= 0.0 && halo <= 0.001) {
			discard;
		}
		float z = sqrt(max(1.0 - r2 / 0.9, 0.0));
		vec3 n = normalize(vec3(uv / 0.95, z));
		vec3 l = normalize(vec3(light_dir, 0.45));
		float diff = clamp(dot(n, l), 0.0, 1.0);
		float term = smoothstep(-0.15, 0.35, dot(n, l));
		// surface coordinates on the sphere, slowly spinning
		vec3 sp = n * 2.2 + vec3(TIME * spin, 0.0, seed);
		vec3 surf;
		if (kind == 1) {
			// gas giant bands
			float band = sin(n.y * 9.0 + fbm(sp * 1.5) * 2.5 + seed) * 0.5 + 0.5;
			float storm = fbm(sp * 3.0 + vec3(0.0, 0.0, 5.0));
			surf = mix(color_a.rgb, color_b.rgb, band);
			surf = mix(surf, color_c.rgb, smoothstep(0.62, 0.8, storm) * 0.6);
		} else if (kind == 3) {
			// living world: oceans, continents, clouds, polar caps
			float h = fbm(sp * 1.4);
			float land = smoothstep(0.48, 0.52, h);
			surf = mix(color_c.rgb, color_a.rgb, land);
			surf = mix(surf, color_b.rgb, land * smoothstep(0.6, 0.75, h));
			float cloud = smoothstep(0.55, 0.72, fbm(sp * 2.6 + vec3(TIME * 0.04, 0.0, 3.0)));
			surf = mix(surf, vec3(1.0), cloud * 0.85);
			surf = mix(surf, vec3(0.95), smoothstep(0.75, 0.9, abs(n.y)));
		} else if (kind == 2) {
			float h = fbm(sp * 2.0);
			surf = mix(color_a.rgb, color_b.rgb, h);
			surf = mix(surf, vec3(1.0), smoothstep(0.6, 0.8, h) * 0.5);
		} else if (kind == 5) {
			float h = fbm(sp * 4.0);
			float crater = smoothstep(0.35, 0.3, noise(sp * 6.0));
			surf = mix(color_a.rgb, color_b.rgb, h) * (1.0 - crater * 0.4);
		} else {
			float h = fbm(sp * 2.4);
			float crater = (kind == 6) ? smoothstep(0.3, 0.26, noise(sp * 5.0)) : 0.0;
			surf = mix(color_a.rgb, color_b.rgb, h);
			surf = mix(surf, color_c.rgb, smoothstep(0.62, 0.78, h) * 0.5);
			surf *= 1.0 - crater * 0.35;
		}
		vec3 lit = surf * (0.08 + 0.92 * diff) * (0.3 + 0.7 * term);
		// night side city glow on living worlds
		if (kind == 3) {
			float city = smoothstep(0.52, 0.56, fbm(sp * 1.4)) * noise(sp * 30.0);
			lit += vec3(1.0, 0.8, 0.4) * city * (1.0 - term) * 0.6;
		}
		// rim light / atmosphere
		float rim = pow(1.0 - z, 3.0);
		lit += color_c.rgb * rim * atmosphere * 0.8 * (0.3 + 0.7 * term);
		vec3 hcol = color_c.rgb * (0.3 + 0.7 * term);
		col = vec4(mix(hcol, lit, body), max(body, halo * 0.8));
		COLOR = col;
	}
}
