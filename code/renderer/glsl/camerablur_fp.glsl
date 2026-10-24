uniform sampler2D u_TextureMap;
uniform sampler2D u_ScreenDepthMap;
uniform vec2      u_CameraBlurDir;
uniform float     u_CameraBlurNearThreshold;
uniform float     u_CameraBlurNearSoftness;
uniform vec4      u_CameraBlurSrcBounds; // xy = viewport min, zw = viewport max

varying vec2      var_TexCoords;

void main()
{
    vec2 dir = u_CameraBlurDir;
    float len = length(dir);

    if (len < 0.00001) {
        gl_FragColor = vec4(texture2D(u_TextureMap, var_TexCoords).rgb, 1.0);
        return;
    }

    // Viewmodel использует qglDepthRange(0, 0.3), т.е. вся его
    // геометрия имеет window-depth < 0.3. Гасим размаз для
    // пикселей ниже порога, чтобы оружие в руках оставалось
    // резким, а мир продолжал смазываться.
    float proximity = 1.0;
    if (u_CameraBlurNearThreshold > 0.0) {
        float depth = texture2D(u_ScreenDepthMap, var_TexCoords).r;
        proximity = smoothstep(u_CameraBlurNearThreshold,
                               u_CameraBlurNearThreshold + u_CameraBlurNearSoftness,
                               depth);
    }

    if (proximity < 0.002) {
        gl_FragColor = vec4(texture2D(u_TextureMap, var_TexCoords).rgb, 1.0);
        return;
    }

    vec2 effectiveDir = dir * proximity;

    // С r_renderScale != 100 сцена рендерится только в под-прямоугольник
    // полного render target'а, остальное остаётся чёрным. Выборка цвета
    // за пределами этого под-прямоугольника затягивает чёрную область
    // в кадр и даёт жёсткую чёрную полосу по краю вьюпорта. Прижимаем
    // каждый tap к границам под-прямоугольника — это эквивалентно тому,
    // что делает GL_CLAMP_TO_EDGE в случае полного вьюпорта.
    vec2 srcMin = u_CameraBlurSrcBounds.xy;
    vec2 srcMax = u_CameraBlurSrcBounds.zw;

    vec3  sum   = texture2D(u_TextureMap, var_TexCoords).rgb * 0.40;
    float total = 0.40;

    for (int i = 1; i <= 4; i++) {
        float t = float(i) / 4.0;
        float w = (1.0 - t) * 0.35;
        vec2 pos = clamp(var_TexCoords + effectiveDir * t, srcMin, srcMax);
        vec2 neg = clamp(var_TexCoords - effectiveDir * t, srcMin, srcMax);
        sum += texture2D(u_TextureMap, pos).rgb * w;
        sum += texture2D(u_TextureMap, neg).rgb * w;
        total += w * 2.0;
    }

    gl_FragColor = vec4(sum / total, 1.0);
}