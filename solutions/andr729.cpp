#include <iostream>
#include <cassert>
#include <vector>

using u64 = uint64_t;

// constexpr u64 N = 15;
// constexpr u64 M = 20;

/*******************/
// Global parameters:

u64 n;
u64 m;

constexpr u64 MAX_ROUND = 400;
u64 round_number;

constinit bool global_params_set = false;

/*******************/


struct Pos {
	u64 x = 0;
	u64 y = 0;
};

template <typename T>
struct QuadDirStorage {
	T vals[4];

	T& up() {
		return vals[0];
	}
	T& down() {
		return vals[1];
	}
	T& left() {
		return vals[2];
	}
	T& right() {
		return vals[3];
	}
};

void skipNewLine() {
	char c;
	std::cin >> std::noskipws >> c;
	assert(c == '\n');
}


int main() {
	std::ios_base::sync_with_stdio(false);

	{
		std::cin >> n >> m;
		// assert(n == N);
		// assert(m == M);
		skipNewLine();
		global_params_set = true;
	}

	QuadDirStorage<std::vector<Pos>> bullets;
	
	std::vector<Pos> walls;

	Pos red_player;
	Pos blue_player;

	for (u64 i = 0; i < n; i++) {
		for (u64 j = 0; j < m; j++) {
			for (u64 dummy = 0; dummy < 4; dummy++) {
				char c;
				std::cin >> std::noskipws >> c;
				switch (c) {
					case ' ':
						break;
					case '#':
						walls.push_back({i, j});
						break;
					case 'R':
						red_player = {i, j};
						break;
					case 'B':
						blue_player = {i, j};
						break;
					case '>':
						bullets.right().push_back({i, j});
						break;
					case '<':
						bullets.left().push_back({i, j});
						break;
					case '^':
						bullets.up().push_back({i, j});
						break;
					case 'v':
						bullets.down().push_back({i, j});
						break;
					default:
						throw std::logic_error("Invalid character");
				}
		

			}
		}
		skipNewLine();
	}

	std::cin >> round_number;

	char who_are_we;
	std::cin >> who_are_we;





}


