#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace our
{

    class ShaderProgram
    {

    private:
        // Shader Program Handle (OpenGL object name)
        GLuint program;

    public:
        ShaderProgram()
        {
            // TODO: (Req 1) Create A shader program
            program = glCreateProgram();
        }
        ~ShaderProgram()
        {
            // TODO: (Req 1) Delete a shader program
            glDeleteProgram(program);
        }

        bool attach(const std::string &filename, GLenum type) const;

        bool link() const;

        void use()
        {
            glUseProgram(program);
        }

        GLuint getUniformLocation(const std::string &name)
        {
            // TODO: (Req 1) Return the location of the uniform with the given name
            GLuint uniform_location = glGetUniformLocation(program, name.c_str());
            return uniform_location;
        }

        void set(const std::string &uniform, GLfloat value)
        {
            // TODO: (Req 1) Send the given float value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniform1f(uniform_location, value);
        }

        void set(const std::string &uniform, GLuint value)
        {
            // TODO: (Req 1) Send the given unsigned integer value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniform1ui(uniform_location, value);
        }

        void set(const std::string &uniform, GLint value)
        {
            // TODO: (Req 1) Send the given integer value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniform1i(uniform_location, value);
        }

        void set(const std::string &uniform, glm::vec2 value)
        {
            // TODO: (Req 1) Send the given 2D vector value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniform2fv(uniform_location, 1, glm::value_ptr(value));
        }

        void set(const std::string &uniform, glm::vec3 value)
        {
            // TODO: (Req 1) Send the given 3D vector value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniform3fv(uniform_location, 1, glm::value_ptr(value));
        }

        void set(const std::string &uniform, glm::vec4 value)
        {
            // TODO: (Req 1) Send the given 4D vector value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniform4fv(uniform_location, 1, glm::value_ptr(value));
        }

        void set(const std::string &uniform, glm::mat4 matrix)
        {
            // TODO: (Req 1) Send the given matrix 4x4 value to the given uniform
            GLuint uniform_location = getUniformLocation(uniform);
            if (uniform_location == -1)
                return;
            glUniformMatrix4fv(uniform_location, 1, GL_FALSE, glm::value_ptr(matrix));
        }

        // TODO: (Req 1) Delete the copy constructor and assignment operator.
        // Question: Why do we delete the copy constructor and assignment operator?
        ShaderProgram(const ShaderProgram &) = delete;
        ShaderProgram &operator=(const ShaderProgram &) = delete;

        // We delete the copy constructor as both objects will point to the same memory and cause crashes like double freeing.
        // We delete the assignment operator for the same reason as the copy constructor, and also to prevent resource leaks.
    };

}

#endif