#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

// Vulkan-style GLSL requires all uniforms to be inside a single std140
// block with binding = 0. Qt Quick passes custom properties as members of
// this block, matched by name via SPIR-V reflection.
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float u_time;
    vec2 u_resolution;
    int u_blobCount;
    vec4 u_baseColor;
    vec4 u_color1;
    vec4 u_color2;
    vec4 u_color3;
    vec4 u_color4;
    vec2 u_pos1;
    vec2 u_pos2;
    vec2 u_pos3;
    vec2 u_pos4;
    float u_radius1;
    float u_radius2;
    float u_radius3;
    float u_radius4;
    float u_energy;
    int u_rhythmEnabled;
};

const float GAMMA = 1.7;

void main()
{
    vec2 uv = qt_TexCoord0;

    vec4 colors[4] = vec4[](u_color1, u_color2, u_color3, u_color4);
    vec2 positions[4] = vec2[](u_pos1, u_pos2, u_pos3, u_pos4);
    float radii[4] = float[](u_radius1, u_radius2, u_radius3, u_radius4);

    int count = clamp(u_blobCount, 1, 4);
    float rhythmScale = (u_rhythmEnabled > 0) ? (1.0 + u_energy * 0.45) : 1.0;

    vec3 accum = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < count; ++i) {
        float d = distance(uv, positions[i]);
        float r = radii[i] * rhythmScale;
        float w = pow(max(0.0, 1.0 - d / r), 2.4);
        accum += colors[i].rgb * w;
        totalWeight += w;
    }

    vec3 blended = totalWeight > 0.001 ? (accum / totalWeight) : u_baseColor.rgb;
    float blend = clamp(totalWeight * 1.35, 0.0, 1.0);
    vec3 col = mix(u_baseColor.rgb, blended, blend);

    if (u_rhythmEnabled > 0) {
        col *= (1.0 + u_energy * 0.18);
    }

    // Gamma-like compression to keep transitions soft and avoid banding.
    col = pow(col, vec3(1.0 / GAMMA));

    fragColor = vec4(col, 1.0) * qt_Opacity;
}
