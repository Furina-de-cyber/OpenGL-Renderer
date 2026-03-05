#include "shader.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

FShader::FShader(std::string vertexPath, std::string fragmentPath, std::string geometryPath) {
    std::string vertexCode;
    std::string fragmentCode;

    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    std::string geometryCode;
    std::ifstream gShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    if (geometryPath != defaultGeometryPath)
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        if (geometryPath != defaultGeometryPath) gShaderFile.open(geometryPath);

        std::stringstream vShaderStream, fShaderStream, gShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        if (geometryPath != defaultGeometryPath) gShaderStream << gShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();
        if (geometryPath != defaultGeometryPath) gShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        if (geometryPath != defaultGeometryPath) geometryCode = gShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "SHADER_FILE_ERROR:" << e.what() << std::endl;
    }

    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();
    const char* geometrySource = geometryPath == defaultGeometryPath ? "" : geometryCode.c_str();

    int success = 0;
    char infoLog[1024] = {};
    GLuint vertex = 0, fragment = 0, geometry = 0;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    if (geometryPath != defaultGeometryPath) geometry = glCreateShader(GL_GEOMETRY_SHADER);

    glShaderSource(vertex, 1, &vertexSource, NULL);
    glShaderSource(fragment, 1, &fragmentSource, NULL);
    if (geometryPath != defaultGeometryPath) glShaderSource(geometry, 1, &geometrySource, NULL);

    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 1024, NULL, infoLog);
        std::cout << "ERROR AT:" << vertexPath << std::endl;
        std::cout << "FAILED_TO_COMPILE_VERTEX_SHADER\n" << infoLog << std::endl;
    }
    if (geometryPath != defaultGeometryPath) {
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(geometry, 1024, NULL, infoLog);
            std::cout << "ERROR AT:" << geometryPath << std::endl;
            std::cout << "FAILED_TO_COMPILE_GEOMETRY_SHADER\n" << infoLog << std::endl;
        }
    }

    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 1024, NULL, infoLog);
        std::cout << "ERROR AT:" << fragmentPath << std::endl;
        std::cout << "FAILED_TO_COMPILE_FRAGMENT_SHADER\n" << infoLog << std::endl;
    }

    mProgramHandle = glCreateProgram();
    glAttachShader(mProgramHandle, vertex);
    if (geometryPath != defaultGeometryPath) glAttachShader(mProgramHandle, geometry);
    glAttachShader(mProgramHandle, fragment);

    glLinkProgram(mProgramHandle);
    glGetProgramiv(mProgramHandle, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(mProgramHandle, 1024, NULL, infoLog);
        std::cout << "FAILED_TO_LINK\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geometryPath != defaultGeometryPath) glDeleteShader(geometry);
}

FShader::FShader(std::string computePath) {
    std::string computeCode;
    std::ifstream cShaderFile;

    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        cShaderFile.open(computePath);

        std::stringstream cShaderStream;
        cShaderStream << cShaderFile.rdbuf();

        cShaderFile.close();

        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "SHADER_FILE_ERROR:" << e.what() << std::endl;
    }

    const char* computeSource = computeCode.c_str();

    int success = 0;
    char infoLog[1024] = {};
    GLuint compute = 0;

    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &computeSource, NULL);

    glCompileShader(compute);
    glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(compute, 1024, NULL, infoLog);
        std::cout << "ERROR AT:" << computePath << std::endl;
        std::cout << "FAILED_TO_COMPILE_COMPUTE_SHADER\n" << infoLog << std::endl;
    }

    mProgramHandle = glCreateProgram();
    glAttachShader(mProgramHandle, compute);

    glLinkProgram(mProgramHandle);
    glGetProgramiv(mProgramHandle, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(mProgramHandle, 1024, NULL, infoLog);
        std::cout << "FAILED_TO_LINK\n" << infoLog << std::endl;
    }

    glDeleteShader(compute);
}

void FShaderCache::removeShader(const std::string& name) {
    auto it = mShaders.find(name);
    if (it != mShaders.end()) {
        mShaders.erase(it);
    }
}
