uniform sampler2D u_TextureMap;
uniform float u_MovementBlurStrength;

varying vec2 var_TexCoords;

void main()
{
	vec2 dir = var_TexCoords - vec2(0.5);
	float dist = length(dir);
	float falloff = smoothstep(0.05, 0.5, dist);
	float strength = u_MovementBlurStrength * falloff * 0.05;

	// Безопасное деление: в центре экрана dist == 0, и dir/dist
	// даёт NaN. max() даёт stepVec == (0,0), и результат равен
	// исходному пикселю — плавный переход без hard-cutoff'а.
	float safeDist = max(dist, 0.0001);
	vec2 stepVec = (dir / safeDist) * strength;

	vec3 sum = texture2D(u_TextureMap, var_TexCoords).rgb;
	float total = 1.0;

	float w1 = 0.75;
	sum += texture2D(u_TextureMap, var_TexCoords + stepVec * 1.0).rgb * w1;
	sum += texture2D(u_TextureMap, var_TexCoords - stepVec * 1.0).rgb * w1;
	total += w1 * 2.0;

	float w2 = 0.50;
	sum += texture2D(u_TextureMap, var_TexCoords + stepVec * 2.0).rgb * w2;
	sum += texture2D(u_TextureMap, var_TexCoords - stepVec * 2.0).rgb * w2;
	total += w2 * 2.0;

	float w3 = 0.25;
	sum += texture2D(u_TextureMap, var_TexCoords + stepVec * 3.0).rgb * w3;
	sum += texture2D(u_TextureMap, var_TexCoords - stepVec * 3.0).rgb * w3;
	total += w3 * 2.0;

	gl_FragColor = vec4(sum / total, 1.0);
};