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
#include <keyframe.h>
#include <text.h>
#include <plane.h>
#include <cube.h>
#include "link_m.h"


#include "skybox_m.h"

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


// Globals
unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;
GLFWwindow* mainWindow = NULL;
glm::mat4 projection, view, model;
Shader* skyboxShader;
Shader* modelShader;
Shader* modelZoomingShader;
Shader* cubeShader;
Shader* linkShader;
Shader* clawShader;
Shader* textShader = NULL;

// for arcball
float arcballSpeed = 0.2f;
static Arcball modelArcBall(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);
bool arcballCamRot = true;

// for camera
glm::vec3 cameraOrigPos(0.0f, 0.0f, 9.0f);
glm::vec3 cameraPos;
glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);

bool isHiding[7];
bool isZooming[7];
bool inventoryMode;
bool isAnimEnd;
bool isHavingObj;
bool isSelecting;

// Function prototypes

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
GLFWwindow* glAllInit();
void render();
void drawModels();


void loadTexture();
unsigned int loadTexture(char const* path, bool vflip);
unsigned int loadCubemap(vector<std::string> faces);

void initLinks();
void drawLinks(Link* clink, float t, glm::mat4 cmodel, Shader* shader);
void drawLinks2(Link* clink, float t, glm::mat4 cmodel, Shader* shader);

// for texture
unsigned int cubemapTexture;
unsigned int texture;
unsigned int transparentTexture;
unsigned int transparentTextureFliping;
unsigned int floorTexture;

// model
Model* buzz;
Model* woody;
Model* mousy;
Model* potato;
Model* wheezy;
Model* rex;
Cube* cube;
Plane* plane;


// text
Text* text = NULL;
bool isTexting = false;
string grabbingObject = "object";

// skybox
SkyBox* skybox;
vector<std::string> faces{
        "res/textures/right.jpg",
        "res/textures/left.jpg",
        "res/textures/top.png",
        "res/textures/bottom.jpg",
        "res/textures/back.jpg",
        "res/textures/front_symmetry.jpg"
};

// for animation
enum RenderMode { INIT, ANIM };
RenderMode renderMode;                  // current rendering mode
float beginT;                           // animation beginning time (in sec)
float timeT;                            // current time (in sec)
const float animEndTime = 2.0f;         // ending time of animation (in sec)
Link* root, * root2;
int targetKey;                          // 눌려진 key

// for lighting
glm::vec3 lightSize(0.2f, 0.2f, 0.2f);
glm::vec3 lightPos(10.0f, 10.0f, 10.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
float ambientStrength = 0.1f;

int main()
{
    mainWindow = glAllInit();
    skyboxShader = new Shader("res/shaders/skybox.vs", "res/shaders/skybox.frag");
    modelShader = new Shader("res/shaders/modelLoading.vs", "res/shaders/modelLoading.frag");
    modelZoomingShader = new Shader("res/shaders/modelZooming.vs", "res/shaders/modelZooming.frag");
    cubeShader = new Shader("res/shaders/cube.vs", "res/shaders/cube.fs");
    linkShader = new Shader("res/shaders/Link.vs", "res/shaders/Link.fs");
    clawShader = new Shader("res/shaders/clawShader.vs", "res/shaders/clawShader.fs");
    textShader = new Shader("res/shaders/text.vs", "res/shaders/text.frag");

    // Projection initialization
    projection = glm::perspective(glm::radians(73.0f),
        (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    skyboxShader->use();
    skyboxShader->setMat4("projection", projection);
    modelShader->use();
    modelShader->setMat4("projection", projection);
    modelZoomingShader->use();
    modelZoomingShader->setMat4("projection", projection);
    cubeShader->use();
    cubeShader->setMat4("projection", projection);

    linkShader->use();
    linkShader->setMat4("projection", projection);
    linkShader->setVec3("lightColor", lightColor);
    linkShader->setVec3("lightPos", lightPos);
    linkShader->setFloat("ambientStrength", ambientStrength);

    clawShader->use();
    clawShader->setMat4("projection", projection);

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

    cube = new Cube();
    plane = new Plane(1.5f, 0.7f, 0.0f, 3.0f);
    plane->texCoords[6] = 0.9;
    plane->texCoords[7] = 0.9;
    plane->updateVBO();

    // load cube texture
    loadTexture();
    transparentTexture = loadTexture("res/textures/claw.png", true);
    transparentTextureFliping = loadTexture("res/textures/clawFliping.png", true);
    floorTexture = loadTexture("res/textures/metal.png", false);
    //cubeShader->use();
    //cubeShader->setInt("texture1", 0);

    // initialize animation data
    initLinks();
    timeT = 0.0f;
    renderMode = INIT;


    // initialize text
    text = new Text((char*)"fonts/arial.ttf", textShader, SCR_WIDTH, SCR_HEIGHT);

    // loop
    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        render();

        glfwSwapBuffers(mainWindow);
    }

    glfwTerminate();
    return 0;
}

void drawModels() {
    // draw buzz
    modelShader->use();

    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.15f, 0.15f, 0.15f));
    model = glm::translate(model, glm::vec3(70.0f, -5.0f, -150.0f));
    if (isZooming[4]) {
        model = glm::translate(model, glm::vec3(-68.0f, -15.0f, 150.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        model = model * modelArcBall.createRotationMatrix();
    }

    model = glm::rotate(model, (float)5, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.25, glm::vec3(0.0f, 0.0f, 1.0f));

    view = glm::lookAt(cameraPos, camTarget, camUp);
    modelShader->setMat4("view", view);


    modelZoomingShader->use();
    modelZoomingShader->setMat4("view", view);

    modelZoomingShader->setVec3("light.position", lightPos);
    modelZoomingShader->setVec3("viewPos", cameraPos);

    // light properties
    modelZoomingShader->setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    modelZoomingShader->setVec3("light.diffuse", 0.7f, 0.7f, 0.7f);
    modelZoomingShader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    // material properties
    modelZoomingShader->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
    modelZoomingShader->setFloat("material.shininess", 64.0f);

    if (!isHiding[4]) {
        modelShader->use();
        modelShader->setMat4("model", model);
        buzz->Draw(modelShader);
    }

    if (isZooming[4]) {
        modelZoomingShader->use();
        modelZoomingShader->setMat4("model", model);

        buzz->Draw(modelZoomingShader);
    }




    // draw woody
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.15f, 0.15f, 0.15f));
    model = glm::translate(model, glm::vec3(-65.0f, -5.0f, -150.0f));
    if (isZooming[3]) {
        model = glm::translate(model, glm::vec3(65.0f, -15.0f, 150.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        model = model * modelArcBall.createRotationMatrix();
    }

    model = glm::rotate(model, (float)5, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.15, glm::vec3(0.0f, 0.0f, 1.0f));

    if (!isHiding[3]) {
        modelShader->use();
        modelShader->setMat4("model", model);
        woody->Draw(modelShader);
    }

    if (isZooming[3]) {
        modelZoomingShader->use();
        modelZoomingShader->setMat4("model", model);

        woody->Draw(modelZoomingShader);
    }



    // draw mousy
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.06f, 0.06f, 0.06f));
    model = glm::translate(model, glm::vec3(-140.0f, -230.0f, -300.0f));
    if (isZooming[5]) {
        model = glm::translate(model, glm::vec3(142.0f, 190.0f, 300.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        model = model * modelArcBall.createRotationMatrix();
    }

    if (!isHiding[5]) {
        modelShader->use();
        modelShader->setMat4("model", model);
        mousy->Draw(modelShader);
    }

    if (isZooming[5]) {
        modelZoomingShader->use();
        modelZoomingShader->setMat4("model", model);

        mousy->Draw(modelZoomingShader);
    }


    // draw rex
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.028f, 0.028f, 0.028f));
    model = glm::translate(model, glm::vec3(320.0f, 410.0f, -600.0f));
    if (isZooming[2]) {
        model = glm::translate(model, glm::vec3(-320.0f, -470.0f, 600.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        model = model * modelArcBall.createRotationMatrix();
    }

    model = glm::rotate(model, (float)0.4, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.75, glm::vec3(0.0f, 1.0f, 0.0f));


    if (!isHiding[2]) {
        modelShader->use();
        modelShader->setMat4("model", model);
        rex->Draw(modelShader);
    }
    if (isZooming[2]) {
        modelZoomingShader->use();
        modelZoomingShader->setMat4("model", model);

        rex->Draw(modelZoomingShader);
    }





    // draw wheezy
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
    model = glm::translate(model, glm::vec3(85.0f, -110.0f, -150.0f));
    if (isZooming[6]) {
        model = glm::translate(model, glm::vec3(-85.0f, 110.0f, 155.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        model = model * modelArcBall.createRotationMatrix();
    }

    model = glm::rotate(model, (float)4.7, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, (float)-0.15, glm::vec3(0.0f, 0.0f, 1.0f));


    if (!isHiding[6]) {
        modelShader->use();
        modelShader->setMat4("model", model);
        wheezy->Draw(modelShader);
    }
    if (isZooming[6]) {
        modelZoomingShader->use();
        modelZoomingShader->setMat4("model", model);

        wheezy->Draw(modelZoomingShader);
    }




    // draw cube
    model = glm::mat4(1.0f);

    model = glm::scale(model, glm::vec3(4.5f, 4.5f, 4.5f));
    model = glm::translate(model, glm::vec3(-2.5f, 4.0f, -5.0f));
    if (isZooming[1]) {
        model = glm::translate(model, glm::vec3(2.5f, -4.0f, 5.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        model = model * modelArcBall.createRotationMatrix();
    }

    model = glm::rotate(model, (float)0.75, glm::vec3(1.0f, 0.0f, 0.0f));

    cubeShader->use();
    cubeShader->setMat4("model", model);
    cubeShader->setMat4("view", view);

    // be sure to activate shader when setting uniforms/drawing objects
    cubeShader->setVec3("light.position", lightPos);
    cubeShader->setVec3("viewPos", cameraPos);

    // light properties
    cubeShader->setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    cubeShader->setVec3("light.diffuse", 0.7f, 0.7f, 0.7f);
    cubeShader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    // material properties
    cubeShader->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
    cubeShader->setFloat("material.shininess", 64.0f);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (!isHiding[1] || isZooming[1])
        cube->draw(cubeShader);
}

void render()
{
    //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    isAnimEnd = false;
    if (renderMode == ANIM) {
        float cTime = (float)glfwGetTime(); // current time
        timeT = cTime - beginT;
    }

    // draw 6 Objects.
    drawModels();



    // draw Link1
    linkShader->use();
    linkShader->setMat4("view", view);
    model = glm::mat4(1.0f);

    if (targetKey % 2 == 0)
        timeT = 0;
    drawLinks(root, timeT / animEndTime, model, linkShader);



    // draw Link2
    if (renderMode == ANIM) {
        float cTime = (float)glfwGetTime(); // current time
        timeT = cTime - beginT;
    }
    model = glm::mat4(1.0f);
    if (targetKey % 2 == 1)
        timeT = 0;
    drawLinks2(root2, timeT / animEndTime, model, linkShader);


    bool hasZoomingObj = false;
    for (int i = 1; i <= 6; i++) {
        if (isZooming[i]) {
            hasZoomingObj = true;
            break;
        }
    }

    if (hasZoomingObj) {
        // drawing a floor
        glBindTexture(GL_TEXTURE_2D, floorTexture);

        clawShader->use();
        model = glm::mat4(1.0);
        model = glm::translate(model, glm::vec3(-18.0f, -7.0f, -4.5f));
        model = glm::scale(model, glm::vec3(13.0f, 8.0f, 0.0f));

        view = glm::lookAt(cameraPos, camTarget, camUp);
        clawShader->setMat4("view", view);
        clawShader->setMat4("model", model);
        plane->draw(clawShader);
    }

    // Skybox
    skyboxShader->use();
    view = glm::mat4(glm::mat3(glm::lookAt(cameraPos, camTarget, camUp)));

    // draw skybox.
    skyboxShader->setMat4("view", view);
    glDepthFunc(GL_LEQUAL);
    skybox->draw(skyboxShader, cubemapTexture);
    glDepthFunc(GL_LESS);

    if (isTexting && isAnimEnd) {
        // Drawing texts
        text->RenderText("You got " + grabbingObject + "!", 500.0f, 53.0f, 1.0f, glm::vec3(0.0, 0.0f, 0.0f));
    }

    if (isSelecting) {
        text->RenderText("Select 1 to 6", 40.0f, 660.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
    }

    if (inventoryMode && !isHavingObj) {
        text->RenderText("you don't have " + grabbingObject + "!", 430.0f, 53.0f, 1.0f, glm::vec3(0.0, 0.0f, 0.0f));
    }

    bool isAllCollected = true;
    for (int i = 1; i <= 6; i++) {
        if (!isHiding[i]) {
            isAllCollected = false;
            break;
        }
    }

    if (isAllCollected && !hasZoomingObj)
        text->RenderText("you have collected All toys!", 430.0f, 300.0f, 0.8f, glm::vec3(0.3, 0.7f, 0.9f));

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

unsigned int loadTexture(char const* path, bool vflip)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    if (vflip) stbi_set_flip_vertically_on_load(true);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void initLinks()
{
    // Origin
    // root link: yellow
    root = new Link("ROOT", glm::vec3(1.0, 1.0, 0.0), true, 1,
        glm::vec3(0.5, 5.0, 0.5),   // size
        glm::vec3(0.0, 0.0, 0.0),   // offset
        glm::vec3(-12.0, -10.0, -5.0),   // trans1 w.r.t. origin (because root)
        glm::vec3(-12.0, -2.0, -5.0),   // trans2 w.r.t. origin (because root)
        glm::vec3(-12.0, 2.0, -5.0),   // trans3 w.r.t. origin (because root)
        glm::vec3(0.0, 0.0, 0.0),   // no rotation
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0));  // no rotation

    // left upper arm: red
    root->child[0] = new Link("LEFT_ARM_UPPER", glm::vec3(1.0, 0.0, 0.0), false, 1,
        glm::vec3(3.0, 0.5, 0.5),  // size
        glm::vec3(1.5, 0.0, 0.0),  // offset
        glm::vec3(0.0, 2.75, 0.0),  // trans1
        glm::vec3(0.0, 2.5, 0.0),  // trans2
        glm::vec3(0.0, 2.5, 0.0),  // trans3
        glm::vec3(0.0, 0.0, 0.0),  // rotation about parent
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 110.0));


    root->child[0]->child[0] = new Link("LEFT_ARM_LOWER", glm::vec3(1.0, 0.5, 0.0), false, 1,
        glm::vec3(3.0, 0.5, 0.5),  // size
        glm::vec3(1.5, 0.0, 0.0),  // offset
        glm::vec3(3.0, 0.0, 0.0),
        glm::vec3(3.0, 0.0, 0.0),
        glm::vec3(3.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, -50.0),
        glm::vec3(0.0, 0.0, -50.0),
        glm::vec3(0.0, 0.0, -40.0));

    root->child[0]->child[0]->child[0] = new Link("CLAW", glm::vec3(1.0, 0.5, 0.05), false, 0,
        glm::vec3(3.0, 0.5, 0.5),  // size
        glm::vec3(0.5, 0.0, 0.0),  // offset
        glm::vec3(4.0, 0, 0.0),
        glm::vec3(4.0, 0, 0.0),
        glm::vec3(4.0, 0, 0.0),
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0));

    root->child[0]->child[0]->child[0]->plane->texCoords[6] = 0.9;
    root->child[0]->child[0]->child[0]->plane->texCoords[7] = 0.9;
    root->child[0]->child[0]->child[0]->plane->updateVBO();


    // ===========================================================================================
    // 2번째 Link

    root2 = new Link("ROOT2", glm::vec3(0.607, 0.639, 0.92), true, 1,
        glm::vec3(0.5, 5.0, 0.5),   // size
        glm::vec3(0.0, 0.0, 0.0),   // offset
        glm::vec3(12.5, -10.0, -5.0),   // trans1 w.r.t. origin (because root)
        glm::vec3(12.5, -2.0, -5.0),   // trans2 w.r.t. origin (because root)
        glm::vec3(12.5, 2.0, -5.0),   // trans3 w.r.t. origin (because root)
        glm::vec3(0.0, 0.0, 0.0),   // no rotation
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0));  // no rotation

    // left upper arm
    root2->child[0] = new Link("LEFT_ARM_UPPER2", glm::vec3(0.392, 0.435, 0.831), false, 1,
        glm::vec3(3.0, 0.5, 0.5),  // size
        glm::vec3(-1.5, 0.0, 0.0),  // offset
        glm::vec3(0.0, 2.75, 0.0),  // trans1
        glm::vec3(0.0, 2.5, 0.0),  // trans2
        glm::vec3(0.0, 2.5, 0.0),  // trans3
        glm::vec3(0.0, 0.0, 0.0),  // rotation about parent
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, -110.0));

    // left low arm
    root2->child[0]->child[0] = new Link("LEFT_ARM_LOWER2", glm::vec3(0.141, 0.184, 0.607), false, 1,
        glm::vec3(3.0, 0.5, 0.5),  // size
        glm::vec3(-1.5, 0.0, 0.0),  // offset
        glm::vec3(-3.0, 0.0, 0.0),
        glm::vec3(-3.0, 0.0, 0.0),
        glm::vec3(-3.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 50.0),
        glm::vec3(0.0, 0.0, 50.0),
        glm::vec3(0.0, 0.0, 30.0));

    root2->child[0]->child[0]->child[0] = new Link("CLAW2", glm::vec3(1.0, 0.5, 0.05), false, 0,
        glm::vec3(3.0, 0.5, 0.5),  // size
        glm::vec3(-0.5, 0.0, 0.0),  // offset
        glm::vec3(-4.0, 0, 0.0),
        glm::vec3(-4.0, 0, 0.0),
        glm::vec3(-4.0, 0, 0.0),
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 0.0));

    root2->child[0]->child[0]->child[0]->plane->texCoords[0] = 0.1;
    root2->child[0]->child[0]->child[0]->plane->texCoords[1] = 0.9;
    root2->child[0]->child[0]->child[0]->plane->updateVBO();
}

void drawLinks2(Link* clink, float t, glm::mat4 cmodel, Shader* shader) {
    if (clink->name == "CLAW2") {
        // texture할 수 있는 clawShader로 바꾼다.
        shader = clawShader;
    }

    if (t > 1.0) {
        t = 1.0f;
        isHiding[targetKey] = true;
        isAnimEnd = true;
    }

    glm::mat4 thisMat = glm::mat4(1.0f);

    // accumulate the parent's transformation
    thisMat = thisMat * cmodel;

    // if root, interpolates the translation
    glm::vec3 ctrans;

    if (t < 0.5)
        ctrans = glm::mix(clink->trans1, clink->trans2, t + 0.5);
    else
        ctrans = glm::mix(clink->trans2, clink->trans3, t - 0.5);

    thisMat = glm::translate(thisMat, ctrans);


    glm::vec3 euler;
    if (t < 0.5)
        euler = glm::mix(clink->rot1, clink->rot2, t + 0.5);
    else
        euler = glm::mix(clink->rot2, clink->rot3, t - 0.5);


    glm::quat q = glm::quat(euler);
    glm::mat4 crot = q.operator glm::mat4x4();

    thisMat = thisMat * crot;



    // render the link
    shader->use();
    shader->setMat4("model", thisMat);
    if (shader == clawShader) {
        shader->setMat4("view", view);
        glBindTexture(GL_TEXTURE_2D, transparentTextureFliping);
        clink->plane->draw(shader);
    }
    else
        clink->draw(shader);

    // recursively call the drawLinks for the children
    for (int i = 0; i < clink->nChild; i++) {
        drawLinks2(clink->child[i], t, thisMat, shader);
    }


}

void drawLinks(Link* clink, float t, glm::mat4 cmodel, Shader* shader)
{
    if (clink->name == "CLAW") {
        // texture할 수 있는 clawShader로 바꾼다.
        shader = clawShader;
    }

    if (t > 1.0) {
        t = 1.0f;
        isHiding[targetKey] = true;
        isAnimEnd = true;
    }

    glm::mat4 thisMat = glm::mat4(1.0f);

    // accumulate the parent's transformation
    thisMat = thisMat * cmodel;

    // if root, interpolates the translation
    glm::vec3 ctrans;

    if (t < 0.5)
        ctrans = glm::mix(clink->trans1, clink->trans2, t + 0.5);
    else
        ctrans = glm::mix(clink->trans2, clink->trans3, t - 0.5);

    thisMat = glm::translate(thisMat, ctrans);


    glm::vec3 euler;
    if (t < 0.5)
        euler = glm::mix(clink->rot1, clink->rot2, t + 0.5);
    else
        euler = glm::mix(clink->rot2, clink->rot3, t - 0.5);


    glm::quat q = glm::quat(euler);
    glm::mat4 crot = q.operator glm::mat4x4();

    thisMat = thisMat * crot;



    // render the link
    shader->use();
    shader->setMat4("model", thisMat);
    if (shader == clawShader) {
        shader->setMat4("view", view);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        clink->plane->draw(shader);
    }
    else
        clink->draw(shader);

    // recursively call the drawLinks for the children
    for (int i = 0; i < clink->nChild; i++) {
        drawLinks(clink->child[i], t, thisMat, shader);
    }

}

GLFWwindow* glAllInit()
{
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Term Project", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }

    // Define the viewport dimensions
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // OpenGL options
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
    modelZoomingShader->use();
    modelZoomingShader->setMat4("projection", projection);
    skyboxShader->use();
    skyboxShader->setMat4("projection", projection);
    linkShader->use();
    linkShader->setMat4("projection", projection);
    cubeShader->use();
    cubeShader->setMat4("projection", projection);
    clawShader->use();
    clawShader->setMat4("projection", projection);

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        modelArcBall.init(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);
        cameraPos = cameraOrigPos;

        for (int i = 1; i <= 6; i++) {
            isZooming[i] = false;
            isHiding[i] = false;
        }
        inventoryMode = false;
        isHavingObj = false;
        isAnimEnd = false;
        isSelecting = false;
        grabbingObject = "object";

        initLinks();
        timeT = 0.0f;
        renderMode = INIT;


    }
    else if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        inventoryMode = !inventoryMode;
        isSelecting = inventoryMode;

        isTexting = false;

        int zoomingTarget = -1;
        for (int i = 1; i <= 6; i++) {
            if (isZooming[i]) {
                zoomingTarget = i;
                break;
            }
        }
        if (zoomingTarget == -1) return;
        isZooming[zoomingTarget] = !isZooming[zoomingTarget];

    }

    else if (GLFW_KEY_1 <= key && key <= GLFW_KEY_6 && action == GLFW_PRESS) {
        if (inventoryMode) {
            int targetKey = key - GLFW_KEY_1 + 1;
            modelArcBall.init(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);

            for (int i = 1; i <= 6; i++) {
                if (i == targetKey) continue;

                if (isZooming[i]) return;
            }
            // 획득하지 못했다면 zooming 할 수 없다.
            if (!isHiding[targetKey]) {
                isHavingObj = false;

                switch (key)
                {
                case GLFW_KEY_1:
                    grabbingObject = "Cube";
                    break;
                case GLFW_KEY_2:
                    grabbingObject = "Rex";
                    break;
                case GLFW_KEY_3:
                    grabbingObject = "Woody";
                    break;
                case GLFW_KEY_4:
                    grabbingObject = "Buzz";
                    break;
                case GLFW_KEY_5:
                    grabbingObject = "Mousy";
                    break;
                case GLFW_KEY_6:
                    grabbingObject = "Wheezy";
                    break;
                default:
                    break;
                }
                return;
            }
            isHavingObj = true;
            isSelecting = !isSelecting;
            isZooming[targetKey] = !isZooming[targetKey];

        }
        else {
            isTexting = true;
            isHavingObj = true;

            if (renderMode == INIT) {
                renderMode = ANIM;
                beginT = glfwGetTime();
            }
            else {  // renderMode == ANIM
                renderMode = INIT;
                timeT = 0.0f;

                root->child[0]->child[0]->cube = new Cube(1.5, 0.0, 0.0, 3.0, 0.5, 0.5);
                root->child[0]->child[0]->cube->initBuffers();

                root->child[0]->child[0]->child[0]->plane = new Plane(0.5, 0.0, 0.0, 3.0);
                root->child[0]->child[0]->child[0]->plane->texCoords[6] = 0.9;
                root->child[0]->child[0]->child[0]->plane->texCoords[7] = 0.9;
                root->child[0]->child[0]->child[0]->plane->updateVBO();
            }

            switch (key)
            {
            case GLFW_KEY_1:
                grabbingObject = "Cube";
                targetKey = 1;
                root->child[0]->rot3 = glm::radians(glm::vec3(0.0, 0.0, 130.0));
                root->trans1 = glm::vec3(-12.0, -10.0, -5.0);   // trans1 w.r.t. origin (because root)
                root->trans2 = glm::vec3(-12.0, -2.0, -5.0);   // trans2 w.r.t. origin (because root)
                root->trans3 = glm::vec3(-12.0, 2.0, -5.0);   // trans3 w.r.t. origin (because root)

                break;
            case GLFW_KEY_2:
                grabbingObject = "Rex";
                targetKey = 2;

                root2->child[0]->rot3 = glm::radians(glm::vec3(0.0, 0.0, -110.0));
                root2->trans1 = glm::vec3(12.5, -10.0, -5.0);   // trans1 w.r.t. origin (because root)
                root2->trans2 = glm::vec3(12.5, -2.0, -5.0);   // trans2 w.r.t. origin (because root)
                root2->trans3 = glm::vec3(12.5, 2.0, -5.0);   // trans3 w.r.t. origin (because root)

                break;
            case GLFW_KEY_3:
                grabbingObject = "Woody";
                targetKey = 3;

                root->child[0]->rot3 = glm::radians(glm::vec3(0.0, 0.0, 40.0));
                root->trans1 = glm::vec3(-12.0, -10.0, -5.0);   // trans1 w.r.t. origin (because root)
                root->trans2 = glm::vec3(-12.0, -2.0, -5.0);   // trans2 w.r.t. origin (because root)
                root->trans3 = glm::vec3(-12.0, 2.0, -5.0);   // trans3 w.r.t. origin (because root)
                break;
            case GLFW_KEY_4:
                grabbingObject = "Buzz";
                targetKey = 4;

                root2->child[0]->rot3 = glm::radians(glm::vec3(0.0, 0.0, -40.0));
                root2->trans1 = glm::vec3(12.5, -10.0, -5.0);   // trans1 w.r.t. origin (because root)
                root2->trans2 = glm::vec3(12.5, -2.0, -5.0);   // trans2 w.r.t. origin (because root)
                root2->trans3 = glm::vec3(12.5, 2.0, -5.0);   // trans3 w.r.t. origin (because root)
                break;
            case GLFW_KEY_5:
                grabbingObject = "Mousy";
                targetKey = 5;

                root->trans1 = glm::vec3(-12.0, -6.0, -5.0);  // trans1 w.r.t. origin (because root)
                root->trans2 = glm::vec3(-12.0, -6.0, -5.0);   // trans2 w.r.t. origin (because root)
                root->trans3 = glm::vec3(-12.0, -6.0, -5.0);   // trans3 w.r.t. origin (because root)

                root->child[0]->rot3 = glm::radians(glm::vec3(0.0, 0.0, 20.0));

                break;
            case GLFW_KEY_6:
                grabbingObject = "Wheezy";
                targetKey = 6;

                root2->trans1 = glm::vec3(12.5, -6.0, -5.0);   // trans1 w.r.t. origin (because root)
                root2->trans2 = glm::vec3(12.5, -6.0, -5.0);   // trans2 w.r.t. origin (because root)
                root2->trans3 = glm::vec3(12.5, -6.0, -5.0);   // trans3 w.r.t. origin (because root)

                root2->child[0]->rot3 = glm::radians(glm::vec3(0.0, 0.0, -20.0));
                break;
            default:
                break;
            }
        }

    }

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    modelArcBall.mouseButtonCallback(window, button, action, mods);
}

void cursor_position_callback(GLFWwindow* window, double x, double y) {
    modelArcBall.cursorCallback(window, x, y);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraPos[2] -= (yoffset * 0.5);
}