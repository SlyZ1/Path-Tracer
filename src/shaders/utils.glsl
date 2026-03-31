vec2 ratio(vec2 vec){
    return vec2(vec.x * texSize.x / texSize.y, vec.y);
}

vec3 reflect(vec3 I, vec3 N) {
    return I - 2.0 * dot(I, N) * N;
}

vec3 refract(vec3 I, vec3 N, float n) {
    vec3 r_perp = n * (I - N * dot(N, I));
    vec3 r_para = - sqrt(abs(1 - dot(r_perp, r_perp))) * N;
    return normalize(r_perp + r_para);
}

float luminanceMean(vec3 c){
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void stop(inout Hit hit, bool touchedLight){
    hit.t = touchedLight ? -2 : -1;
}