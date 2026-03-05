#pragma once
#include "../core.h"

class FShader {
public:
	FShader(std::string computePath);
	FShader(std::string vertexPath, std::string fragmentPath, std::string geometryPath = "ignore");
	~FShader() {
		glDeleteProgram(mProgramHandle);
	}
	GLuint getProgramHandle() const { return mProgramHandle; }
	void use() const{ glUseProgram(mProgramHandle); }
	void end() const{ glUseProgram(0); }

	void uniform1i(int input, const std::string& name) const{
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniform1i(loc, input);
	}
	void uniform1f(float input, const std::string& name) const {
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniform1f(loc, input);
	}
	void uniform4x4f(glm::mat4 input, const std::string& name) const {
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(input));
	}
	void uniform3x3f(glm::mat3 input, const std::string& name) const {
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(input));
	}
	void uniform4f(glm::vec4 input, const std::string& name) const {
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniform4fv(loc, 1, glm::value_ptr(input));
	}
	void uniform3f(glm::vec3 input, const std::string& name) const {
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniform3fv(loc, 1, glm::value_ptr(input));
	}
	void uniform2f(glm::vec2 input, const std::string& name) const {
		GLuint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		glUniform2fv(loc, 1, glm::value_ptr(input));
	}

	GLint getSamplerIndex(const std::string& name){
		if (mSamplerCache.contains(name)) return mSamplerCache[name];
		GLint loc = glGetUniformLocation(mProgramHandle, name.c_str());
		mSamplerCache[name] = loc;
		return loc;
	}

	GLuint getUBOIndex(const std::string& name) {
		if (mUBOCache.contains(name)) return mUBOCache[name];
		GLuint loc = glGetUniformBlockIndex(mProgramHandle, name.c_str());
		mUBOCache[name] = loc;
		return loc;
	}

private:
	GLuint mProgramHandle = 0;
	std::string defaultGeometryPath = "ignore";

	std::unordered_map<std::string, GLuint> mSamplerCache = {};
	std::unordered_map<std::string, GLuint> mUBOCache = {};
};

class FShaderCache {
public:
	FShaderCache() = default;
	std::shared_ptr<FShader> search(std::string shaderName) {
		if (mShaders.count(shaderName)) return mShaders[shaderName];
		else return nullptr;
	}

	bool createShader(std::string name, std::string vertexPath, std::string fragmentPath) {
		if (search(name) != 0) return false;
		else {
			std::shared_ptr<FShader> vsfsPtr = std::make_shared<FShader>(vertexPath, fragmentPath);
			mShaders[name] = vsfsPtr;
		}
		return true;
	}

	bool createShader(std::string name, std::string vertexPath, std::string fragmentPath, std::string geometryPath) {
		if (search(name) != 0) return false;
		else {
			std::shared_ptr<FShader> vsfscsPtr = std::make_shared<FShader>(vertexPath, fragmentPath, geometryPath);
			mShaders[name] = vsfscsPtr;
		}
		return true;
	}

	bool createShader(std::string name, std::string computePath) {
		if (search(name) != 0) return false;
		else {
			std::shared_ptr<FShader> csPtr = std::make_shared<FShader>(computePath);
			mShaders[name] = csPtr;
		}
		return true;
	}

	void removeShader(const std::string& name);
private:
	std::unordered_map<std::string, std::shared_ptr<FShader>> mShaders = {};
};