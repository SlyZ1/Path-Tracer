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

Spectrum fresnel(float cos1, Spectrum cos2, Spectrum n){
    Spectrum Fp = (n * cos1 - cos2) / (n * cos1 + cos2);
    Spectrum Fs = (Spectrum(cos1) - n * cos2) / (Spectrum(cos1) + n * cos2);
    return 0.5 * (Fp * Fp + Fs * Fs);
}

Spectrum fresnel(Spectrum cos1, Spectrum cos2, Spectrum n){
    Spectrum Fp = (n * cos1 - cos2) / (n * cos1 + cos2);
    Spectrum Fs = (cos1 - n * cos2) / (cos1 + n * cos2);
    return 0.5 * (Fp * Fp + Fs * Fs);
}

float fresnel(float cos1, float cos2, float n){
    float Fp = (n * cos1 - cos2) / (n * cos1 + cos2);
    float Fs = (cos1 - n * cos2) / (cos1 + n * cos2);
    return 0.5 * (Fp * Fp + Fs * Fs);
}

// https://jcgt.org/published/0003/04/03/paper.pdf
Spectrum artisticEta(Spectrum r, Spectrum g){
    r = min(r, 1 - EPS);
    Spectrum sqrtR = sqrt(r);
    return g * (1 - r) / (1 + r) + (1 - g) * (1 + sqrtR) / (1 - sqrtR);
}

Spectrum artisticK(Spectrum r, Spectrum eta){
    r = min(r, 1 - EPS);
    Spectrum etaP1 = eta + 1;
    Spectrum etaM1 = eta - 1;
    return sqrt((r * etaP1 * etaP1 - etaM1 * etaM1) / (1 - r));
}

float fresnelConductor(float cos, float eta, float k){
    float e2k2 = eta*eta + k*k;
    float cos2e = cos*2*eta;
    float cos2 = cos*cos;

    float Rp = (e2k2*cos2 - cos2e + 1) / (e2k2*cos2 + cos2e + 1);
    float Rt = (e2k2 - cos2e + cos2) / (e2k2 + cos2e + cos2);

    return 0.5 * (Rp + Rt);
}

Spectrum fresnelConductor(float cos, Spectrum eta, Spectrum k){
    Spectrum e2k2 = eta*eta + k*k;
    Spectrum cos2e = cos*2*eta;
    float cos2 = cos*cos;

    Spectrum Rp = (e2k2*cos2 - cos2e + 1) / (e2k2*cos2 + cos2e + 1);
    Spectrum Rt = (e2k2 - cos2e + cos2) / (e2k2 + cos2e + cos2);

    return 0.5 * (Rp + Rt);
}

#ifdef SPECTRAL
Spectrum airyConductor(float cosI, float filmN, float filmL, Spectrum lambda, Spectrum eta, Spectrum k){
    float sinT2 = (1.0 - cosI*cosI) / (filmN*filmN);
    if (sinT2 > 1 - EPS) return fresnelConductor(0, eta, k);
    float cosT = sqrt(max(1.0 - sinT2, 0.0));

    Spectrum r12 = Spectrum(sqrt(fresnel(cosI, cosT, filmN)));
    Spectrum r23 = Spectrum(sqrt(fresnelConductor(cosT, eta, k)));

    Spectrum phi = Spectrum(4.0 * PI * filmN * filmL * cosT) / lambda;
    
    Spectrum num = r12*r12 + r23*r23 + 2.0*r12*r23*cos(phi);
    Spectrum denom = 1 + r12*r12*r23*r23 + 2.0*r12*r23*cos(phi);

    return clamp(num / denom, 0.0, 1.0);
}

Spectrum airyDielectric(float cosI, float filmN, float filmL, Spectrum lambda, SpectralParam glassN, bool inside){
    SpectralParam n1 = inside ? glassN : SpectralParam(1.0);
    float n2 = filmN;
    SpectralParam n3 = inside ? SpectralParam(1.0) : glassN;
    
    SpectralParam sinT2 = (n1*n1 / (n2*n2)) * (1.0 - cosI*cosI);
    if (any(greaterThan(sinT2, SpectralParam(1.0 - EPS)))) return Spectrum(1.0);
    SpectralParam cosT = sqrt(max(1.0 - sinT2, 0.0));

    SpectralParam sinO2 = (n2*n2 / (n3*n3)) * (1.0 - cosT*cosT);
    SpectralParam cosO = sqrt(max(1.0 - sinO2, 0.0));

    Spectrum r12 = Spectrum(sqrt(fresnel(cosI, cosT, n2/n1)));
    Spectrum r23 = Spectrum(sqrt(fresnel(cosT, cosO, n3/n2)));

    Spectrum phi = Spectrum(4.0 * PI * n2 * filmL * cosT) / lambda;
    
    Spectrum num = r12*r12 + r23*r23 + 2.0*r12*r23*cos(phi);
    Spectrum denom = 1.0 + r12*r12*r23*r23 + 2.0*r12*r23*cos(phi);

    return clamp(num / denom, 0.0, 1.0);
}
#endif