vec4 schlickFresnel(float VdotN, vec4 F0)
{
    float minusDot = clamp(1 - VdotN, 0, 1);
    float dot2 = minusDot * minusDot;
    float dot5 = dot2 * dot2 * minusDot;
    return F0 + (vec4(1.0) - F0) * dot5;
}

vec3 schlickFresnel(float VdotN, vec3 F0)
{
    float minusDot = clamp(1 - VdotN, 0, 1);
    float dot2 = minusDot * minusDot;
    float dot5 = dot2 * dot2 * minusDot;
    return F0 + (vec3(1.0) - F0) * dot5;
}

float schlickFresnel(float VdotN, float F0)
{
    float minusDot = clamp(1 - VdotN, 0, 1);
    float dot2 = minusDot * minusDot;
    float dot5 = dot2 * dot2 * minusDot;
    return F0 + (1.0 - F0) * dot5;
}

vec4 fresnel(float cos1, vec4 cos2, vec4 n){
    vec4 Fp = (n * cos1 - cos2) / (n * cos1 + cos2);
    vec4 Fs = (vec4(cos1) - n * cos2) / (vec4(cos1) + n * cos2);
    return 0.5 * (Fp * Fp + Fs * Fs);
}

float fresnel(float cos1, float cos2, float n){
    float Fp = (n * cos1 - cos2) / (n * cos1 + cos2);
    float Fs = (cos1 - n * cos2) / (cos1 + n * cos2);
    return 0.5 * (Fp * Fp + Fs * Fs);
}

vec4 airy(float cosI, float filmN, float filmL, vec4 lambda, vec4 spectrumValue){
    float sinT2 = (1.0 - cosI*cosI) / (filmN*filmN);
    float cosT = sqrt(max(1.0 - sinT2, 0.0));

    vec4 r12 = vec4(sqrt(fresnel(cosI, cosT, filmN)));
    vec4 r23 = vec4(sqrt(schlickFresnel(cosT, spectrumValue)));

    vec4 phi = vec4(4.0 * PI * filmN * filmL * cosT) / lambda;
    
    vec4 num = r12*r12 + r23*r23 + 2.0*r12*r23*cos(phi);
    vec4 denom = 1 + r12*r12*r23*r23 + 2.0*r12*r23*cos(phi);

    return clamp(num / (denom + vec4(EPS)), 0.0, 1.0);
}