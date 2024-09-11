import argparse

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
