// screen.gdshader -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
// The game's pixels arrive as bytes B,G,R,x (see SFProgram.raw_bgrx):
// swap red and blue and make every pixel opaque.
shader_type canvas_item;

void fragment() {
	COLOR = vec4(texture(TEXTURE, UV).bgr, 1.0);
}
