#pragma once
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class IOHandler
{
private:
    bool started;
    glm::vec2 lastMouse;
public:
    bool mouseOn;
    bool keysOn;

    glm::vec3 eye;
    glm::mat4 ONB;

    IOHandler();
    ~IOHandler();

    void Cursor_Button();
    void Cursor_Pos(double xpos, double ypos);
    void Key();

};

