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
// a painted map of the whole surface (equirectangular: longitude across,
// north pole at the top), used instead of the procedural one when set
uniform bool has_map = false;
uniform sampler2D surface_map : source_color, filter_linear_mipmap, repeat_enable;

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

// 2D cellular noise: x = distance to the nearest cell centre, y = to
// the second nearest (y - x is small along the lanes between cells)
vec2 voronoi(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	float d1 = 8.0;
	float d2 = 8.0;
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 g = vec2(float(x), float(y));
			vec2 o = vec2(hash(vec3(i + g, 1.0)), hash(vec3(i + g, 7.0)));
			float d = length(g + o - f);
			if (d < d1) {
				d2 = d1;
				d1 = d;
			} else if (d < d2) {
				d2 = d;
			}
		}
	}
	return vec2(d1, d2);
}

void fragment() {
	vec2 uv = UV * 2.0 - 1.0;
	if (kind != 4) {
		// planets were laid out for a 1.25-radius quad; the quad is now
		// larger so the atmosphere can fade out before its edge
		uv *= extent / 1.25;
	}
	float r2 = dot(uv, uv);
	float r = sqrt(r2);
	vec4 col = vec4(0.0);
	if (kind == 4) {
		// star: a granulated photosphere (convection cells with dark lanes
		// between them), slow mottling, a few sunspots with penumbrae,
		// limb darkening from white-gold to deep orange, then prominences,
		// a corona and streamers that fade out well inside the quad
		float rr = r * extent;          // distance in body radii
		float t = TIME * 0.15;
		vec3 c = vec3(0.0);
		float a = 0.0;
		if (rr < 1.0) {
			float mu = sqrt(max(1.0 - rr * rr, 0.0));
			// surface coordinates squeezed toward the limb like a sphere's
			vec2 s = uv * extent / (0.35 + mu) * 1.3 + vec2(seed * 0.37, seed * 0.11);
			// granules: small convection cells, low contrast, two sizes;
			// faded out where they would be smaller than a few pixels
			vec2 g1 = s * 46.0 + vec2(t * 0.6, -t * 0.4);
			float sharp = 1.0 - smoothstep(0.12, 0.35, length(fwidth(g1)));
			vec2 cells = voronoi(g1);
			vec2 cells2 = voronoi(g1 * 0.43 + vec2(5.0, 1.0));
			float gran = smoothstep(0.0, 0.5, cells.y - cells.x) * 0.6 + smoothstep(0.0, 0.6, cells2.y - cells2.x) * 0.4;
			gran = mix(0.6, gran, sharp);
			float mott = fbm(vec3(s * 3.0, t * 0.5));
			float spots = fbm(vec3(s * 1.7 + vec2(11.0, 3.0), seed + t * 0.05));
			float umbra = smoothstep(0.75, 0.77, spots);
			float penum = smoothstep(0.71, 0.75, spots) - umbra;
			float bright = 1.0 + 0.14 * gran + 0.22 * (mott - 0.5);
			// faculae: bright flecks near the limb
			bright += smoothstep(0.62, 0.8, fbm(vec3(s * 6.0, t))) * (1.0 - mu) * 0.3;
			bright *= 1.0 - penum * 0.35 - umbra * 0.75;
			float limb = 0.45 + 0.55 * pow(mu, 0.5);
			vec3 hot = vec3(1.0, 0.84, 0.5);
			vec3 warm = vec3(1.0, 0.56, 0.16);
			vec3 deep = vec3(0.86, 0.28, 0.05);
			vec3 disc = mix(deep, mix(warm, hot, pow(mu, 1.6)), smoothstep(0.0, 0.45, mu)) * bright * limb;
			disc = mix(disc, vec3(0.28, 0.07, 0.01), umbra * 0.85);
			disc = mix(disc, disc * vec3(0.9, 0.55, 0.3), penum * 0.6);
			// soft anti-aliased edge
			float edge = smoothstep(1.0, 0.985, rr);
			c = disc;
			a = edge;
		}
		float ang = atan(uv.y, uv.x);
		float fade = smoothstep(extent, extent * 0.5, rr);
		float out_r = max(rr - 1.0, 0.0);
		// corona: a hot inner glow and long faint streamers
		float n = fbm(vec3(cos(ang) * 2.0, sin(ang) * 2.0, t * 0.6 + seed));
		float glow = exp(-out_r * 4.5) * (0.65 + 0.4 * n);
		float streak = fbm(vec3(ang * 3.0, seed, t * 0.3)) ;
		glow += exp(-out_r * 1.6) * smoothstep(0.5, 0.8, streak) * 0.35;
		// prominences: arcs of hot gas standing just off the limb
		float prom = fbm(vec3(ang * 9.0, out_r * 9.0 - t * 1.5, seed * 1.7));
		prom = smoothstep(0.62, 0.78, prom) * exp(-out_r * 9.0) * step(1.0, rr);
		vec3 corona = mix(vec3(1.0, 0.55, 0.18), vec3(1.0, 0.85, 0.55), exp(-out_r * 6.0));
		vec3 outer = corona * glow + vec3(1.0, 0.35, 0.12) * prom * 1.4;
		float oa = clamp(glow + prom, 0.0, 1.0) * fade;
		c = mix(outer, c, a);
		a = max(a, oa);
		COLOR = vec4(c, a);
	} else {
		if (kind == 5) {
			// minor bodies are lumpy, not round
			float ang = atan(uv.y, uv.x);
			float lump = fbm(vec3(cos(ang) * 1.3, sin(ang) * 1.3, seed * 0.37)) - 0.5;
			uv /= 1.0 + lump * 0.24;
			r2 = dot(uv, uv);
			r = sqrt(r2);
		}
		float body = 1.0 - smoothstep(0.94, 0.97, r);
		float halo = atmosphere * exp(-max(r - 0.94, 0.0) * 14.0) * (1.0 - body) * smoothstep(extent / 1.25, extent / 1.25 * 0.9, r);
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
		if (has_map) {
			// latitude and longitude of this point, the globe turning slowly;
			// of two equal longitudes the one without a jump here is used, so
			// the mipmap does not draw a seam where the map's ends meet
			float lon = atan(n.x, n.z) / TAU + TIME * spin * 0.08 + seed;
			float u1 = fract(lon);
			float u2 = fract(lon + 0.5) - 0.5;
			float u = fwidth(u1) <= fwidth(u2) + 0.0001 ? u1 : u2;
			float v = 0.5 + asin(clamp(n.y, -1.0, 1.0)) / PI;
			surf = texture(surface_map, vec2(u, v)).rgb;
			if (kind == 3) {
				float cloud = smoothstep(0.55, 0.75, fbm(sp * 2.6 + vec3(TIME * 0.04, 0.0, 3.0)));
				surf = mix(surf, vec3(1.0), cloud * 0.6);
			}
		} else if (kind == 1) {
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
			// dark regolith pocked with craters whose walls catch the light
			float c0 = noise(sp * 5.0);
			float h = fbm(sp * 3.0);
			vec3 e = vec3(0.04, 0.0, 0.0);
			float hx = fbm((sp + e.xyy) * 3.0) - h + (noise((sp + e.xyy) * 5.0) - c0) * 1.2;
			float hy = fbm((sp + e.yxy) * 3.0) - h + (noise((sp + e.yxy) * 5.0) - c0) * 1.2;
			vec3 nb = normalize(n - vec3(hx, hy, 0.0) * 2.0);
			diff = clamp(dot(nb, l), 0.0, 1.0);
			term = smoothstep(-0.05, 0.25, dot(nb, l));
			float crater = smoothstep(0.32, 0.26, c0);
			surf = mix(color_a.rgb, color_b.rgb, h) * (1.0 - crater * 0.25);
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
			// sparse points of light on the continents, not a speckled sheet
			float land_n = has_map ? smoothstep(0.0, 0.1, max(surf.r, surf.g) - surf.b) : smoothstep(0.52, 0.56, fbm(sp * 1.4));
			float city = land_n * smoothstep(0.72, 0.9, noise(sp * 38.0)) * smoothstep(0.3, 0.7, noise(sp * 6.0 + vec3(4.0)));
			lit += vec3(1.0, 0.82, 0.5) * city * (1.0 - term) * 0.8;
		}
		// rim light / atmosphere
		float rim = pow(1.0 - z, 3.0);
		lit += color_c.rgb * rim * atmosphere * 0.8 * (0.3 + 0.7 * term);
		vec3 hcol = color_c.rgb * (0.3 + 0.7 * term);
		col = vec4(mix(hcol, lit, body), max(body, halo * 0.8));
		COLOR = col;
	}
}
