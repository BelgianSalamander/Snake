#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <snake.h>
#include <time.h>
#include <random>
#include <algorithm>

#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

const int MAX_GAMES = 1;

std::vector<float*> colours;
int currentGames = 0;
fd_set listener;

timeval timeout;

static int gamesInProgress;

std::vector<Player> players;
std::vector<SnakeGrid> games;

static void checkForIncomingConnections() {
	fd_set copy = listener;

	int socketCount = select(0, &copy, nullptr, nullptr, &timeout);

	if (socketCount) {
		SOCKET sock = copy.fd_array[0];

		SOCKET client = accept(sock, nullptr, nullptr);

		Player player;
		player.socket = client;

		char color[3];
		ZeroMemory(color, 3);

		int bytesIn = recv(client, color, 3, 0); //Get Snake Colour

		if (bytesIn < 3) {
			char buff[4096];
			strerror_s(buff, 3);
			std::cout << buff << std::endl;
			std::cout << "Only sent " << bytesIn << " bytes" << std::endl;
			std::cout << color[0] << " " << color[1] << " " << color[2] << std::endl;
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

		player.colourId = colours.size() - 1;
		
		char name[64];
		ZeroMemory(name, 64);

		bytesIn = recv(client, name, 64, 0);

		player.nameLength = bytesIn;
		player.name = name;

		std::cout << name << " joined!" << std::endl;

		send(client, (char*) &player.colourId, 4, 0);

		players.push_back(player);
	}
}

static void checkForGameStart() {
	if (games.size() >= MAX_GAMES) return;
	std::vector<Player> availablePlayers;

	for (Player player : players) {
		if (!player.inGame) {
			availablePlayers.push_back(player);
		}
	}

	if (availablePlayers.size() >= 2) {
		std::random_shuffle(availablePlayers.begin(), availablePlayers.end());
		Player& playerOne = availablePlayers[0];
		Player& playerTwo = availablePlayers[1];
		SnakeGrid newGrid(10, 50, 50, 800, 800);
		newGrid.addSnake(2, 2, 3, DOWN, playerOne);
		newGrid.addSnake(7, 7, 3, UP, playerTwo);
		newGrid.startGame();
		games.push_back(newGrid);
	}
}

int main() {
	std::srand(time(NULL));

	colours.push_back(new float[] {0.1f, 0.1f, 0.1f, 1.0f});
	colours.push_back(new float[] {1.0f, 1.0f, 0.0f, 1.0f});

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

		for (SnakeGrid grid : games) {
			grid.draw();
		}

		glfwSwapBuffers(window);

		glfwPollEvents();

		checkForIncomingConnections();

		checkForGameStart();
	}

}