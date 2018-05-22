// Assignment 3, Part 1 and 2
//
// Modify this file according to the lab instructions.
//

#include "utils.h"
#include "utils2.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>
#include <algorithm>

bool ambientToggle = true;
bool diffuseToggle = true;
bool specularToggle = true;
bool gammaToggle = false;
bool normalToggle = false;
bool cubemapToggle = false;
float zoom = 45.0f;
float shine = 2048.0f;

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    POSITION = 0,
    NORMAL = 1
};

// Struct for representing an indexed triangle mesh
struct Mesh {	
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};

// Struct for representing a vertex array object (VAO) created from a
// mesh. Used for rendering.
struct MeshVAO {
    GLuint vao;
    GLuint vertexVBO;
    GLuint normalVBO;
    GLuint indexVBO;
    int numVertices;
    int numIndices;
};

// Struct for resources and state
struct Context {
    int width;
    int height;
    float aspect;
    GLFWwindow *window;
    GLuint program;
    Trackball trackball;
    Mesh mesh;
    MeshVAO meshVAO;
    GLuint defaultVAO;
    GLuint cubemap;
    float elapsed_time;
};

// Returns the value of an environment variable
std::string getEnvVar(const std::string &name)
{
    char const* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::string();
    }
    else {
        return std::string(value);
    }
}

// Returns the absolute path to the shader directory
std::string shaderDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/src/shaders/";
}

// Returns the absolute path to the 3D model directory
std::string modelDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/3d_models/";
}

// Returns the absolute path to the cubemap texture directory
std::string cubemapDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/cubemaps/";
}

void loadMesh(const std::string &filename, Mesh *mesh)
{
    OBJMesh obj_mesh;
    objMeshLoad(obj_mesh, filename);
    mesh->vertices = obj_mesh.vertices;
    mesh->normals = obj_mesh.normals;
    mesh->indices = obj_mesh.indices;
}

void createMeshVAO(Context &ctx, const Mesh &mesh, MeshVAO *meshVAO)
{
    // Generates and populates a VBO for the vertices
    glGenBuffers(1, &(meshVAO->vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    auto verticesNBytes = mesh.vertices.size() * sizeof(mesh.vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, mesh.vertices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the vertex normals
    glGenBuffers(1, &(meshVAO->normalVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    auto normalsNBytes = mesh.normals.size() * sizeof(mesh.normals[0]);
    glBufferData(GL_ARRAY_BUFFER, normalsNBytes, mesh.normals.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the element indices
    glGenBuffers(1, &(meshVAO->indexVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    auto indicesNBytes = mesh.indices.size() * sizeof(mesh.indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNBytes, mesh.indices.data(), GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(meshVAO->vao));
    glBindVertexArray(meshVAO->vao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    glBindVertexArray(ctx.defaultVAO); // unbinds the VAO

    // Additional information required by draw calls
    meshVAO->numVertices = mesh.vertices.size();
    meshVAO->numIndices = mesh.indices.size();
}

void initializeTrackball(Context &ctx)
{
    double radius = double(std::min(ctx.width, ctx.height)) / 2.0;
    ctx.trackball.radius = radius;
    glm::vec2 center = glm::vec2(ctx.width, ctx.height) / 2.0f;
    ctx.trackball.center = center;
}

void init(Context &ctx)
{
    ctx.program = loadShaderProgram(shaderDir() + "mesh.vert",
                                    shaderDir() + "mesh.frag");

    loadMesh((modelDir() + "gargo.obj"), &ctx.mesh);
    createMeshVAO(ctx, ctx.mesh, &ctx.meshVAO);

    // Load cubemap texture(s)
	ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/");
    

    initializeTrackball(ctx);
}

// MODIFY THIS FUNCTION
void drawMesh(Context &ctx, GLuint program, const MeshVAO &meshVAO)
{
    // Define uniforms
    glm::mat4 model = trackballGetRotationMatrix(ctx.trackball);
	//glm::mat4 view = glm::mat4();
    //glm::mat4 projection = glm::ortho(-ctx.aspect, ctx.aspect, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 view = glm::lookAt(
		glm::vec3(-2, -1.5, 1.5), // Camera is at (4,3,3), in World Space
		glm::vec3(0.0, 0.0, 0.0), // and looks at the origin
		glm::vec3(0.0, 1.0, 0.0)  // Head is up (set to 0,-1,0 to look upside-down)
	);
	glm::mat4 projection = glm::perspective(glm::radians(zoom), 1.0f, 0.1f, 100.0f);
	glm::mat4 mv = view * model;
    glm::mat4 mvp = projection * mv;
	glm::vec3 lightpos = glm::vec3(3.0, 0.0, 0.0);
	glm::vec4 ambient_color = glm::vec4(0.2, 0.0, 0.2, 1.0); //kinda black
	glm::vec4 diffuse_color = glm::vec4(1.0, 0.0, 1.0, 1.0); //purple
	glm::vec4 specular_color = glm::vec4(1.0, 1.0, 1.0, 1.0); //white
	float shininess = shine;
    // ...

    // Activate program
    glUseProgram(program);

    // Bind textures
	if (shine <= 0.125) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/0.125");
	}
	else if (shine > 0.125 && shine <= 0.5) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/0.5");
	}
	else if (shine > 0.5 && shine <= 2) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/2");
	}
	else if (shine > 2 && shine <= 8) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/8");
	}
	else if (shine > 8 && shine <= 32) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/32");
	}
	else if (shine > 32 && shine <= 128) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/128");
	}
	else if (shine > 128 && shine <= 512) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/512");
	}
	else if (shine > 512) {
		ctx.cubemap = loadCubemap(cubemapDir() + "/Forrest/prefiltered/2048");
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.cubemap);


    // Pass uniforms
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"), 1, GL_FALSE, &mv[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);
    glUniform1f(glGetUniformLocation(program, "u_time"), ctx.elapsed_time);
	glUniform3fv(glGetUniformLocation(program, "u_lightpos"), 1, &lightpos[0]);
	glUniform4fv(glGetUniformLocation(program, "u_ambient"), 1, &ambient_color[0]);
	glUniform4fv(glGetUniformLocation(program, "u_diffuse"), 1, &diffuse_color[0]);
	glUniform4fv(glGetUniformLocation(program, "u_specular"), 1, &specular_color[0]);
	glUniform1f(glGetUniformLocation(program, "u_shininess"), shininess);
	glUniform1i(glGetUniformLocation(program, "u_ambient_toggle"), ambientToggle);
	glUniform1i(glGetUniformLocation(program, "u_diffuse_toggle"), diffuseToggle);
	glUniform1i(glGetUniformLocation(program, "u_specular_toggle"), specularToggle);
	glUniform1i(glGetUniformLocation(program, "u_gamma_toggle"), gammaToggle);
	glUniform1i(glGetUniformLocation(program, "u_normal_toggle"), normalToggle);
	glUniform1i(glGetUniformLocation(program, "u_cubemap_toggle"), cubemapToggle);
	glUniform1i(glGetUniformLocation(program, "u_cubemap"), 0);
    // ...

    // Draw!
    glBindVertexArray(meshVAO.vao);
    glDrawElements(GL_TRIANGLES, meshVAO.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(ctx.defaultVAO);
}

void display(Context &ctx)
{
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST); // ensures that polygons overlap correctly
    drawMesh(ctx, ctx.program, ctx.meshVAO);
}

void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = loadShaderProgram(shaderDir() + "mesh.vert",
                                     shaderDir() + "mesh.frag");
}

void mouseButtonPressed(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        trackballStartTracking(ctx->trackball, glm::vec2(x, y));
    }
}

void mouseButtonReleased(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        trackballStopTracking(ctx->trackball);
    }
}

void moveTrackball(Context *ctx, int x, int y)
{
    if (ctx->trackball.tracking) {
        trackballMove(ctx->trackball, glm::vec2(x, y));
    }
}

void errorCallback(int /*error*/, const char* description)
{
    std::cerr << description << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) { return; }  // Skip other handling

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        reloadShaders(ctx);
    }
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		ambientToggle = !ambientToggle;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		diffuseToggle = !diffuseToggle;
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		specularToggle = !specularToggle;
	}
	if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		gammaToggle = !gammaToggle;
	}
	if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
		normalToggle = !normalToggle;
	}
	if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
		cubemapToggle = !cubemapToggle;
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		zoom += 10.0f;
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		zoom -= 10.0f;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		shine = shine * 4;
	}
	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		shine = shine / 4;
	}
}

void charCallback(GLFWwindow* window, unsigned int codepoint)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) { return; }  // Skip other handling
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        mouseButtonPressed(ctx, button, x, y);
    }
    else {
        mouseButtonReleased(ctx, button, x, y);
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling   

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    moveTrackball(ctx, x, y);
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    ctx->aspect = float(width) / float(height);
    ctx->trackball.radius = double(std::min(width, height)) / 2.0;
    ctx->trackball.center = glm::vec2(width, height) / 2.0f;
    glViewport(0, 0, width, height);
}

int main(void)
{
    Context ctx;

    // Create a GLFW window
    glfwSetErrorCallback(errorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.width = 500;
    ctx.height = 500;
    ctx.aspect = float(ctx.width) / float(ctx.height);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, keyCallback);
    glfwSetCharCallback(ctx.window, charCallback);
    glfwSetMouseButtonCallback(ctx.window, mouseButtonCallback);
    glfwSetCursorPosCallback(ctx.window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);

    // Load OpenGL functions
    glewExperimental = true;
    GLenum status = glewInit();
    if (status != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize GUI
    ImGui_ImplGlfwGL3_Init(ctx.window, false /*do not install callbacks*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.defaultVAO);
    glBindVertexArray(ctx.defaultVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    init(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsed_time = glfwGetTime();
        ImGui_ImplGlfwGL3_NewFrame();
        display(ctx);
        ImGui::Render();
        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
