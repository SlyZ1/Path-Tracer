#version 430 core

in vec4 vClipPos;
out vec4 FragColor;
layout (location = 1) uniform sampler2D screenTexture;

uniform sampler2D selectionTexture;
uniform vec2 texSize;

vec3 toneMap(vec3 color, float exposure, float gamma) {
    color *= exposure;
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0/gamma));
}

void main()
{
    vec2 uv = (vClipPos.xy + vec2(1)) * 0.5;
    vec3 color = texture(screenTexture, uv).xyz;
    //color = toneMap(color, 2.0, 2.2);

    int outlineSize = 4;
    vec4 outlineColor = vec4(0.0);
    float centerSelection = texture(selectionTexture, uv).x;
    if (centerSelection < 0.5){
        for(int i = -outlineSize+1; i <= outlineSize-1; i++) {
            for(int j = -outlineSize+1; j <= outlineSize-1; j++) {
                float selection = texture(selectionTexture, uv + vec2(i, j) / texSize).x;
                if (selection > 0.5) {
                    outlineColor.rgb = vec3(1.0, 0.68, 0.0);
                    outlineColor.a += 1.0 / (outlineSize * outlineSize);
                }
            }
        }
    }

    FragColor = vec4(mix(color, outlineColor.rgb, clamp(outlineColor.a, 0.0, 1.0)), 1.0);
}