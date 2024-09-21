#include <iostream>
#include <cassert>
#include <vector>
#include <array>
#include <optional>

namespace {

// @TODO: put evaluation config in one place
// @opt: optional constexpr n, m
// @opt: debug/release mode (with macros, so we don't waste opt passes)

using u64 = uint64_t;
using i64 = int64_t;

// @TODO: make it bigger when cutoffs are efficient
constexpr u64 AB_DEPTH = 4;

// constexpr u64 N = 15;
// constexpr u64 M = 20;

/*******************/
// Global parameters:

static u64 n;
static u64 m;

constexpr u64 MAX_ROUND = 400;
static u64 round_number;

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

enum class Direction {
	UP    = 0,
	DOWN  = 1,
	LEFT  = 2,
	RIGHT = 3,
};

[[gnu::cold]]
constexpr char dirToChar(Direction dir) {
	switch (dir) {
		case Direction::UP:
			return '^';
		case Direction::DOWN:
			return 'v';
		case Direction::LEFT:
			return '<';
		case Direction::RIGHT:
			return '>';
		default:
			assert(false);
	}
}

constexpr std::array<Direction, 4> DIRECTION_ARRAY = {
	Direction::UP,
	Direction::DOWN,
	Direction::LEFT,
	Direction::RIGHT
};

constexpr Direction moveToDir(Move move) {
	switch (move) {
		case Move::GO_UP:
			return Direction::UP;
		case Move::GO_DOWN:
			return Direction::DOWN;
		case Move::GO_LEFT:
			return Direction::LEFT;
		case Move::GO_RIGHT:
			return Direction::RIGHT;
		case Move::SHOOT_UP:
			return Direction::UP;
		case Move::SHOOT_DOWN:
			return Direction::DOWN;
		case Move::SHOOT_LEFT:
			return Direction::LEFT;
		case Move::SHOOT_RIGHT:
			return Direction::RIGHT;
		default:
			assert(false);
	}
}

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
	
	constexpr bool operator==(const Vec& other) const = default;

	constexpr Vec operator+(const Vec& other) const {
		return {x + other.x, y + other.y};
	}

	constexpr Vec operator+=(const Vec& other) {
		x += other.x;
		y += other.y;
		return *this;
	}
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
		// @opt: test []
		return this->bullets.at(x * m + y).b;
	}
	
	bool getRef(u64 x, u64 y) const {
		// @opt: test []
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

	const BoolLayer& getBullets(Direction dir) const {
		return bullets.get(dir);
	}

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

	/**
	 * @note For debug print purposes
	 */
	[[gnu::cold]]
	Direction oneDirAt(Vec pos) const {
		for (auto dir: DIRECTION_ARRAY) {
			if (bullets.get(dir).get(pos)) {
				return dir;
			}
		}
		assert(false);
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
};


struct SurvivalData {
	// opt: using i32 here may be faster
	// due to lower stack usage
	// @note: signed, so -1 will not cause bugs

	/**
	 * @note: round count is capped
	 */
	i64 round_count = 0;

	/**
	 * @brief 0 if round count was not capped.
	 * count of ghost at the end if it was.
	 */
	i64 ghost_count = 0;

	double doubleScore() const {
		constexpr static double ROUND_COEFF = 100.0;
		return round_count * ROUND_COEFF + ghost_count;
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

	// @note: conditional: if other player is chilling
	// @note: unconditional: no mather what other player will do

	SurvivalData hero_conditional;
	SurvivalData hero_unconditional;

	SurvivalData enemy_conditional;
	SurvivalData enemy_unconditional;

public:
	constexpr PositionEvaluation(bool hero_hit, bool enemy_hit):
		hero_hit(hero_hit), enemy_hit(enemy_hit) {
		
		// only of these may hold:
		assert(not (isWining() and isLosing()));
		assert(not (isWining() and isDraw()));
		assert(not (isLosing() and isDraw()));
	}

	constexpr PositionEvaluation(
		SurvivalData hero_conditional,
		SurvivalData hero_unconditional,
	    SurvivalData enemy_conditional,
		SurvivalData enemy_unconditional):
		
		hero_hit(false), enemy_hit(false),
		hero_conditional(hero_conditional),
		hero_unconditional(hero_unconditional),
		enemy_conditional(enemy_conditional),
		enemy_unconditional(enemy_unconditional)
	{}


	constexpr static PositionEvaluation losing() {
		return PositionEvaluation(true, false);
	}

	constexpr static PositionEvaluation wining() {
		return PositionEvaluation(false, true);
	}

	constexpr bool isWining() const {
		// direct:
		if (not hero_hit and enemy_hit) return true;

		// @TODO: do we want it?
		// It does not take into account the round count.
		// indirect:
		if (hero_unconditional.round_count > enemy_conditional.round_count)
			return true;

		return false;
	}
	constexpr bool isLosing() const {
		if (hero_hit and not enemy_hit) return true;

		// @TODO: do we want it?
		// It does not take into account round count.
		// indirect:
		if (hero_conditional.round_count < enemy_unconditional.round_count)
			return true;

		return false;
	}
	constexpr bool isDraw() const {
		return hero_hit and enemy_hit;
	}

	constexpr bool determined() const {
		return isWining() or isLosing() or isDraw();
	}

	constexpr i64 determinedToInt() const {
		if (isWining()) {
			return 1;
		}
		if (isLosing()) {
			return -1;
		}
		if (isDraw()) {
			return 0;
		}
		assert(false);
	}

	/**
	 * @todo: double vs i64
	 */
	double getDoubleScore() const {
		// Those values are arbitrary for now:
		constexpr static double HERO_C_COEFF  = 16.0;
		constexpr static double HERO_U_COEFF  = 1.0;
		constexpr static double ENEMY_U_COEFF = -1.0;
		constexpr static double ENEMY_C_COEFF = -8.0;

		return 
			HERO_C_COEFF  * hero_conditional.doubleScore() +
			HERO_U_COEFF  * hero_unconditional.doubleScore() +
			ENEMY_U_COEFF * enemy_unconditional.doubleScore() +
			ENEMY_C_COEFF * enemy_conditional.doubleScore();
	}

	/**
	 * @brief eval1 < eval2, means that eval2 is better for hero
	 */
	bool operator<(const PositionEvaluation& other) const {
		// it checks if "we are worse"

		// @note this will get much more complicated..

		if (this->determined() and other.determined()) {
			return this->determinedToInt() < other.determinedToInt();
		}

		// now we know that at most one is determined:
		// if other is, we are not
		if (other.isWining()) {
			// we are worse
			return true;
		}
		if (other.isLosing()) {
			// non-determined is better then losing (even tho it might also be lost)
			return false;
		}
		
		if (this->isWining()) {
			// we are better
			return false;
		}
		if (this->isLosing()) {
			// we are worse
			return true;
		}

		// now both are non-determined or
		// one is non-determined and the other is drawing
		// @TODO: draw score should be zero, but that has to be tested

		return this->getDoubleScore() < other.getDoubleScore();
	}

	bool operator>(const PositionEvaluation& other) const {
		return other < *this;
	}
};

struct PlayerPositions {
private:
	Vec player_positions[2];
public:
	PlayerPositions(Vec hero, Vec enemy): player_positions{hero, enemy} {}

	Vec getPosition(Player player) const {
		// @OPT: use static cast here
		if (player == Player::HERO) {
			return player_positions[0];
		} else {
			return player_positions[1];
		}
	}

	Vec& getPosition(Player player) {
		// @OPT: use static cast here
		if (player == Player::HERO) {
			return player_positions[0];
		} else {
			return player_positions[1];
		}
	}

	Vec getHeroPosition() const {
		return player_positions[0];
	}

	Vec getEnemyPosition() const {
		return player_positions[1];
	}
};

struct DebugPrintLayer {
private:
	std::vector<std::vector<char>> data;
public:
	
	[[gnu::cold]]
	DebugPrintLayer(): data(n, std::vector<char>(m, ' ')) {}

	[[gnu::cold]]
	void apply(const BoolLayer& layer, char c) {
		for (i64 i = 0; i < i64(n); i++) {
			for (i64 j = 0; j < i64(m); j++) {
				if (layer.get({i, j})) {
					data[i][j] = c;
				}
			}
		}
	}

	[[gnu::cold]]
	void apply(const BulletLayer& layer) {
		for (auto dir: DIRECTION_ARRAY) {
			apply(layer.getBullets(dir), dirToChar(dir));
		}
	}

	[[gnu::cold]]
	void apply(const PlayerPositions& players) {
		data[players.getHeroPosition().x][players.getHeroPosition().y] = 'H';
		data[players.getEnemyPosition().x][players.getEnemyPosition().y] = 'E';
	}

	[[gnu::cold]]
	void print() const {
		for (i64 i = 0; i < i64(n); i++) {
			for (i64 j = 0; j < i64(m); j++) {
				std::cout << data[i][j];
			}
			std::cout << "\n";
		}
	}
};

struct GhostPlayerLayer {
private:
	BoolLayer ghosts;
public:
	GhostPlayerLayer(Vec initial_pos): ghosts() {
		ghosts.set(initial_pos, true);
	}

	const BoolLayer& getGhosts() const {
		return ghosts;
	}

	u64 ghostCount() const {
		u64 count = 0;
		for (i64 i = 0; i < i64(n); i++) {
			for (i64 j = 0; j < i64(m); j++) {
				count += ghosts.get({i, j});
			}
		}
		return count;
	}

	void eliminateGhostsAt(const BoolLayer& eliminations) {
		for (i64 i = 0; i < i64(n); i++) {
			for (i64 j = 0; j < i64(m); j++) {
				Vec pos = {i, j};
				if (eliminations.get(pos)) {
					ghosts.set(pos, false);
				}
			}
		}
	}

	void eliminateGhostsAt(const BulletLayer& bullets) {
		// @opt: test if it is faset to use isBulletAt

		eliminateGhostsAt(bullets.getBullets(Direction::UP));
		eliminateGhostsAt(bullets.getBullets(Direction::DOWN));
		eliminateGhostsAt(bullets.getBullets(Direction::LEFT));
		eliminateGhostsAt(bullets.getBullets(Direction::RIGHT));
	}

	void moveGhostsEverywhere() {
		// @todo: no bound checks here -- should not be needed
		// @opt: try to avoid copy
		// (can be done, but the question is will it be more efficient).

		BoolLayer new_ghosts = ghosts;

		for (i64 i = 0; i < i64(n); i++) {
			for (i64 j = 0; j < i64(m); j++) {
				Vec pos = {i, j};
				if (ghosts.get(pos)) {
					for (auto dir: DIRECTION_ARRAY) {
						Vec new_pos = pos + dirToVec(dir);
						new_ghosts.set(new_pos, true);
					}
				}
			}
		}

		ghosts = std::move(new_ghosts);
	}
};

struct GameState {
	BoolLayer walls;
	BulletLayer bullets;

	PlayerPositions players;
	
	void moveBullets() {
		bullets.moveBulletsWithWalls(walls);
	}

	[[gnu::cold]]
	void debugPrint() const {
		DebugPrintLayer printer;
		printer.apply(players);
		printer.apply(bullets);
		printer.apply(walls, '#');
		printer.print();
		std::cout << "\n";
	}

	void applyMove(Move hero_move, Move enemy_move) {
		// 1. first perform players actions:

		PlayerPositions old_positions = players;

		for (auto [player, move]:
			{std::pair{Player::HERO, hero_move},
			{Player::ENEMY, enemy_move}}) {
			
			// @TODO: for now we don't bound check here
			// @TODO: for now we allow "walk into walk"
			// sensible move check should disallow it

			// @TODO this switch could be refactored somehow..
			// @OPT: switches are slow due to switch value check
			switch (move) {
				case Move::GO_UP:
					players.getPosition(player) += dirToVec(Direction::UP);
					break;
				case Move::GO_DOWN:
					players.getPosition(player) += dirToVec(Direction::DOWN);
					break;
				case Move::GO_LEFT:
					players.getPosition(player) += dirToVec(Direction::LEFT);
					break;
				case Move::GO_RIGHT:
					players.getPosition(player) += dirToVec(Direction::RIGHT);
					break;
				case Move::SHOOT_UP:
					bullets.addBullet(players.getPosition(player), Direction::UP);
					break;
				case Move::SHOOT_DOWN:
					bullets.addBullet(players.getPosition(player), Direction::DOWN);
					break;
				case Move::SHOOT_LEFT:
					bullets.addBullet(players.getPosition(player), Direction::LEFT);
					break;
				case Move::SHOOT_RIGHT:
					bullets.addBullet(players.getPosition(player), Direction::RIGHT);
					break;
				case Move::WAIT:
					// do nothing
					break;
			}			
		}

		if (players.getHeroPosition() == players.getEnemyPosition()) {
			players = old_positions;
		}
		
		// 2. move bullets

		this->moveBullets();

		// @note: we don't check for players hit here
		// it is done in the evaluation function and "isTerminal".
	}

	bool isMoveSensible(Move move, Player player) const {
		if (move == Move::WAIT) {
			// we assume that wait is always sensible
			// so nothing can break in the algorithm
			return true;
		}
		auto dir = moveToDir(move);
		auto pos = players.getPosition(player);
		auto new_pos = pos + dirToVec(dir);

		// we don't shoot and don't walk into walls
		// waiting is no worse then that
		return not walls.get(new_pos);
	}

	bool isTerminal() const {
		return bullets.isBulletAt(players.getHeroPosition()) or bullets.isBulletAt(players.getEnemyPosition());
	}

	PositionEvaluation evaluate() const {
		bool hero_hit = bullets.isBulletAt(players.getHeroPosition());
		bool enemy_hit = bullets.isBulletAt(players.getEnemyPosition());

		if (hero_hit or enemy_hit) {
			return {
				bullets.isBulletAt(players.getHeroPosition()),
				bullets.isBulletAt(players.getEnemyPosition())
			};
		}

		// no hero hit
		// not enemy hit

		constexpr static u64 MAX_ROUND_LOOKUP = 10;

		SurvivalData hero_u;
		SurvivalData hero_c;
		SurvivalData enemy_u;
		SurvivalData enemy_c;

		auto lookup_bullets = this->bullets;

		GhostPlayerLayer hero_c_ghosts(players.getHeroPosition());
		GhostPlayerLayer hero_u_ghosts(players.getHeroPosition());

		GhostPlayerLayer enemy_c_ghosts(players.getEnemyPosition());
		GhostPlayerLayer enemy_u_ghosts(players.getEnemyPosition());

		BulletLayer hero_c_bullets;
		BulletLayer enemy_c_bullets;

		// @TODO: make it less boilerplate'y

		for (u64 i = 0; i < MAX_ROUND_LOOKUP; i++) {
			// sim step:

			// TODO: ghost shoot:
			// Hero c adds to hero_c_bullets
			// Enemy c add to enemy_c_bullets
			
			// move ghosts:
			hero_c_ghosts.moveGhostsEverywhere();
			hero_u_ghosts.moveGhostsEverywhere();
			enemy_c_ghosts.moveGhostsEverywhere();
			enemy_u_ghosts.moveGhostsEverywhere();

			// move bullets:
			lookup_bullets.moveBulletsWithWalls(walls);
			hero_c_bullets.moveBulletsWithWalls(walls);
			enemy_c_bullets.moveBulletsWithWalls(walls);

			// elim ghosts with walls:
			hero_c_ghosts.eliminateGhostsAt(walls);
			hero_u_ghosts.eliminateGhostsAt(walls);
			enemy_c_ghosts.eliminateGhostsAt(walls);
			enemy_u_ghosts.eliminateGhostsAt(walls);

			// elim ghost with bullets:
			hero_c_ghosts.eliminateGhostsAt(lookup_bullets);
			hero_u_ghosts.eliminateGhostsAt(lookup_bullets);
			enemy_c_ghosts.eliminateGhostsAt(lookup_bullets);
			enemy_u_ghosts.eliminateGhostsAt(lookup_bullets);

			hero_u_ghosts.eliminateGhostsAt(enemy_c_bullets);
			enemy_u_ghosts.eliminateGhostsAt(hero_c_bullets);

			if (hero_c_ghosts.ghostCount() > 0) {
				hero_c.round_count = i + 1;
			}
			if (hero_u_ghosts.ghostCount() > 0) {
				hero_u.round_count = i + 1;
			}
			if (enemy_c_ghosts.ghostCount() > 0) {
				enemy_c.round_count = i + 1;
			}
			if (enemy_u_ghosts.ghostCount() > 0) {
				enemy_u.round_count = i + 1;
			}
		}

		hero_c.ghost_count  = hero_c_ghosts.ghostCount();
		hero_u.ghost_count  = hero_u_ghosts.ghostCount();
		enemy_c.ghost_count = enemy_c_ghosts.ghostCount();
		enemy_u.ghost_count = enemy_u_ghosts.ghostCount();
	
		return {
			hero_c,
			hero_u,
			enemy_c,
			enemy_u
		};
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
	template<bool INITIAL>
	struct ABRetType {
		using type = PositionEvaluation;
	};

	template<>
	struct ABRetType<true> {
		using type = Move;
	};

	constinit static u64 leaf_counter = 0;

	template<bool INITIAL = false, bool IS_HERO_TURN>
	auto alphaBeta(
		ABGameState state,
		u64 remaining_depth,
		PositionEvaluation alpha,
		PositionEvaluation beta)
	-> ABRetType<INITIAL>::type	{
		
		static_assert(implies(INITIAL, IS_HERO_TURN), "Initial call should be hero turn");
		
		// @opt: for now we just pass copies of the state, but
		// in the future we should pass references to the state
		// and apply/undo diffs.

		const u64 next_remaining_depth = IS_HERO_TURN ? remaining_depth : remaining_depth - 1;

		if constexpr (IS_HERO_TURN) {
			if (remaining_depth == 0 or state.state.isTerminal()) {
				if constexpr (INITIAL) {
					assert(false); // static_assertion fails hare
				}
				else {
					leaf_counter++;
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

/** 
 * @note: it sets globals n, m, round_number
 * @return GameState 
 */
GameState readInput() {
	{
		std::cin >> ::n >> ::m;
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

	std::cin >> ::round_number;
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

		.players = { who_are_we == 'R' ? red_player : blue_player,
		             who_are_we == 'R' ? blue_player : red_player }
	};

	return game_state;
}

[[maybe_unused]]
[[gnu::cold]]
void exampleScenario(GameState game_state) {
	using enum Move;
	std::cout << std::boolalpha;

	game_state.debugPrint();

	game_state.applyMove(WAIT, WAIT);
	game_state.debugPrint();

	game_state.applyMove(WAIT, WAIT);
	game_state.debugPrint();
	std::cout << game_state.isTerminal() << "\n";

	game_state.applyMove(WAIT, GO_UP);
	game_state.debugPrint();
	std::cout << game_state.isTerminal() << "\n";

	game_state.applyMove(SHOOT_UP, WAIT);
	game_state.debugPrint();

	game_state.applyMove(SHOOT_DOWN, GO_DOWN);
	game_state.debugPrint();

	game_state.applyMove(GO_UP, GO_LEFT);
	game_state.debugPrint();

	game_state.applyMove(GO_RIGHT, GO_LEFT);
	game_state.debugPrint();

	game_state.applyMove(GO_UP, GO_UP);
	game_state.debugPrint();
}

[[maybe_unused]]
[[gnu::cold]]
void ghostTest(GameState game_state) {
	auto walls = game_state.walls;
	auto bullets = game_state.bullets;

	auto ghosts = GhostPlayerLayer(game_state.players.getHeroPosition());

	for (u64 i = 0; i < 20; i++) {
		// print:
		DebugPrintLayer printer;
		printer.apply(ghosts.getGhosts(), 'G');
		printer.apply(bullets);
		printer.apply(walls, '#');
		printer.print();
		std::cout << "\n";
		std::cout << std::flush;

		// move:
		ghosts.moveGhostsEverywhere();
		bullets.moveBulletsWithWalls(walls);

		// eliminations:
		ghosts.eliminateGhostsAt(bullets);
		ghosts.eliminateGhostsAt(walls);
		
		
	}
}

}

int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(nullptr);

	auto game_state = readInput();

	// standard:
	// auto best_move = findBestHeroMove(std::move(game_state));
	// std::cout << moveToIndex(best_move) << "\n";

	// std::cerr << "leafs: " << alpha_beta::leaf_counter << "\n";

	// exampleScenario(game_state);
	ghostTest(game_state);
	
}


