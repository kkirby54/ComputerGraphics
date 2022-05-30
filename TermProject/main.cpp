// Std. Includes
#include <iostream>
#include <cmath>
#include <string>

// GLEW
//#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
#include <shader.h>
#include <arcball.h>
#include <Model.h>
#include <cube.h>
#include "link.h"
#include <keyframe.h>
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

#include <skybox.h>

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


// Globals
unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;
GLFWwindow *mainWindow = NULL;
glm::mat4 projection, view, model;
Shader * skyboxShader;
Shader* modelShader;
Shader* cubeShader;
Shader* linkShader;

// for arcball
float arcballSpeed = 0.2f;
static Arcball camArcBall(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true );
static Arcball modelArcBall(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);
bool arcballCamRot = true;

// for camera
glm::vec3 cameraOrigPos(0.0f, 0.0f, 9.0f);
glm::vec3 cameraPos;
glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);

glm::vec3 modelPan(0.0f, 0.0f, 0.0f);



// Function prototypes

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action , int mods);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double x, double y);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
GLFWwindow *glAllInit();
void render();

void loadTexture();
unsigned int loadCubemap(vector<std::string> faces);

void initLinks();
void drawLinks(Link* root, float t, glm::mat4 cmodel, Shader* shader);

// for texture
unsigned int cubemapTexture;
unsigned int texture;
unsigned int texture2;

// skybox
SkyBox* skybox;

// model
Model* buzz;
Model* woody;
Model* mousy;
Model* potato;
Model* wheezy;
Model* rex;
Cube* cube;

// for skybox
vector<std::string> faces{
        "res/textures/right.jpg",
        "res/textures/left.jpg",
        "res/textures/top.png",
        "res/textures/bottom.jpg",
        "res/textures/back.jpg",
        "res/textures/front.jpg"
};

// for animation
enum RenderMode { INIT, ANIM };
RenderMode renderMode;                  // current rendering mode
float beginT;                           // animation beginning time (in sec)
float timeT;                            // current time (in sec)
const float animEndTime = 2.0f;         // ending time of animation (in sec)
Link* root;

// for lighting
glm::vec3 lightSize(0.2f, 0.2f, 0.2f);
glm::vec3 lightPos(20.0f, 20.0f, 20.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
float ambientStrenth = 0.5f;

int main( )
{
    mainWindow = glAllInit();
    skyboxShader = new Shader("res/shaders/skybox.vs", "res/shaders/skybox.frag");
    modelShader = new Shader("res/shaders/modelLoading.vs", "res/shaders/modelLoading.frag");
    cubeShader = new Shader("res/shaders/cube.vs", "res/shaders/cube.fs");
    linkShader = new Shader("res/shaders/basic_lighting(Link).vs", "res/shaders/basic_lighting(Link).fs");

    // Projection initialization
    projection = glm::perspective(glm::radians(73.0f),
                                  (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    skyboxShader->use();
    skyboxShader->setMat4("projection", projection);
    modelShader->use();
    modelShader->setMat4("projection", projection);
    cubeShader->use();
    cubeShader->setMat4("projection", projection);
    linkShader->use();
    linkShader->setMat4("projection", projection);
    linkShader->setVec3("lightColor", lightColor);
    linkShader->setVec3("lightPos", lightPos);
    linkShader->setFloat("ambientStrenth", ambientStrenth);


    // Camera 위치
    cameraPos = cameraOrigPos;
    
    // skybox init
    skybox = new SkyBox();
    cubemapTexture = loadCubemap(faces);
    skyboxShader->use();
    skyboxShader->setInt("skybox", 0);

    // Load models
    buzz = new Model((GLchar*)"res/models/buzz_standing/buzzlightyear_standing.dae");
    woody = new Model((GLchar*)"res/models/woody/woody.dae");
    mousy = new Model((GLchar*)"res/models/mousy/mousy.dae");
    wheezy = new Model((GLchar*)"res/models/wheezy/wheezy.dae");
    rex = new Model((GLchar*)"res/models/rex/rex.obj");

    cube = new Cube(-3.0f, 5.0f, 0.0f, 1.5);

    // load cube texture
    loadTexture();

    // initialize animation data
    initLinks();
    timeT = 0.0f;
    renderMode = INIT;

    // loop
    while( !glfwWindowShouldClose( mainWindow ) )
    {
        glfwPollEvents( );
        
        render();
        
        glfwSwapBuffers( mainWindow );
    }
    
    glfwTerminate( );
    return 0;
}


void render()
{
    //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if (renderMode == ANIM) {
        float cTime = (float)glfwGetTime(); // current time
        timeT = cTime - beginT;
    }

    // 기존
    /*glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    view = view * camArcBall.createRotationMatrix();
    */
    
    // draw buzz
    modelShader->use();

    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));
    
    model = glm::rotate(model, (float)5, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float) -0.25, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(100.0f, 0.0f, 0.0f));
    model = glm::translate(model, modelPan);
    model = model * modelArcBall.createRotationMatrix();
    view = glm::lookAt(cameraPos, camTarget, camUp);
    
    modelShader->setMat4("model", model);
    modelShader->setMat4("view", view);
    buzz->Draw(modelShader);

    // draw woody
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));

    model = glm::rotate(model, (float)5, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.15, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-35.0f, -20.0f, 0.0f));
    model = model * modelArcBall.createRotationMatrix();
    modelShader->setMat4("model", model);
    woody->Draw(modelShader);



    // draw mousy
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));
    
    model = glm::translate(model, modelPan);
    model = model * modelArcBall.createRotationMatrix();

    model = glm::translate(model, glm::vec3(-75.0f, -230.0f, 0.0f));

    modelShader->setMat4("model", model);
    mousy->Draw(modelShader);


    // draw wheezy
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));
    model = glm::translate(model, glm::vec3(115.0f, -100.0f, 0));

    model = glm::rotate(model, (float)4.7, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.15, glm::vec3(0.0f, 0.0f, 1.0f));


    //model = glm::translate(model, glm::vec3(100.0f, 0.0f, -90.0f));

    model = glm::translate(model, modelPan);
    model = model * modelArcBall.createRotationMatrix();


    modelShader->setMat4("model", model);
    wheezy->Draw(modelShader);

    // draw rex
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
    model = glm::translate(model, glm::vec3(430.0f, 410.0, 0.0f));

    //model = glm::rotate(model, (float)4.7, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)0.4, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.75, glm::vec3(0.0f, 1.0f, 0.0f));

    //model = glm::translate(model, glm::vec3(400.0f, 0.0, 0.0f));

    model = glm::translate(model, modelPan);
    model = model * modelArcBall.createRotationMatrix();

    modelShader->setMat4("model", model);
    rex->Draw(modelShader);


    // Robot arms with Animation 구현.
    // + 
    linkShader->use();
    linkShader->setMat4("view", view);
    model = glm::mat4(1.0f);
    model = modelArcBall.createRotationMatrix();
    drawLinks(root, timeT / animEndTime, model, linkShader);

    // draw cube
    model = glm::mat4(1.0f);
    model = modelArcBall.createRotationMatrix();
    cubeShader->use();
    cubeShader->setMat4("model", model);
    cubeShader->setMat4("view", view);
    glBindTexture(GL_TEXTURE_2D, texture);

    cube->draw(cubeShader);



    // Skybox
    skyboxShader->use();
    view = glm::mat4(glm::mat3(glm::lookAt(cameraPos, camTarget, camUp)));
    view = view * camArcBall.createRotationMatrix();

    // skybox 그리기.
    skyboxShader->setMat4("view", view);
    glDepthFunc(GL_LEQUAL);
    skybox->draw(skyboxShader, cubemapTexture);
    glDepthFunc(GL_LESS);
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void loadTexture() {

    // Create texture ids.
    glGenTextures(1, &texture);

    // All upcomming GL_TEXTURE_2D operations now on "texture" object
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters for wrapping.
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture parameters for filtering.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* image = stbi_load("res/textures/container.bmp", &width, &height, &nrChannels, 0);
    if (!image) {
        printf("texture %s loading error ... \n", "container.bmp");
    }
    else printf("texture %s loaded\n", "container.bmp");

    GLenum format;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

}


void initLinks()
{
    //Link(string name, glm::vec3 color, bool isRoot, int nChild,
    //     glm::vec3 size,
    //     glm::vec3 offset,
    //     glm::vec3 trans1,
    //     glm::vec3 trans2,
    //     glm::vec3 rot1,       //angles in degree
    //     glm::vec3 rot2)       //angles in degree

    // root link: yellow
    root = new Link("ROOT", glm::vec3(1.0, 1.0, 0.0), true, 1,
        glm::vec3(1.0, 5.0, 1.0),   // size
        glm::vec3(0.0, 0.0, 0.0),   // offset
        glm::vec3(-4.0, -2.0, 0.0),   // trans1 w.r.t. origin (because root)
        glm::vec3(0.0, 0.0, 0.0),   // trans2 w.r.t. origin (because root)
        glm::vec3(0.0, 0.0, 0.0),   // no rotation
        glm::vec3(0.0, 0.0, 0.0));  // no rotation

// left upper arm: red
    root->child[0] = new Link("LEFT_ARM_UPPER", glm::vec3(1.0, 0.0, 0.0), false, 1,
        glm::vec3(4.0, 1.0, 1.0),  // size
        glm::vec3(2.0, 0.0, 0.0),  // offset
        glm::vec3(0.0, 2.5, 0.0),  // trans1
        glm::vec3(0.0, 2.5, 0.0),  // trans2
        glm::vec3(0.0, 0.0, 0.0),  // rotation about parent
        glm::vec3(0.0, 0.0, 60.0));

    // left low arm: orange
    root->child[0]->child[0] = new Link("LEFT_ARM_LOWER", glm::vec3(1.0, 0.5, 0.0), false, 0,
        glm::vec3(2.0, 1.0, 1.0),  // size
        glm::vec3(1.0, 0.0, 0.0),  // offset
        glm::vec3(4.0, 0.0, 0.0),
        glm::vec3(4.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, -30.0, -50.0));
}

void drawLinks(Link* clink, float t, glm::mat4 cmodel, Shader* shader)
{

    if (t > 1.0) t = 1.0f;

    glm::mat4 thisMat = glm::mat4(1.0f);

    // accumulate the parent's transformation
    thisMat = thisMat * cmodel;

    // if root, interpolates the translation
    glm::vec3 ctrans = glm::mix(clink->trans1, clink->trans2, t);
    thisMat = glm::translate(thisMat, ctrans);

    // interpolates the rotation
    //glm::quat q = glm::slerp(clink->q1, clink->q2, t);
    glm::vec3 euler = glm::mix(clink->rot1, clink->rot2, t);
    glm::quat q = glm::quat(euler);

    glm::mat4 crot = q.operator glm::mat4x4();

    thisMat = thisMat * crot;

    // render the link
    shader->use();
    shader->setMat4("model", thisMat);
    clink->draw(shader);

    // recursively call the drawLinks for the children
    for (int i = 0; i < clink->nChild; i++) {
        drawLinks(clink->child[i], t, thisMat, shader);
    }

}

GLFWwindow *glAllInit()
{
    // Init GLFW
    glfwInit( );
    // Set all the required options for GLFW
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
    
    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow *window = glfwCreateWindow( SCR_WIDTH, SCR_HEIGHT, "Term Project", nullptr, nullptr );
    
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate( );
        exit(-1);
    }
    
    glfwMakeContextCurrent( window );
    
    // Set the required callback functions
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    if ( GLEW_OK != glewInit( ) )
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }
    
    // Define the viewport dimensions
    glViewport( 0, 0, SCR_WIDTH, SCR_HEIGHT );
    
    // OpenGL options
    glClearColor( 0.05f, 0.05f, 0.05f, 1.0f );
    glEnable( GL_DEPTH_TEST );
    
    return(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    projection = glm::perspective(glm::radians(73.0f),
                                  (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    modelShader->use();
    modelShader->setMat4("projection", projection);
    skyboxShader->use();
    skyboxShader->setMat4("projection", projection);
    linkShader->use();
    linkShader->setMat4("projection", projection);
    cubeShader->use();
    cubeShader->setMat4("projection", projection);

}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        camArcBall.init(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);
        modelArcBall.init(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);
        cameraPos = cameraOrigPos;
        modelPan[0] = modelPan[1] = modelPan[2] = 0.0f;
    }
    else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        arcballCamRot = !arcballCamRot;
        if (arcballCamRot) {
            cout << "ARCBALL: Camera rotation mode" << endl;
        }
        else {
            cout << "ARCBALL: Model  rotation mode" << endl;
        }
    }
    else if (key == GLFW_KEY_LEFT) {
        modelPan[0] -= 0.5;
    }
    else if (key == GLFW_KEY_RIGHT) {
        modelPan[0] += 0.5;
    }
    else if (key == GLFW_KEY_DOWN) {
        modelPan[1] -= 0.5;
    }
    else if (key == GLFW_KEY_UP) {
        modelPan[1] += 0.5;
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (renderMode == INIT) {
            renderMode = ANIM;
            beginT = glfwGetTime();
        }
        else {  // renderMode == ANIM
            renderMode = INIT;
            timeT = 0.0f;
        }
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (arcballCamRot)
        camArcBall.mouseButtonCallback( window, button, action, mods );
    else
        modelArcBall.mouseButtonCallback( window, button, action, mods );
}

void cursor_position_callback(GLFWwindow *window, double x, double y) {
    if (arcballCamRot)
        camArcBall.cursorCallback( window, x, y );
    else
        modelArcBall.cursorCallback( window, x, y );
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    cameraPos[2] -= (yoffset * 0.5);
}