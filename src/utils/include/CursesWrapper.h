#pragma once
#include <ncurses.h>

class CursesWrapper
{
public:
    CursesWrapper() = default;

    int Init()
    {
        printf("Starting ncurses\n");
        if ( (m_mainWindow = initscr()) == nullptr )
        {
            fprintf(stderr, "Error calling initscr()\n");
            return -1;
        }

        keypad(stdscr, TRUE);
        timeout(0);
        nonl();
        noecho();
        m_oldCursor = curs_set(0);
        refresh();

        return 0;
    }

    ~CursesWrapper()
    {
        printf("Stopping ncurses\n");
        // Stops curses and cleans up.
        nl();
        delwin(m_mainWindow);
        curs_set(m_oldCursor);
        endwin();
        refresh();
    }

private:
    WINDOW* m_mainWindow = nullptr;
    int m_oldCursor = 0;
};
