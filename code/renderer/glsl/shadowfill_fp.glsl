uniform sampler2D u_DiffuseMap;

uniform vec4  u_LightOrigin;
uniform float u_LightRadius;

uniform int   u_AlphaTest;
uniform float u_AlphaTestRef;

varying vec3 var_Position;
varying vec2 var_TexCoords;

void main()
{
	/*
	 * Alpha-tested materials such as foliage must not
	 * cast shadows through their transparent pixels.
	 */
	if (u_AlphaTest == U_ATEST_EQUAL)
	{
		if (texture2D(u_DiffuseMap, var_TexCoords).a != u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_GREATEREQUAL)
	{
		if (texture2D(u_DiffuseMap, var_TexCoords).a < u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_LESS)
	{
		if (texture2D(u_DiffuseMap, var_TexCoords).a >= u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_LESSEQUAL)
	{
		if (texture2D(u_DiffuseMap, var_TexCoords).a > u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_NOTEQUAL)
	{
		if (texture2D(u_DiffuseMap, var_TexCoords).a == u_AlphaTestRef)
			discard;
	}
	else if (u_AlphaTest == U_ATEST_GREATER)
	{
		if (texture2D(u_DiffuseMap, var_TexCoords).a <= u_AlphaTestRef)
			discard;
	}

#if defined(USE_DEPTH)

#if !defined(USE_SUN_SHADOW)
	float depth =
		length(u_LightOrigin.xyz - var_Position) /
		u_LightRadius;

	depth = clamp(depth, 0.0, 1.0);

	gl_FragDepth = depth;
#endif

#endif

	gl_FragColor = vec4(0.0);
}