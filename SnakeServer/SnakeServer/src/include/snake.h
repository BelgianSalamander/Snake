#pragma once
#include <vector>
#include <queue>
#include <string>
#include <set>

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
	int nameLength;
	bool inGame = false;
};


struct Snake {
	int headX, headY;
	std::queue<Point> snake;
	bool inGame = true;
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
		std::vector<Snake> snakes;
		std::set<Point> tilesToBroadcast;
	public:
		SnakeGrid(int gridSize, int x, int y, int width, int height);
		void draw();
		void addSnake(int x, int y, int length, int direction, Player& player);
		void addFood();
		void moveSnake(int snake, int direction);
		void startGame();
		Snake getSnake(int n) { return snakes[n]; };
};
