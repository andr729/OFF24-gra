
# File containing Game class which is responsible for simulating
# the game played by two "exec" players

import subprocess
import shutil
from dataclasses import dataclass

from .logic import *

RUN_IN_ISOLATE = False

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
				# @TODO: this fails when exec_str in not given explicitly as relative path
				out = subprocess.check_output(exec_str, input = self.showForUser(who), text=True, timeout = 0.5)
				return self.parseOutput(out)
			except subprocess.TimeoutExpired:
				print("Warning: Exec hit timeout")
				return MoveProfile.WAIT
			except ValueError:
				print(f"Warning: {exec_str} returned invalid value -- surrendering.")
				return MoveProfile.SURRENDER
			except subprocess.CalledProcessError:
				print(f"Warning: {exec_str} returned non zero code -- surrendering.")
				return MoveProfile.SURRENDER
		
	def performMoveWithExec(self):
		"""Return list of hits or "tie" string if round limit was hit."""

		red_move  = self.runExec(self.red_player_exec, who = PlayersID.RED)
		blue_move = self.runExec(self.blue_player_exec, who = PlayersID.BLUE)

		out = self.game_state.applyMove(Move([red_move, blue_move]))
		
		self.round_number += 1
		
		return out
