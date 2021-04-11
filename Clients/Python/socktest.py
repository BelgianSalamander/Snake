import socket
from random import shuffle, randint, choice

UP = 0
RIGHT = 1
DOWN = 2
LEFT = 3

DIRECTIONS = [(0, 1), (1, 0), (0, -1), (-1, 0)]

BLANK = 0
FOOD = 1

PORT = 42069

DIR_NAMES = {
    UP : 'up',
    LEFT : 'left',
    DOWN : 'down',
    RIGHT : 'right'
}

class SnakeClient:
    def __init__(self, ip = '127.0.0.1'):
        self.ip = ip
        self.port = PORT
        self.id = 0
        self.x, self.y = 0, 0

    def connect(self):
        self.set_ID()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.ip, self.port))

        self.sock.sendall(bytes(self.color))
        self.sock.sendall(bytes(self.name, 'utf-8'))

        print("Waiting for Player ID")
        self.id = int.from_bytes(self.sock.recv(4), byteorder = 'little')
        print("Received Player ID. Connection Succesful!")
        print(self.id)

    def set_ID(self):
        self.name = "Jules"
        self.color = (255, 0, 255)

    def process_instrucion(self):
        instruction = int.from_bytes(self.sock.recv(1), 'little')
        if(instruction == 0):
            print("Disconnected")
            exit()

        if instruction == 0x01:
            self.size = int.from_bytes(self.sock.recv(4), 'little')
            self.grid = [[BLANK] * self.size for i in range(size)]
            print("Game Started!")

        if instruction == 0x02:
            amount = int.from_bytes(self.sock.recv(4), 'little')
            print('Received', amount, 'updates!')
            full = self.sock.recv(12 * amount)
            print(full)
            for i in range(amount):
                x_index = int.from_bytes(full[i * 12 : i * 12 + 4], 'little')
                y_index = int.from_bytes(full[i * 12 + 4 : i * 12 + 8], 'little')
                value = int.from_bytes(full[i * 12 + 8 : i * 12 + 12], 'little')
                self.grid[x_index][y_index] = value
                print(x_index, y_index, value)
            print('\n' . join(' '.join(str(val) for val in row) for row in self.grid))

        if instruction == 0x03:
            head = self.sock.recv(8)
            self.x = int.from_bytes(head[:4], 'little')
            self.y = int.from_bytes(head[4:], 'little')

        if instruction == 0x04:
            move = self.select_move()
            print(f"Moving {DIR_NAMES[move]}!")
            self.sock.send(bytes(self.select_move()))

    def is_safe(move):
        new_x = self.x + DIRECTIONS[MOVE][0]
        new_y = self.y + DIRECTIONS[MOVE][0]

        if(not(0 <= new_x < self.size and 0 <= new_y < self.size)):
            return False
        return not self.grid[new_x][new_y]
    
    def select_move(self):
        available = [move for move in range(3) if is_safe(move)]
        if(len(available) == 0):
            return randint(0,3)
        else:
            return choice(available)

if __name__ == "__main__":
    client = SnakeClient()
    client.connect()
    while True:
        client.process_instrucion()