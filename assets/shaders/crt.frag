#version 120

uniform sampler2D u_texture;
uniform vec2 u_sourceResolution;
uniform float u_time;
uniform float u_intensity;
uniform float u_glitch;
uniform float u_outputScale;

float random(vec2 coordinate)
{
    return fract(sin(dot(coordinate, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 barrel(vec2 uv, float amount)
{
    vec2 centered = uv * 2.0 - 1.0;
    float radius2 = dot(centered, centered);
    centered *= 1.0 + radius2 * (0.075 * amount);
    return centered * 0.5 + 0.5;
}

void main()
{
    float strength = clamp(u_intensity, 0.0, 1.0);
    vec2 uv = barrel(gl_TexCoord[0].xy, strength);

    // A short horizontal tear that only appears during deliberate glitch pulses.
    float tearBand = step(0.92, sin(uv.y * 47.0 + u_time * 21.0) * 0.5 + 0.5);
    float tearNoise = random(vec2(floor(u_time * 13.0), floor(uv.y * 38.0))) - 0.5;
    uv.x += tearBand * tearNoise * 0.018 * u_glitch;
    uv.y += sin(u_time * 31.0) * 0.00045 * u_glitch;

    vec2 inside = step(vec2(0.0), uv) * step(uv, vec2(1.0));
    float screenMask = inside.x * inside.y;
    vec2 texel = 1.0 / u_sourceResolution;

    // Slightly separated electron beams create color fringing on bright glyph edges.
    float separation = (0.65 + 1.75 * u_glitch) * strength;
    float red = texture2D(u_texture, uv + vec2(texel.x * separation, 0.0)).r;
    float green = texture2D(u_texture, uv).g;
    float blue = texture2D(u_texture, uv - vec2(texel.x * separation, 0.0)).b;
    vec3 color = vec3(red, green, blue);

    // A small phosphor bloom, kept directional to resemble a defocused beam.
    vec3 bloom = texture2D(u_texture, uv + vec2(texel.x * 1.8, 0.0)).rgb;
    bloom += texture2D(u_texture, uv - vec2(texel.x * 1.8, 0.0)).rgb;
    bloom += texture2D(u_texture, uv + vec2(0.0, texel.y * 1.5)).rgb;
    bloom += texture2D(u_texture, uv - vec2(0.0, texel.y * 1.5)).rgb;
    color += bloom * (0.095 * strength);

    // Two interleaved scan frequencies prevent the pattern looking perfectly digital.
    float scanA = 0.86 + 0.14 * sin(uv.y * u_sourceResolution.y * 3.14159265);
    float scanB = 0.95 + 0.05 * sin(uv.y * u_sourceResolution.y * 1.57079632 + u_time * 2.1);
    color *= mix(1.0, scanA * scanB, 0.78 * strength);

    // Fine RGB phosphor triads. The pattern follows output pixels, like a physical mask.
    float scale = max(u_outputScale, 0.5);
    float triadIndex = mod(floor(gl_FragCoord.x / scale), 3.0);
    vec3 triad = triadIndex < 1.0 ? vec3(1.07, 0.91, 0.91)
               : triadIndex < 2.0 ? vec3(0.91, 1.07, 0.91)
                                  : vec3(0.91, 0.91, 1.07);
    color *= mix(vec3(1.0), triad, 0.48 * strength);

    vec2 vignetteUv = uv * (1.0 - uv.yx);
    float vignette = pow(clamp(vignetteUv.x * vignetteUv.y * 17.0, 0.0, 1.0), 0.22);
    color *= mix(1.0, vignette, 0.68 * strength);

    float noise = random(gl_FragCoord.xy + vec2(floor(u_time * 60.0), u_time)) - 0.5;
    float flicker = 1.0 + sin(u_time * 43.0) * 0.007 * strength;
    color = color * flicker + noise * (0.018 * strength + 0.035 * u_glitch);

    // A dim rolling band and a brief exposure surge complete the analog instability.
    float roll = exp(-pow(fract(uv.y - u_time * 0.065) - 0.5, 2.0) / 0.0028);
    color += vec3(0.018, 0.045, 0.022) * roll * strength;
    color *= 1.0 + u_glitch * 0.14;

    // Soft black edge around the curved glass.
    float edge = smoothstep(0.0, 0.012, uv.x) * smoothstep(0.0, 0.012, uv.y) *
                 smoothstep(0.0, 0.012, 1.0 - uv.x) * smoothstep(0.0, 0.012, 1.0 - uv.y);
    color *= screenMask * edge;
    gl_FragColor = vec4(max(color, vec3(0.0)), gl_Color.a);
}
