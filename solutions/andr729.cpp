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

enum class Move {
	GO_UP       = 0,
	GO_DOWN     = 1,
	GO_LEFT     = 2,
	GO_RIGHT    = 3,
	SHOOT_UP    = 4,
	SHOOT_DOWN  = 5,
	SHOOT_LEFT  = 6,
	SHOOT_RIGHT = 7,
	WAIT        = 8,
};

enum class Player {
	HERO  = 0,
	ENEMY = 1,
};

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

constexpr Vec dirToVec(Direction dir) {
	// we have our dimension switched
	// so x is i, y is j
	// for this reason here
	// Also x is inverted so we can print it from 0 to n-1 (not reversed)
	switch (dir) {
		case Direction::UP:
			return {-1, 0};
		case Direction::DOWN:
			return {1, 0};
		case Direction::LEFT:
			return {0, -1};
		case Direction::RIGHT:
			return {0, 1};
	}
	assert(false);
}

constexpr Direction flip(Direction dir) {
	switch (dir) {
		case Direction::UP:
			return Direction::DOWN;
		case Direction::DOWN:
			return Direction::UP;
		case Direction::LEFT:
			return Direction::RIGHT;
		case Direction::RIGHT:
			return Direction::LEFT;
	}
	assert(false);
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
		assert(false);
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
	
	bool getRef(u64 x, u64 y) const {
		return this->bullets.at(x * m + y).b;
	}

public:
	BoolLayer(): bullets(n * m, {false}) {}

	bool get(Vec pos) const {
		return getRef(pos.x, pos.y);
	}

	void set(Vec pos, bool val) {
		getRef(pos.x, pos.y) = val;
	}

	static BoolLayer fromVec(const std::vector<Vec>& vecs) {
		BoolLayer res;
		for (auto vec: vecs) {
			res.set(vec, true);
		}
		return res;
	}
};

struct BulletLayer {
private:
	QuadDirStorage<BoolLayer> bullets;

public:
	BulletLayer() {}
	BulletLayer(const BulletLayer& other) = default;
	BulletLayer(BulletLayer&& other) = default;
	
	BulletLayer(QuadDirStorage<BoolLayer> values): bullets(values) {};

	BulletLayer& operator=(BulletLayer&& other) = default;

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

	void moveBulletsWithWalls(const BoolLayer& walls ) {
		// ideally we want to do it inplace for performance..
		// for now we ignore it
		BulletLayer new_bullets;

		for (auto dir: DirectionArr) {
			for (u64 i = 0; i < n; i++) {
				for (u64 j = 0; j < m; j++) {
					Vec pos = {i64(i), i64(j)};
					if (bullets.get(dir).get(pos)) {
						Vec new_pos = pos;
						new_pos.x += dirToVec(dir).x;
						new_pos.y += dirToVec(dir).y;

						Direction new_dir = dir;

						// @TODO: for now we don't check for out of bounds here.
						// it should never happen for valid inputs.

						if (walls.get(new_pos)) {
							new_dir = flip(dir);
							new_pos = pos;
						}

						new_bullets.addBullet(new_pos, new_dir);
					}
				}
			}
		}


		// @TODO: make sure it actually moves:
		*this = std::move(new_bullets);
	}

	[[gnu::cold]]
	void debugPrint() {
		// simple for now
		for (u64 i = 0; i < n; i++) {
			for (u64 j = 0; j < m; j++) {
				Vec pos = {i64(i), i64(j)};
				if (isBulletAt(pos)) {
					std::cout << "*";
				} else {
					std::cout << " ";
				}
			}
			std::cout << "\n";
		}
	}
};

struct GameState {
	BoolLayer walls;
	BulletLayer bullets;
	Vec hero_pos;
	Vec enemy_pos;
	
	void moveBullets() {
		bullets.moveBulletsWithWalls(walls);
	}

	void debugPrint() {
		char out[n][m];
		for (u64 i = 0; i < n; i++) {
			for (u64 j = 0; j < m; j++) {
				out[i][j] = ' ';
			}
		}

		out[hero_pos.x][hero_pos.y] = 'U';
		out[enemy_pos.x][enemy_pos.y] = 'E';

		for (u64 i = 0; i < n; i++) {
			for (u64 j = 0; j < m; j++) {
				Vec pos = {i64(i), i64(j)};
				if (walls.get(pos)) {
					out[i][j] = '#';
				} else if (bullets.isBulletAt(pos)) {
					out[i][j] = '*';
				}
			}
		}

		for (u64 i = 0; i < n; i++) {
			for (u64 j = 0; j < m; j++) {
				std::cout << out[i][j];
			}
			std::cout << "\n";
		}
	}
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

	for (i64 i = 0; i < i64(n); i++) {
		for (i64 j = 0; j < i64(m); j++) {
			for (u64 dummy = 0; dummy < 4; dummy++) {
				char c;
				std::cin >> std::noskipws >> c;

				Vec p = {i, j};

				switch (c) {
					case ' ':
						break;
					case '#':
						walls.push_back(p);
						break;
					case 'R':
						red_player = p;
						break;
					case 'B':
						blue_player = p;
						break;
					case '>':
						bullets.right().push_back(p);
						break;
					case '<':
						bullets.left().push_back(p);
						break;
					case '^':
						bullets.up().push_back(p);
						break;
					case 'v':
						bullets.down().push_back(p);
						break;
					default:
						throw std::logic_error("Invalid character");
				}
		

			}
		}
		skipNewLine();
	}

	std::cin >> round_number;
	skipNewLine();
	
	char who_are_we;
	std::cin >> who_are_we;
	assert(who_are_we == 'R' || who_are_we == 'B');

	GameState game_state = {
		.walls = BoolLayer::fromVec(walls),

		.bullets = {{
			BoolLayer::fromVec(bullets.up()), 
			BoolLayer::fromVec(bullets.down()),
			BoolLayer::fromVec(bullets.left()),
			BoolLayer::fromVec(bullets.right())
		}},

		.hero_pos = who_are_we == 'R' ? red_player : blue_player,
		.enemy_pos = who_are_we == 'R' ? blue_player : red_player
	};


	// debug print:
	for (int i = 0; i < 10; i++) {
		game_state.debugPrint();
		game_state.moveBullets();
		std::cout << "\n\n----------------\n\n";
	}

}


