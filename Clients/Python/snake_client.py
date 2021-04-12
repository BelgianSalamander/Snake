import socket
import time
from random import shuffle, randint, choice

UP = 0
RIGHT = 1
DOWN = 2
LEFT = 3

DIRECTIONS = [(0, 1), (1, 0), (0, -1), (-1, 0)]

BLANK = 0
FOOD = 1

PORT = 42069

class SnakeClient:
    def __init__(self, ip = '127.0.0.1'):
        self.ip = ip
        self.port = PORT
        self.id = 0
        self.x, self.y = 0, 0
        self.name = "Unnamed Client"
        self.color = (255, 0, 255)

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
        raise NotImplementedError

    def on_game_start(self):
        pass

    def on_square_updated(self, x, y, value):
        pass

    def process_instrucion(self):
        instruction = int.from_bytes(self.sock.recv(1), 'little')
        if(instruction == 0):
            print("Disconnected")
            exit()

        if instruction == 0x01:
            self.size = int.from_bytes(self.sock.recv(4), 'little')
            self.grid = [[BLANK] * self.size for i in range(self.size)]
            self.on_game_start()

        if instruction == 0x02:
            amount = int.from_bytes(self.sock.recv(4), 'little')
            full = self.sock.recv(12 * amount)
            for i in range(amount):
                x_index = int.from_bytes(full[i * 12 : i * 12 + 4], 'little')
                y_index = int.from_bytes(full[i * 12 + 4 : i * 12 + 8], 'little')
                value = int.from_bytes(full[i * 12 + 8 : i * 12 + 12], 'little')
                self.grid[x_index][y_index] = value
                self.on_square_updated(x_index, y_index, value)

        if instruction == 0x03:
            head = self.sock.recv(8)
            self.x = int.from_bytes(head[:4], 'little')
            self.y = int.from_bytes(head[4:], 'little')

        if instruction == 0x04:
            move = self.select_move()
            self.sock.send(move.to_bytes(1, 'little'))

        if instruction == 0x05:
            print("Game finished")
    
    def select_move(self):
        raise NotImplementedError

    def run(self):
        self.connect()
        while True:
            self.process_instrucion()