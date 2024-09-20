#include <iostream>
#include <cassert>
#include <vector>
#include <array>
#include <optional>

using u64 = uint64_t;
using i64 = int64_t;

// @TODO: make it bigger when cutoffs are efficient
constexpr u64 AB_DEPTH = 3;

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

constexpr bool implies(bool a, bool b) {
	return (not a) or b;
}

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

constexpr std::array<Move, 9> MOVE_ARRAY = {
	Move::GO_UP,
	Move::GO_DOWN,
	Move::GO_LEFT,
	Move::GO_RIGHT,
	Move::SHOOT_UP,
	Move::SHOOT_DOWN,
	Move::SHOOT_LEFT,
	Move::SHOOT_RIGHT,
	Move::WAIT
};

auto moveToIndex(Move move) {
	return static_cast<std::underlying_type_t<Move>>(move);
}

enum class Player {
	HERO  = 0,
	ENEMY = 1,
};

struct Vec {
	i64 x = 0;
	i64 y = 0;

	constexpr Vec operator+(const Vec& other) const {
		return {x + other.x, y + other.y};
	}

	constexpr Vec operator+=(const Vec& other) {
		x += other.x;
		y += other.y;
		return *this;
	}
};

enum class Direction {
	UP    = 0,
	DOWN  = 1,
	LEFT  = 2,
	RIGHT = 3,
};

constexpr std::array<Direction, 4> DIRECTION_ARRAY = {
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

	QuadDirStorage() = default;

	QuadDirStorage(T up, T down, T left, T right):
		vals{up, down, left, right} {}
	
	QuadDirStorage(const QuadDirStorage& other) = default;
	QuadDirStorage(QuadDirStorage&& other)      = default;

	QuadDirStorage& operator=(const QuadDirStorage& other) = default;
	QuadDirStorage& operator=(QuadDirStorage&& other)      = default;

	// eh... C++ boilerplate

	T& up() {
		return vals[0];
	}
	const T& up() const {
		return vals[0];
	}
	
	T& down() {
		return vals[1];
	}
	const T& down() const {
		return vals[1];
	}

	T& left() {
		return vals[2];
	}
	const T& left() const {
		return vals[2];
	}

	T& right() {
		return vals[3];
	}
	const T& right() const {
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

	const T& get(Direction dir) const{
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
	
	BoolLayer(const BoolLayer& other) = default;
	BoolLayer(BoolLayer&& other)      = default;

	BoolLayer& operator=(const BoolLayer& other) = default;
	BoolLayer& operator=(BoolLayer&& other)      = default;

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
	BulletLayer() = default;
	
	BulletLayer(QuadDirStorage<BoolLayer> bullets): bullets(std::move(bullets)) {}

	BulletLayer(const BulletLayer& other) = default;
	BulletLayer(BulletLayer&& other)      = default;
	
	BulletLayer& operator=(BulletLayer&& other) = default;

	void addBullet(Vec pos, Direction dir) {
		bullets.get(dir).set(pos, true);
	}

	bool isBulletAt(Vec pos) const {
		bool res = false;
		for (auto dir: DIRECTION_ARRAY) {
			res |= bullets.get(dir).get(pos);
		}
		return res;
	}

	void moveBulletsWithWalls(const BoolLayer& walls ) {
		// ideally we want to do it inplace for performance..
		// for now we ignore it
		BulletLayer new_bullets;

		for (auto dir: DIRECTION_ARRAY) {
			for (u64 i = 0; i < n; i++) {
				for (u64 j = 0; j < m; j++) {
					Vec pos = {i64(i), i64(j)};
					if (bullets.get(dir).get(pos)) {
						Vec new_pos = pos;
						new_pos += dirToVec(dir);

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

struct PositionEvaluation {
private:
	bool hero_hit;
	bool enemy_hit;

	// Fow nowe want here:
	// Conditional survival:
	// * only_hero_survival (round count + ghost count on max) 
	// * only_enemy_survival (round count + ghost count on max)
	//
	// Unconditional survival:
	// * hero_survival_with_ghost_enemy
	// * enemy_survival_with_ghost_hero
	//
	// Future:
	// * ghost don't do illogical moves
	// 

public:

	// TODO: delete
	constexpr PositionEvaluation(): hero_hit(false), enemy_hit(false) {}


	constexpr static PositionEvaluation losing() {
		// @TODO
		return PositionEvaluation();
	}

	constexpr static PositionEvaluation wining() {
		// @TODO
		return PositionEvaluation();
	}

	bool operator<(const PositionEvaluation& other) const {
		return false; // for now every instance is equal
	}

	bool operator>(const PositionEvaluation& other) const {
		return other < *this;
	}
};

struct GameState {
	BoolLayer walls;
	BulletLayer bullets;

	// hero -- 0, enemy -- 1
	Vec player_positions[2];
	
	void moveBullets() {
		bullets.moveBulletsWithWalls(walls);
	}

	[[gnu::cold]]
	void debugPrint() {
		char out[n][m];
		for (u64 i = 0; i < n; i++) {
			for (u64 j = 0; j < m; j++) {
				out[i][j] = ' ';
			}
		}

		out[player_positions[0].x][player_positions[0].y] = 'U';
		out[player_positions[1].x][player_positions[1].y] = 'E';

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

	void applyMove(Move hero_move, Move enemy_move) {
		// @TODO
	}

	bool isMoveSensible(Move move, Player player) const {
		// @TODO: use it for optimizations in the future
		return true;
	}

	bool isTerminal() const {
		return bullets.isBulletAt(player_positions[0]) or bullets.isBulletAt(player_positions[1]);
	}

	PositionEvaluation evaluate() const {
		// @TODO...
		// this will be the bigger part of the code
		return {};
	}
};

struct ABGameState {
	GameState state;
	
	// For now we assume that the game is full-information game, 
	// and that we have to commit our move first.
	// This approach reduces possibility of random bad moves,
	// but is not optimal.
	// We might try to change it in the future.  
	std::optional<Move> hero_move_commit;
};


namespace alpha_beta {
	// PSUEDO CODE from https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning:
	// function alphabeta(node, depth, α, β, maximizingPlayer) is
    // if depth == 0 or node is terminal then
    //     return the heuristic value of node
    // if maximizingPlayer then
    //     value := −∞
    //     for each child of node do
    //         value := max(value, alphabeta(child, depth − 1, α, β, FALSE))
    //         if value > β then
    //             break (* β cutoff *)
    //         α := max(α, value)
    //     return value
    // else
    //     value := +∞
    //     for each child of node do
    //         value := min(value, alphabeta(child, depth − 1, α, β, TRUE))
    //         if value < α then
    //             break (* α cutoff *)
    //         β := min(β, value)
    //     return value

	template<bool INITIAL>
	struct ABRetType {
		using type = PositionEvaluation;
	};

	template<>
	struct ABRetType<true> {
		using type = Move;
	};

	template<bool INITIAL = false, bool IS_HERO_TURN>
	auto alphaBeta(
		ABGameState state,
		u64 remaining_depth,
		PositionEvaluation alpha,
		PositionEvaluation beta)
	-> ABRetType<INITIAL>::type	{
		
		static_assert(implies(INITIAL, IS_HERO_TURN), "Initial call should be hero turn");
		
		// @TODO: for now we just pass copies of the state, but
		// in the future we should pass references to the state
		// and apply/undo diffs.

		const u64 next_remaining_depth = IS_HERO_TURN ? remaining_depth : remaining_depth - 1;

		if constexpr (IS_HERO_TURN) {
			if (remaining_depth == 0 or state.state.isTerminal()) {
				if constexpr (INITIAL) {
					assert(false); // static_assertion fails hare
				}
				else {
					return state.state.evaluate();
				}
			}
		}

		if constexpr (IS_HERO_TURN) {
			PositionEvaluation value = PositionEvaluation::losing();
			Move best_move = Move::WAIT;

			for (auto move: MOVE_ARRAY)
			if (state.state.isMoveSensible(move, Player::HERO)) {
			
				ABGameState new_state = state;
				new_state.hero_move_commit = move;

				auto move_value = alphaBeta<false, not IS_HERO_TURN>(
					std::move(new_state),
					next_remaining_depth,
					alpha,
					beta
				);

				if (move_value > value) {
					value = move_value;
					best_move = move;
				}

				if (value > beta) {
					break; // β cutoff
				}

				alpha = std::max(alpha, value);
			}

			if constexpr (INITIAL) {
				return best_move;
			}
			else {
				return value;
			}
		}
		else {
			static_assert(not INITIAL, "Enemy move should not be a initial position");

			PositionEvaluation value = PositionEvaluation::wining();

			for (auto move: MOVE_ARRAY)
			if (state.state.isMoveSensible(move, Player::ENEMY)) {

				ABGameState new_state = state;
				new_state.hero_move_commit = {};

				auto hero_move = state.hero_move_commit.value();
				auto enemy_move = move;

				new_state.state.applyMove(hero_move, enemy_move);

				auto move_value = alphaBeta<false, not IS_HERO_TURN>(
					std::move(new_state),
					next_remaining_depth,
					alpha,
					beta
				);

				value = std::min(value, move_value);

				if (value < alpha) {
					break; // α cutoff
				}

				beta = std::min(beta, value);
			}

			return value;
		}
	}
}

Move findBestHeroMove(GameState state) {
	ABGameState ab_state = {std::move(state), std::nullopt};
	return alpha_beta::alphaBeta<true, true>(
		std::move(ab_state),
		AB_DEPTH,
		PositionEvaluation::losing(),
		PositionEvaluation::wining()
	);
}


int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(nullptr);

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

		.player_positions = { who_are_we == 'R' ? red_player : blue_player,
		                      who_are_we == 'R' ? blue_player : red_player }
	};

	// standard:
	auto best_move = findBestHeroMove(std::move(game_state));
	std::cout << moveToIndex(best_move) << "\n";


	// // debug print:
	// for (int i = 0; i < 10; i++) {
	// 	game_state.debugPrint();
	// 	game_state.moveBullets();
	// 	std::cout << "\n\n----------------\n\n";
	// }

}


