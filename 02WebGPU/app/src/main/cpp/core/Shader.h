#ifndef __shaderH__
#define __shaderH__

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <GLES2/gl2.h>
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>

#include "Vector.h"

class Shader{

public:

    Shader(std::string vertex, std::string fragment, AAssetManager* assetManager);
    Shader(Shader* shader);
    virtual ~Shader();

    GLuint m_program;

    void loadMatrix(const char* location, const Matrix4f matrix);
    void loadMatrixArray(const char* location, const std::vector<Matrix4f> matrixArray, const short count);
    void loadVector(const char* location, Vector3f vector);
    void loadFloat2(const char* location, float value[2]);
    void loadFloat(const char* location, float value);
    void loadBool(const char* location, bool value);

protected:

    GLuint createProgram(std::string vertex, std::string fragment);
    GLuint loadShaderProgramFromAsset(GLenum type, const char *pszFilename);
    GLuint compileShader(GLenum type, const char *pszSource);
    GLuint linkShaders(GLuint vertShader, GLuint fragShader);

    void cleanup();

    AAssetManager* m_assetManager;

};
#endif // __shaderH__