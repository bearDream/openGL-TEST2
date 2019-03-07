//
//  main.cpp
//  openGL-TEST2
//
//  Created by Lax Zhang on 2019/3/1.
//  Copyright © 2019 Lax Zhang. All rights reserved.
//
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>

#include "header/fonts/FontsManager.hpp"
#include "header/texture/texture.hpp"
#include "header/shader/shader.hpp"
#include "header/camera/camera.hpp"
#include "vertices/vertices_cube.hpp"
#include "header/sphere/sphere.hpp"

using namespace std;

int init();
void setVertices();
void setTextures();
void setShadows();
void calculateInLoop();
void processShadowInLoop(Shader &shader, glm::mat4 lightSpaceMatrix);
void processObjectInLoop(Shader &shader, glm::mat4 lightSpaceMatrix);
void framebuffer_size_callback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);
void mouseCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow *, double, double);
void renderScene(Shader &shader);
void renderLightSource(Shader &shader);
void renderText(Shader &shader, string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);

// basic param
const int window_width = 1280;
const int  window_height = 720;
int retina_width, retina_height;  // 一般而言，retina显示屏则将宽高乘2即可解决
const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
GLFWwindow *window;
bool is_mouse = false;

// camera
Camera camera(glm::vec3(0.17f, 2.58f, 10.02f));
float lastX = window_width / 2;
float lastY = window_height / 2;
bool firstMouse = true;

// font manage.
FontsManager fontsManager;

// timing
float initial_time, deltaTime =0.0f;
float lastTime = 0.0f;
int frame = 0, frameCount = 0;

// 光空间变换矩阵
glm::mat4 lightSpaceMatrix;

// 顶点/缓冲/索引
GLuint Fl_VAO, Fl_VBO, cube_VAO, cube_VBO, lighterVAO, sphereVAO, sphereVBO, sphereEBO, TextVAO, Text_VBO;
GLuint depthMap, depthMapFBO;
vector<GLuint> sphere_indices;

// 纹理ID
GLuint floorTextureID, boxTextureID, sunTextureID, moonTextureID;

// 光源位置
glm::vec3 lightPos = glm::vec3(1.0, 1.0f, 1.0f);

// 资源地址
char texture_floor[255] = "resources/images/wood.png";
char texture_box[255] = "resources/images/container2.png";
char texture_sun[255] = "resources/images/2k_sun.jpg";
char texture_moon[255] = "resources/images/2k_moon.jpg";
char font_roman[255] = "resources/fonts/Times New Roman.ttf";

int main(int argc, const char * argv[]) {
    
    // 1. 初始化
    if(init() == -1){
        glfwTerminate();
        return 0;
    }
    // 2. 编译着色器
    Shader simpleDepthShader("shaders/shader_depth.vs", "shaders/shader_depth.fs");
    Shader shadowShader("shaders/shader_shadow.vs", "shaders/shader_shadow.fs");
    Shader lampShader("shaders/shader_lighter.vs", "shaders/shader_lighter.fs");
    Shader textShader("shaders/shader_fonts.vs", "shaders/shader_fonts.fs");

    // 3. 顶点设置
    setVertices();
    
    // 4. 纹理设置
    setTextures();
    
    // 5. 阴影设置
    setShadows();

    // 6. Game Looping.
    deltaTime = lastTime = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // 6.1 处理输入事件
        processInput(window);
        // 6.2 循环中的一些变量计算
        calculateInLoop();
        
        // 6.3 刷屏
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 6.6 阴影处理
        processShadowInLoop(simpleDepthShader, lightSpaceMatrix);
        
        // 6.7 重置 viewport
        glViewport(0, 0, retina_width, retina_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 6.8 物体渲染
        processObjectInLoop(shadowShader, lightSpaceMatrix);
        
        // 6.9. 渲染光源
        renderLightSource(lampShader);
        
        // 6.10. 渲染字体(字体位置不能超出window的宽高)
        renderText(textShader, "Press <ctrl> to call out Mouse", 840.0f, 25.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0));
        renderText(textShader, "Press <ESC> to exit this Demo", 840.0f, 75.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0));
        renderText(textShader, "Press <W><A><S><D> to move out camera", 840.0f, 50.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0));
        renderText(textShader, "FPS: " + to_string(frame), 25.0f, 25.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0));
        renderText(textShader, "Camera position: (" +
                   to_string(camera.camPos.x).substr(0, to_string(camera.camPos.x).find(".")+3).append(",") +
                   to_string(camera.camPos.y).substr(0, to_string(camera.camPos.y).find(".")+3).append(",") +
                   to_string(camera.camPos.z).substr(0, to_string(camera.camPos.z).find(".")+3).append(")"),
                   10.0f, 705.0f, 0.3f, glm::vec3(1.0, 1.0, 1.0));
        
        glfwSwapBuffers(window); // 颜色缓冲交换
        glfwPollEvents(); // 处理事件
    }
    // 7. 释放
    glDeleteVertexArrays(1, &cube_VAO);
    glDeleteVertexArrays(1, &lighterVAO);
    glDeleteVertexArrays(1, &Fl_VAO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteVertexArrays(1, &TextVAO);
    glDeleteBuffers(1, &cube_VBO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &Text_VBO);
    glDeleteBuffers(1, &Fl_VBO);
    glDeleteBuffers(1, &sphereEBO);

    glfwTerminate();
    return 0;
}

void processShadowInLoop(Shader &shader, glm::mat4 lightSpaceMatrix){
    // 1. 渲染深度贴图
    shader.use();
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTextureID);
    renderScene(shader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void processObjectInLoop(Shader &shader, glm::mat4 lightSpaceMatrix){
    // 2. 生成阴影贴图
    shader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) window_width / (float) window_height, 0.1f, 100.0f);
    glm::mat4 view = camera.getViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    
    shader.setVec3("lightColor", glm::vec3(1.0f));
    shader.setVec3("viewPos", camera.camPos);
    shader.setVec3("lightPos", lightPos);
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader.setInt1("diffuseTexture", 0);
    shader.setInt1("shadowMap", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTextureID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    renderScene(shader);
}

void renderLightSource(Shader &shader){
    // 光源设置
    shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) window_width / (float) window_height, 0.1f, 100.0f);
    model = glm::translate(model, lightPos);
    //    model = glm::scale(model, glm::vec3(0.2f));
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setInt1("diffuseTexture", 0.0);
    // 绘制光源
    glBindVertexArray(sphereVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sunTextureID);
    glDrawElements(GL_TRIANGLES, (int)sphere_indices.size(), GL_UNSIGNED_INT, (void*)0);
}

void renderScene(Shader &shader){
    // 渲染地板
    shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    glBindVertexArray(Fl_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // 渲染箱子物体
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.2f, 0.0f, 0.2));
    shader.setMat4("model", model);
    glBindVertexArray(cube_VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.4));
    shader.setMat4("model", model);
    glBindVertexArray(cube_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, -0.12f, 2.0));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    glBindVertexArray(cube_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // 绘制球体
    model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) window_width / (float)window_height, 0.1f, 100.0f);
    view = camera.getViewMatrix();
    model = glm::translate(model, glm::vec3(0.8f, 0.0f, 1.3f));
    model = glm::rotate(model, (float) glfwGetTime() * glm::radians(55.0f), glm::vec3(1.0f, 0.3f, 0.5f));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    glBindVertexArray(sphereVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, moonTextureID);
    glDrawElements(GL_TRIANGLES, (int)sphere_indices.size(), GL_UNSIGNED_INT, (void*)0);
}

void renderText(Shader &shader, string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color){
    
    shader.use();
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(window_width), 0.0f, static_cast<GLfloat>(window_height));
    shader.setMat4("projection", projection);
    shader.setFloat3("textColor", color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(TextVAO);
    
    string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        FontsManager::Character ch = fontsManager.Characters[*c];
        float xPos = x + ch.Bearing.x * scale;
        float yPos = y - (ch.size.y - ch.Bearing.y) * scale;
        
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        
        float vertices[6][4] = {
            { xPos,     yPos + h,   0.0, 0.0 },
            { xPos,     yPos,       0.0, 1.0 },
            { xPos + w, yPos,       1.0, 1.0 },
            
            { xPos,     yPos + h,   0.0, 0.0 },
            { xPos + w, yPos,       1.0, 1.0 },
            { xPos + w, yPos + h,   1.0, 0.0 }
        };
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, Text_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // 绘制四边形
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


int init(){
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // 创建窗口
    window = glfwCreateWindow(window_width, window_height, "GLFW Shadow", NULL, NULL);
    if (window == NULL) {
        cout<< "Failed Create Window" << endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    // 窗口调整大小 回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseCallback); // 设置鼠标回调函数
    glfwSetScrollCallback(window, scrollCallback); // 设置鼠标滚轮回调函数
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // 设置窗口获取焦点
    // 初始化GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cout << "Failed init Glew." << endl;
        return -1;
    }
    // 处理字体
    fontsManager.load_fonts(font_roman);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 获取实际窗口大小
    glfwGetFramebufferSize(window, &retina_width, &retina_height);
    // 若不启用 GL_DEPTH_TEST 则会出现物体的后静和前景覆盖的问题
    glEnable(GL_DEPTH_TEST);
    return 0;
}

void setVertices(){
    // 球体
    Sphere sphere(0.5f, 60, 60);
    vector<float> sphere_vertices = sphere.getVertices();
    sphere_indices = sphere.getIndices();
    
    // =======立方体=======
    glGenVertexArrays(1, &cube_VAO);
    glGenBuffers(1, &cube_VBO);
    
    glBindBuffer(GL_ARRAY_BUFFER, cube_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(cube_VAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*(sizeof(float)), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*(sizeof(float)), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*(sizeof(float)), (void*)(6 * sizeof(float)));
    
    // =======球体=======
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size()*sizeof(float),&sphere_vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere_indices.size()*sizeof(float),&sphere_indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*(sizeof(float)), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*(sizeof(float)), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6*(sizeof(float)), (void*)(6 * sizeof(float)));
    
    // ===== 地板的顶点设置======
    glGenVertexArrays(1, &Fl_VAO);
    glGenBuffers(1, &Fl_VBO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Fl_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindVertexArray(Fl_VAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glBindVertexArray(0);
    
    // =======字体四边形的顶点设置======
    glGenVertexArrays(1, &TextVAO);
    glGenBuffers(1, &Text_VBO);
    
    glBindVertexArray(TextVAO);
    glBindBuffer(GL_ARRAY_BUFFER, Text_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void setTextures(){
    // ===纹理加载=====
    floorTextureID = loadTexture(texture_floor);
    sunTextureID = loadTexture(texture_sun);
    boxTextureID = loadTexture(texture_box);
    moonTextureID = loadTexture(texture_moon);
}

void setShadows(){
    // ======阴影设置========
    glGenFramebuffers(1, &depthMapFBO);
    
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0}; // 防止重复阴影的出现
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void calculateInLoop(){
    float current = glfwGetTime();
    deltaTime = current - lastTime;
    lastTime = current;
    
    // 计算FPS
    double currentTime = glfwGetTime();
    frameCount++;
    if ( currentTime - initial_time >= 1.0 ){
        frame = frameCount;
        frameCount = 0;
        initial_time += 1;
    }
    
    // 6.4 光源日出日落位移
    lightPos.x = sin((float)glfwGetTime()) * 3.0f;
    lightPos.z = sin((float)glfwGetTime()) * 3.0f;
    lightPos.y = (abs(cos(glfwGetTime()))+1.0) * 3.0f;
    
    // 6.5 光空间变换矩阵
    glm::mat4 lightProjection, lightView;
    GLfloat near_plane = 1.0f, far_plane = 20.5f;
    lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, near_plane, far_plane);
    lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(1.0f));
    lightSpaceMatrix = lightProjection * lightView;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window){
    float current = glfwGetTime();
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
    if ( current - initial_time >= 1.0 ){
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
            cout << is_mouse << endl;
            if (is_mouse){
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // 设置窗口获取焦点
                is_mouse = false;
            }else{
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);  // 设置窗口获取焦点
                is_mouse = true;
            }
        }
    }
}

void mouseCallback(GLFWwindow* window, double x_pos, double y_pos){
    if (is_mouse) {
        return;
    }
    if (firstMouse) {
        lastX = x_pos;
        lastY = y_pos;
        firstMouse = false;
    }
    
    float x_offset = x_pos - lastX;
    float y_offset = lastY - y_pos;
    lastX = x_pos;
    lastY = y_pos;
    
    camera.processMouseMovement(x_offset, y_offset);
}

void scrollCallback(GLFWwindow *window, double x_offset, double y_offset){
    camera.processMouseScroll(y_offset);
}
