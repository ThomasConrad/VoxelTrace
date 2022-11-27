#include "IOHandler.hpp"
#include <iostream>
#include <GLFW/glfw3.h> //For glfw keys
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

#define scale 1e-3f

enum keyNames {
    MOVE_FORWARD = GLFW_KEY_W,
    MOVE_BACKWARD = GLFW_KEY_S,
    MOVE_LEFT = GLFW_KEY_A,
    MOVE_RIGHT = GLFW_KEY_D,
    ROTATE_CCW = GLFW_KEY_Q,
    ROTATE_CW = GLFW_KEY_E,
    MOVE_UPWARDS = GLFW_KEY_LEFT_SHIFT,
    MOVE_DOWNWARDS = GLFW_KEY_LEFT_CONTROL
};

int keysNums[] = {
    MOVE_BACKWARD,
    MOVE_FORWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    ROTATE_CCW,
    ROTATE_CW,
    MOVE_UPWARDS,
    MOVE_DOWNWARDS
};

std::unordered_map<int, bool> keys;

IOHandler::IOHandler(float length, glm::mat4* ONB, glm::vec4* eye, float fov) : length{length}, ONB{ONB}, eye{eye}, fov{ fov }
{
    dist = 1.0f / tan(fov / 2.0f);
    *eye = glm::vec4(0., 0., 1., dist);
    started = false;
    for (int key : keysNums) {
        keys[key] = false;
    }
    *ONB = glm::mat4(1.0);
    rot = glm::quat_cast(*ONB);
}

IOHandler::~IOHandler()
{
}

void IOHandler::Tick(float time) {
    glm::vec4 change = (float)keys[MOVE_RIGHT] * (*ONB)[0] -
                       (float)keys[MOVE_LEFT] * (*ONB)[0] +
                       (float)keys[MOVE_UPWARDS] * (*ONB)[1] -
                       (float)keys[MOVE_DOWNWARDS] * (*ONB)[1] +
                       (float)keys[MOVE_BACKWARD] * (*ONB)[2] -
                       (float)keys[MOVE_FORWARD] * (*ONB)[2];
    change.w = 0.0f;

    *eye += scale * change * time * length,0.0;
    float dAngle = (float)keys[ROTATE_CW] * time - (float)keys[ROTATE_CCW] * time;
    rot = glm::rotate(rot, scale * dAngle, -glm::vec3((*ONB)[2]));
    *ONB = glm::mat4_cast(rot);
    //*ONB = glm::rotate(*ONB, scale*dAngle, );
}


void IOHandler::Cursor_Button(){}

void IOHandler::Cursor_Pos(double xpos, double ypos){
    if (!started){
        started = true;
        return;
    }
    glm::vec2 pos(xpos,ypos);
    glm::vec2 delta = (lastMouse-pos)*scale;
    if (glm::dot(delta,delta) < 1000){
        //*ONB = glm::rotate(*ONB, delta.x, glm::vec3(0.0,1.0,0.0));
        rot = glm::rotate(rot, delta.x, glm::vec3(0.0, 1.0, 0.0));
        // 
        //*ONB = glm::rotate(*ONB, delta.y, glm::vec3(1.0,0.0,0.0));
        rot = glm::rotate(rot, delta.y, glm::vec3(1.0, 0.0, 0.0));

    }
    lastMouse = glm::vec2(xpos,ypos);
    return;
}

void IOHandler::Key(int key, int scancode, int action, int mods) {

    if (keys.contains(key)) {
        keys[key] = action == 0 ? false : true;
    }


    std::cout << key << " " << scancode << " " << action << " " << mods << std::endl;
}
