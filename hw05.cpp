//Student: Jake Wise
// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstdlib>
#include <Windows.h>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
using namespace std;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/ext.hpp>
enum class Player { RED, BLUE };

class GamePiece {
	public:
		Player player;
		int row;
		int col;

		GamePiece(Player p, int r, int c) {
			player = p;
			row = r;
			col = c;
		}
};

GamePiece ***gameboard; //6 rows, 7 columns
Player active; //red or blue
bool isEnd = false; //if the game has ended by meeting a condition(red win, blue win, draw)
bool isDraw = false;
bool redWin = false;
bool blueWin = false;
int placeCount = 0;

bool checkWin(int row, int col) {
	int vertical = 1;
	int horizontal = 1;
	int diagonal1 = 1;
	int diagonal2 = 1;

	//check vertical
	for(int i=row+1; i<6 && gameboard[i][col]->player==active; i++) {vertical++;} //up
	for(int i=row-1; i>=0 && gameboard[i][col]->player==active; i--) {vertical++;} //down
	if(vertical >= 4)return true;

	//check horizontal
	for(int j=col-1; j>=0 && gameboard[row][j]->player==active; j--){horizontal++;} //left
	for(int j=col+1; j<7 && gameboard[row][j]->player==active; j++){horizontal++;} //right
	if(horizontal >= 4) return true;

	//check diagonal 1
	for(int i=row-1, j=col-1; i>=0 && j>=0 && gameboard[i][j]->player==active; i--,j--){diagonal1++;} //up and left
	for(int i=row+1, j=col+1; i<6 && j<7 && gameboard[i][j]->player==active; i++,j++){diagonal1++;} //down and right
	if(diagonal1 >= 4) return true;

	//check diagonal 2
	for(int i=row-1, j=col+1; i>=0 && j<7 && gameboard[i][j]->player==active; i--,j++){diagonal2++;} //up and right
	for(int i=row+1, j=col-1; i<6 && j>=0 && gameboard[i][j]->player==active; i++,j--){diagonal2++;} //down and left
	if(diagonal2 >= 4) return true;

	return false;
}

void checkEnd(int row, int col) {
	if(placeCount==42) {
		isDraw = true;
	} 
	if(checkWin(row, col)) {
		if(active==Player::RED) redWin=true;
		if(active==Player::BLUE) blueWin=true;
	}
}

bool placePiece(Player current, int col) {
	cout << "Hello" << endl;
	//check row to be placed in
	for(int i=5; i>=0; i++) {
		if(gameboard[i][col] == nullptr) {
			gameboard[i][col] = new GamePiece(current, i, col);
			placeCount++;
			//checkEnd(i, col);
			return true; //move is valid if spot is empty
		}
	}
	return false; //invalid move since column is full
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	//if (key == GLFW_KEY_P && action == GLFW_PRESS)
		//is_paused = !is_paused;
}


int main( void )
{
	// Initialise GLFW
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
	window = glfwCreateWindow( 1024, 768, "HW4", NULL, NULL);
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

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    //glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Load the texture
	//GLuint Texture = loadDDS("uvmap.DDS");
	//GLuint Texture = loadBMP_custom("rock.bmp");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	// Read our .obj file
	//std::vector<glm::vec3> vertices;
	//std::vector<glm::vec2> uvs;
	//std::vector<glm::vec3> normals; // Won't be used at the moment.
	//bool res = loadOBJ("stone.obj", vertices, uvs, normals);

	// Load it into a VBO

	//GLuint vertexbuffer;
	//glGenBuffers(1, &vertexbuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	//glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	//GLuint uvbuffer;
	//glGenBuffers(1, &uvbuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	//glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	//glfwSetKeyCallback(window, key_callback);
	
	//initialize gameboard
	gameboard = new GamePiece**[6];
	for(int i=0; i<6; i++) {
		gameboard[i] = new GamePiece*[7];
		for(int j=0; j<7;j++) {
			gameboard[i][j] = nullptr;
		}
	}

	active = Player::RED;
	bool pressed = false; //check if a key is pressed
	do{
		if (!isEnd) {
			int placeCol = 0;
			int placeRow = 0;
			bool validPlace = false;
			//get user input here

			if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column1" << endl;
					placeCol = 0;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else {
				pressed = false;
			}
			if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column2" << endl;
					placeCol = 1;
					validPlace = placePiece(active, placeCol);
				}
			}
			else {
				pressed = false;
			}
			if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column3" << endl;
					placeCol = 2;
					validPlace = placePiece(active, placeCol);
				}
			}
			else {
				pressed = false;
			}
			if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column4" << endl;
					placeCol = 3;
					validPlace = placePiece(active, placeCol);
				}
			}
			else {
				pressed = false;
			}
			if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column5" << endl;
					placeCol = 4;
					validPlace = placePiece(active, placeCol);
				}
			}
			else {
				pressed = false;
			}
			if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column6" << endl;
					placeCol = 5;
					validPlace = placePiece(active, placeCol);
				}
			}
			else {
				pressed = false;
			}
			if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column7" << endl;
					placeCol = 6;
					validPlace = placePiece(active, placeCol);
				}
			}
			else {
				pressed = false;
			}

			if (validPlace) {
				if (isDraw || redWin || blueWin) {
					//do end game stuff here
					if (redWin) cout << "Congratulations Red player!" << endl;
					if (blueWin) cout << "Congratulations Blue player!" << endl;
					isEnd = true;
				}
				else {
					//switch player
					if (active == Player::RED) {
						active = Player::BLUE;
					}
					else if (active == Player::BLUE) {
						active = Player::RED;
					}
				}
			}
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		//computeMatricesFromInputs();
		//glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
		//glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ViewMatrix = glm::lookAt(
										glm::vec3(0, 0, 2),           
										glm::vec3(0, 0, 0), 
										glm::vec3(0, 1, 0)                  
		);
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Bind our texture in Texture Unit 0
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		//glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		//glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		//glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		//glDrawArrays(GL_TRIANGLES, 0, vertices.size() );

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	//glDeleteBuffers(1, &vertexbuffer);
	//glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(programID);
	//glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

