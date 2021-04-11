#include <iostream>
#include <snake.h>
#include <tuple>
#include <random>
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

std::tuple<int, int> directions[4] = {
	std::make_tuple(0,1),   //UP
	std::make_tuple(1,0),   //RIGHT
	std::make_tuple(0,-1),  //DOWN
	std::make_tuple(-1,0)   //LEFT
};


SnakeGrid::SnakeGrid(int gridSize, int x, int y, int width, int height) {
	size = gridSize;
	startX = x;
	startY = y;
	displayWidth = width;
	displayHeight = height;

	squareWidth = displayWidth / size;
	squareHeight = displayHeight / size;

	grid = new unsigned int* [size];

	for (int y = 0; y < size; y++) {
		grid[y] = new unsigned int[size] {0};
	}	
}

void SnakeGrid::draw() {
	//Draw background
	drawRectangle(startX, startY, displayWidth, displayHeight, colours[0]);

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			if (grid[x][y]) {
				float* colour = colours[grid[x][y]];
				drawRectangle(startX + x * squareWidth, startY + y * squareHeight, squareWidth, squareHeight, colour);
			}
		}
	}
}

void SnakeGrid::addSnake(int x, int y, int length, int direction, Player& player) {
	Snake snake;
	snake.headX = x;
	snake.headY = y;
	snake.player = player;

	std::tuple<int, int> directionTuple = directions[direction];
	int dirX = std::get<0>(directionTuple);
	int dirY = std::get<1>(directionTuple);

	x += length * dirX;
	y += length * dirY;

	for (int i = 0; i < length; i++) {
		x -= dirX;
		y -= dirY;
		snake.snake.push(Point{x, y});
		setSquare(x, y, player.colourId);
	}

	snakes.push_back(snake);
}

void SnakeGrid::setSquare(int x, int y, unsigned int val) {
	grid[x][y] = val;
	tilesToBroadcast.insert(Point{ x, y });
}

void SnakeGrid::addFood() {
	std::vector<Point> freePoints;

	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			if (!grid[x][y]) {
				freePoints.push_back(Point{ x, y });
			}
		}
	}

	Point chosenPoint = freePoints[rand() % freePoints.size()];
	setSquare(chosenPoint.x, chosenPoint.y, 1);
}

void SnakeGrid::moveSnake(int snakeIndex, int direction) {
	std::tuple<int, int> directionTuple = directions[direction];
	Snake& snake = snakes[snakeIndex];

	if (!snake.inGame) {
		return;
	}

	int newX = snake.headX + std::get<0>(directionTuple);
	int newY = snake.headY + std::get<1>(directionTuple);

	snake.headX = newX;
	snake.headY = newY;

	if (newX < 0 || newX >= size || newY < 0 || newY >= size) {
		deleteSnake(snakeIndex);
		return;
	}

	unsigned int gridVal = grid[newX][newY];
	if (gridVal <= 1) {
		snake.snake.push(Point{ newX, newY });
		setSquare(newX, newY, snake.player.colourId);
		if (!gridVal) {
			Point clearPoint = snake.snake.front();
			snake.snake.pop();
			setSquare(clearPoint.x, clearPoint.y, 0);
		}else {
			addFood();
		}
	}
	else {
		deleteSnake(snakeIndex);
	}
}

void SnakeGrid::deleteSnake(int index) {
	Snake& snake = snakes[index];

	while (!snake.snake.empty()) {
		Point point = snake.snake.front();
		setSquare(point.x, point.y, 0);
		snake.snake.pop();
	}

	snake.inGame = false;
}

void SnakeGrid::startGame() {
	char startInstruction[5];
	startInstruction[0] = 0x01;
	memcpy(startInstruction + 1, &size, 4);

	for (Snake snake : snakes) {
		send(snake.player.socket, startInstruction, 5, 0);
	}

	for (int i = 0; i < 3; i++) {
		addFood();
	}

	broadcastChanges();
	sendHeads();
	queryMoves();
}

void SnakeGrid::broadcastChanges() {
	int amountOfUpdates = tilesToBroadcast.size();
	char* updateInstructions = (char*) _malloca(5 + 12 * amountOfUpdates);
	updateInstructions[0] = 0x02;
	memcpy(updateInstructions + 1, &amountOfUpdates, 4);
	int address = 5;
	for (Point point : tilesToBroadcast) {
		memcpy(updateInstructions + address, &point.x, 4);
		memcpy(updateInstructions + address + 4, &point.y, 4);
		memcpy(updateInstructions + address + 8, &grid[point.x][point.y], 4);
		address += 12;
	}

	for (Snake snake : snakes) {
		send(snake.player.socket, updateInstructions, 5 + 12 * amountOfUpdates, 0);
	}
}

void SnakeGrid::sendHeads() {
	char headInstruction[9];
	headInstruction[0] = 0x03;
	for (Snake snake : snakes) {
		memcpy(headInstruction + 1, &snake.headX, 4);
		memcpy(headInstruction + 5, &snake.headY, 4);
		send(snake.player.socket, headInstruction, 9, 0);
	}
}

void SnakeGrid::queryMoves() {
	char query[1];
	query[0] = 0x04;
	for (Snake snake : snakes) {
		send(snake.player.socket, query, 1, 0);
	}
}