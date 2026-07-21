#ifndef UTILS_HPP
#define UTILS_HPP

#include <glm/glm.hpp>
#include <string>
#include <cstdio>
#include <format>

using namespace glm;
using namespace std;

class Utils {
public:
    static string toString(const glm::vec2& v, int precision = 3);
    static string toString(const glm::vec3& v, int precision = 3);
    static string toString(const glm::vec4& v, int precision = 3);
};

#endif