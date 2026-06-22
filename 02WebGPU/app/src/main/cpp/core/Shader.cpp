#include "Shader.h"

Shader::Shader(std::string vertex, std::string fragment, AAssetManager* assetManager){
    m_assetManager = assetManager;
    m_program = createProgram(vertex, fragment);
}

Shader::Shader(Shader* shader){
    m_program = shader->m_program;
}

Shader::~Shader(){
    cleanup();
}

//OpenGL specifies matrices as column-major to get row-major just transpose it
void Shader::loadMatrix(const char* location, const Matrix4f matrix){
    glUniformMatrix4fv(glGetUniformLocation(m_program, location), 1, true, &matrix[0][0]);
}

void Shader::loadMatrixArray(const char* location, const std::vector<Matrix4f> matrixArray, const short count) {
    glUniformMatrix4fv(glGetUniformLocation(m_program, location), count, GL_FALSE, matrixArray[0][0]);
}

void Shader::loadVector(const char* location,  Vector3f vector){
    glUniform3fv(glGetUniformLocation(m_program, location), 1, &vector[0]);
}

void Shader::loadFloat2(const char* location, float value[2]){
    glUniform1fv(glGetUniformLocation(m_program, location), 2, value);
}

void Shader::loadFloat(const char* location, float value){
    glUniform1f(glGetUniformLocation(m_program, location), value);
}

void Shader::loadBool(const char* location, bool value){
    glUniform1i(glGetUniformLocation(m_program, location), value);
}

GLuint Shader::createProgram(std::string vertex, std::string fragment) {
    GLuint vshader = loadShaderProgramFromAsset(GL_VERTEX_SHADER, vertex.c_str());
    GLuint fshader = loadShaderProgramFromAsset(GL_FRAGMENT_SHADER, fragment.c_str());
    return linkShaders(vshader, fshader);
}

GLuint Shader::loadShaderProgramFromAsset(GLenum type, const char *pszFilename){

    AAsset* asset = AAssetManager_open(m_assetManager, pszFilename, AASSET_MODE_BUFFER);
    size_t length = AAsset_getLength(asset);

    GLuint shader = 0;
    if(asset){
        char* buffer = (char*) malloc(length + 1);
        AAsset_read(asset, buffer, length);
        buffer[length] = 0;
        shader = compileShader(type, reinterpret_cast<const char *>(&buffer[0]));
        AAsset_close(asset);
        free(buffer);
    }
    return shader;
}

GLuint Shader::compileShader(GLenum type, const char *pszSource){

    GLuint shader = glCreateShader(type);

    if (shader){
        glShaderSource(shader, 1, &pszSource, NULL);
        glCompileShader(shader);
    }
    return shader;
}

GLuint Shader::linkShaders(GLuint vertShader, GLuint fragShader){

    GLuint program = glCreateProgram();

    if (program){
        if (vertShader)
            glAttachShader(program, vertShader);

        if (fragShader)
            glAttachShader(program, fragShader);

        glLinkProgram(program);

        // Mark the two attached shaders for deletion. These two shaders aren't
        // deleted right now because both are already attached to a shader
        // program. When the shader program is deleted these two shaders will
        // be automatically detached and deleted.

        if (vertShader)
            glDeleteShader(vertShader);

        if (fragShader)
            glDeleteShader(fragShader);

    }
    return program;
}

void Shader::cleanup(){
    if (m_program){
        glDeleteProgram(m_program);
        m_program = 0;
    }
}