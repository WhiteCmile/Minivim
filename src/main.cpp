#include <ncurses.h>
#include <fstream>
#include <cstring>
#include <vector>

#define back_color 1 //color of background
#define file_color 2 //color of the first window, show user content of the file
#define info_color 3 //color of the secound window, show user some information about the file and the cursor
#define cmd_color 4 //color of the third window, show user the command and some warning

char *file_name, *mode; //file_name refers to the file that user want to edit, mode refers to the argument like "-t", "-R"
WINDOW *win, *win1, *win2, *win_line_number; // win is the first window, win1 is the second window, win2 is the third window, win_line_number shows user the line numbers
std :: vector<char> line[10005]; // these vectors records the characters of each line
std :: vector<char> command; // command records the characters of the command that is shown presently.
std :: vector<std :: vector<char> > command_history, command_history_copy; // command_history is used to record all the commands that user have input, while command_history_copy can be used to switch command by arrow keys
int command_position; // this shows which command is being printed in command_history_copy
std :: fstream my_file; // this used to input and output the characters to the file that used had input
struct Coordinate
{
    int x, y, suppose_y; // suppose_y is used to update cursor's position when using arrow keys.
    // x, y indicates now the cursor is on which line and which character
    Coordinate() { x = y = suppose_y = 0; }
    void add(int delta_x, int delta_y) { x += delta_x, y += delta_y; }
    void Update_Y() { suppose_y = y; }
} cursor, command_cursor; // cursor is the position of cursor in normal mode and insert mode, command_cursor is the position of cursor in command mode

namespace Init_All_The_Windows
{    
    int line_number, column_number, last_line_up_limit, line_down_limit, line_up_limit, column_left_limit, column_right_limit, max_line_number;
    // line_number is the line number of win, column_number is the column number of win, while the limits shows which lines and columns should be printed. max_line_number records the number of the last line that have characthers input.
    int input_mode; // input_mode shows which mode now user is in. input_mode is 0, 1, 2, showing user is in normal, insert, command respectively.
    bool modify_flag, file_exist_flag, read_only_flag, write_flag, last_input_d_flag; 
    // modify_flag records if the file is modified but unsaved, read_only_flag indicates if the file is permitted to be modified, write_flag shows if the file can be rewrited. last_input_d_flag is used in "dd" shortcut key.
    void Init_Windows()
    {
        initscr(), raw(), noecho();

        // init color
        start_color();
        init_pair(back_color, COLOR_WHITE, COLOR_BLACK);
        init_pair(file_color, COLOR_WHITE, COLOR_BLACK);
        init_pair(info_color, COLOR_WHITE, COLOR_CYAN);
        init_pair(cmd_color, COLOR_WHITE, COLOR_RED);

        // set stdscr color
        wbkgd(stdscr, COLOR_PAIR(back_color));
        keypad(stdscr, 1), wrefresh(stdscr);

        line_number = LINES, column_number = COLS;
        if (line_number < 10) {
            fprintf(stderr, "window line size is small than 8.");
            endwin();
            exit(0);
        }
        // set initial limits of win
        line_down_limit = line_number - 3, line_up_limit = 0;
        last_line_up_limit = -1;
        column_left_limit = 0, column_right_limit = column_number - 7;
        command_cursor.x = 0;

        // create new windows.
        win_line_number = newwin(line_number - 2, 4, 0, 0);
        win = newwin(line_number - 2, COLS - 6, 0, 6);
        win1 = newwin(1, COLS, line_number - 2, 0);
        win2 = newwin(1, COLS, line_number - 1, 0);
        wbkgd(win_line_number, COLOR_PAIR(file_color)), keypad(win_line_number, 1);
        wbkgd(win, COLOR_PAIR(file_color)), keypad(win, 1);
        wbkgd(win1, COLOR_PAIR(info_color)), keypad(win1, 1);
        wbkgd(win2, COLOR_PAIR(cmd_color)), keypad(win2, 1);
        wrefresh(win), wrefresh(win1), wrefresh(win2), wrefresh(win_line_number);
    }
}
using namespace Init_All_The_Windows;

namespace Screen_Update_And_Cursor_Update
{
    void Print_Cursor_Position() // used to display information in this form: [Mode], Filename, Cursor line and column
    {
        wclear(win1);
        switch(input_mode)
        {
            case 0 : wprintw(win1, "[Normal Mode]   "); break;
            case 1 : wprintw(win1, "[Insert Mode]   "); break;
            case 2 : wprintw(win1, "[Command Mode]   "); break;
        }
        wprintw(win1, "%s     [%d, %d]", file_name, cursor.x, cursor.y);
        wrefresh(win1);
    }
    void Update_Cursor_Position() // move cursor to its right position in win
    {
        Print_Cursor_Position();
        wmove(win, cursor.x - line_up_limit, cursor.y - column_left_limit);
        wrefresh(win);
    }
    void Print_Line_Number_To_Screen() // show people the line numbers
    {
        if(last_line_up_limit == line_up_limit) return;
        wclear(win_line_number);
        for(int i = 0; i < line_number - 2; i++)
        {
            wmove(win_line_number, i, 3);
            int x = i + line_up_limit + 1, pos = 3;
            while(x)
            {
                wprintw(win_line_number, "%d", x % 10);
                x /= 10, pos--;
                wmove(win_line_number, i, pos);
            }
        }
        last_line_up_limit = line_up_limit;
        wrefresh(win_line_number);
    }
    void Print_To_Screen() // print all valid characters on the screen
    {
        wclear(win2), wrefresh(win2);
        Print_Line_Number_To_Screen();
        wmove(win, 0, 0), wclear(win);
        for(int i = line_up_limit; i <= std :: min(max_line_number, line_down_limit); i++)
        {
            bool flag = 0, flag1 = 0;
            for(int j = column_left_limit; j <= std :: min(column_right_limit, (int) line[i].size() - 1); j++)
                flag = 1, flag1 |= line[i][j] == '\n', wprintw(win, "%c", line[i][j]);
            if(!flag && i < std :: min(max_line_number, line_down_limit)) wmove(win, i - line_up_limit + 1, 0);
        }
        Update_Cursor_Position();
    }
    void Print_Page_Up_Down(int opt) // move the window up or down because the motion of cursor
    {
        line_up_limit += opt, line_down_limit += opt;
        Print_To_Screen();
    }
    void Print_Page_Left_Right(int opt) // move the window left or right because the motion of cursor
    {
        column_left_limit += opt, column_right_limit += opt;
        Print_To_Screen();
    }
    bool Check_Cursor() // check if the cursor is beyond the limits
    {
        bool flag = 1;
        if(cursor.x < line_up_limit) Print_Page_Up_Down(cursor.x - line_up_limit), flag = 0;
        if(cursor.x > line_down_limit) Print_Page_Up_Down(cursor.x - line_down_limit), flag = 0;
        if(cursor.y < column_left_limit) Print_Page_Left_Right(cursor.y - column_left_limit), flag = 0;
        if(cursor.y > column_right_limit) Print_Page_Left_Right(cursor.y - column_right_limit), flag = 0;
        return flag;
    }
}
using namespace Screen_Update_And_Cursor_Update;

namespace Input_Output_process
{
    int Check_Input_Char(int ch) // figure out the chars that user input, and execute corresponding functions
    {
        if(ch == 27) return 0;
        if(ch == 9 || ch == 10 || ch == 13) return 1;
        if(ch >= 32 && ch <= 57) return 1;
        if(ch >= 58 && ch <= 126) return 1;
        if(ch == KEY_BACKSPACE) return 2;
        if(ch == KEY_UP) return 3;
        if(ch == KEY_DOWN) return 4;
        if(ch == KEY_LEFT) return 5;
        if(ch == KEY_RIGHT) return 6;
        if(ch == KEY_HOME) return 7;
        if(ch == KEY_END) return 8;
        if(ch == KEY_DC) return 9;
    }
    void Input_File_Process() // used to process file input
    {
        read_only_flag = mode[1] == 'R';
        write_flag = mode[1] == 'w';
        if(!read_only_flag && !write_flag) my_file.open(file_name, std :: ios :: in | std :: ios :: trunc);
        else my_file.open(file_name, std :: ios :: in);
        my_file >> std :: noskipws;
        Print_Line_Number_To_Screen();
        if(my_file.is_open())
        {
            file_exist_flag = 1;
            char ch; int flag;
            while(my_file >> ch)
            {
                line[cursor.x].push_back(ch), cursor.y++;
                if(ch == '\n') cursor.x++, cursor.y = 0, max_line_number = cursor.x;
            }
            if(!line[cursor.x].size()) cursor.x--, max_line_number = cursor.x;
        }
        cursor.Update_Y();
        my_file.close();
    }
    void Output_File_Process(bool flag) // used to process file output
    {
        my_file.open(file_name, std :: ios :: out | std :: ios :: trunc);
        for(int i = 0; i <= max_line_number; i++)
            for(auto ch : line[i]) my_file << ch;
        my_file.close();
        if(!flag) 
        {
            endwin();
            exit(0);
        }
    }
}
using namespace Input_Output_process;

namespace Mode_Transfer
{
    void Change_Mode_To_Normal() // change current mode to normal mode
    {
        if(input_mode == 2) command_cursor.y = 0, command.clear();
        input_mode = 0;
        Update_Cursor_Position();
    }
    void Change_Mode_To_Insert() // change current mode to insert mode
    {
        if(read_only_flag) 
        {
            wclear(win2), wprintw(win2, "The file is read only, you can't modify it."), wrefresh(win2);
            return Update_Cursor_Position();
        }
        input_mode = 1;
        Update_Cursor_Position();
    }
}
using namespace Mode_Transfer;

namespace Insert_Process
{
    void Line_Add_Char(int ch) // add a char in any position of a line
    {
        line[cursor.x].push_back(ch);
        for(int i = line[cursor.x].size() - 1; i > cursor.y; i--) line[cursor.x][i] = line[cursor.x][i - 1];
        line[cursor.x][cursor.y] = ch;
        wmove(win, cursor.x - line_up_limit, 0);
        for(int i = column_left_limit; i <= std :: min(column_right_limit, (int) line[cursor.x].size() - 1); i++) wprintw(win, "%c", line[cursor.x][i]);
    }
    void Line_Delete_Char(int pos) // delete a char in any position of a line
    {
        for(int i = pos; i < line[cursor.x].size() - 1; i++) line[cursor.x][i] = line[cursor.x][i + 1];
        line[cursor.x].pop_back();
    }
    void Print_Insert_Character(int ch) // process the char that user input, update the window and information that the program stores
    {
        modify_flag = 1;
        if(ch == 9)
        {
            for(int i = 0; i < 4; i++)
            {
                Line_Add_Char(' '), cursor.y++;
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
        else if(ch != 10 && ch != 13)
        {
            Line_Add_Char(ch), cursor.y++;
            if(Check_Cursor()) Update_Cursor_Position();
        }
        else
        {
            Line_Add_Char('\n'), cursor.y++;
            if(cursor.y != line[cursor.x].size())
            {
                max_line_number++;
                for(int i = max_line_number; i >= cursor.x + 2; i--) line[i] = line[i - 1];
                line[cursor.x + 1].clear();
                for(int i = cursor.y; i < line[cursor.x].size(); i++) line[cursor.x + 1].push_back(line[cursor.x][i]);
                for(int i = line[cursor.x].size() - 1; i >= cursor.y; i--) line[cursor.x].pop_back();
            }
            else if(line[cursor.x].size() > 1 && line[cursor.x][cursor.y - 1] == '\n' && line[cursor.x][cursor.y - 2] == '\n')
            {   
                max_line_number++;
                for(int i = max_line_number; i >= cursor.x + 2; i--) line[i] = line[i - 1];
                line[cursor.x + 1].clear(), line[cursor.x + 1].push_back('\n');
                line[cursor.x].pop_back();
            }
            cursor.x++, cursor.y = 0;
            Print_To_Screen();
            max_line_number = std :: max(max_line_number, cursor.x);
            if(Check_Cursor()) Update_Cursor_Position();
        }
        cursor.Update_Y();
    }
    void Print_Insert_Backspace() // process when user press "Backspace" in insert mode
    {
        modify_flag = 1;
        if(cursor.y)
        {
            Line_Delete_Char(cursor.y - 1), cursor.y--, Print_To_Screen();
            if(Check_Cursor()) Update_Cursor_Position();
        }
        else
        {
            if(cursor.x)
            {
                cursor.y = line[cursor.x - 1].size() - 1, line[cursor.x - 1].pop_back();
                for(auto c : line[cursor.x]) line[cursor.x - 1].push_back(c);
                for(int i = cursor.x; i < max_line_number; i++) line[i] = line[i + 1];
                line[max_line_number].clear(), max_line_number--;
                cursor.x--, Print_To_Screen();
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
        cursor.Update_Y();
    }
    void Print_Insert_Delete() // process when user press "Delete" in insert mode
    {
        modify_flag = 1;
        if(cursor.y < line[cursor.x].size() - 1)
        {
            Line_Delete_Char(cursor.y), Print_To_Screen();
            if(Check_Cursor()) Update_Cursor_Position();
        }
        else if(cursor.y == line[cursor.x].size() - 1)
        {
            if(cursor.x != max_line_number)
            {
                line[cursor.x].pop_back();
                for(auto c : line[cursor.x + 1]) line[cursor.x].push_back(c);
                for(int i = cursor.x + 1; i < max_line_number; i++) line[i] = line[i + 1];
                line[max_line_number].clear(), max_line_number--;
                Print_To_Screen();
                if(Check_Cursor()) Update_Cursor_Position();
            }
            else 
            {
                Line_Delete_Char(cursor.y), Print_To_Screen();
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
        cursor.Update_Y();
    }
}
using namespace Insert_Process;

namespace Command_Process
{
    void Warn_User_Of_Unsave() // when user input ":q", check if the file is saved
    {
        wclear(win2), wprintw(win2, "You have modified the file but you did not save it!"), wrefresh(win2);
        Change_Mode_To_Normal();
    }
    void Warn_User_Of_No_Such_Command() // when user input a invalid command, warn user that the command is not permitted
    {
        wclear(win2), wprintw(win2, "There is no such command!"), wrefresh(win2);
        Change_Mode_To_Normal();
    }
    void Warn_User_Of_Exceeding_Max_Line_Number() // user can't jump to line whose number exceeds the max_line_number
    {
        wclear(win2), wprintw(win2, "The line number you input exceeds the max line number of the file!"), wrefresh(win2);
        Change_Mode_To_Normal();
    }
    void Jump_To_Line(int pos) // jump to the given line
    {
        cursor.x = pos, cursor.y = 0;
        Print_Page_Up_Down(pos - line_up_limit);
        if(Check_Cursor()) Update_Cursor_Position();
    }
    bool Check_Command_Jump() // check if the jump command is valid
    {
        if(command.size() <= 5) return 0;
        bool flag = (command[1] == 'j') & (command[2] == 'm') & (command[3] == 'p'), flag1 = 1;
        if(!flag) return 0;
        int x = 0;
        for(int i = 5; i < command.size(); i++)
        {
            flag &= std :: isdigit(command[i]);
            if(!flag) break;
            x = x * 10 + command[i] - '0';
            if(x >= 10000) x = 0, flag1 = 0;
        }
        if(!flag) return 0;
        if(!flag1 || x - 1 > max_line_number) Warn_User_Of_Exceeding_Max_Line_Number();
        else Jump_To_Line(x - 1);
        Change_Mode_To_Normal();
        return 1;
    }
    void Substitute_Words(int l1, int r1, int l2, int r2) // substitute the word with another word that user input
    {
        int len1 = r1 - l1 + 1, len2 = r2 - l2 + 1;
        std :: vector<int> begin_pos;
        std :: vector<char> new_line;
        for(int x = 0; x <= max_line_number; x++)
        {
            int pos = l1;
            for(int i = 0; i < line[x].size(); i++)
            {
                if(line[x][i] == command[pos]) pos++;
                else pos = l1;
                if(pos == r1 + 1) begin_pos.push_back(i - len1 + 1);
            }
            begin_pos.push_back(line[x].size());
            if(begin_pos.size() == 1) { begin_pos.clear(); continue; }
            pos = 0;
            for(auto y : begin_pos)
            {
                while(pos < y) new_line.push_back(line[x][pos]), pos++;
                if(y != line[x].size()) 
                    for(int i = l2; i <= r2; i++) new_line.push_back(command[i]);
                pos = y + len1;
            }
            line[x] = new_line;
            begin_pos.clear(), new_line.clear();
        }
        wclear(win2), wprintw(win2, "Substitution has succeeded!"), wrefresh(win2);
        cursor.x = 0, cursor.y = 0, cursor.suppose_y = 0;
        Print_To_Screen();
        if(Check_Cursor()) Update_Cursor_Position();
        Change_Mode_To_Normal();
    }
    bool Check_Command_Substitute() // check if the substitute command is valid
    {
        if(command.size() <= 10) return 0;
        bool flag = (command[1] == 's') & (command[2] == 'u') & (command[3] == 'b') & (command[4] == ' ') & (command[5] == '"');
        if(!flag) return 0;
        int init_word_l = 6, init_word_r = 0, sub_word_l = 0, sub_word_r = 0;
        for(int i = 6; i < command.size(); i++) 
            if(command[i] == '"') { init_word_r = i - 1; break; }
        if(!init_word_r) return 0;
        if((command.size() - 1) - (init_word_r + 2) + 1 < 3) return 0;
        flag &= (command[init_word_r + 2] == ' ') & (command[init_word_r + 3] == '"');
        if(!flag) return 0;
        sub_word_l = init_word_r + 4;
        for(int i = sub_word_l; i < command.size(); i++)
            if(command[i] == '"') { sub_word_r = i - 1; break; }
        if(sub_word_r + 1 != command.size() - 1) return 0;
        if(read_only_flag)
        {
            wclear(win2), wprintw(win2, "The file is read only, you can't modify it."), wrefresh(win2);
            Change_Mode_To_Normal(); 
            return 1;
        }
        Substitute_Words(init_word_l, init_word_r, sub_word_l, sub_word_r);
        return 1;
    }
    void Check_Command() // check which command that user input
    {
        command_history.push_back(command);
        if(command.size() == 2)
        {
            if(command[1] == 'w') 
            {
                Output_File_Process(1);
                wclear(win2), wrefresh(win2);
                Change_Mode_To_Normal();
                modify_flag = 0;
            }
            else if(command[1] == 'q') 
            {
                if(!modify_flag) Output_File_Process(0);
                else Warn_User_Of_Unsave();
            }
            return;
        }
        else if(command.size() == 3)
        {
            if(command[1] == 'w' && command[2] == 'q') Output_File_Process(0);
            else if(command[1] == 'q' && command[2] == '!') { endwin(); exit(0); }
            return;
        }
        else
        {
            if(Check_Command_Jump()) return;
            if(Check_Command_Substitute()) return;
        }
        Warn_User_Of_No_Such_Command();
    }
    void Print_Command_To_Screen() // print the command that user choose and input on the screen
    {
        wclear(win2), wmove(win2, command_cursor.x, 0);
        for(auto ch : command) wprintw(win2, "%c", ch);
        wmove(win2, command_cursor.x, command_cursor.y), wrefresh(win2);
    }
    void Print_Command_Backspace() // process when user press "Backspace" in command_mode
    {
        if(command_cursor.y == 1) return;
        for(int i = command_cursor.y - 1; i < command.size() - 1; i++) command[i] = command[i + 1];
        command.pop_back();
        command_cursor.y--;
        Print_Command_To_Screen();
    }
    void Print_Command_Character(char ch) // update the window and information when user input a new character
    {
        if(ch == '\n') return Check_Command();
        command.push_back(ch);
        for(int i = command.size() - 1; i > command_cursor.y; i--) command[i] = command[i - 1];
        command[command_cursor.y] = ch;
        command_cursor.y++;
        Print_Command_To_Screen();
    }
    void Print_Command_Key(int opt) // update cursor's position when user press arrow keys in command mode
    {
        if(command_cursor.y + opt <= 0 || command_cursor.y + opt > command.size()) return;
        command_cursor.y += opt;
        wmove(win2, command_cursor.x, command_cursor.y);
        wrefresh(win2);
    }
    void Print_Command_Home() // update cursor's position when user press "home" in command mode
    {
        command_cursor.y = 1;
        wmove(win2, command_cursor.x, command_cursor.y);
        wrefresh(win2);
    }
    void Print_Command_End() // update cursor's position when user press "end" in command mode
    {
        command_cursor.y = command.size();
        wmove(win2, command_cursor.x, command_cursor.y);
        wrefresh(win2);
    }
    void Print_Command_Delete() // update window and information when useer press "delete" in command mode
    {
        if(command_cursor.y == command.size()) return;
        wprintw(win2, "%c", '!');
        for(int i = command_cursor.y; i < command.size() - 1; i++) command[i] = command[i + 1];
        command.pop_back();
        Print_Command_To_Screen();
    }
    void Print_Command_Up() // browse command previously entered by arrow keys in command mode
    {
        if(!command_position) return;
        command_position--, command = command_history_copy[command_position];
        command_cursor.y = command.size();
        Print_Command_To_Screen();
    }
    void Print_Command_Down() // browse command previously entered by arrow keys in command mode
    {
        if(command_position == command_history_copy.size() - 1) return;
        command_position++, command = command_history_copy[command_position];
        command_cursor.y = command.size();
        Print_Command_To_Screen();
    }
}
using namespace Command_Process;

namespace Cursor_Move
{
    int Find_Column_Limit() // return the max number that cursor.y can be in this line
    {
        if(line[cursor.x].size() == 1 && *--line[cursor.x].end() == '\n') return 0;
        if(input_mode == 0)
            return line[cursor.x].size() - 1 - (*--line[cursor.x].end() == '\n');
        else return line[cursor.x].size() - (*--line[cursor.x].end() == '\n');
    }
    void Print_Key_Up() // process when user press arrow key up
    {
        last_input_d_flag = 0;
        if(input_mode == 2) return Print_Command_Up();
        if(cursor.x)
        {
            cursor.x--;
            if(Find_Column_Limit() < cursor.suppose_y)
            {
                cursor.y = Find_Column_Limit();
                if(Check_Cursor()) Update_Cursor_Position();
                if(cursor.y < column_number) Print_Page_Left_Right(-column_left_limit);
            }
            else 
            {
                cursor.y = cursor.suppose_y;
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
    }
    void Print_Key_Down() // process when user press arrow key down
    {
        last_input_d_flag = 0;
        if(input_mode == 2) return Print_Command_Down();
        if(cursor.x < max_line_number)
        {
            cursor.x++;
            if(Find_Column_Limit() < cursor.suppose_y)
            {
                cursor.y = Find_Column_Limit();
                if(Check_Cursor()) Update_Cursor_Position();
                if(cursor.y < column_number) Print_Page_Left_Right(-column_left_limit);
            }
            else 
            {
                cursor.y = cursor.suppose_y;
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
    }
    void Print_Key_Left() // process when user press arrow key left
    {
        last_input_d_flag = 0;
        if(input_mode == 2) return Print_Command_Key(-1);
        if(!cursor.y)
        {
            if(cursor.x) cursor.x--, cursor.y = Find_Column_Limit();
        }
        else cursor.y--;
        if(Check_Cursor()) Update_Cursor_Position();
        cursor.Update_Y();
    }
    void Print_Key_Right() // process when user press arrow key right
    {
        last_input_d_flag = 0;
        if(input_mode == 2) return Print_Command_Key(1);
        if(cursor.y == Find_Column_Limit())
        {
            if(cursor.x != max_line_number) cursor.y = 0, cursor.x++;
        }
        else if(cursor.y < Find_Column_Limit()) cursor.y++;
        if(Check_Cursor()) Update_Cursor_Position();
        cursor.Update_Y();
    }
    void Print_Key_Home() // process when user press "home"
    {
        last_input_d_flag = 0;
        if(input_mode == 2) return Print_Command_Home();
        cursor.y = 0;
        if(Check_Cursor()) Update_Cursor_Position();
        cursor.Update_Y();
    }
    void Print_Key_End() // process when user press "end"
    {
        last_input_d_flag = 0;
        if(input_mode == 2) return Print_Command_End();
        cursor.y = line[cursor.x].size() - (!input_mode) - (line[cursor.x][line[cursor.x].size() - 1] == '\n');
        if(Check_Cursor()) Update_Cursor_Position();
        cursor.Update_Y();
    }
    void Move_Forward_One_Word() // move forward one word in normal mode by inputting shortcut key
    {
        int pos = -1;
        for(int i = cursor.y; i < line[cursor.x].size(); i++)
            if(line[cursor.x][i] == ' ') { pos = i; break; }
        if(pos + 1 > line[cursor.x].size() - 1 || line[cursor.x][pos + 1] == '\n' || pos == -1)
        {
            if(cursor.x == max_line_number) return;
            cursor.x++, cursor.y = 0;
        }
        else cursor.y = pos;
        bool flag = 1;
        while(flag)
        {
            if(line[cursor.x][cursor.y] != ' ' && line[cursor.x][cursor.y] != '\n') break;
            if(cursor.y == line[cursor.x].size() - 1)
            {
                if(cursor.x == max_line_number) break;
                else cursor.x++, cursor.y = 0;
            }
            else cursor.y++;
        }
        if(Check_Cursor()) Update_Cursor_Position();
        cursor.Update_Y();
    }
    void Move_Back_One_Word() // move back one word in normal mode by inputting shortcut key
    {
        int pos = -1;
        for(int i = cursor.y; i >= 0; i--)
            if(line[cursor.x][i] == ' ') { pos = i; break; }
        int rem = pos; pos = -1;
        for(int i = rem; i >= 0; i--)
            if(line[cursor.x][i] != ' ') { pos = i; break; }
        if(pos == -1)
        {
            rem = cursor.x;
            while(1)
            {
                if(!cursor.x) { cursor.x = rem; return; }
                cursor.x--;
                bool flag = 1;
                for(auto c : line[cursor.x]) flag &= (c == ' ') || (c == '\n');
                if(!flag)
                {
                    for(int i = line[cursor.x].size() - 1; i >= 0; i--)
                        if(line[cursor.x][i] != ' ' && line[cursor.x][i] != '\n') { cursor.y = i; break; }
                    break;
                }
            }
        }
        else cursor.y = pos;
        bool flag = 0;
        for(int i = cursor.y; i >= 0; i--) 
            if(line[cursor.x][i] == ' ') { flag = 1; cursor.y = i + 1; break; }
        if(!flag) cursor.y = 0;
        if(Check_Cursor()) Update_Cursor_Position();
        cursor.Update_Y();
    }
}
using namespace Cursor_Move;

void Delete_The_Entire_Line() // delete the entire line when user input shortcut key "dd" in normal mode
{
    if(cursor.x == max_line_number)
    {
        line[cursor.x].clear();
        if(max_line_number) max_line_number--, cursor.x--;
    }
    else
    {
        for(int i = cursor.x; i < max_line_number; i++) line[i] = line[i + 1];
        line[max_line_number].clear(), max_line_number--;
    }
    cursor.y = 0, Print_To_Screen();
    if(Check_Cursor()) Update_Cursor_Position();
    cursor.Update_Y();
}

namespace Process_Input_Keys
{
    void Print_Input(int ch) // check the character that user input
    {
        if(!input_mode) 
        {
            if(ch == 'i') return Change_Mode_To_Insert();
            if(ch == ':') 
            {
                input_mode = 2;
                Print_Cursor_Position(), Print_Command_Character(ch);
                command_history_copy = command_history;
                command_history_copy.push_back(command);
                command_position = command_history_copy.size() - 1;
                return;
            }
            if(ch == '$') { Print_Key_End(); last_input_d_flag = 0; return; }
            if(ch == 'O') { Print_Key_Home(); last_input_d_flag = 0; return; }
            if(ch == 'd') 
            {
                if(!last_input_d_flag) last_input_d_flag = 1;
                else
                {
                    if(read_only_flag)
                    {
                        wclear(win2), wprintw(win2, "The file is read only, you can't modify it."), wrefresh(win2);
                        Update_Cursor_Position(), last_input_d_flag = 0;
                        return;
                    }
                    Delete_The_Entire_Line();
                    last_input_d_flag = 0;
                }
                return;
            }
            if(ch == 'w') { Move_Forward_One_Word(), last_input_d_flag = 0; return; }
            if(ch == 'b') { Move_Back_One_Word(), last_input_d_flag = 0; return; }
            last_input_d_flag = 0;
            return;
        }
        else if(input_mode == 2) return Print_Command_Character(ch);
        else return Print_Insert_Character(ch);
    }
    void Print_Backspace() // process when user press "Backspace"
    {
        last_input_d_flag = 0;
        if(!input_mode) return;
        else if(input_mode == 2) return Print_Command_Backspace();
        else return Print_Insert_Backspace();
    }
    void Print_Key_Esc() // process when user press "Esc"
    {
        last_input_d_flag = 0;
        if(!input_mode) return;
        Change_Mode_To_Normal();
    }
    void Print_Key_Delete() // process when user press "delete"
    {
        last_input_d_flag = 0;
        if(!input_mode) return;
        else if(input_mode == 2) return Print_Command_Delete();
        else return Print_Insert_Delete();
    }
}
using namespace Process_Input_Keys;

int main(int argc, char *argv[])
{ 
    mode = argv[1], file_name = argv[2];

    Init_Windows();

    Input_File_Process();

    cursor.x = cursor.y = cursor.suppose_y = 0;
    Print_To_Screen();

    int ch = 0, flag;
    while (1) 
    {
        ch = getch(), flag = Check_Input_Char(ch);
        if(input_mode != 2) wclear(win2), wrefresh(win2), Update_Cursor_Position();
        switch(flag)
        {
            case 0: Print_Key_Esc(); break;
            case 1: Print_Input(ch); break;
            case 2: Print_Backspace(); break;
            case 3: Print_Key_Up(); break;
            case 4: Print_Key_Down(); break;
            case 5: Print_Key_Left(); break;
            case 6: Print_Key_Right(); break;
            case 7: Print_Key_Home(); break;
            case 8: Print_Key_End(); break;
            case 9: Print_Key_Delete(); break;
        }
    }

    endwin(); /* End curses mode */
    return 0;
}
