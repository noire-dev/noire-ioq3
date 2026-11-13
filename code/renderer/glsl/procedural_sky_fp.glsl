uniform vec3 u_SkyTopColor;
uniform vec3 u_SkyBottomColor;
uniform float u_SkyFadeBias;
uniform float u_SkyBrightness;
uniform vec3 u_DuskColor;
uniform float u_DuskIntensity;
uniform float u_DuskScale;
uniform float u_SunSize;
uniform vec3 u_SunColor;
uniform vec3 u_SunDirection;
uniform sampler2D u_SkyTexture;
uniform int u_UseSkyTexture;
uniform int u_StarsLayers;
uniform float u_StarsScale;
uniform float u_StarsFade;
uniform vec2 u_StarsSpeed;
uniform float u_Time;
varying vec3 var_SkyDirection;
void main() {
    const float PI = 3.14159265358979323846;
    vec3 dir = normalize(var_SkyDirection);
    float h = clamp(dir.z, 0.0, 1.0);
    float bias = clamp(u_SkyFadeBias, 0.05, 8.0);
    float a = pow(h, bias);
    float b = pow(1.0 - h, bias);
    float gradient = a / max(a + b, 0.00001);
    vec3 color = mix(u_SkyBottomColor, u_SkyTopColor, gradient);

    vec3 sunDir = normalize(u_SunDirection);
    float sunDot = clamp(dot(dir, sunDir), -1.0, 1.0);

    float sunElevation = asin(clamp(sunDir.z, -1.0, 1.0));
    float rayElevation = asin(clamp(dir.z, -1.0, 1.0));

    float duskScale = clamp(u_DuskScale, 2.0, 80.0);

    float horizonWidth = radians(clamp(duskScale * 0.75, 3.0, 40.0));
    float azimuthWidth = radians(clamp(duskScale * 2.0, 10.0, 150.0));
    float sunHaloWidth = radians(clamp(duskScale * 0.85, 3.0, 90.0));

    // Twilight: maximum near the horizon, including slightly below it.
    float twilight = exp(
        -pow((sunElevation - radians(-1.5)) / radians(8.0), 2.0)
    );

    // Atmospheric scattering along the horizon.
    float horizonGlow = exp(
        -pow(max(rayElevation, 0.0) / horizonWidth, 2.0)
    );

    // Calculate sun and sky azimuth.
    vec2 rayAzimuth = dir.xy;
    vec2 sunAzimuth = sunDir.xy;

    float rayAzimuthLength = length(rayAzimuth);
    float sunAzimuthLength = length(sunAzimuth);

    float azimuthAngle = PI;

    if (rayAzimuthLength > 0.0001 && sunAzimuthLength > 0.0001) {
        rayAzimuth /= rayAzimuthLength;
        sunAzimuth /= sunAzimuthLength;

        azimuthAngle = acos(
            clamp(dot(rayAzimuth, sunAzimuth), -1.0, 1.0)
        );
    }

    float azimuthGlow = exp(
        -pow(azimuthAngle / azimuthWidth, 2.0)
    );

    // Soft glow around the sun.
    float sunAngle = acos(sunDot);

    float sunHalo = exp(
        -pow(sunAngle / sunHaloWidth, 2.0)
    );

    // Combine atmospheric effects.
    float duskField = twilight * horizonGlow *
        (0.40 * azimuthGlow + 0.60 * sunHalo);

    float duskAmount = clamp(
        duskField * max(u_DuskIntensity, 0.0) * 0.85,
        0.0,
        0.85
    );

    vec3 duskTint = mix(color, u_DuskColor, 0.65);

    color = mix(color, duskTint, duskAmount);

    if (u_SunSize > 0.0) {
        float outerSun = cos(radians(clamp(u_SunSize, 0.001, 90.0)));
        float innerSun = cos(radians(clamp(u_SunSize * 0.80, 0.001, 90.0)));
        float sunMask = smoothstep(outerSun, innerSun, sunDot);
        color = mix(color, u_SunColor, sunMask);
    }

    if (u_UseSkyTexture != 0 && u_StarsFade > 0.0 && u_StarsLayers > 0) {
        float longitude = atan(dir.y, dir.x) / (2.0 * PI) + 0.5;
        float latitude = asin(clamp(dir.z, -1.0, 1.0)) / PI + 0.5;
        vec2 baseUV = vec2(longitude, latitude);
        vec3 layerColor = vec3(0.0);
        float layerAlpha = 0.0;
        float layerWeight = 0.0;
        int layerCount = u_StarsLayers;
        for (int layer = 0; layer < 8; ++layer) {
            if (layer >= layerCount) break;
            float layerIndex = float(layer);
            float layerScale = u_StarsScale * pow(1.7, layerIndex);
            vec2 uv = baseUV * layerScale + u_Time * u_StarsSpeed * (1.0 + layerIndex * 0.37);
            vec4 sampleColor = texture2D(u_SkyTexture, uv);
            layerColor += sampleColor.rgb * sampleColor.a;
            layerWeight += sampleColor.a;
            layerAlpha = max(layerAlpha, sampleColor.a);
        }
        vec3 overlay = layerColor / max(layerWeight, 0.0001);
        color = mix(color, overlay, clamp(layerAlpha * u_StarsFade, 0.0, 1.0));
    }

    gl_FragColor = vec4(color * u_SkyBrightness, 1.0);
}
