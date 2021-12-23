// Modified Author : Edwin Kaburu
// Moon - Earth Simulation
// Copyright (c) EK, 2021

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "CameraArcball.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Mesh.h"
#include "Misc.h"
#include "Sphere.h"
#include "VecMat.h"
#include "Widgets.h"
#include <time.h>

// App Window, Camera
int			winWidth = 1000, winHeight = 800;
CameraAB	camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -10));

// Night Sky 
Mesh nightSkySphere;
const char* nightSkyTextureFile = "./nightskyT.png";

// Moon Mesh and Texture Files
Mesh moonSphere;
const char* moonTextureFile = "./moontexture.jpg";
const char* moonNormalFile = "./moonnormal.jpg";

// Earth Mesh and Texture Files
Mesh earthSphere;
const char* earthTextureFile = "./Earth.tga";
const char* earthNormalFile = "./earth_normals.jpg";

// Interactive Lighting
// Light 1
vec3		light(2.7f, 2.3f, 5.3f);
Mover		lightMover;
void* picked = NULL;

// Timer and degPerSec
time_t  startTime = clock();

// Earth's axis is tilted 23.5 degrees
static float degPerSec = 23.0f;

// Main Delta Time
float main_dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;

// Texture Scale
float        textureScale = 1;

// Controls And State Attribute
bool rotateAxis = false;
bool onlyMoon, displaySky = false;

float dt, radAngs;
float cos0, sin0;

// Speed of Orbit Rotation
float orbitSpeed = 2;

void Display(GLFWwindow* w) {
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	vec4 xLight = camera.modelview * vec4(light, 1);
	GLuint meshShader = UseMeshShader();


	SetUniform(meshShader, "light", (vec3*)&xLight);
	SetUniform(meshShader, "textureScale", textureScale);



	// Are We in Motion : Rotating About Axis
	dt = ((float)(clock() - startTime) / CLOCKS_PER_SEC);

	// Calculate New RadAngs, To Rotate About the Y-Axis
	radAngs = (3.14159265f / 180.f) * dt * degPerSec;

	// Send Result to Shader
	SetUniform(meshShader, "radAng", radAngs);

	// Multiply radAngs by Orbit Speed
	radAngs = radAngs * orbitSpeed;

	// Find cos and sine for orbit trajectory
	cos0 = cos(radAngs), sin0 = sin(radAngs);

	
	if (onlyMoon)
	{
		// Transform Moon
		moonSphere.transform = Translate(0.0f, 0.3f, 0.0f);

		// Display Moon
		moonSphere.Display(camera, rotateAxis);
	}
	else
	{
		// Transform Moon and Decrease Scale
		moonSphere.transform = Translate(
			(cos0 * 1.5f + sin0 * 1.5f),
			0.3f,
			(-sin0 * 2.0f + cos0 * 2.0f)) * Scale(0.30f);

		// Display Moon
		moonSphere.Display(camera, rotateAxis);

		// Display Earth 
		earthSphere.Display(camera, rotateAxis);
	}

	if (displaySky)
	{
		// Increase Scale
		nightSkySphere.transform = Scale(25.0f);

		// Display Night Sky
		nightSkySphere.Display(camera);
	}


	UseDrawShader(camera.fullview);
	Disk(light, 20, vec3(1, 0, 0));

	glDisable(GL_DEPTH_TEST);
	glFlush();
}


void UVAxes(vec3 p1, vec3 p2, vec3 p3, vec2 t1, vec2 t2, vec2 t3, vec3& uAxis, vec3& vAxis) {
	vec3 dp1 = p2 - p1, dp2 = p3 - p1;                  // position differences
	vec2 dt1 = t2 - t1, dt2 = t3 - t1;                  // texture differences
	uAxis = dt2[1] * dp1 - dt1[1] * dp2;                // u,v axes for triangle
	vAxis = dt1[0] * dp2 - dt2[0] * dp1;				// local v axis
}


// Make Sphere Object
void MakeSphere(Mesh& sphereObj) {
	int npts = UnitSphere(500, sphereObj.points, sphereObj.uvs, sphereObj.triangles);

	vec3 uAxis, vAxis;

	// Re-size the Sphere normals, us  and vs
	sphereObj.normals.resize(npts);
	sphereObj.us.resize(npts);
	sphereObj.vs.resize(npts);

	// Iterate through the triangles, accumalating the U, V axes at the triangle vertces
	for (size_t i = 0; i < sphereObj.triangles.size(); i++)
	{
		int3 t = sphereObj.triangles[i];

		UVAxes(sphereObj.points[t.i1], sphereObj.points[t.i2], sphereObj.points[t.i3],
			sphereObj.uvs[t.i1], sphereObj.uvs[t.i2], sphereObj.uvs[t.i3], uAxis, vAxis);

		// accumulate triangle u , v axis at each vertex
		sphereObj.us[t.i1] += uAxis; sphereObj.us[t.i2] += uAxis; sphereObj.us[t.i3] += uAxis;
		sphereObj.vs[t.i1] += vAxis; sphereObj.vs[t.i2] += vAxis; sphereObj.vs[t.i3] += vAxis;

	}

	// Normalize the result
	for (size_t i = 0; i < npts; i++) {
		sphereObj.normals[i] = sphereObj.points[i];

		sphereObj.us[i] = normalize(sphereObj.us[i]);
		sphereObj.vs[i] = normalize(sphereObj.vs[i]);
	}

	SetVertexNormals(sphereObj.points, sphereObj.triangles, sphereObj.normals);

	sphereObj.Buffer();
}
// Mouse Handlers
int WindowHeight(GLFWwindow* w) {
	int width, height;
	glfwGetWindowSize(w, &width, &height);
	return height;
}

void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	y = WindowHeight(w) - y; // invert y for upward-increasing screen space
	picked = NULL;
	if (action == GLFW_PRESS && butn == GLFW_MOUSE_BUTTON_LEFT) {
		if (MouseOver((float)x, (float)y, light, camera.fullview)) {
			picked = &light;
			lightMover.Down(&light, (int)x, (int)y, camera.modelview, camera.persp);
		}
		else {
			picked = &camera;
			camera.MouseDown((int)x, (int)y, Shift());
		}
	}
	if (action == GLFW_RELEASE)
		camera.MouseUp();
}

void MouseMove(GLFWwindow* w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		y = WindowHeight(w) - y;
		if (picked == &light)
			lightMover.Drag((int)x, (int)y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.MouseDrag((int)x, (int)y);
	}
}

void MouseWheel(GLFWwindow* w, double xoffset, double direction) {
	camera.MouseWheel(direction, Shift());
}

void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Get Camera Position
	vec3 cameraPos = camera.Position();
	// Forward
	vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
	// Back
	vec3 cameraBack = vec3(0.0f, 0.0f, 1.0f);
	// Up
	vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
	// Down
	vec3 cameraDown = vec3(0.0f, -1.0f, 0.0f);

	vec3 cameraLeft = vec3(-1.0f, 0.0f, 0.0f);

	vec3 cameraRight = vec3(1.0f, 0.0f, 0.0f);

	// Keys : Walk Around
	// W - Move Camera Forward
	// S - Move Camera BackWards
	// A - Move Camera Left
	// D - Move Camera Right

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			
			// Move Camera Forward
			camera.Move(cameraFront);
			break;
		case GLFW_KEY_S:
			// Move Camera Backwards
			camera.Move(cameraBack);
			break;
		case GLFW_KEY_A:
			// Move Camera Left
			camera.Move(cameraLeft);
			break;
		case GLFW_KEY_D:
			// Move Camera Right
			camera.Move(cameraRight);
			break;
		case GLFW_KEY_R:
			// Rotation About the Y-Axis
			// Swap between true and false
			rotateAxis = !rotateAxis;
			break;
		case GLFW_KEY_O:
			// Increase Orbit Speed
			orbitSpeed += 0.5;
			break;
		case GLFW_KEY_P:
			// Decrease Orbit Speed
			if (orbitSpeed > 0.5)
			{
				orbitSpeed -= 0.5;
			}
			break;
		case GLFW_KEY_M:
			// View Moon True or False
			onlyMoon = !onlyMoon;
			break;
		case GLFW_KEY_N:
			// View Night Sky, True or False
			displaySky = !displaySky;
			break;
		default:
			break;
		}
	}
}

// List Commands
const char* usage = "\
    W: Move Camera Forward\n\
    A: Move Camera Left\n\
    S: Move Camera Backwards\n\
    D: Move Camera Right\n\
    R: Toggle Rotation Around Y-Axis\n\
    O: Increase Orbit Speed\n\
    P: Decrease Orbit Speed\n\
    M: Toggle Moon View\n\
    N: Toggle Night Sky View\n\
";


// Application

void Resize(GLFWwindow* w, int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int argc, char** argv) {
	// anti-alias, app window, GL context
	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow* w = glfwCreateWindow(winWidth, winHeight, "Unit Sphere: Edwin Kaburu Simplified Solar System", NULL, NULL);

	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// build, buffer sphere
	// Make NightSky Sphere
	MakeSphere(nightSkySphere);

	// Make Moon Sphere
	MakeSphere(moonSphere);

	// Make Earth Sphere
	MakeSphere(earthSphere);

	// Night Sky Texture
	nightSkySphere.textureUnit = 1;
	nightSkySphere.textureName = LoadTexture(nightSkyTextureFile, nightSkySphere.textureUnit, true);

	// Moon Texture and Normal
	moonSphere.textureUnit = 2;
	moonSphere.textureName = LoadTexture(moonTextureFile, moonSphere.textureUnit, true);

	moonSphere.normalUnit = 3;
	moonSphere.normalName = LoadTexture(moonNormalFile, moonSphere.normalUnit, true);

	// Earth Texture and Normal

	earthSphere.textureUnit = 4;
	earthSphere.textureName = LoadTexture(earthTextureFile, earthSphere.textureUnit, true);

	earthSphere.normalUnit = 5;
	earthSphere.normalName = LoadTexture(earthNormalFile, earthSphere.normalUnit, true);

	// callbacks
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetWindowSizeCallback(w, Resize);

	// Keyboard CallBack
	glfwSetKeyCallback(w, Keyboard);

	printf("Usage:\n%s\n", usage);

	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// close
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
	return 0;
}