#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <snake.h>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <json/json.h>

std::stack<Point> gridCoords;

void initGrids();

Point SNAKE_START_DEFAULT[] = {
	Point {2,2},
	Point {7,7}
};

int SNAKE_DIRECTIONS_DEFAULT[] = {
	DOWN,
	UP
};

Point GRID_POS_DEFAULT[] = {
	Point {50, 500},
	Point {50, 50}
};

//Constants (will be taken from json)
int MAX_GAMES = 2;
int SNAKES_PER_GAME = 2;
int SNAKE_LENGTH = 3;
int FOOD_PER_GAME = 3;
Point* SNAKE_START;
int* SNAKE_DIRECTIONS;

int GRID_SIZE = 10;
int GRID_WIDTH = 350;
int GRID_HEIGHT = 350;


std::vector<float*> colours;
int currentGames = 0;
fd_set listener;

timeval timeout;

static int gamesInProgress;

std::vector<Player*> players;
std::vector<SnakeGrid> games;

std::unordered_map<std::string, int> namesUsed;

static void loadConstants(const std::string constants) {
	std::ifstream constants_file("res/config.json", std::ifstream::binary);
	Json::Value constantsJson;
	constants_file >> constantsJson;
	constantsJson = constantsJson[constants];

	MAX_GAMES = constantsJson.get("max_games", MAX_GAMES).asInt();
	SNAKES_PER_GAME = constantsJson.get("snakes_per_game", SNAKES_PER_GAME).asInt();
	SNAKE_LENGTH = constantsJson.get("snake_length", SNAKE_LENGTH).asInt();
	FOOD_PER_GAME = constantsJson.get("food_per_game", FOOD_PER_GAME).asInt();


	//Load in snake start positions
	Json::Value snakeStartTemp = constantsJson["snake_start"];
	if (!snakeStartTemp.isNull()) {
		SNAKE_START = (Point*) _malloca(sizeof(Point) * snakeStartTemp.size());
		for (int index = 0; index < snakeStartTemp.size(); index++) {
			SNAKE_START[index] = Point{ snakeStartTemp[index][0].asInt(), snakeStartTemp[index][1].asInt() };
		}
	}
	else {
		SNAKE_START = SNAKE_START_DEFAULT;
	}

	//Load in snake start directions
	Json::Value snakeDirectionsTemp = constantsJson["snake_directions"];
	if (!snakeDirectionsTemp.isNull()) {
		SNAKE_DIRECTIONS = (int*)_malloca(sizeof(int) * snakeDirectionsTemp.size());
		for (int index = 0; index < snakeDirectionsTemp.size(); index++) {
			SNAKE_DIRECTIONS[index] = snakeDirectionsTemp[index].asInt();
		}
	}
	else {
		SNAKE_DIRECTIONS = SNAKE_DIRECTIONS_DEFAULT;
	}

	GRID_SIZE = constantsJson.get("grid_size", GRID_SIZE).asInt();
	GRID_WIDTH = constantsJson.get("grid_width", GRID_WIDTH).asInt();
	GRID_HEIGHT = constantsJson.get("grid_height", GRID_HEIGHT).asInt();


	//Setup gridCoords stack
	while (!gridCoords.empty()) {
		gridCoords.pop();
	}

	Json::Value gridPosTemp = constantsJson["grid_pos"];
	if (!gridPosTemp.isNull()) {
		for (int index = gridPosTemp.size() - 1; index >= 0; index--) {
			gridCoords.push(Point{ gridPosTemp[index][0].asInt(), gridPosTemp[index][1].asInt() });
		}
	}
	else {
		for (Point point: GRID_POS_DEFAULT) {
			gridCoords.push(point);
		}
	}

}

static void checkForIncomingConnections() {
	fd_set copy = listener;

	int socketCount = select(0, &copy, nullptr, nullptr, &timeout);

	if (socketCount) {
		SOCKET sock = copy.fd_array[0];

		SOCKET client = accept(sock, nullptr, nullptr);

		Player* player = new Player;
		player->socket = client;

		char color[3];
		ZeroMemory(color, 3);

		int bytesIn = recv(client, color, 3, 0); //Get Snake Colour

		if (bytesIn < 3) {
			char buff[4096];
			strerror_s(buff, 3);
			std::cout << buff << std::endl;
			closesocket(client);
			return;
		}
		else {
			colours.push_back(new float[] {
				((unsigned char)color[0]) / 255.0f,
				((unsigned char)color[1]) / 255.0f,
				((unsigned char)color[2]) / 255.0f,
				1.0f
			});
		}

		player->colourId = colours.size() - 1;
		
		char name[64];
		ZeroMemory(name, 64);

		bytesIn = recv(client, name, 64, 0);

		namesUsed[name] += 1;
		if (namesUsed[name] == 1) {
			player->name = name;
		}
		else {
			player->name = std::string(name) + " " + std::to_string(namesUsed[name]);
		}

		std::cout << player->name << " joined!" << std::endl;

		send(client, (char*) &player->colourId, 4, 0);

		players.push_back(player);
	}
}

static void checkForGameStart() {
	if (games.size() >= MAX_GAMES) return;
	std::vector<Player*> availablePlayers;

	for (Player* player : players) {
		if (!player->inGame) {
			availablePlayers.push_back(player);
		}
	}

	if (availablePlayers.size() >= SNAKES_PER_GAME) {
		std::random_shuffle(availablePlayers.begin(), availablePlayers.end());
		Point gridPos = gridCoords.top();
		gridCoords.pop();
		SnakeGrid newGrid(GRID_SIZE, gridPos.x, gridPos.y, GRID_WIDTH, GRID_HEIGHT);
		for (int i = 0; i < SNAKES_PER_GAME; i++) {
			Player* player = availablePlayers[i];
			Point snakeStartPos = SNAKE_START[i];
			newGrid.addSnake(snakeStartPos.x, snakeStartPos.y, SNAKE_LENGTH, SNAKE_DIRECTIONS[i], player);
		}
		newGrid.startGame();
		games.push_back(newGrid);
	}
}

static void checkForFinishedGames() {
	for (int i = games.size() - 1; i >= 0; i--) {
		if (games[i].ended) {
			for (Snake& snake : games[i].snakes) {
				snake.player->inGame = false;
				if (!(snake.player->connected)) {
					for (int i = min(snake.player->colourId - 2, players.size() - 1); i >= 0; i--) {
						if (players[i] == snake.player) {
							players.erase(players.begin() + i);
							break;
						}
					}
				}
			}
			std::cout << games.size() << ", ";
			gridCoords.push(Point{ games[i].getStartX(), games[i].getStartY() });
			games.erase(games.begin() + i);
			std::cout << games.size() << std::endl;
		}
	}
}

int main() {
	loadConstants("quad");
	std::srand(time(NULL));

	colours.reserve(1024);

	colours.push_back(new float[] {0.1f, 0.1f, 0.1f, 1.0f}); //Grid Background Color
	colours.push_back(new float[] {1.0f, 1.0f, 0.0f, 1.0f}); //Food Color

	GLFWwindow* window;

	if (!glfwInit()) {
		return -1;
	};

	window = glfwCreateWindow(900, 900, "Snake", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		std::cout << "Error!" << std::endl;
	}

	initAbstractions();

	//Setup Socket
	timeout.tv_sec = 0;
	timeout.tv_usec = 10;

	initGrids();

	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	if (WSAStartup(ver, &wsData) != 0) {
		std::cerr << "Can't Initialise winsock! Quitting" << std::endl;
		return -1;
	}

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		std::cerr << "Can't create a socket! Quitting!" << std::endl;
		return -1;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	inet_pton(AF_INET, IP, &(hint.sin_addr));

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	listen(listening, SOMAXCONN);

	FD_ZERO(&listener);

	FD_SET(listening, &listener);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		for (SnakeGrid& grid : games) {
			grid.checkForInboundMoves();
			grid.draw();
		}

		glfwSwapBuffers(window);

		glfwPollEvents();

		checkForIncomingConnections();

		checkForFinishedGames();

		checkForGameStart();
	}

	for (Player* player : players) {
		delete player;
	}

}