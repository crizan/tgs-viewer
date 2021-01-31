#include <zlib.h>
#include <rlottie.h>

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

constexpr int READ_SIZE = 10000;

std::string extract_gzip(const char* path) {
    gzFile zfp;

    zfp = gzopen(path, "rb");

    std::vector<char> bytes;
    int size = 0;

    while(1) {
        int bytes_read;
        bytes.resize(size + READ_SIZE);
        bytes_read = gzread(zfp, bytes.data() + size, READ_SIZE);

        size += bytes_read;
        bytes.resize(size);
        if(bytes_read < READ_SIZE)
            break;
    }

    gzclose(zfp);
    return std::string(bytes.begin(), bytes.end());
}

void create_textures(std::unique_ptr<rlottie::Animation>& animation, std::vector<uint32_t>& textures, int width, int height) {
    textures.resize(animation->totalFrame());
    glGenTextures(animation->totalFrame(), textures.data());

    std::vector<uint32_t> buffer(width * height);
    rlottie::Surface surface(buffer.data(), width, height, width * 4);

    int frame = 0;
    for(auto texture : textures) {
        animation->renderSync(frame++, surface);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
    }
}

void delete_textures(std::vector<uint32_t>& textures) {
    glDeleteTextures(textures.size(), textures.data());
    textures.clear();
}

struct GLFWUserPointer {
    bool last_resize = false;
    bool resize = false;
    int w, h;
};


void recreate_textures(std::unique_ptr<rlottie::Animation>& animation, std::vector<uint32_t>& textures, GLFWUserPointer *user_pointer) {
    delete_textures(textures);
    create_textures(animation, textures, user_pointer->w, user_pointer->h);
}

void resize_callback(GLFWwindow* window, int width, int height) {
    GLFWUserPointer *user_pointer = (GLFWUserPointer *)glfwGetWindowUserPointer(window);
    user_pointer->resize = true;
    user_pointer->w = width;
    user_pointer->h = height;
    glViewport(0, 0, width, height);
}

const char* vertex_shader =
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;"
"layout (location = 1) in vec2 aTexCoord;"
"out vec2 TexCoord;"
"void main()"
"{"
"   gl_Position = vec4(aPos, 0.0, 1.0);"
"   TexCoord = aTexCoord;"
"}"
;

const char* fragment_shader =
"#version 330 core\n"
"out vec4 FragColor;"
"in vec2 TexCoord;"
"uniform sampler2D Texture;"
"void main()"
"{"
"   FragColor = texture(Texture, TexCoord);"
"}"
;

int main(int argc, char* argv[]) {
    if(argc < 2) return 1;

    std::string data = extract_gzip(argv[1]);

    std::vector<uint32_t> textures;
    std::unique_ptr<rlottie::Animation> animation;

    animation = rlottie::Animation::loadFromData(data, "*");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(512, 512, "Lottie Viewer", NULL, NULL);
    if(window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 512, 512);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    float vertices[] = {
        1.f, 1.f, 1.f, 0.f,
        1.f, -1.f, 1.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f, 1.f, 0.f, 0.f
    };
    uint32_t indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    uint32_t VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    uint32_t vertex, fragment;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader, NULL);
    glCompileShader(vertex);

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader, NULL);
    glCompileShader(fragment);

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glUseProgram(program);

    create_textures(animation, textures, 512, 512);

    GLFWUserPointer user_pointer = {};
    glfwSetWindowUserPointer(window, &user_pointer);

    auto start_time = std::chrono::high_resolution_clock::now();
    float frame_duration = animation->duration() / animation->totalFrame();
    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        std::chrono::duration<double> dur = std::chrono::high_resolution_clock::now() - start_time;
        double seconds = dur.count();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[(int)(seconds / frame_duration) % animation->totalFrame()]);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        user_pointer.last_resize = user_pointer.resize;
        user_pointer.resize = false;
        glfwPollEvents();
        if(user_pointer.last_resize == true && user_pointer.resize == false) recreate_textures(animation, textures, &user_pointer);
    }

    delete_textures(textures);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();

    return 0;
}
