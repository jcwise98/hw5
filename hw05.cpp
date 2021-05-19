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
#include <glm/gtx/transform.hpp>
#include <glm/gtx/transform2.hpp>

using namespace glm;
using namespace std;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/text2D.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/ext.hpp>

glm::vec3 lerp(glm::vec3 p0, glm::vec3 p1, float t)
{
	return glm::vec3(
		p0.x + (p1.x - p0.x) * t,
		p0.y + (p1.y - p0.y) * t,
		p0.z + (p1.z - p0.z) * t
	);
}

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

enum class Player { RED, YELLOW };

class GamePiece {
public:

	static const int FRAME_LENGTH = 2000;

	glm::mat4 model;
	Player player;
	int row;
	int col;
	int frame;

	GamePiece::GamePiece(Player p, int r, int c): player(p), model(1.0), row(r), col(c), frame(0) {
		model *= glm::mat4(1.0) * glm::translate(glm::vec3(0, -5, -3 + c));
	}

	void GamePiece::update() {
		if (frame <= FRAME_LENGTH) {
			model = glm::mat4(1.0) * glm::translate(lerp(glm::vec3(0, -5, -3 + col), glm::vec3(0, row - 2.5, -3 + col), (float)(++frame) / FRAME_LENGTH));
		}
	}
};

GamePiece*** gameboard; //6 rows, 7 columns
Player active; //red or yellow
bool isEnd = false; //if the game has ended by meeting a condition(red win, yellow win, draw)
bool isDraw = false;
bool redWin = false;
bool yellowWin = false;
int placeCount = 0;

bool checkWin(int row, int col) {
	cout << "checkWin" << endl;
	cout << "row: " << row << ", col: " << col << endl;
	int vertical = 1;
	int horizontal = 1;
	int diagonal1 = 1;
	int diagonal2 = 1;

	//check vertical
	for (int i = row + 1; i < 6 && gameboard[i][col] != nullptr && gameboard[i][col]->player == active; i++) { vertical++; } //down
	//for(int i=row-1; i>=0 && gameboard[i][col] != nullptr && gameboard[i][col]->player==active; i--) {vertical++;} //up
	if (vertical >= 4)return true;

	//check horizontal
	for (int j = col - 1; j >= 0 && gameboard[row][j] != nullptr && gameboard[row][j]->player == active; j--) { horizontal++; } //left
	for (int j = col + 1; j < 7 && gameboard[row][j] != nullptr && gameboard[row][j]->player == active; j++) { horizontal++; } //right
	if (horizontal >= 4) return true;

	//check diagonal 1
	for(int i=row-1, j=col-1; i>=0 && j>=0 && gameboard[i][j] != nullptr && gameboard[i][j]->player==active; i--,j--){diagonal1++;} //up and left
	for (int i = row + 1, j = col + 1; i < 6 && j < 7 && gameboard[i][j] != nullptr && gameboard[i][j]->player == active; i++, j++) { diagonal1++; } //down and right
	if (diagonal1 >= 4) return true;

	//check diagonal 2
	for(int i=row-1, j=col+1; i>=0 && j<7 && gameboard[i][j] != nullptr && gameboard[i][j]->player==active; i--,j++){diagonal2++;} //up and right
	for (int i = row + 1, j = col - 1; i < 6 && j >= 0 && gameboard[i][j] != nullptr && gameboard[i][j]->player == active; i++, j--) { diagonal2++; } //down and left
	if (diagonal2 >= 4) return true;

	return false;
}

void checkEnd(int row, int col) {
	if (placeCount == 42) {
		isDraw = true;
	}
	if (checkWin(row, col)) {
		if (active == Player::RED) redWin = true;
		if (active == Player::YELLOW) yellowWin = true;
	}
}

bool placePiece(Player current, int col) {
	//check row to be placed in
	for (int i = 5; i >= 0; i--) {
		cout << "attempt row " << i << endl;
		if (gameboard[i][col] == nullptr) {
			cout << "placed in row: " << i << endl;
			gameboard[i][col] = new GamePiece(current, i, col);
			placeCount++;
			checkEnd(i, col);
			return true; //move is valid if spot is empty
		}
	}
	return false; //invalid move since column is full
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	//if (key == GLFW_KEY_P && action == GLFW_PRESS)
		//is_paused = !is_paused;
}


int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "HW4", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
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

	// Resource Initialization
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwPollEvents();
	//glfwSetCursorPos(window, 1024 / 2, 768 / 2);
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearColor(0.0f, 0.0f, 0.1f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Textures
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint BlueTexture = loadDDS("Blue.dds");
	GLuint YellowTexture = loadDDS("Yellow.dds");
	GLuint RedTexture = loadDDS("Red.dds");

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
	/*controlPoints.push_back(glm::vec3(11, -3, 0));
	controlPoints.push_back(glm::vec3(11, -3, 0));
	controlPoints.push_back(glm::vec3(11, -3, 0));*/
	controlPoints.push_back(glm::vec3(11, -3, 0));
	//controlPoints.push_back(glm::vec3(0, -3, 11));
	//controlPoints.push_back(glm::vec3(-11, -3, 0));
	//controlPoints.push_back(glm::vec3(0, -3, -11));

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

	initText2D("Holstein.DDS");


	//initialize gameboard
	gameboard = new GamePiece * *[6];
	for (int i = 0; i < 6; i++) {
		gameboard[i] = new GamePiece * [7];
		for (int j = 0; j < 7; j++) {
			gameboard[i][j] = nullptr;
		}
	}

	//randomize starting player
	srand(time(NULL));
	int result = rand() % 2;
	cout << result << endl;
	if (result) {
		active = Player::RED;
	}
	else {
		active = Player::YELLOW;
	}
	bool pressed = false; //check if a key is pressed
	do {
		char text[256];
		if (!isEnd) {
			if (active == Player::RED) sprintf(text, "Red Player's Turn");
			if (active == Player::YELLOW) sprintf(text, "Yellow Player's Turn");
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
			else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column2" << endl;
					placeCol = 1;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column3" << endl;
					placeCol = 2;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column4" << endl;
					placeCol = 3;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column5" << endl;
					placeCol = 4;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column6" << endl;
					placeCol = 5;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
				if (!pressed) {
					cout << "column7" << endl;
					placeCol = 6;
					validPlace = placePiece(active, placeCol);
					pressed = true;
				}
			}
			else {
				pressed = false;
			}

			if (validPlace) {
				if (isDraw || redWin || yellowWin) {
					//do end game stuff here
					if (redWin) {
						cout << "Congratulations Red player!" << endl;
						sprintf(text, "Red player wins!");
					}
					if (yellowWin) {
						cout << "Congratulations Yellow player!" << endl;
						sprintf(text, "Yellow player wins!");
					}
					isEnd = true;
				}
				else {
					//switch player
					if (active == Player::RED) {
						active = Player::YELLOW;
					}
					else if (active == Player::YELLOW) {
						active = Player::RED;
					}
				}
			}
		}

		// Game update
		for (int y = 0; y < 6; y++) {
			for (int x = 0; x < 7; x++) {
				GamePiece* piece = gameboard[y][x];
				if (piece != nullptr) {
					piece->update();
				}
			}
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);
		glUniform1i(TextureID, 0);
		glActiveTexture(GL_TEXTURE0);

		// Compute the MVP matrix from keyboard and mouse input
		//computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
		glm::mat4 ViewMatrix = glm::lookAt(cameraPoints[index], glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));//getViewMatrix();
		//glm::mat4 ProjectionMatrix = getProjectionMatrix();
		//glm::mat4 ViewMatrix = getViewMatrix();
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
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		glm::vec3 lightPos = glm::vec3(-5, -0.5, 0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);


		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		// DRAWING COIN ------------------------------------------------------------
		glBindTexture(GL_TEXTURE_2D, RedTexture);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, coinvertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, coinuvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		for (int y = 0; y < 6; y++) {
			for (int x = 0; x < 7; x++) {
				GamePiece* piece = gameboard[y][x];
				if (piece != nullptr && piece->player == Player::RED) {
					MVP = ProjectionMatrix * ViewMatrix * piece->model;
					glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
					//glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
					glDrawArrays(GL_TRIANGLES, 0, coinvertices.size());
					//cout << "draw" << "(" << piece->col << "," << piece->row << ")(" << x << "," << y << ")" << endl;
				}
			}
		}

		glBindTexture(GL_TEXTURE_2D, YellowTexture);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, coinvertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, coinuvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		for (int y = 0; y < 6; y++) {
			for (int x = 0; x < 7; x++) {
				GamePiece* piece = gameboard[y][x];
				if (piece != nullptr && piece->player == Player::YELLOW) {
					MVP = ProjectionMatrix * ViewMatrix * piece->model;
					glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
					glDrawArrays(GL_TRIANGLES, 0, coinvertices.size());
					//cout << "draw" << "(" << piece->col << "," << piece->row << ")(" << x << "," << y << ")" << endl;
				}
			}
		}

		printText2D(text, 100, 500, 30);


		// END DRAWING ------------------------------------------------------------

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

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