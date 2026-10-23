uniform sampler2D u_TextureMap;
uniform float     u_BloomThreshold;
uniform float     u_BloomKnee;

varying vec2      var_TexCoords;

const vec3 LUMA = vec3(0.2125, 0.7154, 0.0721);

void main()
{
	vec4 color = texture2D(u_TextureMap, var_TexCoords);
	float lum = dot(color.rgb, LUMA);

	// Soft-knee из Unreal-style bright-pass.
	// При knee == 0 превращается в жёсткий порог.
	float soft = clamp(lum - u_BloomThreshold + u_BloomKnee, 0.0, 2.0 * u_BloomKnee);
	soft = soft * soft / (4.0 * u_BloomKnee + 0.0001);

	float contribution = max(soft, lum - u_BloomThreshold);
	contribution = max(contribution, 0.0) / max(lum, 0.0001);

	gl_FragColor = vec4(color.rgb * contribution, 1.0);
}