#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 pos;

layout(location = 0) out vec4 outColor;

float tracePlane(vec3 orig, vec3 dir, vec3 planeNormal, vec3 planePoint)
{
    //code to test whether ray intersects plane
    float denom = dot(planeNormal, dir);
    if (abs(denom) < 0.00001)
    {
        return -1.0;
    }
    float t = dot(planePoint - orig, planeNormal) / denom;
    return t;
}

float traceSphere(vec3 orig, vec3 dir, vec3 sphereCenter, float radius) {
    vec3 L = sphereCenter - orig;
    float tca = dot(L, dir);
    if (tca < 0.0){
        return -1.0;
    }
    float d2 = dot(L, L) - tca * tca;
    if (d2 > radius * radius) {
        return -1.0;
    }

    float thc = sqrt(radius * radius - d2);
    float t0 = tca - thc;
    float t1 = tca + thc;

    if (t0 > t1) {
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }

    if (t0 < 0.00001) {
        t0 = t1;
        if (t0 < 0.00001) {
            return -1.0;
        }
    }
    return t0;
}

void main() {
    vec3 orig = vec3(0.0, 0.0, 1.0);
    vec3 dir = normalize(pos - orig);
    vec3 sphereCenter = vec3(0.0, 0.0, -2.0);
    float radius = 0.5;
    float ts = traceSphere(orig, dir, sphereCenter, radius);
    float tp = tracePlane(orig, dir, vec3(0.0, 1.0, 0.0), vec3(0.0, 0.5, 0.0));
    vec3 lightPos = vec3(0.0, -2.0, -2.0);
    float lightPow = 2.0;
    if ((ts < tp && ts > 0.0) || (tp < 0.0 && ts > 0.0)) {
        vec3 hitpos = orig + ts * dir;
        vec3 lightDir = normalize(lightPos - hitpos);
        float shadow = traceSphere(hitpos, lightDir, sphereCenter, radius);
        if (shadow > 0.00) {
            lightPow = 0.0;
        }
        float lightdist = length(hitpos-lightPos);
        outColor = (lightPow/(lightdist*lightdist)+0.05)*vec4(normalize(hitpos-sphereCenter)*0.5+0.5, 1.0);
    } 
    else if (tp > 0.0) {
        vec3 hitpos = orig + tp * dir;
        vec3 lightDir = normalize(lightPos - hitpos);
        float shadow = traceSphere(hitpos, lightDir, sphereCenter, radius);
        if (shadow > 0.0) {
            lightPow = 0.0;
        }
        float lightdist = length(hitpos-lightPos);
        outColor = lightPow/(lightdist*lightdist)*vec4(1.0,1.0,1.0,1.0);
    }
    else {
        outColor = vec4(0.0,0.0,0.0, 1.0);
    }
}
