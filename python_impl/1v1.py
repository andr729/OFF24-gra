# Script for running a simple 1v1

from internal.utils import runWithArgs
from internal.args import getArgParser

def main():
	args = getArgParser().parse_args()
	runWithArgs(args)

if __name__ == "__main__":
	main()
