#ifndef UTILS_HPP
#define UTILS_HPP

#include <glm/glm.hpp>
#include <string>
#include <cstdio>
#include <vector>

using namespace glm;
using namespace std;

class Utils {
public:
    static string toString(const glm::vec2& v, int precision = 3);
    static string toString(const glm::vec3& v, int precision = 3);
    static string toString(const glm::vec4& v, int precision = 3);
    
    template <typename T>
    static vector<T> concat(std::initializer_list<std::reference_wrapper<const vector<T>>> vectors){
        vector<T> result;
        for (const auto& vRef : vectors) {
            const vector<T>& v = vRef.get();
            result.insert(result.end(), v.begin(), v.end());
        }
        return result;
    }
};

#endif