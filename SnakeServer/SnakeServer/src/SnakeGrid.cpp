#include <iostream>
#include <snake.h>
#include <tuple>
#include <random>
#include <chrono>

using namespace std::chrono;

inline static time_t getTime() {
	return duration_cast<milliseconds>(
		system_clock::now().time_since_epoch()).count();
}

timeval snakeTimeout;
std::string dirs[] = {
	"UP",
	"RIGHT",
	"DOWN",
	"LEFT"
};

void initGrids() {
	snakeTimeout.tv_sec = 0;
	snakeTimeout.tv_usec = 10;
}

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

	for (int i = 0; i < snakes.size(); i++) {
		std::string name = snakes[i].player->name + (snakes[i].inGame ? "" : " (DEAD)");
		render_text(name.c_str(), startX + displayWidth + 50, startY + displayHeight - 50 * (i + 1), colours[snakes[i].player->colourId]);
	}
}

void SnakeGrid::addSnake(int x, int y, int length, int direction, Player* player) {
	Snake snake;
	snake.headX = x;
	snake.headY = y;
	snake.player = player;

	player->inGame = true;

	std::tuple<int, int> directionTuple = directions[direction];
	int dirX = std::get<0>(directionTuple);
	int dirY = std::get<1>(directionTuple);

	x += length * dirX;
	y += length * dirY;

	for (int i = 0; i < length; i++) {
		x -= dirX;
		y -= dirY;
		snake.snake.push(Point{x, y});
		setSquare(x, y, player->colourId);
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
	if (!(direction < 4 && direction >= 0)) {
		snakes[snakeIndex].player->connected = false;
		deleteSnake(snakeIndex);
		return;
	}
	std::tuple<int, int> directionTuple = directions[direction];
	Snake& snake = snakes[snakeIndex];

	if (!snake.inGame) {
		return;
	}

	std::cout << snake.player->name << " moved " << dirs[direction] << std::endl;

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
		setSquare(newX, newY, snake.player->colourId);
		if (!gridVal) {
			Point clearPoint = snake.snake.front();
			snake.snake.pop();
			setSquare(clearPoint.x, clearPoint.y, 0);
		}else {
			addFood();
			snake.player->score += 1;
		}
	}
	else {
		deleteSnake(snakeIndex);
	}
}

void SnakeGrid::deleteSnake(int index) {
	Snake& snake = snakes[index];
	std::cout << snake.player->name << " has been deleted" << std::endl;

	while (!snake.snake.empty()) {
		Point point = snake.snake.front();
		setSquare(point.x, point.y, 0);
		snake.snake.pop();
	}

	snake.inGame = false;
	snake.player->score -= 10;
	for (Snake otherSnake : snakes) {
		if (otherSnake.inGame) {
			otherSnake.player->score += 10;
		}
	}
	char lost[1] = { 0x05 };
	send(snake.player->socket, lost, 1, 0);
}

void SnakeGrid::startGame() {
	char startInstruction[5];
	startInstruction[0] = 0x01;
	memcpy(startInstruction + 1, &size, 4);

	for (Snake snake : snakes) {
		send(snake.player->socket, startInstruction, 5, 0);
	}

	for (int i = 0; i < FOOD_PER_GAME; i++) {
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
		if (snake.inGame) {
			send(snake.player->socket, updateInstructions, 5 + 12 * amountOfUpdates, 0);
		}
	}
	tilesToBroadcast.clear();
	sendHeads();
	//printGrid();
}

void SnakeGrid::sendHeads() {
	char headInstruction[9];
	headInstruction[0] = 0x03;
	for (Snake snake : snakes) {
		if (!snake.inGame) return;
		memcpy(headInstruction + 1, &snake.headX, 4);
		memcpy(headInstruction + 5, &snake.headY, 4);
		send(snake.player->socket, headInstruction, 9, 0);
	}
}

void SnakeGrid::queryMoves() {
	timeSinceLastMove = getTime();
	broadcastChanges(); //Update the clients grid
	char query[1];
	query[0] = 0x04;
	FD_ZERO(&snakeFD);
	for (Snake snake : snakes) {
		if (snake.inGame) {
			snake.receivedMove = false;

			send(snake.player->socket, query, 1, 0);
			FD_SET(snake.player->socket, &snakeFD);
		}
	}
}

void SnakeGrid::checkForInboundMoves() {
	fd_set copy = snakeFD;
	int count = select(0, &copy, nullptr, nullptr, &snakeTimeout);
	for (int i = 0; i < count; i++) {
		SOCKET sock = copy.fd_array[i];
		FD_CLR(sock, &snakeFD);
		char receivedMove[1];

		int inBytes = recv(sock, receivedMove, 1, 0);
		if (inBytes != 1) {
			for (int i = 0; i < snakes.size(); i++) {
				if (snakes[i].player->socket == sock) {
					snakes[i].player->connected = false;
					deleteSnake(i);
					break;
				}
			}
		}
		else {
			for (int i = 0; i < snakes.size(); i++) {
				if (snakes[i].player->socket == sock) {
					snakes[i].receivedMove = true;
					snakes[i].nextMove = (int) receivedMove[0];
					break;
				}
			}
		}
	}

	time_t now = getTime();

	if ((!snakeFD.fd_count) && ((getTime() - timeSinceLastMove) > 100)) {
		moveSnakes();
	}
	else if ((getTime() - timeSinceLastMove) > 1000) {
		for (int i = 0; i < snakes.size(); i++) {
			if (!snakes[i].receivedMove) {
				snakes[i].player->connected = false;
				deleteSnake(i);
			}
		}
		std::cout << "Snakes timed out" << std::endl;
		moveSnakes();
	}
}

void SnakeGrid::moveSnakes() {
	for (int i = 0; i < snakes.size(); i++) {
		moveSnake(i, snakes[i].nextMove);
	}
	int amountOfActiveSnakes = 0;
	for (Snake snake : snakes) {
		amountOfActiveSnakes += snake.inGame;
	}
	if (amountOfActiveSnakes <= 1) {
		ended = true;
	}
	else {
		queryMoves();
	}
}

void SnakeGrid::printGrid() {
	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			std::cout << (unsigned int)grid[x][y] << " ";
		}
		std::cout << "\n";
	}
	std::cout.flush();
}