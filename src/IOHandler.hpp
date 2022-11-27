#pragma once
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class IOHandler
{
private:
    bool started;
    glm::vec2 lastMouse;
    float dist;
    glm::quat rot;
public:
    bool mouseOn;
    bool keysOn;
    float fov;
    glm::vec4* eye;
    glm::mat4* ONB;
    float length;

    IOHandler(float length, glm::mat4* ONB, glm::vec4* eye, float fov);
    ~IOHandler();

    void Cursor_Button();
    void Cursor_Pos(double xpos, double ypos);
    void Key(int key, int scancode, int action, int mods);
    void Tick(float time);
};

