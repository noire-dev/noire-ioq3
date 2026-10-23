uniform sampler2D u_DiffuseMap;

#if defined(USE_LIGHTMAP)
uniform sampler2D u_LightMap;
#endif

uniform sampler2D u_NormalMap;
uniform sampler2D u_DeluxeMap;
uniform sampler2D u_SpecularMap;

#if defined(USE_SHADOWMAP)
uniform sampler2D u_ShadowMap;
#endif

#if defined(USE_DLIGHT_SHADOWMAP)
uniform samplerCube u_DlightShadowMap;
#endif

#if defined(USE_CUBEMAP)
uniform samplerCube u_CubeMap;
#endif

uniform vec4      u_EnableTextures; 

#if defined(USE_SHADOWMAP)
uniform vec3  u_PrimaryLightColor;
uniform vec3  u_PrimaryLightAmbient;
#endif

#if defined(USE_LIGHT)
uniform vec4      u_NormalScale;
uniform vec4      u_SpecularScale;
#endif

#if defined(USE_LIGHT)
#if defined(USE_CUBEMAP)
uniform vec4      u_CubeMapInfo;
uniform vec3      u_CubeMapParallaxInfo;
#endif
#endif

uniform int       u_AlphaTest;
uniform int 	  u_AlphaGen;

varying vec4      var_TexCoords;

varying vec4      var_Color;
#if defined(USE_LIGHT)
varying vec4      var_ColorAmbient;
#endif

#if defined(USE_LIGHT)
varying vec4   var_Normal;
varying vec4   var_Tangent;
varying vec4   var_Bitangent;
#endif

#if defined(USE_LIGHT)
varying vec4      var_LightDir;
#endif

#if defined(USE_SHADOWMAP)
varying vec4      var_PrimaryLightDir;
#endif

#if defined(USE_DLIGHT_SHADOWMAP)
varying vec3      var_WorldPosition;
uniform vec4      u_LightOrigin;
uniform float     u_LightRadius;
#endif


#define EPSILON 0.00000001

#if defined(USE_PARALLAXMAP)
float SampleDepth(sampler2D normalMap, vec2 t)
{
  #if defined(SWIZZLE_NORMALMAP)
	return 1.0 - texture2D(normalMap, t).r;
  #else
	return 1.0 - texture2D(normalMap, t).a;
  #endif
}

float RayIntersectDisplaceMap(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	// current size of search window
	float size = 1.0 / float(linearSearchSteps);

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// texture depth at best depth
	float texDepth = 0.0;

	float prevT = SampleDepth(normalMap, dp);
	float prevTexDepth = prevT;

	// search front to back for first point inside object
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;
		
		float t = SampleDepth(normalMap, dp + ds * depth);
		
		if(bestDepth > 0.996)		// if no depth found yet
			if(depth >= t)
			{
				bestDepth = depth;	// store best depth
				texDepth = t;
				prevTexDepth = prevT;
			}
		prevT = t;
	}

	depth = bestDepth;

#if !defined (USE_RELIEFMAP)
	float div = 1.0 / (1.0 + (prevTexDepth - texDepth) * float(linearSearchSteps));
	bestDepth -= (depth - size - prevTexDepth) * div;
#else
	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		float t = SampleDepth(normalMap, dp + ds * depth);
		
		if(depth >= t)
		{
			bestDepth = depth;
			depth -= 2.0 * size;
		}

		depth += size;
	}
#endif

	return bestDepth;
}

float LightRay(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;

	// current size of search window
	float size = 1.0 / float(linearSearchSteps);

	// current height from initial texel depth
	float height = 0.0;

	float startDepth = SampleDepth(normalMap, dp);

	// find a collision or escape
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		height += size;

		if (startDepth < height)
			return 1.0;
		
		float t = SampleDepth(normalMap, dp + ds * height);

		if (startDepth > t + height)
			return 0.0;
	}

	return 1.0;
}
#endif

vec3 CalcDiffuse(vec3 diffuseAlbedo, float NH, float EH, float roughness)
{
	return diffuseAlbedo;
}

vec3 EnvironmentBRDF(float roughness, float NE, vec3 specular)
{
	// from http://community.arm.com/servlet/JiveServlet/download/96891546-19496/siggraph2015-mmg-renaldas-slides.pdf
	float v = 1.0 - max(roughness, NE);
	v *= v * v;
	return vec3(v) + specular;
}

vec3 CalcSpecular(vec3 specular, float NH, float EH, float roughness)
{
	// from http://community.arm.com/servlet/JiveServlet/download/96891546-19496/siggraph2015-mmg-renaldas-slides.pdf
	float rr = roughness*roughness;
	float rrrr = rr*rr;
	float d = (NH * NH) * (rrrr - 1.0) + 1.0;
	float v = (EH * EH) * (roughness + 0.5) + EPSILON;
	return specular * (rrrr / (4.0 * d * d * v));
}


float CalcLightAttenuation(float point, float normDist)
{
	// zero light at 1.0, approximating q3 style
	// also don't attenuate directional light
	float attenuation = (0.5 * normDist - 1.5) * point + 1.0;

	// clamp attenuation
	#if defined(NO_LIGHT_CLAMP)
	attenuation = max(attenuation, 0.0);
	#else
	attenuation = clamp(attenuation, 0.0, 1.0);
	#endif

	return attenuation;
}

#if defined(USE_DLIGHT_SHADOWMAP)

float randomDlight(vec2 p)
{
	const vec2 r = vec2(
		23.1406926327792690,
		2.6651441426902251
	);

	return mod(
		123456789.0,
		1e-7 + 256.0 * dot(p, r)
	);
}

float CalcDlightShadow()
{
	vec3 toSurface =
		var_WorldPosition - u_LightOrigin.xyz;

	float distanceToLight = length(toSurface);

	if (distanceToLight <= 0.0001)
		return 1.0;

	float currentDepth =
		distanceToLight / u_LightRadius;

	if (currentDepth >= 1.0)
		return 1.0;

	vec3 direction =
		-toSurface / distanceToLight;

	const float bias = 0.0015;
	const float filterSize = 0.015;

	/*
	 * Build a tangent space around the cubemap direction.
	 */
	vec3 tangent = normalize(cross(
		direction,
		abs(direction.z) < 0.999
			? vec3(0.0, 0.0, 1.0)
			: vec3(0.0, 1.0, 0.0)
	));

	vec3 bitangent = cross(direction, tangent);

	/*
	 * Same Poisson kernel as the sun shadow PCF.
	 */
	float r = randomDlight(
		var_WorldPosition.xy
	);

	float sinr = sin(r) * filterSize;
	float cosr = cos(r) * filterSize;

	mat2 rmat = mat2(
		cosr, sinr,
		-sinr, cosr
	);

	float shadow = 0.0;
	vec2 offset;

	offset = rmat * vec2(-0.7055767, 0.1965150);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(0.3524343, -0.7791386);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(0.2391056, 0.9189604);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(-0.07580382, -0.09224417);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(0.5784913, -0.002528916);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(0.1928880, 0.4064181);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(-0.6335801, -0.5247476);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(-0.5579782, 0.7491854);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	offset = rmat * vec2(0.7320465, 0.6317794);
	shadow += currentDepth <= textureCube(
		u_DlightShadowMap,
		normalize(direction + tangent * offset.x + bitangent * offset.y)
	).r + bias ? 1.0 : 0.0;

	return shadow * 0.111111;
}

#endif

void main()
{
	vec3 viewDir, lightColor, ambientColor, reflectance;
	vec3 L, N, E, H;
	float NL, NH, NE, EH, attenuation;

#if defined(USE_LIGHT)
	vec3 surfNormal = (!gl_FrontFacing ? var_Normal : -var_Normal).xyz;
	mat3 tangentToWorld = mat3(var_Tangent.xyz, var_Bitangent.xyz, surfNormal);
	viewDir = vec3(var_Normal.w, var_Tangent.w, var_Bitangent.w);
	E = normalize(viewDir);
#endif

	lightColor = var_Color.rgb;

#if defined(USE_LIGHTMAP)
	vec4 lightmapColor = texture2D(u_LightMap, var_TexCoords.zw);
  #if defined(RGBM_LIGHTMAP)
	lightmapColor.rgb *= lightmapColor.a;
  #endif
	lightmapColor.rgb *= lightmapColor.rgb;
	lightColor *= lightmapColor.rgb;
#endif

	vec2 texCoords = var_TexCoords.xy;

#if defined(USE_PARALLAXMAP)
	vec3 offsetDir = E * tangentToWorld;

	offsetDir.xy *= -u_NormalScale.a / offsetDir.z;

	texCoords += offsetDir.xy * RayIntersectDisplaceMap(texCoords, offsetDir.xy, u_NormalMap);
#endif

	vec4 diffuse = texture2D(u_DiffuseMap, texCoords);
	
	float alpha = diffuse.a * var_Color.a;
	if (u_AlphaTest == 1)
	{
		if (alpha == 0.0)
			discard;
	}
	else if (u_AlphaTest == 2)
	{
		if (alpha >= 0.5)
			discard;
	}
	else if (u_AlphaTest == 3)
	{
		if (alpha < 0.5)
			discard;
	}

#if defined(USE_LIGHT)
	L = var_LightDir.xyz;
	L += (texture2D(u_DeluxeMap, var_TexCoords.zw).xyz - vec3(0.5)) * u_EnableTextures.y;
	float sqrLightDist = dot(L, L);
	L /= sqrt(sqrLightDist);

  #if defined(USE_LIGHT_VECTOR)
	attenuation  = CalcLightAttenuation(float(var_LightDir.w > 0.0), var_LightDir.w / sqrLightDist);
  #else
	attenuation  = 1.0;
  #endif

    #if defined(SWIZZLE_NORMALMAP)
	N.xy = texture2D(u_NormalMap, texCoords).ag - vec2(0.5);
    #else
	N.xy = texture2D(u_NormalMap, texCoords).rg - vec2(0.5);
    #endif
	N.xy *= u_NormalScale.xy;
	N.z = sqrt(clamp((0.25 - N.x * N.x) - N.y * N.y, 0.0, 1.0));
	N = tangentToWorld * N;

	N = normalize(N);

	float dlightShadow = 1.0;

#if defined(USE_DLIGHT_SHADOWMAP)
	dlightShadow = CalcDlightShadow();
#endif

  #if defined(USE_SHADOWMAP) 
	vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
	float shadowValue = texture2D(u_ShadowMap, shadowTex).r;

	// surfaces not facing the light are always shadowed
	shadowValue *= clamp(dot(N, var_PrimaryLightDir.xyz), 0.0, 1.0);

	lightColor *= shadowValue * (1.0 - u_PrimaryLightAmbient.r) + u_PrimaryLightAmbient.r;
  #endif

  #if defined(USE_PARALLAXMAP) && defined(USE_PARALLAXMAP_SHADOWS)
	offsetDir = L * tangentToWorld;
	offsetDir.xy *= u_NormalScale.a / offsetDir.z;
	lightColor *= LightRay(texCoords, offsetDir.xy, u_NormalMap);
  #endif


  #if !defined(USE_LIGHT_VECTOR)
	ambientColor = lightColor;
	float surfNL = clamp(dot(surfNormal, L), 0.0, 1.0);

	// reserve 25% ambient to avoid black areas on normalmaps
	lightColor *= 0.75;

	// Scale the incoming light to compensate for the baked-in light angle
	// attenuation.
	lightColor /= max(surfNL, 0.25);

	// Recover any unused light as ambient, in case attenuation is over 4x or
	// light is below the surface
	ambientColor = max(ambientColor - lightColor * surfNL, vec3(0.0));
  #else
	ambientColor = var_ColorAmbient.rgb;
  #endif

	NL = clamp(dot(N, L), 0.0, 1.0);
	NE = clamp(dot(N, E), 0.0, 1.0);
	H = normalize(L + E);
	EH = clamp(dot(E, H), 0.0, 1.0);
	NH = clamp(dot(N, H), 0.0, 1.0);

	vec4 specular = texture2D(u_SpecularMap, texCoords);
	specular *= u_SpecularScale;

	diffuse.rgb *= diffuse.rgb;

	// diffuse rgb is base color
	// specular red is gloss
	// specular green is metallicness
	float gloss = specular.r;
	float metal = specular.g;
	specular.rgb = metal * diffuse.rgb;
	diffuse.rgb *= 1.0 - metal;

	float roughness = exp2(-3.0 * gloss);
	reflectance  = CalcDiffuse(diffuse.rgb, NH, EH, roughness);

  #if defined(r_deluxeSpecular)
    #if defined(USE_LIGHT_VECTOR)
	reflectance += CalcSpecular(specular.rgb, NH, EH, roughness) * r_deluxeSpecular;
    #else
	reflectance += CalcSpecular(specular.rgb, NH, EH, pow(roughness, r_deluxeSpecular));
    #endif
  #endif

	gl_FragColor.rgb = lightColor * reflectance * (attenuation * NL) * dlightShadow;
	gl_FragColor.rgb += ambientColor * diffuse.rgb;
    
  #if defined(USE_CUBEMAP)
	reflectance = EnvironmentBRDF(roughness, NE, specular.rgb);

	vec3 R = reflect(E, N);
	vec3 modifiedR = R;
	modifiedR.z -= (u_CubeMapInfo.y - u_CubeMapInfo.x) / ((u_CubeMapInfo.z + 0.5) + u_CubeMapInfo.w);
	vec3 parallax = u_CubeMapParallaxInfo.xyz + 1.0f / 65536 * viewDir;
	vec3 cubeLightColor = textureCubeLod(u_CubeMap, modifiedR + parallax, ROUGHNESS_MIPS * roughness).rgb * u_EnableTextures.w;

	cubeLightColor *= cubeLightColor;

	gl_FragColor.rgb += cubeLightColor * reflectance;
  #endif

  #if defined(USE_SHADOWMAP)
	vec3 L2, H2;
	float NL2, EH2, NH2;

	L2 = var_PrimaryLightDir.xyz;

	// enable when point lights are supported as primary lights
	//sqrLightDist = dot(L2, L2);
	//L2 /= sqrt(sqrLightDist);

	NL2 = clamp(dot(N, L2), 0.0, 1.0);
	H2 = normalize(L2 + E);
	EH2 = clamp(dot(E, H2), 0.0, 1.0);
	NH2 = clamp(dot(N, H2), 0.0, 1.0);

	reflectance  = CalcSpecular(specular.rgb, NH2, EH2, roughness);

	// bit of a hack, with modulated shadowmaps, ignore diffuse
    #if !defined(SHADOWMAP_MODULATE)
	reflectance += CalcDiffuse(diffuse.rgb, NH2, EH2, roughness);
    #endif

	lightColor = u_PrimaryLightColor;
	lightColor *= shadowValue;

	// enable when point lights are supported as primary lights
	//lightColor *= CalcLightAttenuation(float(u_PrimaryLightDir.w > 0.0), u_PrimaryLightDir.w / sqrLightDist);

  #if defined(USE_PARALLAXMAP) && defined(USE_PARALLAXMAP_SHADOWS)
	offsetDir = L2 * tangentToWorld;
	offsetDir.xy *= u_NormalScale.a / offsetDir.z;
	lightColor *= LightRay(texCoords, offsetDir.xy, u_NormalMap);
  #endif

	gl_FragColor.rgb += lightColor * reflectance * NL2;
  #endif

	gl_FragColor.rgb = sqrt(gl_FragColor.rgb);

#else

	gl_FragColor.rgb = diffuse.rgb * lightColor;

#endif

	#if defined(USE_LIGHT)
if (u_AlphaGen == AGEN_WATER) {
// Schlick-френель: на скользящих углах alpha растёт к 1
float waterFresnel = 0.02 + 0.98 * pow(1.0 - NE, 5.0);
gl_FragColor.a = clamp(0.1 + 0.9 * waterFresnel, 0.0, 1.0) * alpha;
} else {
gl_FragColor.a = alpha;
}
#else
gl_FragColor.a = alpha;
#endif
}
