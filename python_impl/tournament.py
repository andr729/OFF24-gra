# This script might be broken for now. It will be fixed in the future.

from internal.utils import runWithArgs

import os
import time
import subprocess
from dataclasses import dataclass

def runWithTournamentDefaults(red, blue):
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
					out = runWithTournamentDefaults(users[i].exec, users[j].exec)
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
				


if __name__ == "__main__":
	main()
