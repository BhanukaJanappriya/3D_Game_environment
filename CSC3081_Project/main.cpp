// MergedScene.cpp
// Combined terrain + textured house + advanced wind turbine component
// Requires: OpenGL (GL, GLU, GLUT) and SOIL2

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <GL/glut.h>
#include <GL/glu.h>
#include <SOIL2.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Window
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;

// Terrain
const int TERRAIN_SIZE = 50;
const float TERRAIN_SCALE = 2.0f;
const float HEIGHT_SCALE = 3.0f;

// Animation / scene
float _angle = 0.0f;

// Camera (from turbine code)
struct Camera {
    float x = 50.0f, y = 30.0f, z = 80.0f;
    float lookX = 0.0f, lookY = 20.0f, lookZ = 0.0f;
    float pitch = -10.0f, yaw = -30.0f;
    float zoom = 45.0f;
    float speed = 2.0f;
} camera;

// Wind turbine variables (from turbine code)
float windSpeed = 1.0f;
float bladeRotation = 0.0f;
float nacelle_yaw = 0.0f;
float towerSway = 0.0f;
float timeAccumulator = 0.0f;
bool animationEnabled = true;
bool lightingEnabled = true;
int projectionMode = 0; // 0 perspective, 1 ortho

// Textures (single unified set)
GLuint grassTexture = 0;
GLuint sandTexture = 0;
GLuint barrackTexture = 0;
GLuint metalTexture = 0;
GLuint concreteTexture = 0;
GLuint bladeTexture = 0;
GLuint nacelleTexture = 0;
GLuint houseTexture = 0;
GLuint roofTexture = 0;
GLuint waterTexture = 0;
GLuint woodTexture = 0;
GLuint glassTexture = 0;
GLuint windowTexture = 0;
GLuint treeTexture = 0;



// Terrain data
std::vector<std::vector<float>> terrainHeights;
std::vector<std::vector<int>> terrainTextures;

// Turbine parameters
struct TurbineGeometry {
    float baseRadius = 3.5f;
    float topRadius = 1.8f;
    float height = 80.0f;
    int segments = 24;
    float nacelleLength = 12.0f;
    float nacelleWidth = 4.0f;
    float nacelleHeight = 4.5f;
    float bladeLength = 45.0f;
    float hubRadius = 2.2f;
    int bladeSegments = 20;
    float foundationRadius = 8.0f;
    float foundationHeight = 2.0f;
} turbineParams;

// Forward declarations
void init();
void update();
void display();
void reshape(int w, int h);
void renderScene();
void keyboardHandler(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);

void generateTerrain();
void generateMultiTextureTerrain();
void drawTerrain();

GLuint loadTexture(const char* filename);
GLuint createProceduralTexture(int r, int g, int b, int variation);

void setupLighting();
void setupProjection();

void drawHouse();
void drawWindTurbine(); // uses the advanced turbine functions below

// Advanced turbine functions
void drawFoundation();
void drawTurbineTower();
void drawNacelle();
void drawRotorSystem();
void drawHub();
void drawBlade(float angleOffset);
void drawSolidCylinder(float baseRadius, float topRadius, float height, int segments);
void drawEllipsoid(float a, float b, float c, int segments);
void drawTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments);

void applyTexture(GLuint textureID);
void setupMaterials();

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Merged Scene: Terrain, House & Advanced Wind Turbine");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardHandler);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(update);

    init();
    glutMainLoop();
    return 0;
}

// ---------------------- Initialization ----------------------
void init() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

    generateTerrain();
    generateMultiTextureTerrain();

    // Load textures with SOIL2
    barrackTexture = loadTexture("door3.jpg");
    grassTexture = loadTexture("grass.jpg");
    sandTexture = loadTexture("grass.jpg");
    woodTexture = loadTexture("barrack_texture.png");
    glassTexture = loadTexture("glass.png");
    waterTexture = loadTexture("water.jpeg");
    treeTexture = loadTexture("tree.jpg");
    windowTexture = loadTexture("house_windows.jpg");
    roofTexture = loadTexture("house_wood.jpg");
    houseTexture = loadTexture("house_brick.jpg");
    metalTexture = loadTexture("metal_texture.jpeg");
    concreteTexture = loadTexture("concrete_texture.jpeg");
    bladeTexture = loadTexture("blade_texture.jpeg");
    nacelleTexture = loadTexture("nacelle_texture.jpg");

    setupLighting();
    setupMaterials();

    std::cout << "Merged scene initialized. Controls: WASD QE arrows +/- space L P 1/2 R\n";
}

// ---------------------- Update (animation) ----------------------
void update() {
    if (animationEnabled) {
        timeAccumulator += 0.016f; // approx 60fps

        bladeRotation += windSpeed * 2.0f;
        if (bladeRotation >= 360.0f) bladeRotation -= 360.0f;

        nacelle_yaw = sin(timeAccumulator * 0.3f) * 15.0f;

        towerSway = sin(timeAccumulator * 0.8f) * 0.5f + cos(timeAccumulator * 0.6f) * 0.3f;
    }

    // simple global rotation to make scene dynamic
    _angle += 0.02f;
    if (_angle >= 360.0f) _angle -= 360.0f;

    glutPostRedisplay();
}

// ---------------------- Display & Render ----------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    setupProjection();
    setupLighting();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // update camera look vector from pitch/yaw in specialKeys
    gluLookAt(camera.x, camera.y, camera.z,
        camera.lookX, camera.lookY, camera.lookZ,
        0.0f, 1.0f, 0.0f);

    renderScene();

    glutSwapBuffers();
}

void renderScene() {
    // Draw sky as background (disable depth so it won't occlude)
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(0.53f, 0.81f, 0.98f);
    glVertex3f(-1.0f, -1.0f, -0.9f);
    glVertex3f(1.0f, -1.0f, -0.9f);
    glVertex3f(1.0f, 1.0f, -0.9f);
    glVertex3f(-1.0f, 1.0f, -0.9f);
    glEnd();
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);

    // Scene transforms
    glPushMatrix();
    // small sway translation applied to whole base to simulate wind
    glTranslatef(towerSway, 0.0f, towerSway * 0.3f);

    // draw terrain
    drawTerrain();

    // draw house near center
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f);
    drawHouse();
    glPopMatrix();

    // draw one advanced turbine a bit to the back-left
    glPushMatrix();
    glTranslatef(-20.0f, 0.0f, -30.0f);
    // tower built from origin upward; place base at y=0
    drawWindTurbine();
    glPopMatrix();

    // second turbine
    glPushMatrix();
    glTranslatef(30.0f, 0.0f, -25.0f);
    drawWindTurbine();
    glPopMatrix();

    // third turbine
    glPushMatrix();
    glTranslatef(-5.0f, 0.0f, -40.0f);
    drawWindTurbine();
    glPopMatrix();

    glPopMatrix();

    // reset color
    glColor3f(1, 1, 1);
}

// ---------------------- Projection & Resize ----------------------
void setupProjection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);

    if (projectionMode == 0) {
        gluPerspective(camera.zoom, aspect, 1.0f, 500.0f);
    }
    else {
        float size = camera.zoom;
        glOrtho(-size * aspect, size * aspect, -size, size, -200.0, 200.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    setupProjection();
}

// ---------------------- Keyboard & Controls ----------------------
void keyboardHandler(unsigned char key, int x, int y) {
    switch (key) {
    case 27:  // ESC
        std::exit(0);
        break;
    case 'w': case 'W':
        camera.x += (camera.lookX - camera.x) * 0.1f;
        camera.z += (camera.lookZ - camera.z) * 0.1f;
        break;
    case 's': case 'S':
        camera.x -= (camera.lookX - camera.x) * 0.1f;
        camera.z -= (camera.lookZ - camera.z) * 0.1f;
        break;
    case 'a': case 'A':
        camera.x -= camera.speed;
        break;
    case 'd': case 'D':
        camera.x += camera.speed;
        break;
    case 'q': case 'Q':
        camera.y += camera.speed;
        break;
    case 'e': case 'E':
        camera.y -= camera.speed;
        break;
    case ' ':
        animationEnabled = !animationEnabled;
        break;
    case 'l': case 'L':
        lightingEnabled = !lightingEnabled;
        break;
    case 'p': case 'P':
        projectionMode = (projectionMode + 1) % 2;
        reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        break;
    case '1':
        windSpeed = std::max(0.1f, windSpeed - 0.2f);
        break;
    case '2':
        windSpeed = std::min(5.0f, windSpeed + 0.2f);
        break;
    case '+':
        camera.zoom = std::max(10.0f, camera.zoom - 2.0f);
        reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        break;
    case '-':
        camera.zoom = std::min(120.0f, camera.zoom + 2.0f);
        reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        break;
    case 'r': case 'R':
        camera.x = 50.0f; camera.y = 30.0f; camera.z = 80.0f;
        camera.lookX = 0.0f; camera.lookY = 20.0f; camera.lookZ = 0.0f;
        camera.zoom = 45.0f;
        break;
    }
}

void specialKeys(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:    camera.pitch += 2.0f; break;
    case GLUT_KEY_DOWN:  camera.pitch -= 2.0f; break;
    case GLUT_KEY_LEFT:  camera.yaw -= 2.0f; break;
    case GLUT_KEY_RIGHT: camera.yaw += 2.0f; break;
    }
    float pitchRad = camera.pitch * (float)M_PI / 180.0f;
    float yawRad = camera.yaw * (float)M_PI / 180.0f;
    camera.lookX = camera.x + cosf(pitchRad) * sinf(yawRad) * 50.0f;
    camera.lookY = camera.y + sinf(pitchRad) * 50.0f;
    camera.lookZ = camera.z + cosf(pitchRad) * cosf(yawRad) * 50.0f;
}

// ---------------------- Terrain generation & drawing ----------------------
void generateTerrain() {
    terrainHeights.resize(TERRAIN_SIZE + 1);
    for (int i = 0; i <= TERRAIN_SIZE; ++i) {
        terrainHeights[i].resize(TERRAIN_SIZE + 1);
        for (int j = 0; j <= TERRAIN_SIZE; ++j) {
            float h = sin(i * 0.3f) * cos(j * 0.3f) * HEIGHT_SCALE;
            h += sin(i * 0.1f) * sin(j * 0.15f) * HEIGHT_SCALE * 2.0f;
            h += sin(i * 0.05f) * cos(j * 0.08f) * HEIGHT_SCALE * 0.5f;
            terrainHeights[i][j] = h;
        }
    }
}

void generateMultiTextureTerrain() {
    terrainTextures.resize(TERRAIN_SIZE + 1);
    srand(42);
    for (int i = 0; i <= TERRAIN_SIZE; ++i) {
        terrainTextures[i].resize(TERRAIN_SIZE + 1);
        for (int j = 0; j <= TERRAIN_SIZE; ++j) {
            float height = terrainHeights[i][j];
            float rf = (rand() % 100) / 100.0f;
            if (height < -2.0f) terrainTextures[i][j] = (rf > 0.7f) ? 3 : 2;
            else if (height < 2.0f) terrainTextures[i][j] = (rf > 0.6f) ? 3 : 0;
            else if (height < 5.0f) terrainTextures[i][j] = (rf > 0.5f) ? 1 : 0;
            else terrainTextures[i][j] = 1;
            if (rf > 0.95f) terrainTextures[i][j] = rand() % 4;
        }
    }
}

void drawTerrain() {
    glEnable(GL_TEXTURE_2D);
    for (int i = 0; i < TERRAIN_SIZE; ++i) {
        for (int j = 0; j < TERRAIN_SIZE; ++j) {
            int texType = terrainTextures[i][j];
            switch (texType) {
            case 0: glBindTexture(GL_TEXTURE_2D, grassTexture); break;
            default: glBindTexture(GL_TEXTURE_2D, sandTexture); break;
            }
            float x1 = (i - TERRAIN_SIZE / 2) * TERRAIN_SCALE;
            float x2 = ((i + 1) - TERRAIN_SIZE / 2) * TERRAIN_SCALE;
            float z1 = (j - TERRAIN_SIZE / 2) * TERRAIN_SCALE;
            float z2 = ((j + 1) - TERRAIN_SIZE / 2) * TERRAIN_SCALE;
            float y1 = terrainHeights[i][j];
            float y2 = terrainHeights[i + 1][j];
            float y3 = terrainHeights[i + 1][j + 1];
            float y4 = terrainHeights[i][j + 1];

            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glTexCoord2f(0, 0); glVertex3f(x1, y1, z1);
            glTexCoord2f(1, 0); glVertex3f(x2, y2, z1);
            glTexCoord2f(1, 1); glVertex3f(x2, y3, z2);
            glTexCoord2f(0, 1); glVertex3f(x1, y4, z2);
            glEnd();
        }
    }
    glDisable(GL_TEXTURE_2D);
}

// ---------------------- House (fixed texture coords & no invalid stack ops) ----------------------
void drawHouse() {
    // Place house at current model origin
    // Walls - use houseTexture
    applyTexture(houseTexture);
    glBegin(GL_QUADS);
    // Front wall (z = +2)
    glTexCoord2f(0, 0); glVertex3f(-2.0f, -2.0f, 2.0f);
    glTexCoord2f(1, 0); glVertex3f(2.0f, -2.0f, 2.0f);
    glTexCoord2f(1, 1); glVertex3f(2.0f, 2.0f, 2.0f);
    glTexCoord2f(0, 1); glVertex3f(-2.0f, 2.0f, 2.0f);
    // Back wall (z = -2)
    glTexCoord2f(0, 0); glVertex3f(-2.0f, -2.0f, -2.0f);
    glTexCoord2f(1, 0); glVertex3f(-2.0f, 2.0f, -2.0f);
    glTexCoord2f(1, 1); glVertex3f(2.0f, 2.0f, -2.0f);
    glTexCoord2f(0, 1); glVertex3f(2.0f, -2.0f, -2.0f);
    // Right wall (x = +2)
    glTexCoord2f(0, 0); glVertex3f(2.0f, -2.0f, -2.0f);
    glTexCoord2f(1, 0); glVertex3f(2.0f, 2.0f, -2.0f);
    glTexCoord2f(1, 1); glVertex3f(2.0f, 2.0f, 2.0f);
    glTexCoord2f(0, 1); glVertex3f(2.0f, -2.0f, 2.0f);
    // Left wall (x = -2)
    glTexCoord2f(0, 0); glVertex3f(-2.0f, -2.0f, -2.0f);
    glTexCoord2f(1, 0); glVertex3f(-2.0f, -2.0f, 2.0f);
    glTexCoord2f(1, 1); glVertex3f(-2.0f, 2.0f, 2.0f);
    glTexCoord2f(0, 1); glVertex3f(-2.0f, 2.0f, -2.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Roof - use roofTexture (triangles & quads with texcoords)
    applyTexture(roofTexture);
    // front triangle
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f, 2.0f, 2.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(2.5f, 2.0f, 2.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, 4.0f, 2.0f);
    glEnd();
    // back triangle
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f, 2.0f, -2.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, 4.0f, -2.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(2.5f, 2.0f, -2.0f);
    glEnd();
    // left quad (sloped)
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f, 2.0f, 2.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0f, 4.0f, 2.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0f, 4.0f, -2.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.5f, 2.0f, -2.0f);
    glEnd();
    // right quad (sloped)
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(2.5f, 2.0f, 2.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(2.5f, 2.0f, -2.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0f, 4.0f, -2.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 4.0f, 2.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Door with texture (barrackTexture)
    applyTexture(barrackTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -2.0f, 2.01f);
    glTexCoord2f(1, 0); glVertex3f(0.5f, -2.0f, 2.01f);
    glTexCoord2f(1, 1); glVertex3f(0.5f, 0.0f, 2.01f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.0f, 2.01f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Windows (plain color)
    glColor3f(0.5f, 0.8f, 1.0f);
    glBegin(GL_QUADS);
    // left window
    glVertex3f(-1.5f, 0.5f, 2.01f);
    glVertex3f(-0.5f, 0.5f, 2.01f);
    glVertex3f(-0.5f, 1.5f, 2.01f);
    glVertex3f(-1.5f, 1.5f, 2.01f);
    // right window
    glVertex3f(0.5f, 0.5f, 2.01f);
    glVertex3f(1.5f, 0.5f, 2.01f);
    glVertex3f(1.5f, 1.5f, 2.01f);
    glVertex3f(0.5f, 1.5f, 2.01f);
    glEnd();

    // restore color
    glColor3f(1, 1, 1);
}

// ---------------------- Advanced Wind Turbine (integrated) ----------------------
void drawWindTurbine() {
    // Place base at current model origin (y=0) and build upward
    glPushMatrix();

    // Foundation
    drawFoundation();

    // Tower (rotate so axis points up)
    glPushMatrix();
    glTranslatef(0.0f, turbineParams.foundationHeight, 0.0f);
    drawTurbineTower();
    glPopMatrix();

    // Nacelle & rotor at top
    glPushMatrix();
    glTranslatef(0.0f, turbineParams.foundationHeight + turbineParams.height, 0.0f);
    glRotatef(nacelle_yaw, 0.0f, 1.0f, 0.0f);
    // nacelle body
    drawNacelle();
    // move forward from nacelle center to rotor mount and draw rotor
    glTranslatef(turbineParams.nacelleLength * 0.6f, 0.0f, 0.0f);
    drawRotorSystem();
    glPopMatrix();

    glPopMatrix();
}

void drawFoundation() {
    glPushMatrix();
    applyTexture(concreteTexture);
    // Put foundation center slightly below world origin so tower sits on top
    glTranslatef(0.0f, -turbineParams.foundationHeight * 0.5f, 0.0f);

    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawSolidCylinder(turbineParams.foundationRadius, turbineParams.foundationRadius,
        turbineParams.foundationHeight, 32);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, turbineParams.foundationHeight * 0.8f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawTorus(turbineParams.foundationRadius * 1.1f, 0.5f, 24, 16);
    glPopMatrix();
    glPopMatrix();
}

void drawTurbineTower() {
    applyTexture(metalTexture);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawSolidCylinder(turbineParams.baseRadius, turbineParams.topRadius,
        turbineParams.height, turbineParams.segments);
    glPopMatrix();
}

void drawNacelle() {
    applyTexture(nacelleTexture);
    glPushMatrix();
    // place the ellipsoid oriented along X
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    drawEllipsoid(turbineParams.nacelleLength, turbineParams.nacelleHeight, turbineParams.nacelleWidth, 20);
    // vents / details
    glColor3f(0.3f, 0.3f, 0.3f);
    for (int i = 0; i < 8; ++i) {
        float angle = i * 45.0f * (float)M_PI / 180.0f;
        float r = turbineParams.nacelleWidth * 0.9f;
        float x = cosf(angle) * r;
        float z = sinf(angle) * r;
        glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glScalef(0.2f, 0.8f, 0.2f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }
    glColor3f(1, 1, 1);
    glPopMatrix();
}

void drawRotorSystem() {
    // hub
    drawHub();

    // blades (3)
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        // rotate so blades spin around X (use bladeRotation) and offset blades by 120 degrees
        glRotatef(bladeRotation + i * 120.0f, 1.0f, 0.0f, 0.0f);
        drawBlade(i * 120.0f);
        glPopMatrix();
    }
}

void drawHub() {
    applyTexture(metalTexture);
    glColor3f(0.8f, 0.8f, 0.8f);
    glutSolidSphere(turbineParams.hubRadius, 16, 16);
    glColor3f(1, 1, 1);
    // small bolts
    for (int i = 0; i < 12; ++i) {
        float angle = i * 30.0f * (float)M_PI / 180.0f;
        float x = cosf(angle) * turbineParams.hubRadius * 0.8f;
        float z = sinf(angle) * turbineParams.hubRadius * 0.8f;
        glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glutSolidSphere(0.15f, 8, 8);
        glPopMatrix();
    }
}

void drawBlade(float angleOffset) {
    applyTexture(bladeTexture);
    glColor3f(0.95f, 0.95f, 0.95f);

    int segments = turbineParams.bladeSegments;
    float segLen = turbineParams.bladeLength / segments;

    for (int i = 0; i < segments; ++i) {
        float t1 = (float)i / (float)segments;
        float t2 = (float)(i + 1) / (float)segments;
        float width1 = turbineParams.hubRadius * (1.0f - t1 * 0.8f);
        float width2 = turbineParams.hubRadius * (1.0f - t2 * 0.8f);
        float thick1 = width1 * 0.15f;
        float thick2 = width2 * 0.15f;
        float y1 = t1 * turbineParams.bladeLength;
        float y2 = t2 * turbineParams.bladeLength;
        float twist1 = t1 * 25.0f;
        float twist2 = t2 * 25.0f;

        glPushMatrix();
        glTranslatef(0.0f, y1, 0.0f);
        glRotatef(twist1, 0.0f, 1.0f, 0.0f);

        glBegin(GL_QUADS);
        // top
        glNormal3f(0, 0, 1);
        glTexCoord2f(0.0f, t1); glVertex3f(-width1, 0.0f, thick1);
        glTexCoord2f(1.0f, t1); glVertex3f(width1, 0.0f, thick1);
        glTexCoord2f(1.0f, t2); glVertex3f(width2, segLen, thick2);
        glTexCoord2f(0.0f, t2); glVertex3f(-width2, segLen, thick2);
        // bottom
        glNormal3f(0, 0, -1);
        glTexCoord2f(0.0f, t1); glVertex3f(-width1, 0.0f, -thick1);
        glTexCoord2f(0.0f, t2); glVertex3f(-width2, segLen, -thick2);
        glTexCoord2f(1.0f, t2); glVertex3f(width2, segLen, -thick2);
        glTexCoord2f(1.0f, t1); glVertex3f(width1, 0.0f, -thick1);
        // leading
        glNormal3f(1, 0, 0);
        glTexCoord2f(0.0f, t1); glVertex3f(width1, 0.0f, thick1);
        glTexCoord2f(0.0f, t1); glVertex3f(width1, 0.0f, -thick1);
        glTexCoord2f(1.0f, t2); glVertex3f(width2, segLen, -thick2);
        glTexCoord2f(1.0f, t2); glVertex3f(width2, segLen, thick2);
        // trailing
        glNormal3f(-1, 0, 0);
        glTexCoord2f(0.0f, t1); glVertex3f(-width1, 0.0f, thick1);
        glTexCoord2f(1.0f, t2); glVertex3f(-width2, segLen, thick2);
        glTexCoord2f(1.0f, t2); glVertex3f(-width2, segLen, -thick2);
        glTexCoord2f(0.0f, t1); glVertex3f(-width1, 0.0f, -thick1);
        glEnd();

        glPopMatrix();
    }

    glColor3f(1, 1, 1);
}

void drawSolidCylinder(float baseRadius, float topRadius, float height, int segments) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluCylinder(quad, baseRadius, topRadius, height, segments, 1);
    // bottom cap
    gluDisk(quad, 0.0, baseRadius, segments, 1);
    // top cap
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, height);
    gluDisk(quad, 0.0, topRadius, segments, 1);
    glPopMatrix();
    gluDeleteQuadric(quad);
}

void drawEllipsoid(float a, float b, float c, int segments) {
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            float u1 = (float)i / segments * M_PI;
            float u2 = (float)(i + 1) / segments * M_PI;
            float v1 = (float)j / segments * (2.0f * M_PI);
            float v2 = (float)(j + 1) / segments * (2.0f * M_PI);

            glBegin(GL_QUADS);
            float x1 = a * cosf(u1) * cosf(v1);
            float y1 = b * sinf(u1);
            float z1 = c * cosf(u1) * sinf(v1);
            glNormal3f(x1 / a, y1 / b, z1 / c);
            glTexCoord2f((float)j / segments, (float)i / segments); glVertex3f(x1, y1, z1);

            float x2 = a * cosf(u2) * cosf(v1);
            float y2 = b * sinf(u2);
            float z2 = c * cosf(u2) * sinf(v1);
            glNormal3f(x2 / a, y2 / b, z2 / c);
            glTexCoord2f((float)j / segments, (float)(i + 1) / segments); glVertex3f(x2, y2, z2);

            float x3 = a * cosf(u2) * cosf(v2);
            float y3 = b * sinf(u2);
            float z3 = c * cosf(u2) * sinf(v2);
            glNormal3f(x3 / a, y3 / b, z3 / c);
            glTexCoord2f((float)(j + 1) / segments, (float)(i + 1) / segments); glVertex3f(x3, y3, z3);

            float x4 = a * cosf(u1) * cosf(v2);
            float y4 = b * sinf(u1);
            float z4 = c * cosf(u1) * sinf(v2);
            glNormal3f(x4 / a, y4 / b, z4 / c);
            glTexCoord2f((float)(j + 1) / segments, (float)i / segments); glVertex3f(x4, y4, z4);
            glEnd();
        }
    }
}

void drawTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments) {
    for (int i = 0; i < majorSegments; ++i) {
        for (int j = 0; j < minorSegments; ++j) {
            float u1 = (float)i / majorSegments * (2.0f * M_PI);
            float u2 = (float)(i + 1) / majorSegments * (2.0f * M_PI);
            float v1 = (float)j / minorSegments * (2.0f * M_PI);
            float v2 = (float)(j + 1) / minorSegments * (2.0f * M_PI);

            glBegin(GL_QUADS);
            // vertex 0
            float x0 = (majorRadius + minorRadius * cosf(v1)) * cosf(u1);
            float y0 = (majorRadius + minorRadius * cosf(v1)) * sinf(u1);
            float z0 = minorRadius * sinf(v1);
            float nx0 = cosf(v1) * cosf(u1);
            float ny0 = cosf(v1) * sinf(u1);
            float nz0 = sinf(v1);
            glNormal3f(nx0, ny0, nz0);
            glTexCoord2f(u1 / (2.0f * M_PI), v1 / (2.0f * M_PI)); glVertex3f(x0, y0, z0);

            // vertex 1
            float x1 = (majorRadius + minorRadius * cosf(v1)) * cosf(u2);
            float y1 = (majorRadius + minorRadius * cosf(v1)) * sinf(u2);
            float z1 = minorRadius * sinf(v1);
            float nx1 = cosf(v1) * cosf(u2);
            float ny1 = cosf(v1) * sinf(u2);
            float nz1 = sinf(v1);
            glNormal3f(nx1, ny1, nz1);
            glTexCoord2f(u2 / (2.0f * M_PI), v1 / (2.0f * M_PI)); glVertex3f(x1, y1, z1);

            // vertex 2
            float x2 = (majorRadius + minorRadius * cosf(v2)) * cosf(u2);
            float y2 = (majorRadius + minorRadius * cosf(v2)) * sinf(u2);
            float z2 = minorRadius * sinf(v2);
            float nx2 = cosf(v2) * cosf(u2);
            float ny2 = cosf(v2) * sinf(u2);
            float nz2 = sinf(v2);
            glNormal3f(nx2, ny2, nz2);
            glTexCoord2f(u2 / (2.0f * M_PI), v2 / (2.0f * M_PI)); glVertex3f(x2, y2, z2);

            // vertex 3
            float x3 = (majorRadius + minorRadius * cosf(v2)) * cosf(u1);
            float y3 = (majorRadius + minorRadius * cosf(v2)) * sinf(u1);
            float z3 = minorRadius * sinf(v2);
            float nx3 = cosf(v2) * cosf(u1);
            float ny3 = cosf(v2) * sinf(u1);
            float nz3 = sinf(v2);
            glNormal3f(nx3, ny3, nz3);
            glTexCoord2f(u1 / (2.0f * M_PI), v2 / (2.0f * M_PI)); glVertex3f(x3, y3, z3);

            glEnd();
        }
    }
}

// ---------------------- Lighting & Materials ----------------------
void setupLighting() {
    if (lightingEnabled) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);

        GLfloat lightPos[] = { 100.0f, 200.0f, 100.0f, 0.0f };
        GLfloat lightAmbient[] = { 0.3f, 0.3f, 0.4f, 1.0f };
        GLfloat lightDiffuse[] = { 1.0f, 0.95f, 0.8f, 1.0f };
        GLfloat lightSpecular[] = { 1.0f, 1.0f, 0.9f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

        GLfloat fillLightPos[] = { -50.0f, 50.0f, 50.0f, 1.0f };
        GLfloat fillLightDiffuse[] = { 0.4f, 0.4f, 0.5f, 1.0f };

        glLightfv(GL_LIGHT1, GL_POSITION, fillLightPos);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, fillLightDiffuse);

        GLfloat globalAmbient[] = { 0.2f, 0.2f, 0.3f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    }
    else {
        glDisable(GL_LIGHTING);
    }
}

void setupMaterials() {
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    GLfloat specular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 64.0f);
}

void applyTexture(GLuint textureID) {
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }
}

// ---------------------- Texture loading & fallback ----------------------
GLuint loadTexture(const char* filename) {
    GLuint textureID = SOIL_load_OGL_texture(
        filename, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y | SOIL_FLAG_MIPMAPS
    );
    if (textureID == 0) {
        std::cerr << "Warning: could not load texture '" << filename << "'. Using procedural fallback.\n";
        return createProceduralTexture(180, 160, 140, 20);
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return textureID;
}

GLuint createProceduralTexture(int r, int g, int b, int variation) {
    const int texSize = 128;
    unsigned char* data = new unsigned char[texSize * texSize * 3];
    for (int i = 0; i < texSize * texSize; ++i) {
        data[i * 3 + 0] = std::max(0, std::min(255, r + (rand() % variation) - variation / 2));
        data[i * 3 + 1] = std::max(0, std::min(255, g + (rand() % variation) - variation / 2));
        data[i * 3 + 2] = std::max(0, std::min(255, b + (rand() % variation) - variation / 2));
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    delete[] data;
    return tex;
}
