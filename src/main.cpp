#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// Constants
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr const char* WINDOW_TITLE = "3D World";
constexpr const char* VERTEX_SHADER_PATH = "res/shaders/vertex_shader.glsl";
constexpr const char* FRAGMENT_SHADER_PATH = "res/shaders/fragment_shader.glsl";

// Camera settings
glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.03f;
float yaw = -90.0f, pitch = 0.0f;
float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2;
bool firstMouse = true;
float sensitivity = 0.1f;

// Utility to check OpenGL errors
void checkOpenGLError(const std::string& context) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in " << context << ": " << err << std::endl;
    }
}

// Shader class for encapsulating shader program
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        ID = createShaderProgram(vertexPath, fragmentPath);
    }

    ~Shader() {
        glDeleteProgram(ID);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    }

private:
    unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);
        unsigned int vertexShader = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);

        unsigned int program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        checkCompileErrors(program, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    unsigned int compileShader(const char* code, GLenum type) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, nullptr);
        glCompileShader(shader);
        checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
        return shader;
    }

    void checkCompileErrors(unsigned int shader, const std::string& type) const {
        int success;
        char infoLog[1024];
        if (type == "PROGRAM") {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
                std::cerr << "Program Linking Error: " << infoLog << std::endl;
            }
        } else {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
                std::cerr << type << " Shader Compilation Error: " << infoLog << std::endl;
            }
        }
    }

    static std::string readFile(const char* filepath) {
        std::ifstream file(filepath);
        std::stringstream buffer;
        if (file) {
            buffer << file.rdbuf();
            file.close();
        } else {
            throw std::ios_base::failure("Failed to read shader file: " + std::string(filepath));
        }
        return buffer.str();
    }
};

// VAO and VBO wrapper
class VertexArray {
public:
    unsigned int ID;

    VertexArray() {
        glGenVertexArrays(1, &ID);
    }

    ~VertexArray() {
        glDeleteVertexArrays(1, &ID);
    }

    void bind() const {
        glBindVertexArray(ID);
    }

    void unbind() const {
        glBindVertexArray(0);
    }
};

class VertexBuffer {
public:
    unsigned int ID;

    VertexBuffer(const void* data, size_t size) {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ARRAY_BUFFER, ID);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    ~VertexBuffer() {
        glDeleteBuffers(1, &ID);
    }

    void bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, ID);
    }

    void unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

// Callback for resizing window
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Mouse input callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = (xpos - lastX) * sensitivity;
    float yOffset = (lastY - ypos) * sensitivity;
    lastX = xpos;
    lastY = ypos;

    yaw += xOffset;
    pitch += yOffset;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Input processing
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float velocity = cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
}

// Main function
int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);

    // Square vertices
    float squareVertices[] = {
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
    };

    // Create and bind VAOs and VBOs
    VertexArray squareVAO;
    VertexBuffer squareVBO(squareVertices, sizeof(squareVertices));

    // Square Setup
    squareVAO.bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    squareVAO.unbind();

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // Render Square
        glm::mat4 squareModel = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -3.0f)); // Position the square
        shader.setMat4("model", squareModel);
        squareVAO.bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}