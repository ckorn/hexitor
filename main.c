#include <stdbool.h>
#include <stdlib.h>

#include <ncurses.h>

#define UNDEF -1

typedef enum
{
    SOURCE,
    LAYOUT
} managed_by;

typedef struct
{
    unsigned char render;
    bool active;
    int source_byte;
    int source_nibble;
    managed_by managed_by;
} element;

typedef struct
{
    int left;
    int top;
    int width;
    int height;
} rect;

typedef struct
{
    WINDOW* window;
    int left;
    int top;
    int width;
    int height;
} pane;

void destroy_pane(pane* pane)
{
    wborder(pane->window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(pane->window);
    delwin(pane->window);
}

void setup_pane(pane* pane)
{
    if (pane->window)
    {
        destroy_pane(pane);
    }

    pane->window = newwin(pane->height, pane->width, pane->top, pane->left);
    box(pane->window, 0, 0);
    wrefresh(pane->window);
}

#define SOURCE_LEN 256
#define MAX_OUTPUT_WIDTH 256
#define MAX_OUTPUT_HEIGHT 256

typedef struct
{
    unsigned char source[SOURCE_LEN];

    element elements[MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT];
    int elements_len;

    int cursor_byte;
    int cursor_nibble;
} state;

char nibble_to_hex(unsigned char nibble)
{
    if (nibble < 10)
    {
        return '0' + nibble;
    }

    return 'a' + nibble - 10;
}

void byte_to_hex(unsigned char byte, char* hex)
{
    unsigned char first = byte >> 4;
    unsigned char second = byte & 0x0f;

    hex[0] = nibble_to_hex(first);
    hex[1] = nibble_to_hex(second);
}

int source_to_element(state* state, int source_byte, int source_nibble)
{
    for (int i = 0; i < state->elements_len; i++)
    {
        if (state->elements[i].source_byte == source_byte &&
            state->elements[i].source_nibble == source_nibble)
        {
            return i;
        }
    }

    exit(1);
}

void render_details(state* state, pane* pane)
{
    wclear(pane->window);

    unsigned char* cursor_start = state->source + state->cursor_byte;

    mvwprintw(pane->window, 1, 1, "Offset: %d", state->cursor_byte);
    mvwprintw(pane->window, 2, 1, "Int8:   %d", *(int8_t*)cursor_start);
    mvwprintw(pane->window, 3, 1, "Uint8:  %d", *(uint8_t*)cursor_start);
    mvwprintw(pane->window, 4, 1, "Int16:  %d", *(int16_t*)cursor_start);
    mvwprintw(pane->window, 5, 1, "Uint16: %d", *(uint16_t*)cursor_start);
    mvwprintw(pane->window, 6, 1, "Int32:  %d", *(int32_t*)cursor_start);
    mvwprintw(pane->window, 7, 1, "Uint32: %d", *(uint32_t*)cursor_start);
    mvwprintw(pane->window, 8, 1, "Int64:  %ld", *(int64_t*)cursor_start);
    mvwprintw(pane->window, 9, 1, "UInt64: %ld", *(uint64_t*)cursor_start);

    box(pane->window, 0, 0);
    wrefresh(pane->window);
}

int main()
{
    state state;
    state.cursor_byte = 0;
    state.cursor_nibble = 0;

    for (int i = 0; i < SOURCE_LEN; i++)
    {
       state.source[i] = (unsigned char)i;
    }

    for (int i = 0; i < MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT; i++)
    {
        state.elements[i].active = false;
    }

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    refresh();

    pane hex_pane;
    hex_pane.left = 10;
    hex_pane.top = 3;

    pane detail_pane;
    detail_pane.left = 10;

    int last_max_x;
    int last_max_y;
    int max_x;
    int max_y;

    getmaxyx(stdscr, max_y, max_x);
    last_max_x = -1;
    last_max_y = -1;

    int ch;
    while ((ch = getch()) != KEY_F(1))
    {
        getmaxyx(stdscr, max_y, max_x);

        if (max_y != last_max_y || max_x != last_max_x)
        {
            // Terminal resized
            last_max_x = max_x;
            last_max_y = max_y;

            hex_pane.width = max_x - 20;
            hex_pane.height = max_y - 20;

            hex_pane.width = max_x - 20;
            hex_pane.height = max_y - 20;
            setup_pane(&hex_pane);

            detail_pane.top = hex_pane.top + hex_pane.height + 3;
            detail_pane.width = max_x - 20;
            detail_pane.height = max_y - hex_pane.height - hex_pane.top - 5;
            setup_pane(&detail_pane);
        }

        int element_index;

        switch (ch)
        {
            case KEY_LEFT:
                if (state.cursor_nibble == 1)
                {
                    state.cursor_nibble = 0;
                }
                else
                {
                    state.cursor_nibble = 1;
                    state.cursor_byte--;
                }

                break;

            case KEY_RIGHT:
                if (state.cursor_nibble == 0)
                {
                    state.cursor_nibble = 1;
                }
                else
                {
                    state.cursor_nibble = 0;
                    state.cursor_byte++;
                }

                break;

            case KEY_UP:
                element_index = source_to_element(&state, state.cursor_byte,
                                                  state.cursor_nibble);
                element_index -= hex_pane.width;
                if (element_index >= 0)
                {
                    state.cursor_byte =
                        state.elements[element_index].source_byte;
                }
                break;

            case KEY_DOWN:
                element_index = source_to_element(&state, state.cursor_byte,
                                                  state.cursor_nibble);
                element_index += hex_pane.width;
                if (element_index < state.elements_len)
                {
                    state.cursor_byte =
                        state.elements[element_index].source_byte;
                }
                break;
        }

        if (state.cursor_byte < 0)
        {
            state.cursor_byte = 0;
            state.cursor_nibble = 0;
        }

        if (state.cursor_byte >= SOURCE_LEN)
        {
            state.cursor_byte = SOURCE_LEN - 1;
            state.cursor_nibble = 1;
        }

        int out_x = 0;
        int out_y = 0;
        char hex[2];

        int render_cursor_x = 0;
        int render_cursor_y = 0;

        int element_i = 0;

        for (int i = 0; i < MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT; i++)
        {
            state.elements[i].active = false;
        }

        state.elements_len = 0;
        for (int i = 0; i < SOURCE_LEN; i++)
        {
            unsigned char byte = state.source[i];
            byte_to_hex(byte, hex);

            if (hex_pane.width - out_x < 2)
            {
                // Newline
                while (out_x < hex_pane.width)
                {
                    element_i = out_y * hex_pane.width + out_x;
                    state.elements[element_i].active = false;
                    state.elements_len++;
                    out_x++;
                }

                out_x = 0;
                out_y++;
            }

            if (out_y > hex_pane.height - 1)
            {
                // Out of bounds
                break;
            }

            if (state.cursor_byte == i)
            {
                render_cursor_x = hex_pane.left + out_x;
                render_cursor_y = hex_pane.top + out_y;

                if (state.cursor_nibble == 1)
                {
                    render_cursor_x++;
                }
            }

            element_i = out_y * hex_pane.width + out_x;
            state.elements[element_i].render = hex[0];
            state.elements[element_i].active = true;
            state.elements[element_i].source_byte = i;
            state.elements[element_i].source_nibble = 0;
            state.elements[element_i].managed_by = SOURCE;
            state.elements_len++;
            out_x++;

            element_i = out_y * hex_pane.width + out_x;
            state.elements[element_i].render = hex[1];
            state.elements[element_i].active = true;
            state.elements[element_i].source_byte = i;
            state.elements[element_i].source_nibble = 1;
            state.elements[element_i].managed_by = SOURCE;
            state.elements_len++;
            out_x++;

            if (out_x < hex_pane.width - 1)
            {
                // Room for a space
                element_i = out_y * hex_pane.width + out_x;
                state.elements[element_i].render = ' ';
                state.elements[element_i].active = true;
                state.elements[element_i].source_byte = UNDEF;
                state.elements[element_i].source_nibble = UNDEF;
                state.elements[element_i].managed_by = LAYOUT;
                state.elements_len++;
                out_x++;
            }
        }

        wclear(hex_pane.window);
        for (int y = 0; y < hex_pane.height; y++)
        {
            for (int x = 0; x < hex_pane.width; x++)
            {
                int src = y * hex_pane.width + x;

                if (!state.elements[src].active)
                {
                    continue;
                }

                mvwaddch(hex_pane.window, y, x, state.elements[src].render);
            }
        }
        wrefresh(hex_pane.window);

        render_details(&state, &detail_pane);

        move(render_cursor_y, render_cursor_x);

        refresh();
    }
}
