# Implementation of the game runner for the OFF24 game.

import random
from dataclasses import dataclass
from enum import Enum
from typing import Tuple, List

BULLET_UP    = "^"
BULLET_DOWN  = "v"
BULLET_RIGHT = ">"
BULLET_LEFT  = "<"

class TileType(Enum):
	STANDARD = 0
	WALL     = 1

class PlayersID(Enum):
	RED  = 0
	BLUE = 1

def showPlayerID(player_id: PlayersID) -> str:
	match player_id:
		case PlayersID.RED:
			return "R"
		case PlayersID.BLUE:
			return "B"

@dataclass
class Bullet:
	position: Tuple[int, int]
	direction: Tuple[int, int]

@dataclass
class Player:
	player_type: PlayersID
	position: Tuple[int, int]
	direction: Tuple[int, int]

	def show(self) -> str:
		return showPlayerID(self.player_type)

class Tile:
	def __init__(self):
		self.type = TileType.STANDARD

	def	showStr(self) -> str:
		if (self.type == TileType.STANDARD):
			return " "
		elif (self.type == TileType.WALL):
			return "#"
		else:
			raise Exception("Invalid TileType")

# @note: when adding new move profile, remember to update runExec function
class MoveProfile(Enum):
	MOVE_UP     = 0
	MOVE_DOWN   = 1
	MOVE_LEFT   = 2
	MOVE_RIGHT  = 3
	SHOOT_UP    = 4
	SHOOT_DOWN  = 5
	SHOOT_LEFT  = 6
	SHOOT_RIGHT = 7
	WAIT        = 8
	SURRENDER   = 9

@dataclass
class Move:
	profiles: List[MoveProfile]

class GameLogic:
	def __init__(self, n: int, m: int, wall_count: int, seed = None):
		self.n = n
		self.m = m

		self.tiles = [[Tile() for _ in range(m)] for _ in range(n)]
		self.bullets = []
		self.players = [Player(PlayersID.RED, (1, 1), (0, 1)), Player(PlayersID.BLUE, (n-2, m-2), (0, 1))]

		if (seed is not None):
			random.seed(seed)
		for _ in range(wall_count // 2):
			x = random.randint(0, n-1)
			y = random.randint(0, m-1)
			self.tiles[x][y].type = TileType.WALL
			self.tiles[n - x - 1][m - y - 1].type = TileType.WALL

		self.tiles[1][1].type = TileType.STANDARD
		self.tiles[n-2][m-2].type = TileType.STANDARD

		# fill border tiles with walls:
		for i in range(n):
			self.tiles[i][0].type   = TileType.WALL
			self.tiles[i][m-1].type = TileType.WALL
		for i in range(m):
			self.tiles[0][i].type   = TileType.WALL
			self.tiles[n-1][i].type = TileType.WALL

	def moveBulletsOneStep(self):
		for bullet in self.bullets:
			x, y = bullet.position
			dx, dy = bullet.direction
			new_x, new_y = x + dx, y + dy

			if (new_x < 0 or new_x >= self.n or new_y < 0 or new_y >= self.m):
				self.bullets.remove(bullet)
			else:
				if self.tiles[new_x][new_y].type == TileType.WALL:
					bullet.direction = (-dx, -dy)
				else:
					bullet.position = (new_x, new_y)

	def applyMove(self, move: Move) -> List[PlayersID]:
		"""Return list of hits"""

		assert len(move.profiles) == len(self.players)
		assert len(self.players) == 2

		output = []

		if move.profiles[0] == MoveProfile.SURRENDER:
			output.append(self.players[0].player_type)
		if move.profiles[1] == MoveProfile.SURRENDER:
			output.append(self.players[1].player_type)
		
		if len(output) > 0:
			return output

		for (player, profile) in zip(self.players, move.profiles):
			player.old_position = player.position
			match profile:
				case MoveProfile.MOVE_UP:
					player.position = (max(player.position[0] - 1, 0), player.position[1])
				case MoveProfile.MOVE_DOWN:
					player.position = (min(player.position[0] + 1, self.n - 1), player.position[1])
				case MoveProfile.MOVE_LEFT:
					player.position = (player.position[0], max(player.position[1] - 1, 0))
				case MoveProfile.MOVE_RIGHT:
					player.position = (player.position[0], min(player.position[1] + 1, self.m - 1))
				case MoveProfile.SHOOT_UP:
					self.bullets.append(Bullet(player.position, (-1, 0)))
				case MoveProfile.SHOOT_DOWN:
					self.bullets.append(Bullet(player.position, (1, 0)))
				case MoveProfile.SHOOT_LEFT:
					self.bullets.append(Bullet(player.position, (0, -1)))
				case MoveProfile.SHOOT_RIGHT:
					self.bullets.append(Bullet(player.position, (0, 1)))
				case MoveProfile.WAIT:
					pass
				case _:
					raise Exception("Invalid MoveProfile")
			if self.tiles[player.position[0]][player.position[1]].type == TileType.WALL:
				player.position = player.old_position

		if self.players[0].position == self.players[1].position:
			self.players[0].position = self.players[0].old_position
			self.players[1].position = self.players[1].old_position

		self.moveBulletsOneStep()

		for player in self.players:
			for bullet in self.bullets:
				if player.position == bullet.position:
					# hit
					output.append(player.player_type)

		# Eliminate duplicates:
		out = list(dict.fromkeys(out))
		
		return output

	def showStr(self, nice: bool) -> str:
		tiles_str = [[[" ", " ", " ", " "] for i in range(self.m)] for _ in range(self.n)]

		for i in range(len(self.tiles)):
			for j in range(len(self.tiles[i])):
				tiles_str[i][j][0] = self.tiles[i][j].showStr()

		for player in self.players:
			x, y = player.position
			tiles_str[x][y][0] = player.show()

		for bullet in self.bullets:
			x, y = bullet.position
			match bullet.direction:
				case (-1, 0):
					tiles_str[x][y][0] = BULLET_UP
				case (1, 0):
					tiles_str[x][y][1] = BULLET_DOWN
				case (0, -1):
					tiles_str[x][y][2] = BULLET_LEFT
				case (0, 1):
					tiles_str[x][y][3] = BULLET_RIGHT

		if not nice:
			return "".join([item for row in tiles_str for tile in (row+["\n"]) for item in tile]) 
		else:
			out = []
			for row in tiles_str:
				for tile in row:
					to_add = " "
					for c in tile[::-1]:
						if c != " ":
							to_add = c
							break
					out.append(to_add)
				out.append("\n")
			return ''.join(out)

	def showForUser(self, nice: bool) -> str:
		output = []
		output.extend([str(self.n), " ", str(self.m)])
		output.append("\n")
		output.append(self.showStr(nice = nice))
		return ''.join(output)