#ifndef MATERIAL
#define MATERIAL

#include <glm/glm.hpp>
#include "rgbspectrum_srgb.hpp"
#include <iostream>
using namespace glm;

enum MatType : int {
    DIFFUSE = 0,
    METAL = 1,
    GLASS = 2,
    GLOSSY = 3,
    EMIT = 4
};

struct Material {
    vec3 color;
    MatType type;
    vec4 data;
    vec4 data2;

    static Material diffuseMaterial(vec3 color, float roughness);
    static Material metalMaterial(
        vec3 color, 
        float fuzziness,
        float filmIOR = 1.0f,
        float filmDepth = 0.0f);
    static Material glassMaterial(
        vec3 color, 
        float fuzziness, 
        float refractionIndex = 1.3f,
        float dispertionFactor = 0.0f,
        float absorptionFactor = 1.0f, 
        float scatteringFactor = 0.0f);
    static Material glossyMaterial(vec3 color, float fuzziness, float metallic);
    static Material emitMaterial(vec3 color, float intensity);
    static vec3 rgbToSigmoidCoeffs(vec3 rgb);

    Material operator+(const Material& other){
        return {
            color + other.color,
            type,
            data + other.data,
            data2 + other.data2,
        };
    }

    Material operator*(float t){
        return {
            color * t,
            type,
            data * t,
            data2 * t
        };
    }

    bool operator==(const Material& other) const {
        bool dataEquals = false;
        switch(type){
            case MatType::DIFFUSE:
                dataEquals = data.x == other.data.x;
                break;
            case MatType::EMIT:
                dataEquals = data.x == other.data.x;
                break;
            case MatType::GLOSSY:
                dataEquals = all(equal(data * vec4(1,1,0,0), other.data * vec4(1,1,0,0)));
                break;
            case MatType::METAL:
                dataEquals = all(equal(data * vec4(1,0,0,0), other.data * vec4(1,0,0,0)));
                dataEquals = dataEquals && all(equal(data2 * vec4(1,1,0,0), other.data2 * vec4(1,1,0,0)));
                break;
            case MatType::GLASS:
                dataEquals = all(equal(data, other.data));
                dataEquals = dataEquals && all(equal(data2 * vec4(1,0,0,0), other.data2 * vec4(1,0,0,0)));
                break;
            default:
                break;
        }
        return color == other.color && type == other.type && dataEquals;
    }
};

#endif