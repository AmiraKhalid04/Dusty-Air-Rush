#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace our {

    class ShaderProgram {

    private:
        //Shader Program Handle (OpenGL object name)
        GLuint program;

    public:
        ShaderProgram(){
            program = glCreateProgram();
        }
        ~ShaderProgram(){
            glDeleteProgram(program);
        }

        bool attach(const std::string &filename, GLenum type) const;

        bool link() const;

        void use() { 
            glUseProgram(program);
        }

        GLuint getUniformLocation(const std::string &name) {
            return glGetUniformLocation(program, name.c_str());
        }

        void set(const std::string &uniform, GLfloat value) {
            glUniform1f(glGetUniformLocation(program, uniform.c_str()), value);
        }

        void set(const std::string &uniform, GLuint value) {
            glUniform1ui(glGetUniformLocation(program, uniform.c_str()), value);
        }

        void set(const std::string &uniform, GLint value) {
            glUniform1i(glGetUniformLocation(program, uniform.c_str()), value);
        }

        void set(const std::string &uniform, glm::vec2 value) {
            glUniform2f(glGetUniformLocation(program, uniform.c_str()), value.x, value.y);
        }

        void set(const std::string &uniform, glm::vec3 value) {
            glUniform3f(glGetUniformLocation(program, uniform.c_str()), value.x, value.y, value.z);
        }

        void set(const std::string &uniform, glm::vec4 value) {
            glUniform4f(glGetUniformLocation(program, uniform.c_str()), value.x, value.y, value.z, value.w);
        }

        void set(const std::string &uniform, glm::mat4 matrix) {
            glUniformMatrix4fv(glGetUniformLocation(program, uniform.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
        }

        // to prevent accidental shallow copies of managed pointers
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram &operator=(ShaderProgram const &) = delete;
    };

}

#endif