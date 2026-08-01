#version 430 core

in vec4 vClipPos;
out vec4 FragColor;
layout (location = 1) uniform sampler2D screenTex;

vec3 toneMap(vec3 color, float exposure, float gamma) {
    color *= exposure;
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0/gamma));
}

void main()
{
    vec2 uv = (vClipPos.xy + vec2(1)) * 0.5;
    vec3 color = texture(screenTex, uv).xyz;
    //color = toneMap(color, 2.0, 2.2);
    FragColor = vec4(color, 1.0);
}