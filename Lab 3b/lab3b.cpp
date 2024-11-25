// Lab 3b, C++ version
// Terrain generation

// Current contents:
// Terrain being generated on CPU (MakeTerrain). Simple terrain as example: Flat surface with a bump.
// Note the constants kTerrainSize and kPolySize for changing the number of vertices and polygon size!

// Things to do:
// Generate a terrain on CPU, with normal vectors
// Generate a terrain on GPU, in the vertex shader, again with normal vectors.
// Generate textures for the surface (fragment shader).

// If you want to use gradient noise, use the code from Lab 1.

#define MAIN
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "VectorUtils4.h"
#include "LittleOBJLoader.h"
#include "LoadTGA.h"
#include <math.h>
#include "noise1234.h"
#include "simplexnoise1234.h"
#include "cellular.h"
// uses framework OpenGL
// uses framework Cocoa

mat4 projectionMatrix;
Model *floormodel;
GLuint grasstex;

// Reference to shader programs
GLuint phongShader, texShader;

#define kTerrainSize 128
#define kPolySize 0.5

// Terrain data. To be initialized in MakeTerrain or in the shader
vec3 vertices[kTerrainSize*kTerrainSize];
vec2 texCoords[kTerrainSize*kTerrainSize];
vec3 normals[kTerrainSize*kTerrainSize];
GLuint indices[(kTerrainSize-1)*(kTerrainSize-1)*3*2];

// These are considered unsafe, but with most C code, write with caution.
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

void MakeTerrain()
{
	// TO DO: This is where your terrain generation goes if on CPU.
	for (int x = 0; x < kTerrainSize; x++)
	for (int z = 0; z < kTerrainSize; z++)
	{
		int ix = z * kTerrainSize + x;

		#define bumpHeight 1.5f
		#define frequency 0.2f
		#define bumpWidth 2.0

		// squared distance to center
//		float h = ( (x - kTerrainSize/2)/bumpWidth * (x - kTerrainSize/2)/bumpWidth +  (z - kTerrainSize/2)/bumpWidth * (z - kTerrainSize/2)/bumpWidth );
//		float y = MAX(0, 3-h) * bumpHeight;
        float y = bumpHeight * noise2(noise1(frequency*x+noise1(x)),noise1(frequency*z));

		vertices[ix] = vec3(x * kPolySize, y + noise1(y), z * kPolySize);
		texCoords[ix] = vec2(x, z);
		normals[ix] = vec3(1,1,0);
	}


	// Make indices
	// You don't need to change this.
	for (int x = 0; x < kTerrainSize-1; x++)
	for (int z = 0; z < kTerrainSize-1; z++)
	{
		// Quad count
		int q = (z*(kTerrainSize-1)) + (x);
		// Polyon count = q * 2
		// Indices
		indices[q*2*3] = x + z * kTerrainSize; // top left
		indices[q*2*3+1] = x+1 + z * kTerrainSize;
		indices[q*2*3+2] = x + (z+1) * kTerrainSize;
		indices[q*2*3+3] = x+1 + z * kTerrainSize;
		indices[q*2*3+4] = x+1 + (z+1) * kTerrainSize;
		indices[q*2*3+5] = x + (z+1) * kTerrainSize;
	}

	// Make normal vectors
	// TO DO: This is where you calculate normal vectors
	for (int x = 0; x < kTerrainSize; x++)
        for (int z = 0; z < kTerrainSize; z++)
            {
                if (x > 0 && x < kTerrainSize - 1 && z > 0 && z < kTerrainSize - 1) {
    vec3 a = vertices[x+1 + z*kTerrainSize] - vertices[x-1 + z*kTerrainSize];
    vec3 b = vertices[x + (z+1)*kTerrainSize] - vertices[x + (z-1)*kTerrainSize];
    normals[z * kTerrainSize + x] = normalize(cross(b, a));
} else {
    normals[z * kTerrainSize + x] = vec3(0, 1, 0); // Example default normal for edges
}

             }

}
vec3 GenerateCheckerboardColor(float x, float z)
{
    int checkSize = 40; // Size of each square
    bool isBlack = (((int)(x / checkSize) + (int)(z / checkSize)) % 2) == 0;
    return isBlack ? SetVec3(0.0f, 0.0f, 0.0f) : SetVec3(1.0f, 1.0f, 1.0f);
}

void GenerateProceduralTexture(vec3 *textureData)
{
    for (int x = 0; x < kTerrainSize; x++) {
        for (int z = 0; z < kTerrainSize; z++) {
            int index = z * kTerrainSize + x;

            // Example: Checkerboard pattern
            textureData[index] = GenerateCheckerboardColor(x, z);

            // Example: Height-based gradient
            // float height = vertices[index].y / maxHeight; // Normalize height
            // textureData[index] = GenerateHeightBasedColor(height);
        }
    }
}
GLuint CreateProceduralTexture(vec3 *textureData)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configure texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Convert vec3 array to GLubyte array (0-255 range for colors)
    GLubyte *texturePixels = new GLubyte[kTerrainSize * kTerrainSize * 3];
    for (int i = 0; i < kTerrainSize * kTerrainSize; i++) {
        texturePixels[i * 3 + 0] = (GLubyte)(textureData[i].x * 255);
        texturePixels[i * 3 + 1] = (GLubyte)(textureData[i].y * 255);
        texturePixels[i * 3 + 2] = (GLubyte)(textureData[i].z * 255);
    }

    // Upload the texture to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kTerrainSize, kTerrainSize, 0, GL_RGB, GL_UNSIGNED_BYTE, texturePixels);

    delete[] texturePixels; // Clean up
    return textureID;
}

void init(void)
{
	// GL inits
	glClearColor(0.4,0.5,0.8,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	printError("GL inits");

	projectionMatrix = frustum(-0.15 * 640/480, 0.1 * 640/480, -0.1, 0.1, 0.2, 300.0);

	// Load and compile shader
	phongShader = loadShaders("phong.vert", "phong.frag");
	texShader = loadShaders("textured.vert", "textured.frag");
	printError("init shader");

	// Upload geometry to the GPU:
	MakeTerrain();
	floormodel = LoadDataToModel(vertices, normals, texCoords, NULL,
			indices, kTerrainSize*kTerrainSize, (kTerrainSize-1)*(kTerrainSize-1)*2*3);

	printError("LoadDataToModel");

// Important! The shader we upload to must be active!
	glUseProgram(phongShader);
	glUniformMatrix4fv(glGetUniformLocation(phongShader, "projectionMatrix"), 1, GL_TRUE, projectionMatrix.m);
	glUniform1i(glGetUniformLocation(phongShader, "tex"), 0); // Texture unit 0

	glUseProgram(texShader);
	glUniformMatrix4fv(glGetUniformLocation(texShader, "projectionMatrix"), 1, GL_TRUE, projectionMatrix.m);
	glUniform1i(glGetUniformLocation(texShader, "tex"), 0); // Texture unit 0

	LoadTGATextureSimple("grass.tga", &grasstex);
	glBindTexture(GL_TEXTURE_2D, grasstex);
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S,	GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T,	GL_REPEAT);

	printError("init arrays");
}

vec3 campos = vec3(kTerrainSize*kPolySize/4, 1.5, kTerrainSize*kPolySize/4);
vec3 forward = vec3(8, 0, 8);
vec3 up = vec3(0, 1, 0);

void display(void)
{
	printError("pre display");

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 worldToView, m;

	if (glutKeyIsDown('a'))
		forward = mat3(Ry(0.03))* forward;
	if (glutKeyIsDown('d'))
		forward = mat3(Ry(-0.03)) * forward;
	if (glutKeyIsDown('w'))
		campos = campos + forward * 0.01;
	if (glutKeyIsDown('s'))
		campos = campos - forward * 0.01;
	if (glutKeyIsDown('q'))
	{
		vec3 side = cross(forward, vec3(0,1,0));
		campos = campos - side * 0.01;
	}
	if (glutKeyIsDown('e'))
	{
		vec3 side = cross(forward, vec3(0,1,0));
		campos = campos + side * 0.01;
	}

	// Move up/down
	if (glutKeyIsDown('z'))
		campos = campos + vec3(0,1,0) * 0.01;
	if (glutKeyIsDown('c'))
		campos = campos - vec3(0,1,0) * 0.01;

	// NOTE: Looking up and down is done by making a side vector and rotation around arbitrary axis!
	if (glutKeyIsDown('+'))
	{
		vec3 side = cross(forward, vec3(0,1,0));
		mat4 m = ArbRotate(side, 0.01);
		forward = mat3(m) * forward;
	}
	if (glutKeyIsDown('-'))
	{
		vec3 side = cross(forward, vec3(0,1,0));
		mat4 m = ArbRotate(side, -0.01);
		forward = m * forward;
	}

	worldToView = lookAtv(campos, campos + forward, up);

	glBindTexture(GL_TEXTURE_2D, grasstex); // The texture is not used but provided as example
	// Floor
	GLuint shader = texShader;
	glUseProgram(shader);
	m = worldToView;
	glUniformMatrix4fv(glGetUniformLocation(shader, "modelviewMatrix"), 1, GL_TRUE, m.m);
	//  DrawModel(floormodel, shader, "inPosition", "inNormal", "inTexCoord");
	GLuint proceduralTexture;

// Generate the procedural texture
vec3 *textureData = new vec3[kTerrainSize * kTerrainSize];
GenerateProceduralTexture(textureData);
proceduralTexture = CreateProceduralTexture(textureData);
delete[] textureData; // Clean up

// Bind the texture for rendering
glBindTexture(GL_TEXTURE_2D, proceduralTexture);

// Render the terrain
glUseProgram(texShader);
glUniform1i(glGetUniformLocation(texShader, "tex"), 0); // Texture unit 0
DrawModel(floormodel, texShader, "inPosition", "inNormal", "inTexCoord");

	printError("display");

	glutSwapBuffers();
}

void keys(unsigned char key, int x, int y)
{
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 2);
	glutInitWindowSize(640,360);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutCreateWindow ("Lab 3b");
	glutRepeatingTimer(20);
	glutDisplayFunc(display);
	glutKeyboardFunc(keys);
	init ();
	glutMainLoop();
}
