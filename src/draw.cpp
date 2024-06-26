#include "disable_all_warnings.h"
#include "opengl_includes.h"
DISABLE_WARNINGS_PUSH()
#ifdef __APPLE__
#include <OpenGL/GLU.h>
#else
#ifdef WIN32
// Windows.h includes a ton of stuff we don't need, this macro tells it to include less junk.
#define WIN32_LEAN_AND_MEAN
// Disable legacy macro of min/max which breaks completely valid C++ code (std::min/std::max won't work).
#define NOMINMAX
// GLU requires Windows.h on Windows :-(.
#include <Windows.h>
#endif
#include <GL/glu.h>
#endif
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()
#include "draw.h"
#include "ray.h"

static void setMaterial(const Material &material) {
    // Set the material color of the shape.
    const glm::vec4 kd4 { material.kd, 1.0F };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, glm::value_ptr(kd4));

    const glm::vec4 zero { 0.0F };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, glm::value_ptr(zero));
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, glm::value_ptr(zero));
}

void drawMesh(const Mesh& mesh) {
    setMaterial(mesh.material);

    glBegin(GL_TRIANGLES);
    for (const Triangle &triangleIndex : mesh.triangles) {
        for (size_t i = 0; i < 3; i++) {
            const Vertex &vertex = mesh.vertices[triangleIndex[i]];
            glNormal3fv(glm::value_ptr(vertex.normal)); // Normal.
            glVertex3fv(glm::value_ptr(vertex.position)); // Position.
        }
    }
    glEnd();
}

static void drawSphereInternal(const glm::vec3 &center, float radius) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    const glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), center);
    glMultMatrixf(glm::value_ptr(transform));
    GLUquadric *quadric = gluNewQuadric();
    gluSphere(quadric, radius, 40, 20);
    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void drawSphere(const Sphere& sphere) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    setMaterial(sphere.material);
    drawSphereInternal(sphere.center, sphere.radius);
    glPopAttrib();
}

void drawSphere(const glm::vec3 &center, float radius, const glm::vec3 &color) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor4f(color.r, color.g, color.b, 1.0F);
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    drawSphereInternal(center, radius);
    glPopAttrib();
}

static void drawAABBInternal(const AxisAlignedBox &box) {
    glPushMatrix();

    // front      back
    // 3 ----- 2  7 ----- 6
    // |       |  |       |
    // |       |  |       |
    // 0 ------1  4 ------5

    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glVertex3f(box.lower.x, box.upper.y, box.lower.z); //3
    glVertex3f(box.upper.x, box.upper.y, box.lower.z); //2
    glVertex3f(box.upper.x, box.lower.y, box.lower.z); //1
    glVertex3f(box.lower.x, box.lower.y, box.lower.z); //0

    glNormal3f(0, 0, 1);
    glVertex3f(box.upper.x, box.lower.y, box.upper.z); //5
    glVertex3f(box.upper.x, box.upper.y, box.upper.z); //6
    glVertex3f(box.lower.x, box.upper.y, box.upper.z); //7
    glVertex3f(box.lower.x, box.lower.y, box.upper.z); //4

    glNormal3f(1, 0, 0);
    glVertex3f(box.upper.x, box.upper.y, box.lower.z); //2
    glVertex3f(box.upper.x, box.upper.y, box.upper.z); //6
    glVertex3f(box.upper.x, box.lower.y, box.upper.z); //5
    glVertex3f(box.upper.x, box.lower.y, box.lower.z); //1

    glNormal3f(-1, 0, 0);
    glVertex3f(box.lower.x, box.lower.y, box.upper.z); //4
    glVertex3f(box.lower.x, box.upper.y, box.upper.z); //7
    glVertex3f(box.lower.x, box.upper.y, box.lower.z); //3
    glVertex3f(box.lower.x, box.lower.y, box.lower.z); //0

    glNormal3f(0, 1, 0);
    glVertex3f(box.lower.x, box.upper.y, box.upper.z); //7
    glVertex3f(box.upper.x, box.upper.y, box.upper.z); //6
    glVertex3f(box.upper.x, box.upper.y, box.lower.z); //2
    glVertex3f(box.lower.x, box.upper.y, box.lower.z); //3

    glNormal3f(0, -1, 0);
    glVertex3f(box.upper.x, box.lower.y, box.lower.z); //1
    glVertex3f(box.upper.x, box.lower.y, box.upper.z); //5
    glVertex3f(box.lower.x, box.lower.y, box.upper.z); //4
    glVertex3f(box.lower.x, box.lower.y, box.lower.z); //0
    glEnd();

    glPopMatrix();
}

void drawAABB(const AxisAlignedBox &box, DrawMode drawMode, const glm::vec3 &color, float transparency) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor4f(color.r, color.g, color.b, transparency);
    switch (drawMode) {
        case DrawMode::FILLED:
            glPolygonMode(GL_FRONT, GL_FILL);
            glPolygonMode(GL_BACK, GL_FILL);
            break;
        case DrawMode::WIREFRAME:
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_LINE);
            break;
    }
    drawAABBInternal(box);
    glPopAttrib();
}

void drawScene(const Scene &scene) {
    for (const Mesh &mesh : scene.meshes) {
        drawMesh(mesh);
    }
}

void drawRay(const Ray &ray, const glm::vec3 &color) {
    const glm::vec3 hitPoint = ray.origin + std::clamp(ray.t, 0.0F, 100.0F) * ray.direction;
    const bool hit = ray.t < std::numeric_limits<float>::max();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);

    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(ray.origin));
    glColor3fv(glm::value_ptr(color));
    glVertex3fv(glm::value_ptr(hitPoint));
    glEnd();

    if (hit) {
        drawSphere(hitPoint, 0.005F, glm::vec3(0.0F, 1.0F, 0.0F));
    }

    glPopAttrib();
}
