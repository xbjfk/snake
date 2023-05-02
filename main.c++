
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <ostream>
#include <random>
#include <termios.h>
#include <unistd.h>
#include <utility>

struct termios orig_termios;

constexpr const char ESC = '\x1b';

void disableRawMode() {
	std::cout << ESC << "[?25h";
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);
	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 5;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}



const int xsize = 30;
const int ysize = 15;

enum facing_t {
	up,
	down,
	left,
	right
} facing = up;

enum class direction_t {
	dir_left,
	dir_right,
	dir_up,
	dir_down,

	dir_updown,
	dir_leftright,

	dir_tl,
	dir_tr,
	dir_bl,
	dir_br,
};


struct snake_cell {
	const int x;
	const int y;
	enum direction_t direction;

	void dirChanged(facing_t to) {
		using enum direction_t;
		switch (direction) {
			// TODO: optimize with magic
			case dir_left:
				switch (to) {
					case up:
						direction = dir_bl;
						break;
					case down:
						direction = dir_tl;
						break;
					default:
						direction = dir_leftright;
						break;
				}
				break;
			case dir_right:
				switch (to) {
					case up:
						direction = dir_br;
						break;
					case down:
						direction = dir_tr;
						break;
					default:
						direction = dir_leftright;
						break;
				}
				break;
			case dir_up:
				switch (to) {
					case left:
						direction = dir_tr;
						break;
					case right:
						direction = dir_tl;
						break;
					default:
						direction = dir_updown;
						break;
				}
				break;
			case dir_down:
				switch (to) {
					case left:
						direction = dir_br;
						break;
					case right:
						direction = dir_bl;
						break;
					default:
						direction = dir_updown;
						break;
				}
				break;
			default: break;
		}
	}
};



std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
std::uniform_int_distribution<> x_distr(0, xsize - 1); // define the range
std::uniform_int_distribution<> y_distr(0, ysize - 1); // define the range

bool snake_has_xy(const std::deque<snake_cell>& snake, const int x, const int y) {
	for (const auto& pos : snake) {
		if (pos.x == x && pos.y == y)
			return true;
	}
	return false;
}

void tick(char c) {
	using enum direction_t;
	static std::deque<snake_cell> snake = {{xsize / 2, ysize / 2, dir_up}};
	static bool firsttick = true;
	static int score = 0;
	switch (c) {
		case 'w':
			if (facing != down)
				facing = up;
			break;
		case 'a':
			if (facing != right)
				facing = left;
			break;
		case 's':
			if (facing != up)
				facing = down;
			break;
		case 'd':
			if (facing != left)
				facing = right;
			break;
		default:
			break;
	}

	std::cout << ESC << "[2J";
	std::cout << ESC << "[H";
	std::cout << "\r┌";

	for (int i = 0; i < xsize; i++) {
		std::cout << "─";
	}

	std::cout << "┐\r\n";
	
	for (int i = 0; i < ysize; i++) {
		std::cout << "│" << std::string(xsize, ' ') << "│\r\n";
	}

	std::cout << "└";

	for (int i = 0; i < xsize; i++) {
		std::cout << "─";
	}

	std::cout << "┘";

	auto &head = snake.back();
	head.dirChanged(facing);
	int newx, newy;
	direction_t newdir;
	switch (facing) {
		case up:
			newx = head.x;
			newy = head.y + 1;
			newdir = dir_up;
			break;
		case down:
			newx = head.x;
			newy = head.y - 1;
			newdir = dir_down;
			break;
		case left:
			newx = head.x - 1;
			newy = head.y;
			newdir = dir_left;
			break;
		case right:
			newx = head.x + 1;
			newy = head.y;
			newdir = dir_right;
			break;
	}

	if (
		newx >= xsize || newy >= ysize || newx < 0 || newy < 0
		|| snake_has_xy(snake, newx, newy)
	) {
		exit(1);
	}

	snake.push_back({newx, newy, newdir});

	static int foodx;
	static int foody;
	if (firsttick || (foodx == newx && foody==newy)){
		if (firsttick) {
			firsttick = false;
		} else {
			score++;
		}
		do {
			foodx = x_distr(gen);
			foody = y_distr(gen);
		} while (
			// TODO: maybe use ordered set for ++ performance
			snake_has_xy(snake, foodx, foody) // Food doesn't overlap snake!
		);
	} else {
		snake.pop_front();
	}

	std::cout << "\r\nScore: " << score;
	std::cout << "\r\nKeybinds: [R]eset, [Q]uit";
	std::cout << "\r\nDebug: Food: " << foodx <<", " << foody << " Head: " << newx << ", " << newy;

	for (const auto& pos : snake) {
		std::cout << ESC << '['<< ysize + 1 - pos.y << ';' << pos.x + 2 << 'H';
		switch (pos.direction) {
			case dir_left: std::cout << "╺"; break;
			case dir_right: std::cout << "╸"; break;
			case dir_up: std::cout << "╻"; break;
			case dir_down: std::cout << "╹"; break;

			case dir_updown: std::cout << "┃"; break;
			case dir_leftright: std::cout << "━"; break;

			case dir_tl: std::cout << "┏"; break;
			case dir_tr: std::cout << "┓"; break;
			case dir_bl: std::cout << "┗"; break;
			case dir_br: std::cout << "┛"; break;
		}
	}
	std::cout << ESC << '['<< ysize + 1 - foody << ';' << foodx + 2 << 'H';
	std::cout << "X";

	std::flush(std::cout);
}

int main() {
	enableRawMode();
	std::cout << ESC << "[?25l";
	while (true) {
		char c = '\0';
		read(STDIN_FILENO, &c, 1);
		if (c == 'q') break;
		tick(c);
	}
	return 0;
}

