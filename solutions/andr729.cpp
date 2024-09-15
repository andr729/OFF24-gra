#include <iostream>
#include <cassert>
#include <vector>
#include <array>

using u64 = uint64_t;
using i64 = int64_t;

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

struct Vec {
	i64 x = 0;
	i64 y = 0;
};

enum class Direction {
	UP    = 0,
	DOWN  = 1,
	LEFT  = 2,
	RIGHT = 3,
};

constexpr std::array<Direction, 4> DirectionArr = {
	Direction::UP,
	Direction::DOWN,
	Direction::LEFT,
	Direction::RIGHT
};

Vec dirToVec(Direction dir) {
	switch (dir) {
		case Direction::UP:
			return {0, -1};
		case Direction::DOWN:
			return {0, 1};
		case Direction::LEFT:
			return {-1, 0};
		case Direction::RIGHT:
			return {1, 0};
	}
}

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

	T& get(Direction dir) {
		switch (dir) {
			case Direction::UP:
				return up();
			case Direction::DOWN:
				return down();
			case Direction::LEFT:
				return left();
			case Direction::RIGHT:
				return right();
		}
	}
};


void skipNewLine() {
	char c;
	std::cin >> std::noskipws >> c;
	assert(c == '\n');
}


// cause std...
struct Bool {
	bool b;
};

struct BoolLayer {
private:
	std::vector<Bool> bullets;
	bool& getRef(u64 x, u64 y) {
		return this->bullets.at(x * m + y).b;
	}
public:
	BoolLayer(): bullets(n * m, {false}) {}

	bool get(Vec pos) {
		return getRef(pos.x, pos.y);
	}

	void set(Vec pos, bool val) {
		getRef(pos.x, pos.y) = val;
	}
};

struct BulletLayer {
private:
	QuadDirStorage<BoolLayer> bullets;

public:
	// BulletLayer() {}
	// BulletLayer(const BulletLayer& other) = default;
	// BulletLayer(BulletLayer&& other) = default;

	// BulletLayer& operator=(BulletLayer&& other) = default;

	void addBullet(Vec pos, Direction dir) {
		bullets.get(dir).set(pos, true);
	}

	bool isBulletAt(Vec pos) {
		bool res = false;
		for (auto dir: DirectionArr) {
			res |= bullets.get(dir).get(pos);
		}
		return res;
	}

	void moveBulletsWithWalls(Vec direction, const BoolLayer& walls ) {
		// ideally we want to do it inplace for performance..
		// for now we ignore it
		BulletLayer new_bullets;

		for (auto dir: DirectionArr) {

		}


		// @TODO: make sure it actually moves:
		*this = std::move(new_bullets);
	}
};

struct GameState {

};

namespace alpha_beta {
	// TODO
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

	QuadDirStorage<std::vector<Vec>> bullets;
	
	std::vector<Vec> walls;

	Vec red_player;
	Vec blue_player;

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


