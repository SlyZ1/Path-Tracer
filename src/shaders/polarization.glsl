mat4 muellerRotator(float theta){
    float s = sin(2.0 * theta);
    float c = cos(2.0 * theta);
    return transpose(mat4(
        1, 0, 0, 0,
        0, c, s, 0,
        0, -s, c, 0,
        0, 0, 0, 0
    ));
}