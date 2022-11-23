#include "IOHandler.hpp"
#include <GLFW/glfw3.h> //For glfw keys
#include <glm/gtc/matrix_transform.hpp>

IOHandler::IOHandler() 
{
    started = false;
    eye = glm::vec3(0.0,0.0,0.0);
    ONB = glm::mat4(1.0);
}

IOHandler::~IOHandler()
{
}

void IOHandler::Cursor_Button(){}

void IOHandler::Cursor_Pos(double xpos, double ypos){
    if (!started){
        started = true;
        return;
    }
    glm::vec2 pos(xpos,ypos);
    glm::vec2 delta = (lastMouse-pos)*0.001f;
    if (glm::dot(delta,delta) < 1000){
        ONB = glm::rotate(ONB, delta.x, glm::vec3(0.0,1.0,0.0));
        ONB = glm::rotate(ONB, delta.y, glm::vec3(1.0,0.0,0.0));
    }
    lastMouse = glm::vec2(xpos,ypos);
    return;
}

void IOHandler::Key(){}

