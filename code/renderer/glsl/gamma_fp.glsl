uniform sampler2D u_TextureMap;
uniform float u_Gamma;

varying vec2 var_TexCoords;

void main()
{
	vec4 color = texture2D(u_TextureMap, var_TexCoords);

	color.rgb = pow(max(color.rgb, vec3(0.0)), vec3(1.0 / u_Gamma));

	gl_FragColor = color;
}