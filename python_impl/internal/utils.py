from runner import *
from args import *

import time
import os

def clearTerminal():
	os.system('clear')

def printColoredBoard(input_board: str):
	print(input_board.replace("R", "\033[91mR\033[0m").replace("B", "\033[94mB\033[0m"))

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
			
		printColoredBoard(game.showForUser(nice = args.nice_print))
		print()

	for _ in range(args.round_count):
		out = game.performMoveWithExec()

		if not args.silent:
			if args.clear_terminal:
				clearTerminal()

			printColoredBoard(game.showForUser(nice = args.nice_print))
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
