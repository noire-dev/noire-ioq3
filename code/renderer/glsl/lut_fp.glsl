uniform sampler2D u_TextureMap;
uniform sampler3D u_LUTMap;
varying vec2 var_TexCoords;
void main()
{
	vec4 color = texture2D(u_TextureMap, var_TexCoords);
	vec3 lut = texture3D(u_LUTMap, clamp(color.rgb, 0.0, 1.0)).rgb;
	gl_FragColor = vec4(lut, color.a);
}