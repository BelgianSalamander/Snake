from snake_client import *
from random import randint

"""
Values Given:

SnakeClient.grid : 2-dimensional which is the same as the server-side grid
SnakeClient.id : Snake's ID. Used in grid. Every snake connected to the same server will have a different id
SnakeClient.x, SnakeClient.y : coordinates of your snake's head

Moves:
  UP    (0)
  RIGHT (1)
  DOWN  (2)
  LEFT  (3)

DIRECTIONS : list with the actual coordinate change associated to each respective move

BLANK (0) : Empty square on the grid
FOOD  (1) : Food on the grid
"""

class MyClient(SnakeClient):

    def on_game_start(self):
        """Called as soon as snake is put in a game
        self.grid will be empty"""

        print("Game Started")

    def on_square_updated(self, x, y, value):
        """Called after a square is updated"""

    def set_ID(self):
        """Called before connecting
        Used to set name and color"""

        self.name = "Dummy"
        self.color = tuple(randint(0,255) for i in range(3))
    
    def select_move(self):
        """This function MUST return a move (or the corresponding number between 0 and 3) in under a second (or you will be disconnected)
        Is called when server requests a move"""

        return RIGHT

client = MyClient()
client.run()
    
