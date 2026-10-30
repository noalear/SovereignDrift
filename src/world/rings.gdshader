shader_type canvas_item;

// A planet's rings seen from above: a radial profile swept round the
// planet, dark where the planet's shadow falls across them.
uniform sampler2D profile : source_color, filter_linear;
// inner and outer edge, as fractions of the quad's half width
uniform float inner = 0.5;
// the planet's drawn radius, same units
uniform float body = 0.4;
// the direction away from the Sun (the shadow's), in the quad's frame
uniform vec2 away = vec2(1.0, 0.0);

void fragment() {
	vec2 p = UV * 2.0 - 1.0;
	float r = length(p);
	if (r > 1.0 || r < inner) {
		discard;
	}
	vec4 c = texture(profile, vec2((r - inner) / (1.0 - inner), 0.5));
	// fine ringlets, a little uneven round the circle
	float ang = atan(p.y, p.x);
	// (faded out when small on screen, before they alias)
	float lets = r * 520.0 + sin(ang * 3.0) * 0.6;
	c.a *= 1.0 - 0.14 * (1.0 - smoothstep(0.6, 2.5, fwidth(lets))) * (0.5 + 0.5 * sin(lets));
	// the shadow: a band as wide as the planet behind it
	float along = dot(p, away);
	float side = abs(p.x * away.y - p.y * away.x);
	float shade = smoothstep(0.0, body * 0.12, along) * (1.0 - smoothstep(body * 0.92, body * 1.0, side));
	c.rgb *= 1.0 - 0.75 * shade;
	// the lit face is a touch brighter than the far one
	c.rgb *= 1.0 + 0.12 * dot(p / max(r, 0.001), -away);
	COLOR = c;
}
