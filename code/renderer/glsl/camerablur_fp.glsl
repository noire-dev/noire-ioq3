uniform sampler2D u_TextureMap;
uniform sampler2D u_ScreenDepthMap;
uniform vec2      u_CameraBlurDir;
uniform float     u_CameraBlurNearThreshold;
uniform float     u_CameraBlurNearSoftness;

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

    vec3  sum   = texture2D(u_TextureMap, var_TexCoords).rgb * 0.40;
    float total = 0.40;

    for (int i = 1; i <= 4; i++) {
        float t = float(i) / 4.0;
        float w = (1.0 - t) * 0.35;
        sum += texture2D(u_TextureMap, var_TexCoords + effectiveDir * t).rgb * w;
        sum += texture2D(u_TextureMap, var_TexCoords - effectiveDir * t).rgb * w;
        total += w * 2.0;
    }

    gl_FragColor = vec4(sum / total, 1.0);
}