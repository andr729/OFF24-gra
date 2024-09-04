# Implementation of the game runner for the OFF24 game.

import random
import time
import subprocess
import argparse
import shutil
import os
import sys
from dataclasses import dataclass
from enum import Enum
from typing import Tuple, List
import types

RUN_IN_ISOLATE = True

def print_colored_board(input_board: str):
	print(input_board.replace("R", "\033[91mR\033[0m").replace("B", "\033[94mB\033[0m"))

def printErr(*args, **kwargs):
	print(*args, **kwargs, file=sys.stderr)

# Following class is a code from: https://stackoverflow.com/questions/12151306/argparse-way-to-include-default-values-in-help
class ExplicitDefaultsHelpFormatter(argparse.ArgumentDefaultsHelpFormatter):
    def _get_help_string(self, action):
        if action.default is None or action.default is False:
            return action.help
        return super()._get_help_string(action)

def getArgParser() -> argparse.ArgumentParser:
	arg_parser = argparse.ArgumentParser(
		prog='off_game',
		description='Game Runner.',
		formatter_class = ExplicitDefaultsHelpFormatter
	)
	arg_parser.add_argument('-r', '--red', type=str, help='Red player executable path', required=True)
	arg_parser.add_argument('-b', '--blue', type=str, help='Blue player executable path', required=True)
	arg_parser.add_argument('-s', '--silent', action='store_true', help='Don\'t print game state after each round.')
	arg_parser.add_argument('-t', '--timeout', type=int, default=500, help='Executables timeout in milliseconds.')
	arg_parser.add_argument('-n', '--height', type=int, default=20, help='Game field height.')
	arg_parser.add_argument('-m', '--width', type=int, default=30, help='Game field width.')
	arg_parser.add_argument('-w', '--wall-count', type=int, default=30, help='Approximated wall count.')
	arg_parser.add_argument('--round-count', type=int, default=100, help='Number of rounds after which there will be tie.')
	arg_parser.add_argument('--seed', type=int, help='Game seed (if not given, then seed will be based of system time).')
	arg_parser.add_argument('--wait', type=int, help='Wait time in millisecond between round.')
	arg_parser.add_argument('--nice-print', action='store_true', help='Print game state in nice way, where each tile is just one char (not four). It might hide some bullets.')
	arg_parser.add_argument('--clear-terminal', action='store_true', help='Clears terminal before prints.')

	return arg_parser

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

class Game:
	def __init__(self, n: int, m: int, wall_count: int, red_player_exec: str, blue_player_exec: str, seed = None):
		self.game_state   = GameLogic(n, m, wall_count, seed)
		self.round_number = 0

		self.red_player_exec  = red_player_exec
		self.blue_player_exec = blue_player_exec

	def showForUser(self, who: PlayersID | None = None, nice: bool = False) -> str:
		output = [self.game_state.showForUser(nice = nice)]
		output.append(str(self.round_number))
		output.append("\n")
		if (who is not None):
			output.append(showPlayerID(who))
			output.append("\n")
		return ''.join(output)
	
	def parseOutput(self, out: str) -> MoveProfile:
		try:
			out_int = int(out)
			if out_int < 0 or out_int > 9:
				print("Warning: Exec returned invalid value -- surrendering")
				return MoveProfile.SURRENDER
			return MoveProfile(out_int)
		except ValueError:
			print("Warning: Exec returned invalid value -- surrendering")
			return MoveProfile.SURRENDER
		except:
			assert False

	
	def runExec(self, exec_str: str, who: PlayersID) -> MoveProfile:
		if RUN_IN_ISOLATE:
			with open("./isolate_running/in/in", "w") as input_file:
				input_file.write(self.showForUser(who))
			with open("./isolate_running/out/user_output", "w") as output_file:
				output_file.write(str(MoveProfile.WAIT.value) + "\n")
		
			shutil.copyfile(exec_str, "./isolate_running/prog/exec")

			# @Fixme: Putting it in the dev null might hide some errors. Beware:
			run_result = subprocess.run(["bash", "./isolate_running/run.sh"], stdout = subprocess.DEVNULL, stderr=subprocess.DEVNULL)
			
			if run_result.returncode != 0:
				print(f"Rule violation by {exec_str}!!!")
				assert False

			out = None
			with open("./isolate_running/out/user_output", "r") as output_file:
				out = output_file.read()

			assert out is not None
			
			return self.parseOutput(out)
		else:
			try:
				out = subprocess.check_output(exec_str, input = self.showForUser(who), text=True, timeout = 0.5)
				return self.parseOutput(out)
			except subprocess.TimeoutExpired:
				print("Warning: Exec hit timeout")
				return MoveProfile.WAIT
			except ValueError:
				print("Warning: Exec returned invalid value -- surrendering")
				return MoveProfile.SURRENDER
		
	def performMoveWithExec(self):
		"""Return list of hits or "tie" string if round limit was hit."""

		red_move  = self.runExec(self.red_player_exec, who = PlayersID.RED)
		blue_move = self.runExec(self.blue_player_exec, who = PlayersID.BLUE)

		out = self.game_state.applyMove(Move([red_move, blue_move]))
		
		self.round_number += 1
		
		return out

def clearTerminal():
	os.system('clear')

def runWithArgs(args):

	game = Game(
		args.height,
		args.width,
		args.wall_count,
		args.red,
		args.blue,
		args.seed
	)

	if not args.silent:
		if args.clear_terminal:
			clearTerminal()
			
		print_colored_board(game.showForUser(nice = args.nice_print))
		print()

	for _ in range(args.round_count):
		out = game.performMoveWithExec()

		if not args.silent:
			if args.clear_terminal:
				clearTerminal()

			print_colored_board(game.showForUser(nice = args.nice_print))
			print()

		if out == "tie":
			print("Tie! (1)")
			return "TIE"

		if len(out) == 2:
			print("Tie! (2)")
			return "TIE"

		if out != []:
			assert len(out) == 1 
			if out[0] == PlayersID.BLUE:
				print("Red player won!")
				return "RED"
			elif out[0] == PlayersID.RED:
				print("Blue player won!")
				return "BLUE"
			
		
		if args.wait is not None:
			time.sleep(args.wait / 1000)

	print("Tie! (3)")
	return "TIE"

def runWithDefault(red, blue):

	# LOL:
	args = lambda x:x

	args.red = red
	args.blue = blue
	args.round_count = 500
	args.height = 15
	args.width = 20
	args.wall_count = 20
	args.nice_print = False
	args.wait = 10
	args.clear_terminal = False
	args.seed = None
	args.silent = False

	return runWithArgs(args)


def grabFiles():
	files = []
	for file in os.listdir("user_submits"):
		if file.endswith(".cpp"):
			files.append("user_submits/" + file)
	return files

def runBash(command):
	print(f"Running: {command}:")
	proc = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
	(out, err) = proc.communicate()
	return out, err

@dataclass
class User():
	name: str
	exec: str
	score: int

def getUsers(solutions, compile = True):
	users = []
	for solution in solutions:
		name = solution[:-4]
		output = f"{name}.exe"
		users.append(User(name, output, 0))
		
		if compile:
			compiler_out = runBash(f"g++ -O3 -static -std=c++20 {solution} -o {output}")
			print(compiler_out)
		
	return users


def main():

	solutions = grabFiles()
	users = getUsers(solutions, compile=False)

	# return

	for i in range(len(users)):
		print(i, " :=", users[i])

	# return 

	runWithDefault(users[2].exec, users[4].exec)

	return


	runWithDefault()

	return 0

	round_count = 3

	for i in range(len(users)):
		for j in range(i+1, len(users)):
			score_b = 0
			score_r = 0
			
			os.system("clear")
			print("Now playing....!", flush=True)
			time.sleep(1)
			print(users[i].name, end = "", flush=True)
			time.sleep(2)
			print("    VS    ", end="", flush=True)
			time.sleep(3)
			print(users[j].name, end="", flush=True)
			time.sleep(4)
			print("\nFIGHT!!!!", flush=True)
			time.sleep(2)
			
			for round in range(round_count):
				os.system("clear")
				print(f"Round {round}!", flush=True)
				time.sleep(1)

				try:
					out = runWithDefault(users[i].exec, users[j].exec)
				except:
					print("Ow no...")
					exit(1)
				
				if out == "TIE":
					print("TIE!!!")
					score_b += 0.5
					score_r += 0.5
				if out == "RED":
					print("RED WON!!!")
					score_r += 1
				if out == "BLUE":
					print("BLUE WON!!!")
					score_b += 1
				
				time.sleep(1)

				print(f"Result of the round: ")
				print(f"   Blue: {score_b}")
				print(f"   Red:  {score_r}")

				time.sleep(5)

			# red i
			# blue j

			if score_b > score_r:
				print("")
				print("Round result: Blue won!!!")
				users[j].score += 1
			elif score_r > score_b:
				print("")
				print("Round result: Red won!!!")
				users[i].score += 1
			else:
				print("")
				print("Round result: TIE!!!")
				users[i].score += 0.5
				users[j].score += 0.5

			time.sleep(1)

			print("Scoreboard: ")
			for u in users:
				print(f"{u.name:30}{u.score:>30}")

			time.sleep(6)
				
	
	# arg_parser = getArgParser()
	# args = arg_parser.parse_args()

	# runWithArgs(args)



if __name__ == "__main__":
	main()
