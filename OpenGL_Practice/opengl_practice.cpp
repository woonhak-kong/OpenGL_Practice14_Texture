#include <iostream>
#include <fstream>

// glew.h�� gl.h�� �����ϱ� ���� �����ؾ��Ѵ�.
#include "GL/glew.h"
#include "GL/freeglut.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#include "Texture.h"

using namespace std;

// ���ϴ� ������ ��
#define FPS 30

#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,0.9,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)

// Camera and transform variables.
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function.
GLfloat pitch, yaw;
GLfloat moveSpeed = 0.1f;
GLfloat turnSpeed = 1.0f;
float rotAngle = 0.0f;

// Texture variables.
GLuint textureID;
GLint width, height, bitDepth;
Texture* pTexture = NULL;
GLuint gSampler;

// Mouse variables.
bool mouseFirst = true, mouseClicked = false;
int lastX, lastY;

GLuint programHandle;
GLuint vaoHandle;
GLuint mvpId;
glm::mat4 MVP, View, Projection;
GLuint setShader(const char* shaderType, const char* shaderName);
char* loadShaderAsString(std::string fileName);

float angle = 0.0f;


// Prototype
void timer(int);
void resetView();
void loadTexture(std::string filename);
void createBuffer();
void calculateView();
void keyDown(unsigned char key, int x, int y);
void keyDownSpecial(int key, int x, int y);
void mouseMove(int x, int y);
void mouseClick(int btn, int state, int x, int y);
void clean();



GLshort cube_indices[] = {
	// Front.
	0, 3, 2,
	2, 0, 1
};

//! Add the texture coordinates to the vertices
GLfloat cube_vertices[] = {
	//	x,		y,		z,	  u,	v
		-1.0f, -1.0f, 1.0f,	0.0f, 0.0f,		// 0. Bottom Left
		1.0f,  -1.0f, 1.0f, 1.0f, 0.0f,		// 1. Bottom Right
		1.0f,   1.0f, 1.0f, 1.0f, 1.0f,		// 2. Top Right
		-1.0f,  1.0f, 1.0f, 0.0f, 1.0f		// 3. Top Left
};

GLfloat colors[] = {
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f
};

void init()
{

	// 1. ���̴� ������Ʈ�� �����Ѵ�.
	GLuint vertShader = setShader("vertex", "basic.vert");
	GLuint fragShader = setShader("fragment", "basic.frag");

	// ������Ʈ �������� ������ OpenGL pipeline�� �������� ���� ���̴��� ��� Ȥ�� ��ġ�ؾ��Ѵ�.

	// 1. ���� Program object�� �����Ѵ�.
	// �� ���α׷� ����
	programHandle = glCreateProgram();
	if (0 == programHandle)
	{
		fprintf(stderr, "Error creating program object.\n");
		exit(1);
	}

	// 2. ���̴����� ���α׷��� ���δ�.
	glAttachShader(programHandle, vertShader);
	glAttachShader(programHandle, fragShader);

	// 3. ���α׷��� ��ũ�Ѵ�. ����
	glLinkProgram(programHandle);

	// 4. ��ũ ���� Ȯ��
	GLint status;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &status);
	if (GL_FALSE == status) {
		fprintf(stderr, "Failed to link shader program!\n");
		GLint logLen;
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH,
			&logLen);
		if (logLen > 0)
		{
			char* log = (char*)malloc(logLen);
			GLsizei written;
			glGetProgramInfoLog(programHandle, logLen,
				&written, log);
			fprintf(stderr, "Program log: \n%s", log);
			free(log);
		}
	}
	// 5. ���� ��ũ�� �����ߴٸ� ���α׷��� OpenGL pipeline�� ��ġ �Ѵ�.
	else
	{
		glUseProgram(programHandle);
	}

	// uniform ���
	mvpId = glGetUniformLocation(programHandle, "mvp");
	gSampler = glGetUniformLocation(programHandle, "texture0");
	assert(gSampler != 0xFFFFFFFF);


	resetView();

	glUniform1i(gSampler, 0);
	//loadTexture("Media/spheremap.png");
	pTexture = new Texture(GL_TEXTURE_2D, "Media/spheremap.png", GL_RGB);
	pTexture->Bind(GL_TEXTURE0);
	if (!pTexture->Load()) {
		exit(0);
	}

	//loadTexture("Media/spheremap.png");
	createBuffer();




	//���� ���
	glEnable(GL_DEPTH_TEST);
	// Ÿ�̸� ��ŸƮ
	timer(0);
}
//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(float scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, glm::vec3(scale));
	MVP = Projection * View * Model;
	glUniformMatrix4fv(mvpId, 1, GL_FALSE, &MVP[0][0]);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vaoHandle);


	// Update the projection or view if perspective.
	Projection = glm::perspective(glm::radians(60.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	calculateView();
	transformObject(0.4f, YZ_AXIS, rotAngle -= 0, glm::vec3(0.0f, 0.0f, 0.0f));
	// 3��° ���� - ���ý��� ��
	//glDrawArrays(GL_QUADS, 0, 4);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	glutSwapBuffers(); // Now for a potentially smoother render.
}

void timer(int) {
	glutPostRedisplay();
	glutTimerFunc(1000 / FPS, timer, 0);
}

void resetView()
{
	position = glm::vec3(0.0f, 0.0f, 5.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0, 1, 0);
	pitch = 0.0f;
	yaw = -90.0f;
}

void loadTexture(std::string filename)
{
	stbi_set_flip_vertically_on_load(true);

	//filename.c_str() to convert to constant char*
	//bitDepth:how many bit perpixel
	//unsigned char* image = stbi_load("Media/spheremap.png", &width, &height, &bitDepth, 0);
	unsigned char* image = stbi_load(filename.c_str(), &width, &height, &bitDepth, 0);
	if (!image) {
		cout << "Unable to load file!" << stbi_failure_reason() << endl;

		exit(0);
		// Could add a return too if you modify init.
	}


	//!Generate a handler for texture object
	glGenTextures(1, &textureID);


	//!This tells openGL if the texture object is 1D, 2D, 3D, etc..
	glBindTexture(GL_TEXTURE_2D, textureID);

	/// @note: all texture objects cannot be available to the shader.
	/// That's why we have texture units sitting between texture objects and shaders.
	/// Then shaders samples from the texture unit.
	/// So between draw calls, we can point to a different texture unit.
	int textureUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnits);
	cout << "The number of my GPU texture units: " << textureUnits;

	//! Load the texture object from CPU to GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width,
		height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	//! Configure the texture state
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


	//!Activate a texture unit!
	glActiveTexture(GL_TEXTURE0);

	//!Set the index of the texture unit into the sampler
	glUniform1i(glGetUniformLocation(programHandle, "texture0"), 0);

	glGenerateMipmap(GL_TEXTURE_2D);

	// Clean up. But we don't want to unbind the texture or we cannot use it.
	stbi_image_free(image);
}

void createBuffer()
{

	// ���� ������Ʈ�� �����Ͽ��� ������, �̰͵��� vertex array obejct(VAO) �� ���´�.
	// VAO �����Ѵ�. (���������� vaoHandle �ʿ�)
	// Create and set-up the vertex array object
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);



	// �����ǰ� ������ �����ϱ� ���� ���۸� �����Ѵ�.
	// Create the buffer objects
	GLuint vboHandles[2];
	// ���� �ΰ�����.
	glGenBuffers(2, vboHandles);
	GLuint positionBufferHandle = vboHandles[0];
	GLuint colorBufferHandle = vboHandles[1];





	// vertex
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cube_vertices[0]) * 5, (GLubyte*)NULL);
	glEnableVertexAttribArray(0); // for Vertex position

	// Now for the UV/ST values, set the layout of the vertex buffer
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(cube_vertices[0]) * 5, (void*)(sizeof(cube_vertices[0]) * 3));
	//Enable the texture coordinate attribute
	glEnableVertexAttribArray(2);


	// Populate the color buffer
	// ���� ���۸� ���ε��Ѵ�.
	glBindBuffer(GL_ARRAY_BUFFER, colorBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
	glEnableVertexAttribArray(1); // for Vertex color



	// �ε����� ���� ������Ʈ
	GLuint indexBufferObjec;
	glGenBuffers(1, &indexBufferObjec);
	// GL_ELEMENT_ARRAY_BUFFER�� ����Ѵ�.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObjec);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

}

void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(position, position + frontVec, upVec);
}

void keyDown(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'w':
			position += frontVec * moveSpeed;
			break;
		case 's':
			position -= frontVec * moveSpeed;
			break;
		case 'a':
			position += rightVec * moveSpeed;
			break;
		case 'd':
			position -= rightVec * moveSpeed;
			break;
		case ' ':
			resetView();
			break;
	}
}

void keyDownSpecial(int key, int x, int y)
{
	switch (key)
	{
		case GLUT_KEY_UP:
			pitch -= turnSpeed;
			break;
		case GLUT_KEY_DOWN:
			pitch += turnSpeed;
			break;
		case GLUT_KEY_LEFT:
			yaw += turnSpeed;
			break;
		case GLUT_KEY_RIGHT:
			yaw -= turnSpeed;
			break;
	}
}

void mouseMove(int x, int y)
{
	//cout << "Mouse pos: " << x << "," << y << endl;
	if (mouseClicked)
	{
		pitch += (GLfloat)((y - lastY) * 0.1);
		yaw -= (GLfloat)((x - lastX) * 0.1);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	/*cout << "Clicked: " << (btn == 0 ? "left " : "right ") << (state == 0 ? "down " : "up ") <<
		"at " << x << "," << y << endl;*/
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		mouseClicked = true;
		glutSetCursor(GLUT_CURSOR_NONE);
		cout << "Mouse clicked." << endl;
	}
	else
	{
		mouseClicked = false;
		glutSetCursor(GLUT_CURSOR_INHERIT);
		cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	glDeleteTextures(1, &textureID);
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);

	//������ ������ ����
	glutInitWindowSize(1024, 720);

	// top-left corner �ʱ� ���������� �ʱ�ȭ
	glutInitWindowPosition(0, 0);

	// ������â ����
	glutCreateWindow("Hello World");


	// glew�� �ʱ�ȭ ���ش�. opengl�� ����ϱ����ؼ�
	GLenum err = glewInit();
	// glewInit()�� �����ν� ��� OpenGL���̺귯���� ã�� ��� ��밡���� �Լ������͸� �ʱ�ȭ�Ѵ�.

	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error initializing GLEW: %s\n",
			glewGetErrorString(err));
	}

	// to compile shader
	init();
	glutDisplayFunc(display);

	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpecial);
	glutMouseFunc(mouseClick);
	//glutPassiveMotionFunc(mouseMove); // or...
	glutMotionFunc(mouseMove); // Requires click to register.
	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	// glut�� �̺�Ʈ ���μ��� loop�� ����.
	glutMainLoop();

	return 0;
}



GLuint setShader(const char* shaderType, const char* shaderName)
{
	GLuint shaderObj;
	if (strcmp(shaderType, "vertex") == 0)
	{
		shaderObj = glCreateShader(GL_VERTEX_SHADER);
	}
	else if (strcmp(shaderType, "fragment") == 0)
	{
		shaderObj = glCreateShader(GL_FRAGMENT_SHADER);
	}


	if (0 == shaderObj)
	{
		fprintf(stderr, "Error creating shader obj.\n");
		exit(1);
	}


	// 2. ���̴� �ҽ��ڵ带 ���̴� ������Ʈ�� �����Ѵ�.
	const GLchar* shaderCode = loadShaderAsString(shaderName);
	// �ҽ� �迭�� ���� �ҽ��ڵ带 ���� �� �ִ�.
	// �迭�� �ҽ��ڵ带 ������.
	const GLchar* codeArray[] = { shaderCode };
	// vertShader object�� codeArray�� �����Ѵ�.
	// ù��° ���ڴ� ���̴� ������Ʈ, �ι�° ���ڴ� �ҽ��ڵ��� �� ���� ���⼭�� shaderCode �Ѱ��� ���� 1
	// ����° ���ڴ� �ڵ带 ���� �迭, �׹�° ���ڴ� �� �ҽ��ڵ��� ���̸� ���� int�迭�̴� ���⼭�� null character�� �־ �ڵ����� Ȯ�εǱ⶧���� NULL�� �־���.
	glShaderSource(shaderObj, 1, codeArray, NULL);

	// 3. ���̴��� ������ �Ѵ�.
	glCompileShader(shaderObj);


	// 4. ������ �Ϸ� Ȯ��.
	GLint result;
	glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &result);
	if (GL_FALSE == result)
	{
		fprintf(stderr, "shader compilation failed!\n");
		GLint logLen;
		glGetShaderiv(shaderObj, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char* log = (char*)malloc(logLen);
			GLsizei written;
			glGetShaderInfoLog(shaderObj, logLen, &written, log);
			fprintf(stderr, "Shader log:\n%s", log);
			free(log);
		}
	}

	return shaderObj;
}

char* loadShaderAsString(std::string fileName)
{
	// Initialize input stream.
	std::ifstream inFile(fileName.c_str(), std::ios::binary);

	// Determine shader file length and reserve space to read it in.
	inFile.seekg(0, std::ios::end);
	int fileLength = inFile.tellg();
	char* fileContent = (char*)malloc((fileLength + 1) * sizeof(char));

	// Read in shader file, set last character to NUL, close input stream.
	inFile.seekg(0, std::ios::beg);
	inFile.read(fileContent, fileLength);
	fileContent[fileLength] = '\0';
	inFile.close();

	return fileContent;
}