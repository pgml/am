#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>

int browser_init()
{
	WINDOW *win;
	int ch;

	if ((win = initscr()) == NULL) {
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
	}

	noecho();
	raw();
	keypad(win, true);

	box(stdscr, 0, 0);

	mvprintw(1, 2, "Hi\n");
	mvprintw(2, 2, "you\n");

	refresh();

	while ((ch = getch()) != 'q') {
		switch (ch) {
			case 'k':
				break;

			case 'j':
				break;

			case 'h':
				break;

			case 'l':
				break;
		}

		wrefresh(win);
		refresh();
	}

	delwin(win);
	endwin();
	refresh();

	return EXIT_SUCCESS;
}
