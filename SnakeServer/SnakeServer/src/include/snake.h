#pragma once
#include <vector>
#include <queue>
#include <string>
#include <set>
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

#define PORT 42069
#define IP "127.0.0.1"

struct Point {
	int x, y;

	bool operator<(const Point& rhs) const noexcept {
		if (x == rhs.x) {
			return y < rhs.y;
		}
		else {
			return x < rhs.x;
		}
	}
};

struct Player {
	unsigned int socket, colourId;
	int score = 1000;
	std::string name;
	bool inGame = false;
	bool connected = true;
};


struct Snake {
	int headX, headY;
	std::queue<Point> snake;
	bool inGame = true;
	bool receivedMove = false;
	int nextMove;
	Player player;
};

extern std::vector<float*> colours;

extern void initAbstractions();
extern void drawRectangle(float startX, float startY, float width, float height, float* color);

class SnakeGrid {
	private:
		unsigned int** grid;
		int startX, startY, displayWidth, displayHeight, size;
		float squareWidth, squareHeight;
		void setSquare(int x, int y, unsigned int val);
		void deleteSnake(int index);
		void queryMoves();
		void broadcastChanges();
		void sendHeads();
		void moveSnakes();
		std::set<Point> tilesToBroadcast;
		fd_set snakeFD;
	public:
		SnakeGrid(int gridSize, int x, int y, int width, int height);
		void draw();
		void addSnake(int x, int y, int length, int direction, Player& player);
		void addFood();
		void moveSnake(int snake, int direction);
		void startGame();
		void checkForInboundMoves();
		void printGrid();
		boolean ended = false;
		Snake getSnake(int n) { return snakes[n]; };
		std::vector<Snake> snakes;
};
