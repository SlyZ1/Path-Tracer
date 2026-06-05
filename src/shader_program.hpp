#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <vector>
#include <filesystem>

#ifndef SHADER_PROGRAM
#define SHADER_PROGRAM

namespace fs = std::filesystem;

using namespace std;

class ShaderProgram {
    private:
        static GLuint currentlyUsedProgram; 
        GLuint m_shaderProgram = 0;
        vector<GLuint> m_shaders = {};
        vector<const char*> m_paths = {};
        vector<int> m_types = {};
        string m_name;

        fs::path extractPath(const string& line);
        string getShaderSource(const char *path);

    public:
        ShaderProgram();
        GLuint id();
        void create();
        void load(int type, const char *path);
        void reload();
        void link();
        void use();
        void dispatch(GLuint x, GLuint y, GLuint z);
        void destroy();
        static void barrier(GLbitfield memBarrier);
        static void indirectBarrier();
        static void indirectDispatch(GLuint buffer, int offset = 0);
        static GLuint getVarLoc(const string& name);

        template<typename T>
        static tuple<unsigned int, unsigned int, unsigned int> 
        addData(const vector<T>& data, const vector<unsigned int>& indices){
            unsigned int VBO, VAO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            return {VBO, VAO, EBO};
        }

        static void linkData(int numCoords, int typesize, int layout, int glType = GL_FLOAT, int normalize = GL_FALSE){
            glVertexAttribPointer(layout, numCoords, glType, normalize, numCoords * typesize, (void*)0);
            glEnableVertexAttribArray(layout);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
};

#endif