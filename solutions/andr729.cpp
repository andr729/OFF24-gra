#include <iostream>
#include <cassert>
#include <vector>
#include <array>
#include <optional>
#include <bitset>

namespace {

// @note: looking at std source code, it seams that bitsets are stored in the type directly

#define CONST_NM 1
#define BIT_SET 1 // @TODO: 0 is not supported yet
#define WEIRD_BIT_OPERATIONS 1
#define MOVE_BULLETS_VECTOR 1
#define NO_BOUND_CHECKS 1
#define NO_OTHER_CHECKS 1
#define UNSAFE_OTHERS 1
#define ALLOW_BIGGER_NM 1
#define STATIC_WALLS 1

using u64 = uint64_t;
using i64 = int64_t;
using i32 = int32_t;

namespace conf {
	// note: we want to optimize it so we have 12/3 here
	constexpr u64 MAX_ROUND_LOOKUP = 8;
	constexpr u64 AB_DEPTH = 4;

	// round vs ghost count
	constexpr double ROUND_COEFF = 1024.0;

	constexpr double TIE_COEFF = 0;

	// @TODO: optimize those parameters:
	// Those values are arbitrary for now:
	constexpr double HERO_C_COEFF  = 8.0;
	constexpr double HERO_U_COEFF  = 1.0;
	constexpr double ENEMY_C_COEFF = -8.0;
	constexpr double ENEMY_U_COEFF = -1.0;
}

/*******************/
// Global parameters:

#if CONST_NM == 1
	constexpr u64 n = 15;
	constexpr u64 m = 20;
	constexpr u64 nm = n * m;
#else
	static u64 n;
	static u64 m;
	static u64 nm;
#endif

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

constexpr i64 moveIndexPos(i64 index, Direction dir) {
	switch (dir) {
		case Direction::UP:
			return index - m;
		case Direction::DOWN:
			return index + m;
		case Direction::LEFT:
			return index - 1;
		case Direction::RIGHT:
			return index + 1;
	}
	assert(false);
}

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
	#if UNSAFE_OTHERS == 1
		return static_cast<Direction>(
			static_cast<std::underlying_type_t<Move>>(move) % 4
		);
	#else
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
	#endif
}

// @OPT: change the order maybe?
// @note: can't just do it now
constexpr std::array<Move, 9> MOVE_ARRAY = {
	Move::GO_UP,
	Move::GO_DOWN,
	Move::GO_LEFT,
	Move::GO_RIGHT,
	Move::SHOOT_UP,
	Move::SHOOT_DOWN,
	Move::SHOOT_LEFT,
	Move::SHOOT_RIGHT,
	Move::WAIT,
};

auto moveToIndex(Move move) {
	return static_cast<std::underlying_type_t<Move>>(move);
}

enum class Player {
	HERO  = 0,
	ENEMY = 1,
};

constexpr u64 playerToIndex(Player player) {
	return static_cast<std::underlying_type_t<Player>>(player);
}

// @TODO: refactor: IdxVec
struct Vec {
	i64 x = 0;
	i64 y = 0;

	constexpr Vec() = default;
	constexpr Vec(i64 x, i64 y): x(x), y(y) {}
	// constexpr Vec(const Vec& other) = default;
	// // constexpr Vec(Vec&& other) = default;

	// constexpr Vec& operator=(const Vec& other) = default;
	
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

constexpr i64 posToIndex(Vec pos) {
	return pos.x * m + pos.y;
}

constexpr Vec dirToVec(Direction dir) {
	// we have our dimension switched
	// so x is i, y is j
	// for this reason here
	// Also x is inverted so we can print it from 0 to n-1 (not reversed)

	// @note: using array is here not faster
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

constexpr std::array<Vec, 4> DIR_VEC_ARRAY = {
	dirToVec(Direction::UP),
	dirToVec(Direction::DOWN),
	dirToVec(Direction::LEFT),
	dirToVec(Direction::RIGHT)
};

constexpr Direction flip(Direction dir) {
	#if UNSAFE_OTHERS == 1
		// 0->1, 1->0, 2->3, 3->2
		auto dir_int = static_cast<std::underlying_type_t<Direction>>(dir);
		dir_int ^= 0b1;
		return static_cast<Direction>(dir_int);
	#else
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
	#endif
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

[[gnu::cold]]
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
	#if BIT_SET == 1
		using DataT = std::bitset<nm>;
		std::bitset<nm> bullets;
	#else
		using DataT = std::vector<Bool>;
		std::vector<Bool> bullets{nm, {false}};
	#endif

	BoolLayer(DataT bullets): bullets(bullets) {}

public:
	#if BIT_SET == 1
		bool atIndex(u64 index) const {
			#if NO_BOUND_CHECKS == 1
				return this->bullets[index];
			#else 
				return this->bullets.test(index);
			#endif
		}
		auto atIndexMut(u64 index) {
			#if NO_BOUND_CHECKS == 1
				return this->bullets[index];
			#else 
				return this->bullets[index];
			#endif
		}

		const auto& getBitset() const {
			return this->bullets;
		}
		auto& getBitsetMut() {
			return this->bullets;
		}
	#else
		bool atIndex(u64 index) const {
			#if NO_BOUND_CHECKS == 1
				return this->bullets[index].b;
			#else 
				return this->bullets.at(index).b;
			#endif
		}

		bool& atIndexMut(u64 index) {
			#if NO_BOUND_CHECKS == 1
				return this->bullets[index].b;
			#else 
				return this->bullets.at(index).b;
			#endif
		}
	#endif

	BoolLayer() = default;


	BoolLayer(const BoolLayer& other) = default;
	BoolLayer(BoolLayer&& other)      = default;

	BoolLayer& operator=(const BoolLayer& other) = default;
	BoolLayer& operator=(BoolLayer&& other)      = default;

	bool get(Vec pos) const {
		return atIndex(posToIndex(pos));
	}

	void set(Vec pos, bool val) {
		atIndexMut(posToIndex(pos)) = val;
	}

	void andAt(Vec pos, bool v) {
		bool curr = atIndex(posToIndex(pos));
		atIndexMut(posToIndex(pos)) = curr and v;
	}

	static BoolLayer fromVec(const std::vector<Vec>& vecs) {
		BoolLayer res;
		for (auto vec: vecs) {
			res.set(vec, true);
		}
		return res;
	}

	BoolLayer negated() const {
		#if BIT_SET == 1
			return BoolLayer(~this->getBitset());
		#else
			BoolLayer res;
			for (i64 i = 0; i < i64(nm); i++) {
				res.atIndexMut(i) = not this->atIndex(i);
			}
			return res;
		#endif
	}
};

struct BulletLayer {
private:
	// @opt: reversing the "storage dims" here might be faster
	QuadDirStorage<BoolLayer> bullets;

public:
	BulletLayer() = default;
	
	BulletLayer(QuadDirStorage<BoolLayer> bullets): bullets(std::move(bullets)) {}

	BulletLayer(const BulletLayer& other) = default;
	BulletLayer(BulletLayer&& other)      = default;
	
	BulletLayer& operator=(const BulletLayer& other) = default;
	BulletLayer& operator=(BulletLayer&& other) = default;

	const BoolLayer& getBullets(Direction dir) const {
		return bullets.get(dir);
	}

	BoolLayer& getBulletsMut(Direction dir) {
		return bullets.get(dir);
	}

	BoolLayer getFlat() const {
		BoolLayer res = bullets.up();
		res.getBitsetMut() |= bullets.down().getBitset();
		res.getBitsetMut() |= bullets.left().getBitset();
		res.getBitsetMut() |= bullets.right().getBitset();
		return res;
	}

	void addBulletAtIndex(i64 pos, Direction dir) {
		bullets.get(dir).atIndexMut(pos) = true;
	}

	#if BIT_SET == 0
		void addBulletAtIndexIf(i64 pos, Direction dir, bool if_) {
			bullets.get(dir).atIndexMut(pos) |= if_;
		}
	#endif

	bool isBulletAtIndex(i64 pos) const {
		bool res = false;
		for (auto dir: DIRECTION_ARRAY) {
			res |= bullets.get(dir).atIndex(pos);
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

	void moveBulletsWithWalls(const std::vector<i64>& walls_indices) {
		#if (BIT_SET == 1) and (MOVE_BULLETS_VECTOR == 1)
			// @TODO: -m/+m/-1/+1 are duplicated here:
			constexpr static std::array<std::pair<Direction, i64>, 4> DIR_SHIFT_ARR = {
				std::pair{Direction::UP, -i64(m)},
				{Direction::DOWN, m},
				{Direction::LEFT, -1},
				{Direction::RIGHT, 1}	
			};
			
			// @note: shifts here are reversed
			this->getBulletsMut(Direction::UP).getBitsetMut() >>= m;
			this->getBulletsMut(Direction::DOWN).getBitsetMut() <<= m;
			this->getBulletsMut(Direction::LEFT).getBitsetMut() >>= 1;
			this->getBulletsMut(Direction::RIGHT).getBitsetMut() <<= 1;
			
			// flip them:	
			for (i64 i: walls_indices) {
				for (auto [dir, shift]: DIR_SHIFT_ARR) {
					// here we don't care about double flips, cause
					// flipped bullets will only be, where there are no walls!
					
					if (bullets.get(dir).atIndex(i)) [[unlikely]] {
						bullets.get(dir).atIndexMut(i) = false;
						bullets.get(flip(dir)).atIndexMut(i - shift) = true;
					}
				}
			}

			// no assign, we do it inp place

		#else
			#error No wall indicies support (yet) for no bitset
			// ideally we want to do it inplace for performance..
			// for now we ignore it
			BulletLayer new_bullets;

			for (u64 i = 0; i < nm; i++) {
				for (auto dir: DIRECTION_ARRAY) {
					if (bullets.get(dir).atIndex(i)) {
						i64 new_pos = moveIndexPos(i, dir);
						Direction new_dir = dir;

						// @note: for now we don't check for out of bounds here.
						// it should never happen for valid inputs.

						// @opt: this if might be eliminatable
						if (walls.atIndex(new_pos)) [[unlikely]] {
							new_dir = flip(dir);
							new_pos = i;
						}

						new_bullets.addBulletAtIndex(new_pos, new_dir);
					}
				}
			}

			*this = std::move(new_bullets);
		#endif
	}
};


struct SurvivalData {
	// @note: using i32 here seams be faster
	// (probably due to lower stack usage)
	// @note: signed, so -1 will not cause bugs

	/**
	 * @note: round count is capped
	 */
	i32 round_count = 0;

	/**
	 * @brief 0 if round count was not capped.
	 * count of ghost at the end if it was.
	 */
	i32 ghost_count = 0;

	double doubleScore() const {
		return round_count * conf::ROUND_COEFF + ghost_count;
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
		if (isDraw()) {
			return
				(conf::HERO_C_COEFF + conf::HERO_U_COEFF +
				 conf::ENEMY_C_COEFF + conf::ENEMY_U_COEFF) * conf::TIE_COEFF; 
		}

		return 
			conf::HERO_C_COEFF  * hero_conditional.doubleScore() +
			conf::HERO_U_COEFF  * hero_unconditional.doubleScore() +
			conf::ENEMY_C_COEFF * enemy_conditional.doubleScore() +
			conf::ENEMY_U_COEFF * enemy_unconditional.doubleScore();
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

	[[gnu::cold]]
	void debugPrint() const {
		if (isWining()) {
			std::cerr << "Eval: Wining\n";
		}
		else if (isLosing()) {
			std::cerr << "Eval: Losing\n";
		}
		else if (isDraw()) {
			std::cerr << "Eval: Draw\n";
		}
		else {
			std::cerr << "Eval: " << getDoubleScore() << "\n";
		}
	}
};

struct PlayerPositions {
private:
	Vec player_positions[2];
public:
	PlayerPositions() = default;
	PlayerPositions(Vec hero, Vec enemy): player_positions{hero, enemy} {}

	Vec getPosition(Player player) const {
		return player_positions[playerToIndex(player)];
	}

	Vec& getPosition(Player player) {
		return player_positions[playerToIndex(player)];
	}

	Vec getHeroPosition() const {
		return player_positions[playerToIndex(Player::HERO)];
	}

	Vec getEnemyPosition() const {
		return player_positions[playerToIndex(Player::ENEMY)];
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

	u64 eliminateGhostsNegative(const BoolLayer& negative_eliminations) {
		#if BIT_SET == 1
				ghosts.getBitsetMut() &= negative_eliminations.getBitset();
				return ghosts.getBitset().count();
		#else
				assert(false); // @TODO:
				for (i64 i = 0; i < i64(nm); i++) {
						ghosts.atIndexMut(i) &= (!eliminations.atIndex(i));
				}
		#endif
	}


	/**
	 * @param bullets 
	 * @return u64 number of ghosts after elimination
	 */
	u64 eliminateGhostsAt(const BulletLayer& bullets) {
		#if BIT_SET == 1
			auto eliminations = bullets.getBullets(Direction::UP).getBitset();
			eliminations |= bullets.getBullets(Direction::DOWN).getBitset();
			eliminations |= bullets.getBullets(Direction::LEFT).getBitset();
			eliminations |= bullets.getBullets(Direction::RIGHT).getBitset();
			
			ghosts.getBitsetMut() &= (~eliminations);

			return ghosts.getBitset().count();
		#else 
			// @note: isBulletAt is way faster then eliminateGhostsAt x4
			u64 count = 0;
			for (i64 i = 0; i < i64(nm); i++) {
				ghosts.atIndexMut(i) &= (!bullets.isBulletAtIndex(i));
				count += ghosts.atIndex(i);
			}
			return count;
		#endif
	}

	void moveGhostsEverywhere(const BoolLayer& negative_walls) {
		// @note: no bound checks here -- should not be needed

		#if BIT_SET == 1
			// @note: shift by 1 only works
			// // because we have walls on borders

			// this seams a little faster:
			#if WEIRD_BIT_OPERATIONS == 1
				// we can do it, but only
				// because we have walls on borders
				
				auto& ghosts_ref = ghosts.getBitsetMut();
				std::bitset<nm> oper_ghosts = ghosts_ref;
				
				oper_ghosts <<= m;
				ghosts_ref |= oper_ghosts;
				
				oper_ghosts >>= 2*m;
				ghosts_ref |= oper_ghosts;

				oper_ghosts <<= m + 1;
				ghosts_ref |= oper_ghosts;

				oper_ghosts >>= 2;
				ghosts_ref |= oper_ghosts;

				ghosts_ref &= negative_walls.getBitset();

			#else
				std::bitset<nm> new_ghosts = ghosts.getBitset();
				new_ghosts |= ghosts.getBitset() << m;
				new_ghosts |= ghosts.getBitset() >> m;
				new_ghosts |= ghosts.getBitset() << 1;
				new_ghosts |= ghosts.getBitset() >> 1;

				new_ghosts &= negative_walls.getBitset();

				this->ghosts.getBitsetMut() = std::move(new_ghosts);
			#endif
		#else
			// Try to avoid copy -- apparently its very similar the way it is
			// It may be that compiler optimizes it much better
			BoolLayer new_ghosts = ghosts;

			for (i64 i = 0; i < i64(nm); i++) {
				if (ghosts.atIndex(i)) {
					for (auto dir: DIRECTION_ARRAY) {
						auto new_pos = moveIndexPos(i, dir);
						new_ghosts.atIndexMut(new_pos) |= 
							negative_walls.atIndex(new_pos);
					}
				}
			}

			// this is not faster :o:
			// for (i64 i = 0; i < i64(nm); i++) {
			// 	new_ghosts.atIndexMut(i) &= 
			// 		(not walls.atIndex(i));
			// }

			this->ghosts = std::move(new_ghosts);
		#endif
	}

	void shootOnLayer(BulletLayer& bullets) const {
		#if BIT_SET == 1
			bullets.getBulletsMut(Direction::UP).getBitsetMut() |= ghosts.getBitset();
			bullets.getBulletsMut(Direction::DOWN).getBitsetMut() |= ghosts.getBitset();
			bullets.getBulletsMut(Direction::LEFT).getBitsetMut() |= ghosts.getBitset();
			bullets.getBulletsMut(Direction::RIGHT).getBitsetMut() |= ghosts.getBitset();
		#else
			for (i64 i = 0; i < i64(nm); i++) {
				bool shoot = ghosts.atIndex(i);
				for (auto dir: DIRECTION_ARRAY) {
					bullets.addBulletAtIndexIf(i, dir, shoot);
				}
			}
		#endif
	}
};

struct GameState {
	#if STATIC_WALLS == 1
		static inline BoolLayer walls;
		static inline BoolLayer negative_walls;
		static inline std::vector<i64> walls_indices;
	#else
		BoolLayer walls;
	#endif

	BulletLayer bullets;
	PlayerPositions players;
	
	void moveBullets() {
		bullets.moveBulletsWithWalls(walls_indices);
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

	// struct BulletDiff {
	// 	i64 index;
	// 	bool value;
	// };
	// struct GoBackData {
	// 	PlayerPositions old_players;
	// 	BulletDiff bullet_diff[2];
	// };

	void applyMove(Move hero_move, Move enemy_move) {
		// 1. first perform players actions:

		// GoBackData go_back = {
		// 	players,
		// 	// @todo: we know, there are no bullets here, since there are walls:
		// 	{{0, false}, {1, false}}
		// };

		PlayerPositions old_positions = players;

		for (auto [player, move]:
			{std::pair{Player::HERO, hero_move},
			{Player::ENEMY, enemy_move}}) {
			
			// @note: for now we don't bound check here
			// @TODO: for now we allow "walk into walk"
			// sensible move check should disallow it
	
			auto player_position_index = posToIndex(players.getPosition(player));
			
			#if UNSAFE_OTHERS
				auto move_index = moveToIndex(move);
				if (move_index < 4) {
					players.getPosition(player) += dirToVec(DIRECTION_ARRAY[move_index]);
				}
				else if (move_index < 8) {
					bullets.addBulletAtIndex(player_position_index, DIRECTION_ARRAY[move_index - 4]);
				}

			#else
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
						bullets.addBulletAtIndex(player_position_index, Direction::UP);
						break;
					case Move::SHOOT_DOWN:
						bullets.addBulletAtIndex(player_position_index, Direction::DOWN);
						break;
					case Move::SHOOT_LEFT:
						bullets.addBulletAtIndex(player_position_index, Direction::LEFT);
						break;
					case Move::SHOOT_RIGHT:
						bullets.addBulletAtIndex(player_position_index, Direction::RIGHT);
						break;
					case Move::WAIT:
						// do nothing
						break;
				}
			#endif	
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
		return bullets.isBulletAtIndex(posToIndex(players.getHeroPosition()))
			or bullets.isBulletAtIndex(posToIndex(players.getEnemyPosition()));
	}

	PositionEvaluation evaluate() const {
		bool hero_hit = bullets.isBulletAtIndex(posToIndex(players.getHeroPosition()));
		bool enemy_hit = bullets.isBulletAtIndex(posToIndex(players.getEnemyPosition()));

		if (hero_hit or enemy_hit) {
			return {
				hero_hit,
				enemy_hit
			};
		}

		// no hero hit
		// no enemy hit

		SurvivalData hero_u;
		SurvivalData hero_c;
		SurvivalData enemy_u;
		SurvivalData enemy_c;

		auto lookup_bullets = this->bullets;

		GhostPlayerLayer hero_c_ghosts(players.getHeroPosition());
		GhostPlayerLayer hero_u_ghosts(players.getHeroPosition());

		GhostPlayerLayer enemy_c_ghosts(players.getEnemyPosition());
		GhostPlayerLayer enemy_u_ghosts(players.getEnemyPosition());

		BulletLayer hero_c_bullets = lookup_bullets;
		BulletLayer enemy_c_bullets = lookup_bullets;

		// @TODO: make it less boilerplate'y

		// @OPT making one struct here for quad opers
		// could be way faster

		u64 hc_count = 0;
		u64 hu_count = 0;
		u64 ec_count = 0;
		u64 eu_count = 0;

		for (u64 i = 0; i < conf::MAX_ROUND_LOOKUP; i++) {
			// sim step:

			// ghost shoots:
			hero_c_ghosts.shootOnLayer(hero_c_bullets);
			enemy_c_ghosts.shootOnLayer(enemy_c_bullets);
			
			// move ghosts:
			hero_c_ghosts.moveGhostsEverywhere(negative_walls);
			hero_u_ghosts.moveGhostsEverywhere(negative_walls);
			enemy_c_ghosts.moveGhostsEverywhere(negative_walls);
			enemy_u_ghosts.moveGhostsEverywhere(negative_walls);

			// move bullets:
			lookup_bullets.moveBulletsWithWalls(walls_indices);
			hero_c_bullets.moveBulletsWithWalls(walls_indices);
			enemy_c_bullets.moveBulletsWithWalls(walls_indices);

			// elim ghosts with walls (done in moveGhostsEverywhere)

			// elim ghost with bullets:
			
			// @OPT: flatten (and negate) lookup_bullets bullets here, might be faster, as compiler might not notice it
			// once this is done, keeping enemy_c_bullets, hero_c_bullets,
			// without lookup_bullets might be faster as well,
			// since there will be faster propagation (less flips)

			auto flat_lookup_bullets_neg = lookup_bullets.getFlat();
			flat_lookup_bullets_neg.getBitsetMut().flip();

			hc_count = hero_c_ghosts.eliminateGhostsNegative(flat_lookup_bullets_neg);
			ec_count = enemy_c_ghosts.eliminateGhostsNegative(flat_lookup_bullets_neg);
			
			hu_count = hero_u_ghosts.eliminateGhostsAt(enemy_c_bullets);
			eu_count = enemy_u_ghosts.eliminateGhostsAt(hero_c_bullets);

			if (hc_count > 0) {
				hero_c.round_count = i + 1;
			}
			if (hu_count > 0) {
				hero_u.round_count = i + 1;
			}
			if (ec_count > 0) {
				enemy_c.round_count = i + 1;
			}
			if (eu_count > 0) {
				enemy_u.round_count = i + 1;
			}
		}

		hero_c.ghost_count  = hc_count;
		hero_u.ghost_count  = hu_count;
		enemy_c.ghost_count = ec_count;
		enemy_u.ghost_count = eu_count;
	
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
		using type = std::pair<Move, PositionEvaluation>;
	};


	constinit static u64 leaf_counter = 0;

	// this does note speed stuff up, cause bitsets lay on stack anyway:
	static inline ABGameState static_states[conf::AB_DEPTH * 2 + 2];

	template<bool INITIAL, bool IS_HERO_TURN>
	auto alphaBeta(
		u64 remaining_depth,
		u64 state_depth,
		PositionEvaluation alpha,
		PositionEvaluation beta)
	-> ABRetType<INITIAL>::type	{

		const auto& state = static_states[state_depth];
		
		static_assert(implies(INITIAL, IS_HERO_TURN), "Initial call should be hero turn");
		
		// @opt: for now we just pass copies of the state, but
		// in the future we should pass references to the state
		// and apply/undo diffs.
		// @note: its not that simple, since bullets create large diffs...

		const u64 next_remaining_depth = IS_HERO_TURN ? remaining_depth : remaining_depth - 1;

		if constexpr (IS_HERO_TURN) {
			if (remaining_depth == 0 or state.state.isTerminal()) [[unlikely]] {
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

				// @note: notice we operate on the same state here:
				auto& new_state = static_states[state_depth];
				new_state.hero_move_commit = move;

				auto move_value = alphaBeta<false, not IS_HERO_TURN>(
					next_remaining_depth,
					state_depth,
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
				if (value.isWining()) {
					break; // win cutoff
				}

				alpha = std::max(alpha, value);
			}

			if constexpr (INITIAL) {
				return { best_move, value };
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

				// @opt
				// we can move bullets once for each "GO" move.
				// we could also try to preprocess "lookup_bullets"
				auto& new_state = static_states[state_depth + 1];

				new_state = state;
				// no need to clear it:
				// new_state.hero_move_commit = {};

				#if NO_OTHER_CHECKS == 1
					auto hero_move = *state.hero_move_commit;
				#else
					auto hero_move = state.hero_move_commit.value();
				#endif
				
				auto enemy_move = move;

				new_state.state.applyMove(hero_move, enemy_move);

				auto move_value = alphaBeta<false, not IS_HERO_TURN>(
					next_remaining_depth,
					state_depth + 1,
					alpha,
					beta
				);

				value = std::min(value, move_value);

				if (value < alpha) {
					break; // α cutoff
				}
				if (value.isLosing()) {
					break; // loss cutoff
				}

				beta = std::min(beta, value);
			}

			return value;
		}
	}
}

[[gnu::cold]]
Move findBestHeroMove(GameState state) {
	ABGameState ab_state = {std::move(state), std::nullopt};
	alpha_beta::static_states[0] = std::move(ab_state);

	auto res = alpha_beta::alphaBeta<true, true>(
		conf::AB_DEPTH,
		0,
		PositionEvaluation::losing(),
		PositionEvaluation::wining()
	);

	res.second.debugPrint();
	std::cerr << "leafs: " << alpha_beta::leaf_counter << "\n";

	return res.first;
}

/** 
 * @note: it sets globals n, m, round_number
 * @return GameState 
 */
[[gnu::cold]]
GameState readInput() {
	u64 local_n, local_m;
	{
		std::cin >> local_n >> local_m;
		#if CONST_NM == 1
			#if ALLOW_BIGGER_NM == 1
				assert(local_n <= n);
				assert(local_m <= m);
			#else
				assert(local_n == n);
				assert(local_m == m);
			#endif
		#else 
			::n = local_n;
			::m = local_m;
			::nm = ::n * ::m;
		#endif
		
		assert(::nm == ::n * ::m);

		skipNewLine();
		global_params_set = true;
	}

	QuadDirStorage<std::vector<Vec>> bullets;
	
	std::vector<Vec> walls;

	Vec red_player;
	Vec blue_player;

	for (i64 i = 0; i < i64(local_n); i++) {
		for (i64 j = 0; j < i64(local_m); j++) {
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
		#if STATIC_WALLS != 1
			.walls = BoolLayer::fromVec(walls),
		#endif
		.bullets = {{
			BoolLayer::fromVec(bullets.up()), 
			BoolLayer::fromVec(bullets.down()),
			BoolLayer::fromVec(bullets.left()),
			BoolLayer::fromVec(bullets.right())
		}},

		.players = { who_are_we == 'R' ? red_player : blue_player,
		             who_are_we == 'R' ? blue_player : red_player }
	};

	#if STATIC_WALLS == 1
		GameState::walls = BoolLayer::fromVec(walls);
		GameState::negative_walls = GameState::walls.negated();
		for (u64 i = 0; i < nm; i++) {
			if (GameState::walls.atIndex(i)) {
				GameState::walls_indices.push_back(i);
			}
		}
	#endif

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
		ghosts.moveGhostsEverywhere(walls);
		assert(false);
		// not implemented:
		// bullets.moveBulletsWithWalls(walls);

		// eliminations:
		ghosts.eliminateGhostsAt(bullets);
		// ghosts.eliminateGhostsAt(walls);
		
		
	}
}

}

int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(nullptr);

	auto game_state = readInput();

	// standard:
	auto best_move = findBestHeroMove(std::move(game_state));
	std::cout << moveToIndex(best_move) << "\n";

	// exampleScenario(game_state);
	// ghostTest(game_state);
	
}







/*
notes:
	3_4 wins with 2_10
	2_10 seams better then 2_32


*/
