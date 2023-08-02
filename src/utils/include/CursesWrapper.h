#pragma once
#include <ncurses.h>

class CursesWrapper
{
public:
    CursesWrapper() = default;

    int Init()
    {
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
