#include <bits/stdc++.h>
using namespace std;
#define all(x) x.begin(), x.end()
#define len(x) (int)x.size()
#define x first
#define y second
using ld = long double;
using i128 = __int128_t;
using pii = pair<int, int>;
using pdi = pair<double, int>;
long timeout = 4 * 1000;
int n_games = 100;

long get_time_in_microseconds()
{
    return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

struct Bullet
{
    pii pos;
    int dir;
};

pii operator+(pii a, pii b) { return {a.first + b.first, a.second + b.second}; }

array<pii, 9> walks = {pii{-1, 0}, pii{1, 0}, pii{0, -1}, pii{0, 1},
                       pii{0, 0}, pii{0, 0}, pii{0, 0}, pii{0, 0}, pii{0, 0}};

int manhat(pii a, pii b) { return abs(a.x - b.x) + abs(a.y - b.y); }

const int max_round_num = 400;
mt19937 ran;
char player_color = '0';
uint64_t start_time;

map<int, char> move_codes = {{0, 'w'}, {1, 's'}, {2, 'a'}, {3, 'd'}, {4, '^'}, {5, 'v'}, {6, '<'}, {7, '>'}, {8, '_'}};
array<array<char, 20>, 15> board;
char boardf(pii pos) { return board[pos.first][pos.second]; }

int start_round = 0;
int n, m;

struct GameState
{
    int round_num;
    bool p_killed, e_killed;

    array<array<int, 20>, 15> has_bullet;
    vector<Bullet> bullets;

    GameState()
    {
        for (auto &h : has_bullet)
            h.fill(0);
    }

    bool pos_has_bullet(pii pos) const
    {
        return has_bullet[pos.x][pos.y];
    }

    pii p_pos, e_pos;

    void move_bullet(int i)
    {
        Bullet &bullet = bullets[i];
        has_bullet[bullet.pos.x][bullet.pos.y]--;
        if (boardf(bullet.pos + walks[bullet.dir]) == '#')
            bullet.dir ^= 1;
        else
            bullet.pos = bullet.pos + walks[bullet.dir];
        has_bullet[bullet.pos.x][bullet.pos.y]++;
    }
    void add_bullet(Bullet bullet)
    {
        bullets.push_back(bullet);
        has_bullet[bullet.pos.x][bullet.pos.y]++;
    }
    void move_bullets()
    {
        round_num++;
        for (int i = 0; i < len(bullets); i++)
            move_bullet(i);
    }

    void move_players(int move_p, int move_e)
    {
        Bullet p_bullet, e_bullet;

        if (4 <= move_e && move_e < 8)
        {
            e_bullet.pos = e_pos;
            e_bullet.dir = move_e - 4;
            if (boardf(e_bullet.pos + walks[e_bullet.dir]) == '#')
                e_bullet.dir ^= 1;
            else
                e_bullet.pos = e_bullet.pos + walks[e_bullet.dir];
            add_bullet(e_bullet);
        }
        if (4 <= move_p && move_p < 8)
        {
            p_bullet.pos = p_pos;
            p_bullet.dir = move_p - 4;
            if (boardf(p_bullet.pos + walks[p_bullet.dir]) == '#')
                p_bullet.dir ^= 1;
            else
                p_bullet.pos = p_bullet.pos + walks[p_bullet.dir];
            add_bullet(p_bullet);
        }
        pii np_pos = p_pos + walks[move_p];
        pii ne_pos = e_pos + walks[move_e];

        if (boardf(ne_pos) == '#')
            ne_pos = e_pos;
        if (boardf(np_pos) == '#')
            np_pos = p_pos;

        if (np_pos == ne_pos)
        {
            np_pos = p_pos;
            ne_pos = e_pos;
        }

        p_pos = np_pos;
        e_pos = ne_pos;
        p_killed = pos_has_bullet(p_pos);
        e_killed = pos_has_bullet(e_pos);
    }

    GameState next_board(int move_p, int move_e) const
    {
        GameState next = *this;
        next.move_bullets();
        next.move_players(move_p, move_e);
        return next;
    }

    pair<vector<int>, vector<int>> get_not_stupid_moves_and_update_board()
    {
        move_bullets();

        bool can_shoot_p = !pos_has_bullet(p_pos),
             can_shoot_e = !pos_has_bullet(e_pos);
        vector<int> p_moves, e_moves;
        p_moves.reserve(9);
        e_moves.reserve(9);
        if (can_shoot_p)
            p_moves.push_back(8);
        if (can_shoot_e)
            e_moves.push_back(8);

        for (int ii = 0; ii < 4; ii++)
        {
            if (boardf(p_pos + walks[ii]) != '#' && !pos_has_bullet(p_pos + walks[ii]))
                p_moves.push_back(ii);
            if (boardf(e_pos + walks[ii]) != '#' && !pos_has_bullet(e_pos + walks[ii]))
                e_moves.push_back(ii);
            if (boardf(p_pos + walks[ii]) != '#' && can_shoot_p)
                p_moves.push_back(ii + 4);
            if (boardf(e_pos + walks[ii]) != '#' && can_shoot_e)
                e_moves.push_back(ii + 4);
        }

        if (len(p_moves) == 0)
        {
            // cerr << "It's p_moves trap\n";
            p_moves = {8};
        }
        if (len(e_moves) == 0)
        {
            // cerr << "Hihi haha\n";
            e_moves = {8};
        }

        return {p_moves, e_moves};
    }
    pair<int, int> get_random_not_stupid_move()
    {
        auto [p, e] = get_not_stupid_moves_and_update_board();
        return {p[ran() % len(p)], e[ran() % len(e)]};
    }

    void read_board()
    {
        cin >> n >> m;
        char c;
        char P;

        ignore = scanf("\n");
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < 4 * m; j++)
            {
                ignore = scanf("%c", &c);
                if (c == '#')
                    board[i][j / 4] = '#';
                else if (c == ' ' && board[i][j / 4] == 0)
                    board[i][j / 4] = ' ';
                else if (c == 'R')
                    p_pos = pii{i, j / 4};
                else if (c == 'B')
                    e_pos = pii{i, j / 4};
                else if (c == '^')
                    add_bullet(Bullet{pii{i, j / 4}, 0});
                else if (c == 'v')
                    add_bullet(Bullet{pii{i, j / 4}, 1});
                else if (c == '<')
                    add_bullet(Bullet{pii{i, j / 4}, 2});
                else if (c == '>')
                    add_bullet(Bullet{pii{i, j / 4}, 3});
            }
            ignore = scanf("\n");
        }

        cin >> round_num;
        start_round = round_num;

        cin >> P;
        player_color = P;
        if (P == 'B')
            swap(e_pos, p_pos);
    }
};

int get_move(GameState state)
{
    return state.get_random_not_stupid_move().x;
}

int main()
{
    GameState game;

    start_time = get_time_in_microseconds();
    ran = mt19937(start_time);

    game.read_board();

    int move = get_move(game);
    cout << move << "\n";

#ifdef _GLIBCXX_DEBUG
    cerr << player_color << ": " << move_codes[move] << "\n";
#endif
}

/*
*/