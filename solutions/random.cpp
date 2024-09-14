#include <random>
#include <iostream>

int main() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> random_move(0, 8);

	std::cout << random_move(gen) << "\n";

	return 0;
}
