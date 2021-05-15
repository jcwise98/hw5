// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>

glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
	glm::vec3 a = p1 * 2.0f;
	glm::vec3 b = (-p0 + p2) * t;
	glm::vec3 c = (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t * t;
	glm::vec3 d = (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t * t * t;
	return (a + b + c + d) * 0.5f;
}

std::vector<glm::vec3> computeCameraPositions(std::vector<glm::vec3> controls, int times)
{
	std::vector<glm::vec3> points;
	size_t size = controls.size();
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < times; j++)
		{
			float t = (float)j / (float)times;
			glm::vec3 p0 = controls[(i - 1) % size];
			glm::vec3 p1 = controls[i];
			glm::vec3 p2 = controls[(i + 1) % size];
			glm::vec3 p3 = controls[(i + 2) % size];
			points.push_back(catmullRom(p0, p1, p2, p3, t));
		}
	}

	return points;
}

int main( void )
{
	// Initialise	 GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Tutorial 07 - Model Loading", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	GLuint programID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Textures
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	GLuint BlueTexture = loadDDS("Blue.dds");
	GLuint YellowTexture = loadDDS("Yellow.dds");
	GLuint RedTexture = loadDDS("Red.dds");

	
	// Get a handle for our "myTextureSampler" uniform
	

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals; // Won't be used at the moment.
	loadOBJ("Board.obj", vertices, uvs, normals);

	std::vector<glm::vec3> coinvertices;
	std::vector<glm::vec2> coinuvs;
	std::vector<glm::vec3> coinnormals; // Won't be used at the moment.
	loadOBJ("Coin.obj", coinvertices, coinuvs, coinnormals);

	std::vector<glm::vec3> controlPoints;
	controlPoints.push_back(glm::vec3(11, 0, 0));
	controlPoints.push_back(glm::vec3(11, 0, 0));
	controlPoints.push_back(glm::vec3(11, 0, 0));
	controlPoints.push_back(glm::vec3(11, 0, 0));
	//controlPoints.push_back(glm::vec3(0, 0, 11));
	//controlPoints.push_back(glm::vec3(-11, 0, 0));
	//controlPoints.push_back(glm::vec3(0, 0, -11));

	bool paused = false;
	bool pressed = false;
	std::vector<glm::vec3> cameraPoints = computeCameraPositions(controlPoints, 500);
	int index = 0;

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	GLuint coinvertexbuffer;
	glGenBuffers(1, &coinvertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, coinvertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, coinvertices.size() * sizeof(glm::vec3), &coinvertices[0], GL_STATIC_DRAW);

	GLuint coinuvbuffer;
	glGenBuffers(1, &coinuvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, coinuvbuffer);
	glBufferData(GL_ARRAY_BUFFER, coinuvs.size() * sizeof(glm::vec2), &coinuvs[0], GL_STATIC_DRAW);

	do{
		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		{
			if (!pressed)
			{
				paused = !paused;
				pressed = true;
			}
		}
		else pressed = false;
		if (!paused) index = (index + 1) % cameraPoints.size();
		
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);
		glUniform1i(TextureID, 0);
		glActiveTexture(GL_TEXTURE0);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = glm::lookAt(cameraPoints[index], glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));//getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP;

		// DRAWING BOARD ------------------------------------------------------------
		glBindTexture(GL_TEXTURE_2D, BlueTexture);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size() );

		// DRAWING COIN ------------------------------------------------------------
		glBindTexture(GL_TEXTURE_2D, YellowTexture);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, coinvertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, coinuvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, coinvertices.size());


		// END DRAWING ------------------------------------------------------------

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &coinvertexbuffer);
	glDeleteBuffers(1, &coinuvbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &BlueTexture);
	glDeleteTextures(1, &YellowTexture);
	glDeleteTextures(1, &RedTexture);
	glDeleteVertexArrays(1, &VertexArrayID);
	glfwTerminate();
	return 0;
}

