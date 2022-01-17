//
//  main.cpp
//  PhysicsSimulator
//
//  Created by Albert Go on 10/31/21.
//

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <math.h>
#include <numeric>

#include "shaderClass.h"
#include "VAO.h"
#include "VAO.h"
#include "EBO.h"
//#include "Camera.h"
using namespace std;

struct PointMass{
    double mass;
    vector<float> position; // {x, y, z}
    vector<float> velocity; // {v_x, v_y, v_z}
    vector<float> acceleration; // {a_x, a_y, a_z}
    vector<float> forces; // {f_x, f_y, f_z}
    vector<float> potential;
    vector<float> kinetic;
};

struct Spring{
    float L0; // resting length
    float L; // current length
    float k; // spring constant
    int m0; // connected to which PointMass
    int m1; // connected to which PointMass
    vector<float> potential;
    float original_L0;
};

const double g = -9.81; //acceleration due to gravity
const double b = 0.999; //damping (optional) Note: no damping means your cube will bounce forever
const float spring_constant = 10000.0f; //this worked best for me given my dt and mass of each PointMass
float T = 0.0;
float dt = 0.001;
bool breathing = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void initialize_masses(vector<PointMass> &masses);
void initialize_springs(vector<Spring> &springs);
void apply_force(vector<PointMass> &masses);
void update_pos_vel_acc(vector<PointMass> &masses);
void update_forces(vector<PointMass> &masses, vector<Spring> &springs);
void reset_forces(vector<PointMass> &masses);
void update_breathing(vector<Spring> &springs);

const unsigned int width = 1000;
const unsigned int height = 1000;

// camera
glm::vec3 cameraPos   = glm::vec3(10.0f, 2.0f, 3.0f); //move the camera such that it is facing the cube
glm::vec3 cameraFront = glm::vec3(0.0f, 1.0f, 0.0f); //make the front of the camera looking towards the positive y axis direction
glm::vec3 cameraUp    = glm::vec3(0.0f, 0.0f, 1.0f); //make the camera such that the z axis is going up

bool firstMouse = true;
bool firstClick = true;
float yaw   = -45.0f;    // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float lastX =  800.0f / 2.0;
float lastY =  600.0 / 2.0;
float fov   =  60.0f;

// timing
float deltaTime = 0.0f;    // time between current frame and last frame
float lastFrame = 0.0f;

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    float cameraSpeed = 2.5 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        cameraPos += cameraSpeed * cameraUp;
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
        cameraPos -= cameraSpeed * cameraUp;
    }
}

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(width, height, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    Shader shaderProgram("/Users/albertgo/Documents/MECS_4510/PSet_3a/PhysicsSimulator/Externals/Resources/Shaders/default.vert", "/Users/albertgo/Documents/MECS_4510/PSet_3a/PhysicsSimulator/Externals/Resources/Shaders/default.frag");
    
    glEnable(GL_DEPTH_TEST);
    
    float grid[] = {
         0.0f,  0.0f,  0.0f,
         0.0f,  0.5f,  0.0f,
         0.5f,  0.5f,  0.0f,
         0.5f,  0.0f,  0.0f,
    };
    VAO vao;
    vao.Bind();
    
    VBO vbo(grid, sizeof(grid));
    
    vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, 3*sizeof(float), (void*)0);
    
    vao.Unbind();
    vbo.Unbind();
    
    // world space positions of cube
    std::vector<glm::vec3> gridPositions;
    int slices = 50;
    for(int j=0; j<slices; ++j) {
        for(int i=0; i<slices; ++i) {
          float row1 =  i * 0.5;
          float row2 =  j * 0.5;
          gridPositions.push_back(glm::vec3(row1, row2, 0));

        }
      }
    
    vector<PointMass> masses;
    vector<Spring> springs;
    initialize_masses(masses);
    initialize_springs(springs);
    
    float prev_T = 0;
    int iterations = 0;
    
    float x0 = masses[0].position[0];
    float y0 = masses[0].position[1];
    float z0 = masses[0].position[2];
    float x1 = masses[1].position[0];
    float y1 = masses[1].position[1];
    float z1 = masses[1].position[2];
    float x2 = masses[2].position[0];
    float y2 = masses[2].position[1];
    float z2 = masses[2].position[2];
    float x3 = masses[3].position[0];
    float y3 = masses[3].position[1];
    float z3 = masses[3].position[2];
    float x4 = masses[4].position[0];
    float y4 = masses[4].position[1];
    float z4 = masses[4].position[2];
    float x5 = masses[5].position[0];
    float y5 = masses[5].position[1];
    float z5 = masses[5].position[2];
    float x6 = masses[6].position[0];
    float y6 = masses[6].position[1];
    float z6 = masses[6].position[2];
    float x7 = masses[7].position[0];
    float y7 = masses[7].position[1];
    float z7 = masses[7].position[2];
    
    vector<float> PE; //total potential energy of the system
    vector<float> KE; //total kinetic energy of the system
    vector<float> TE; //total energy of the system
    
    // render loop
    while(!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = (currentFrame - lastFrame);
        lastFrame = currentFrame;
        
        processInput(window);
        
        // render
        // ------
        glClearColor(0.1f, 0.3f, 0.4f, 1.0f);
//        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shaderProgram.Activate();
        
        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        int viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        int projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
        
        // render the grid
        //-------------------------------------
        for (unsigned int i = 0; i < 2500; i++){
            vao.Bind();
            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            model = glm::translate(model, gridPositions[i]);
            float angle = 20.0f * 0;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            int modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }
        //-------------------------------------
        
        //Update the forces, acceleration, velocity, and position
        //-------------------------------------
//        if (T == 0){
//            apply_force(masses); //apply spinning force (optional)
//        }
//        if (breathing) {
//            update_breathing(springs);
//        }

        update_forces(masses, springs);
        update_pos_vel_acc(masses);
        //-------------------------------------
        
        prev_T = T;
        T = T + dt; //update time that has passed
        
        //Update the position on the actual simulator only after every 50 simulations
        //-------------------------------------
        if (iterations % 1 == 0){
            x0 = masses[0].position[0];
            y0 = masses[0].position[1];
            z0 = masses[0].position[2];
            x1 = masses[1].position[0];
            y1 = masses[1].position[1];
            z1 = masses[1].position[2];
            x2 = masses[2].position[0];
            y2 = masses[2].position[1];
            z2 = masses[2].position[2];
            x3 = masses[3].position[0];
            y3 = masses[3].position[1];
            z3 = masses[3].position[2];
            x4 = masses[4].position[0];
            y4 = masses[4].position[1];
            z4 = masses[4].position[2];
            x5 = masses[5].position[0];
            y5 = masses[5].position[1];
            z5 = masses[5].position[2];
            x6 = masses[6].position[0];
            y6 = masses[6].position[1];
            z6 = masses[6].position[2];
            x7 = masses[7].position[0];
            y7 = masses[7].position[1];
            z7 = masses[7].position[2];
        }
        //-------------------------------------
        
        //Calculate the Potential and Kinetic Energy at this point in time
        //-------------------------------------
        if (iterations % 10 == 0){
            float total_KE = 0;
            float total_PE = 0;
            float total_E = 0;
            
            for (int j=0; j<8; j++){
                float v_x = masses[j].velocity[0];
                float v_y = masses[j].velocity[1];
                float v_z = masses[j].velocity[2];
                
                total_KE += 0.5 * masses[j].mass * (pow(v_x, 2) + pow(v_y, 2) + pow(v_z, 2));
                
                float p_z = masses[j].position[2];

                total_PE += masses[j].mass * 9.81 * p_z;
            }
            
            for (int k=0; k<28; k++){
                float L = springs[k].L;
                float L0 = springs[k].L0;
                
                total_PE += 0.5 * springs[k].k * pow(L-L0, 2);
            }
            total_E = total_PE + total_KE;
            
            PE.push_back(total_PE);
            KE.push_back(total_KE);
            TE.push_back(total_E);
            
            cout << "Calculated Energy in System" << endl;
        }
        //-------------------------------------
        
        // update the cube
        //-------------------------------------
        GLfloat vertices[] =
        {
            x0, y0, z0,      0.83f, 0.70f, 0.44f,
            x1, y1, z1,      0.83f, 0.70f, 0.44f,
            x2, y2, z2,      0.83f, 0.70f, 0.44f,
            x3, y3, z3,      0.83f, 0.70f, 0.44f,
            x4, y4, z4,      0.92f, 0.86f, 0.76f,
            x5, y5, z5,      0.92f, 0.86f, 0.76f,
            x6, y6, z6,      0.92f, 0.86f, 0.76f,
            x7, y7, z7,      0.92f, 0.86f, 0.76f,
        };
        
        GLuint indices[] =
        {
            0, 1, 2,
            0, 2, 3,
            0, 1, 5,
            0, 4, 5,
            1, 2, 6,
            1, 5, 6,
            2, 3, 7,
            2, 6, 7,
            4, 5, 6,
            4, 6, 7,
            0, 3, 7,
            0, 4, 7
        };
        //-------------------------------------
        
        glm::vec3 cubePositions[] = {
            glm::vec3( 10.0f, 10.0f, 0.0f),
        };
        
        VAO VAO1;
        VAO1.Bind();
        
        VBO VBO1(vertices, sizeof(vertices));
        EBO EBO1(indices, sizeof(indices));
        
    //    VAO1.LinkVBO(VBO1, 0);
        VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6*sizeof(float), (void*)0);
        VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6*sizeof(float), (void*)(3*sizeof(float)));
        VAO1.Unbind();
        VBO1.Unbind();
        EBO1.Unbind();
        
        VAO1.Bind();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[0]);
        float angle = 20.0f * 0;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        int modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_LINE_LOOP, 0, 4);
//        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);
        glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(int), GL_UNSIGNED_INT, 0);
        VAO1.Delete();
        VBO1.Delete();
        EBO1.Delete();
        
        reset_forces(masses);
        iterations += 1;
        cout << iterations << endl;
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
    }
    
//    VAO1.Delete();
//    VBO1.Delete();
//    EBO1.Delete();
    shaderProgram.Delete();

    glfwTerminate();
    
    cout << "Potential Energy: ";
    for (int m=0; m<PE.size(); m++){
        if (m==0){
            cout << "[";
        }
        cout << PE[m];
        if (m < PE.size()-1){
            cout << ", ";
        }
    }
    cout << "]" << endl;
    
    cout << "Kinetic Energy: ";
    for (int n=0; n<KE.size(); n++){
        if (n==0){
            cout << "[";
        }
        cout << KE[n];
        if (n < KE.size()-1){
            cout << ", ";
        }
    }
    cout << "]" << endl;
    
    cout << "Total Energy: ";
    for (int p=0; p<TE.size(); p++){
        if (p==0){
            cout << "[";
        }
        cout << TE[p];
        if (p < TE.size()-1){
            cout << ", ";
        }
    }
    cout << "]" << endl;
    
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void initialize_masses(vector<PointMass> &masses){
    //Point Mass of bottom, front left vertex
    //----------------------
    PointMass mass0;
    mass0.mass = 0.5f;
    mass0.position = {-0.25f, -0.25f, 1.0f};
    mass0.velocity = {0.0f, 0.0f, 0.0f};
    mass0.acceleration = {0.0f, 0.0f, 0.0f};
    mass0.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    //Point Mass of bottom, back left vertex
    //----------------------
    PointMass mass1;
    mass1.mass = 0.5f;
    mass1.position = {-0.25f, 0.25f, 1.0f};
    mass1.velocity = {0, 0, 0};
    mass1.acceleration = {0, 0, 0};
    mass1.forces = {0, 0, 0};
    //----------------------
    
    //Point Mass of bottom, back right vertex
    //----------------------
    PointMass mass2;
    mass2.mass = 0.5f;
    mass2.position = {0.25f, 0.25f, 1.0f};
    mass2.velocity = {0.0f, 0.0f, 0.0f};
    mass2.acceleration = {0.0f, 0.0f, 0.0f};
    mass2.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    //Point Mass of bottom, front right vertex
    //----------------------
    PointMass mass3;
    mass3.mass = 0.5f;
    mass3.position = {0.25f, -0.25f, 1.0f};
    mass3.velocity = {0.0f, 0.0f, 0.0f};
    mass3.acceleration = {0.0f, 0.0f, 0.0f};
    mass3.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    //Point Mass of top, front left vertex
    //----------------------
    PointMass mass4;
    mass4.mass = 0.5f;
    mass4.position = {-0.25f, -0.25f, 1.5f};
    mass4.velocity = {0.0f, 0.0f, 0.0f};
    mass4.acceleration = {0.0f, 0.0f, 0.0f};
    mass4.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    //Point Mass of top, back left vertex
    //----------------------
    PointMass mass5;
    mass5.mass = 0.5f;
    mass5.position = {-0.25f, 0.25f, 1.5f};
    mass5.velocity = {0.0f, 0.0f, 0.0f};
    mass5.acceleration = {0.0f, 0.0f, 0.0f};
    mass5.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    //Point Mass of top, back right vertex
    //----------------------
    PointMass mass6;
    mass6.mass = 0.5f;
    mass6.position = {0.25f, 0.25f, 1.5f};
    mass6.velocity = {0.0f, 0.0f, 0.0f};
    mass6.acceleration = {0.0f, 0.0f, 0.0f};
    mass6.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    //Point Mass of top, front right vertex
    //----------------------
    PointMass mass7;
    mass7.mass = 0.5f;
    mass7.position = {0.25f, -0.25f, 1.5f};
    mass7.velocity = {0.0f, 0.0f, 0.0f};
    mass7.acceleration = {0.0f, 0.0f, 0.0f};
    mass7.forces = {0.0f, 0.0f, 0.0f};
    //----------------------
    
    masses = {mass0, mass1, mass2, mass3, mass4, mass5, mass6, mass7};
    
}

void initialize_springs(vector<Spring> &springs){
    
    //Bottom Face of the Cube
    //-----------------------
    Spring spring0;
    spring0.L0 = 0.5f;
    spring0.L = 0.5f;
    spring0.k = spring_constant;
    spring0.m0 = 0;
    spring0.m1 = 1;
    spring0.original_L0 = 0.5f;
    
    Spring spring1;
    spring1.L0 = 0.5f;
    spring1.L = 0.5f;
    spring1.k = spring_constant;
    spring1.m0 = 1;
    spring1.m1 = 2;
    spring1.original_L0 = 0.5f;
    
    Spring spring2;
    spring2.L0 = 0.5f;
    spring2.L = 0.5f;
    spring2.k = spring_constant;
    spring2.m0 = 2;
    spring2.m1 = 3;
    spring2.original_L0 = 0.5f;
    
    Spring spring3;
    spring3.L0 = 0.5f;
    spring3.L = 0.5f;
    spring3.k = spring_constant;
    spring3.m0 = 3;
    spring3.m1 = 0;
    spring3.original_L0 = 0.5f;
    //----------------------
    
    //Cross Springs of Bottom Face
    //----------------------
    Spring spring4;
    spring4.L0 = 0.5f*sqrt(2.0f);
    spring4.L = 0.5f*sqrt(2.0f);
    spring4.k = spring_constant;
    spring4.m0 = 0;
    spring4.m1 = 2;
    spring4.original_L0 = 0.5f*sqrt(2.0f);
    
    Spring spring5;
    spring5.L0 = 0.5f*sqrt(2.0f);
    spring5.L = 0.5f*sqrt(2.0f);
    spring5.k = spring_constant;
    spring5.m0 = 1;
    spring5.m1 = 3;
    spring5.original_L0 = 0.5f*sqrt(2.0f);
    //----------------------
    
    //Vertical Supports of Cube
    //----------------------
    Spring spring6;
    spring6.L0 = 0.5f;
    spring6.L = 0.5f;
    spring6.k = spring_constant;
    spring6.m0 = 0;
    spring6.m1 = 4;
    spring6.original_L0 = 0.5f;
    
    Spring spring7;
    spring7.L0 = 0.5f;
    spring7.L = 0.5f;
    spring7.k = spring_constant;
    spring7.m0 = 1;
    spring7.m1 = 5;
    spring7.original_L0 = 0.5f;
    
    Spring spring8;
    spring8.L0 = 0.5f;
    spring8.L = 0.5f;
    spring8.k = spring_constant;
    spring8.m0 = 2;
    spring8.m1 = 6;
    spring8.original_L0 = 0.5f;
    
    Spring spring9;
    spring9.L0 = 0.5f;
    spring9.L = 0.5f;
    spring9.k = spring_constant;
    spring9.m0 = 3;
    spring9.m1 = 7;
    spring9.original_L0 = 0.5f;
    //---------------------
    
    //Cross Springs of Front Face
    //---------------------
    Spring spring10;
    spring10.L0 = 0.5f*sqrt(2.0f);
    spring10.L = 0.5f*sqrt(2.0f);
    spring10.k = spring_constant;
    spring10.m0 = 0;
    spring10.m1 = 7;
    spring10.original_L0 = 0.5f*sqrt(2.0f);
    
    Spring spring11;
    spring11.L0 = 0.5f*sqrt(2.0f);
    spring11.L = 0.5f*sqrt(2.0f);
    spring11.k = spring_constant;
    spring11.m0 = 3;
    spring11.m1 = 4;
    spring11.original_L0 = 0.5f*sqrt(2.0f);
    //---------------------
    
    //Cross Springs of Left Face
    //---------------------
    Spring spring12;
    spring12.L0 = 0.5f*sqrt(2.0f);
    spring12.L = 0.5f*sqrt(2.0f);
    spring12.k = spring_constant;
    spring12.m0 = 0;
    spring12.m1 = 5;
    spring12.original_L0 = 0.5f*sqrt(2.0f);
    
    Spring spring13;
    spring13.L0 = 0.5f*sqrt(2.0f);
    spring13.L = 0.5f*sqrt(2.0f);
    spring13.k = spring_constant;
    spring13.m0 = 1;
    spring13.m1 = 4;
    spring13.original_L0 = 0.5f*sqrt(2.0f);
    //---------------------
    
    //Cross Springs of Back Face
    //---------------------
    Spring spring14;
    spring14.L0 = 0.5f*sqrt(2.0f);
    spring14.L = 0.5f*sqrt(2.0f);
    spring14.k = spring_constant;
    spring14.m0 = 1;
    spring14.m1 = 6;
    spring14.original_L0 = 0.5f*sqrt(2.0f);
    
    Spring spring15;
    spring15.L0 = 0.5f*sqrt(2.0f);
    spring15.L = 0.5f*sqrt(2.0f);
    spring15.k = spring_constant;
    spring15.m0 = 2;
    spring15.m1 = 5;
    spring15.original_L0 = 0.5f*sqrt(2.0f);
    //---------------------
    
    //Cross Springs of Right Face
    //---------------------
    Spring spring16;
    spring16.L0 = 0.5f*sqrt(2.0f);
    spring16.L = 0.5f*sqrt(2.0f);
    spring16.k = spring_constant;
    spring16.m0 = 2;
    spring16.m1 = 7;
    spring16.original_L0 = 0.5f*sqrt(2.0f);
    
    Spring spring17;
    spring17.L0 = 0.5f*sqrt(2.0f);
    spring17.L = 0.5f*sqrt(2.0f);
    spring17.k = spring_constant;
    spring17.m0 = 3;
    spring17.m1 = 6;
    spring17.original_L0 = 0.5f*sqrt(2.0f);
    //---------------------
    
    //Top Face of the Cube
    //---------------------
    Spring spring18;
    spring18.L0 = 0.5f;
    spring18.L = 0.5f;
    spring18.k = spring_constant;
    spring18.m0 = 4;
    spring18.m1 = 5;
    spring18.original_L0 = 0.5f;
    
    Spring spring19;
    spring19.L0 = 0.5f;
    spring19.L = 0.5f;
    spring19.k = spring_constant;
    spring19.m0 = 5;
    spring19.m1 = 6;
    spring19.original_L0 = 0.5f;
    
    Spring spring20;
    spring20.L0 = 0.5f;
    spring20.L = 0.5f;
    spring20.k = spring_constant;
    spring20.m0 = 6;
    spring20.m1 = 7;
    spring20.original_L0 = 0.5f;
    
    Spring spring21;
    spring21.L0 = 0.5f;
    spring21.L = 0.5f;
    spring21.k = spring_constant;
    spring21.m0 = 7;
    spring21.m1 = 4;
    spring21.original_L0 = 0.5f;
    //---------------------
    
    //Cross Springs of Top Face
    //---------------------
    Spring spring22;
    spring22.L0 = 0.5f*sqrt(2.0f);
    spring22.L = 0.5f*sqrt(2.0f);
    spring22.k = spring_constant;
    spring22.m0 = 4;
    spring22.m1 = 6;
    spring22.original_L0 = 0.5f*sqrt(2.0f);
    
    Spring spring23;
    spring23.L0 = 0.5f*sqrt(2.0f);
    spring23.L = 0.5f*sqrt(2.0f);
    spring23.k = spring_constant;
    spring23.m0 = 5;
    spring23.m1 = 7;
    spring23.original_L0 = 0.5f*sqrt(2.0f);
    //---------------------
    
    //Inner Cross Springs
    //---------------------
    Spring spring24;
    spring24.L0 = 0.5f*sqrt(3.0f);
    spring24.L = 0.5f*sqrt(3.0f);
    spring24.k = spring_constant;
    spring24.m0 = 0;
    spring24.m1 = 6;
    spring24.original_L0 = 0.5f*sqrt(3.0f);
    
    Spring spring25;
    spring25.L0 = 0.5f*sqrt(3.0f);
    spring25.L = 0.5f*sqrt(3.0f);
    spring25.k = spring_constant;
    spring25.m0 = 2;
    spring25.m1 = 4;
    spring25.original_L0 = 0.5f*sqrt(3.0f);
    
    Spring spring26;
    spring26.L0 = 0.5f*sqrt(3.0f);
    spring26.L = 0.5f*sqrt(3.0f);
    spring26.k = spring_constant;
    spring26.m0 = 1;
    spring26.m1 = 7;
    spring26.original_L0 = 0.5f*sqrt(3.0f);
    
    Spring spring27;
    spring27.L0 = 0.5f*sqrt(3.0f);
    spring27.L = 0.5f*sqrt(3.0f);
    spring27.k = spring_constant;
    spring27.m0 = 3;
    spring27.m1 = 5;
    spring27.original_L0 = 0.5f*sqrt(3.0f);
    //---------------------
    
    springs = {spring0, spring1, spring2, spring3, spring4, spring5, spring6, spring7, spring8, spring9, spring10, spring11, spring12, spring13, spring14, spring15, spring16, spring17, spring18, spring19, spring20, spring21, spring22, spring23, spring24, spring25, spring26, spring27};
}

void apply_force(vector<PointMass> &masses){
//    for (int i=0; i<4; i++){
//        masses[i].forces[2] = 1000000.0f;
//    }
    masses[0].forces[1] = 5000.0f;
    masses[1].forces[1] = 5000.0f;
    masses[2].forces[1] = -5000.0f;
    masses[3].forces[1] = -5000.0f;
}

void update_pos_vel_acc(vector<PointMass> &masses){
    
    for (int i=0; i<8; i++){
        float acc_x = masses[i].forces[0]/masses[i].mass;
        float acc_y = masses[i].forces[1]/masses[i].mass;
        float acc_z = masses[i].forces[2]/masses[i].mass;

        masses[i].acceleration[0] = acc_x;
        masses[i].acceleration[1] = acc_y;
        masses[i].acceleration[2] = acc_z;
        
        float vel_x = acc_x*dt + masses[i].velocity[0];
        float vel_y = acc_y*dt + masses[i].velocity[1];
        float vel_z = acc_z*dt + masses[i].velocity[2];
        
        
        masses[i].velocity[0] = vel_x;
        masses[i].velocity[1] = vel_y;
        masses[i].velocity[2] = vel_z;
        
//        float pos_x = 0.5*acc_x*pow(dt, 2) + (vel_x*dt) + masses[i].position[0];
//        float pos_y = 0.5*acc_y*pow(dt, 2) + (vel_y*dt) + masses[i].position[1];
//        float pos_z = 0.5*acc_z*pow(dt, 2) + (vel_z*dt) + masses[i].position[2];
        float pos_x = (vel_x*dt) + masses[i].position[0];
        float pos_y = (vel_y*dt) + masses[i].position[1];
        float pos_z = (vel_z*dt) + masses[i].position[2];
        
        masses[i].position[0] = pos_x;
        masses[i].position[1] = pos_y;
        masses[i].position[2] = pos_z;
    }
}

void reset_forces(vector<PointMass> &masses){
    for(int i=0; i<8; i++){
        masses[i].forces = {0.0f, 0.0f, 0.0f};
    }
}

void update_forces(vector<PointMass> &masses, vector<Spring> &springs){
    
    for (int i=0; i<28; i++){
        
        int p0 = springs[i].m0;
        int p1 = springs[i].m1;

        vector<float> pos0 = masses[p0].position;
        vector<float> pos1 = masses[p1].position;

        float spring_length = sqrt(pow(pos1[0]-pos0[0], 2) + pow(pos1[1]-pos0[1], 2) + pow(pos1[2]-pos0[2], 2));

        springs[i].L = spring_length;
        float force = -springs[i].k*(spring_length-springs[i].L0);

        float x_univ = (pos0[0]-pos1[0])/spring_length;
        float y_univ = (pos0[1]-pos1[1])/spring_length;
        float z_univ = (pos0[2]-pos1[2])/spring_length;
        vector<float> force_unit_dir_2_1 = {x_univ,y_univ,z_univ};
        vector<float> force_unit_dir_1_2 = {-x_univ,-y_univ,-z_univ};

        for (int n = 0; n < 3; n++) {
            masses[p0].forces[n] =  masses[p0].forces[n] + force * force_unit_dir_2_1[n];
            masses[p1].forces[n] =  masses[p1].forces[n] + force * force_unit_dir_1_2[n];
        }
    }
    
    for (int j=0; j<8; j++){
        masses[j].forces[2] = masses[j].forces[2] + masses[j].mass*g;
        
        if (masses[j].position[2] < 0){
            masses[j].forces[2] = -masses[j].position[2]*1000000.0f;
            
//            cout << masses[j].velocity[2] << endl;
//            masses[j].forces[2] = masses[j].forces[2]-(masses[j].mass * masses[j].velocity[2])/dt;
        }
    }
}

void update_breathing(vector<Spring> &springs){
    for (int i=24; i<28; i++){
        springs[i].L0 = springs[i].original_L0 + 0.25f*sin(100.0f*T);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        
        if(firstClick){
            glfwSetCursorPos(window, (width/2), (height/2));
            lastX = xpos;
            lastY = ypos;
            firstClick = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f; // change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstClick = true;
    }
    
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}
