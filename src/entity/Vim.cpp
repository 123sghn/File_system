#include "Vim.h"

Vim::Vim(bool readonly, const string s) : s(s), readonly(readonly) {
    cutBoard = "";
    curState = 0;
    quit = false;
    isSave = false;
    error = false;
    getsize();

    for (int i = 1; i < sizy - dy; i++)spaces += ' ';
    string tmp = "";
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i] == '\n') {
            vs.push_back(tmp);
            tmp.clear();
        }
        else tmp += s[i];
    }
    vs.push_back(tmp);
    vs_save = s;
    init_vs = vs;
    //设置初始的光标位置：文件末尾
    curx = std::min(sizx - 2, (int)vs.size() - 1), cury = vs.back().size() + dy;
    r = vs.size() - 1, l = std::max(0, r - sizx + 2);
    //        cout<<l<<' '<<r<<endl;
    //        getchar();
}

Vim::Vim() { //构造函数
    isSave = false;
    quit = false;
    readonly = false;
    curx = 0, cury = dy;
    cutBoard = "";
    curState = 0;
    for (int i = 0; i < sizy - dy - 1; i++)spaces += ' ';
    vs.push_back("");
    vs_save = "";
    init_vs = vs;
    l = r = 0;
}

void Vim::hideCursor() { //隐藏光标
    CONSOLE_CURSOR_INFO cursor;
    cursor.bVisible = 0;
    cursor.dwSize = 1;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorInfo(hOut, &cursor);
}

void Vim::showCursor() { //显示光标
    CONSOLE_CURSOR_INFO cursor;
    cursor.bVisible = 1;
    cursor.dwSize = 1;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorInfo(hOut, &cursor);
}

void Vim::gotoxy(int x, int y) { //建立函数 移动光标
    COORD position;
    position.Y = x;
    position.X = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), position);
}

void Vim::changePos(int p) {
    if (p == 0 && curx == 0) {
        if (l) {
            l--;
            if (cury > vs[l].size())cury = vs[l].size() + dy;
            if (r - l >= sizx - 1)r--;
        }
    }
    else if (p == 1 && curx == sizx - 2) {
        if (r < vs.size() - 1) {
            l++, r++;
            if (cury > vs[r].size())cury = (int)vs[r].size() + dy;
        }
    }

    else {
        curx += dir[p].x;
        cury += dir[p].y;
        //限制光标位置
        if (curx < 0 || cury < dy || curx + l >= vs.size()) {
            curx -= dir[p].x;
            cury -= dir[p].y;
        }
        else if (cury > vs[curx + l].size() + dy) {
            cury = vs[curx + l].size() + dy;
        }
    }

    gotoxy(curx, cury);
}

string Vim::merge_string() {
    string ans = "";
    for (auto& x : vs) {
        ans += x;
        ans += '\n';
    }
    return ans;
}

pair<bool, string> Vim::exec() { //从键盘中读取一个字母
    print();
    bool dirdel;
    int ch;
    while (ch = _getch()) {
        cnt++;
        //error=false;
        errortp = "";
        getsize();
        dirdel = false;
        if (ch == 224) dirdel = true;
        //            cout<<"char:"<<endl;

        if (dirdel) {
            ch = _getch();
            if (error) {
                curState = 0;
                hideCursor();

                gotoxy(sizx - 1, 0);
                cout << spaces;
                curx = 0, cury = dy;
                gotoxy(curx, cury);
                showCursor();

                error = false;
                errortp = "";
                print(error, errortp);
                continue;
            }

            if (ch == upArrow)changePos(0);
            else if (ch == downArrow)changePos(1);
            else if (ch == leftArrow)changePos(2);
            else if (ch == rightArrow) { changePos(3); }
            else if (ch == del && curState == 1)insert_cmd_mode(ch, true);
        }

        else if (error) {
            curState = 0;
            hideCursor();

            gotoxy(sizx - 1, 0);
            cout << spaces;
            curx = 0, cury = dy;
            gotoxy(curx, cury);
            showCursor();

            error = false;
            errortp = "";
            print(error, errortp);
            continue;
        }

        else if (curState == 0) {
            normal_cmd_mode(ch);
        }
        else if (curState == 1) {
            insert_cmd_mode(ch);
        }
        else if (curState == 2) {
            advanced_cmd_mode(ch);
        }

        if (quit)break;

        print(error, errortp);
    }
    return std::make_pair(isSave, vs_save);

}

void Vim::normal_cmd_mode(int ch) { //普通命令模式
    if (ch == 'd') { //剪切
        cutBoard = vs[curx + l];
        if (vs.size() != 1)vs.erase(vs.begin() + l + curx);
        else vs[0].clear();
        cury = dy;

        if (curx + l == vs.size())curx--;
        if (l)l--, r--;
        else if (r == vs.size())r--;

    }

    else if (ch == 'y') { //复制
        cutBoard = vs[curx + l];
    }
    else if (ch == 'p') { //粘贴
        vs.insert(vs.begin() + curx + 1 + l, cutBoard);
        if (r - l < sizx - 2)r++;
    }
    else if (ch == 'x') { //删除当前光标指向的东西
        if (cury == dy) {
            if (curx == 0 && l == 0)
                return;
            cury = vs[curx + l - 1].size() + dy;
            vs[curx + l - 1] += vs[curx + l];

            vs.erase(vs.begin() + curx + l);

            if (l)l--, r--;
            else {
                if (r == vs.size())r--;
                if (curx)curx--;
            }

            return;

        }

        vs[curx + l].erase(vs[curx + l].begin() + cury - dy - 1);
        cury--;
    }
    else if (ch == '0') { //光标移动到开头
        cury = dy;
    }
    else if (ch == '$') {
        cury = vs[curx + l].size() + dy;
    }
    else if (ch == 'i') { //当前位置插入

        curState = 1;
    }
    else if (ch == 'a') { //后一个位置插入
        curState = 1;
        if (cury < vs[curx + l].size() + dy)cury++;
    }
    else if (ch == ':') { //高级命令模式
        curState = 2;
        curx = sizx - 1;
        cury = 1;
    }
}

void Vim::insert_cmd_mode(int ch, bool fg) { //插入模式
    if (ch == esc) { //退出到普通模式，同时把下面的"-insert-"字样给覆盖
        curState = 0;
        hideCursor();

        gotoxy(sizx - 1, 0);
        cout << spaces;
        gotoxy(curx, cury);
        showCursor();
    }

    else if (ch == backspace) { //删除当前字符
        if (cury == dy) {
            if (curx == 0 && l == 0)
                return;
            cury = vs[curx + l - 1].size() + dy;
            vs[curx + l - 1] += vs[curx + l];

            vs.erase(vs.begin() + curx + l);

            if (l)l--, r--;
            else {
                if (r == vs.size())r--;
                if (curx)curx--;
            }
            return;
        }

        vs[curx + l].erase(vs[curx + l].begin() + cury - dy - 1);
        cury--;
    }
    else if (fg && ch == del) { //删除后面一个字符
        if (cury == vs[curx + l].size() + dy)return;
        vs[curx + l].erase(vs[curx + l].begin() + cury - dy);
    }

    else if (ch == enter) {
        string nexs = vs[curx + l].substr(cury - dy, vs[curx + l].size() - cury + dy);
        string curs = vs[curx + l].substr(0, cury - dy);
        vs.erase(vs.begin() + curx + l);
        vs.insert(vs.begin() + curx + l, curs);
        vs.insert(vs.begin() + curx + 1 + l, nexs);
        curx++, cury = dy;

        if (r - l <= sizx - 3)r++;

        if (curx == sizx - 1)curx--, l++, r++;

    }
    else if (0 <= ch && ch <= 127) {
        vs[curx + l].insert(vs[curx + l].begin() + cury - dy, ch);
        cury++;
    }
}

void Vim::advanced_cmd_mode(int ch) {
    if (ch == esc) {
        cmd.clear();
        curState = 0;
        hideCursor();

        gotoxy(sizx - 1, 0);
        cout << spaces;
        curx = 0, cury = dy;
        gotoxy(curx, cury);
        showCursor();
    }
    else if (ch == backspace) {
        if (!cmd.empty()) {
            cmd.pop_back(), cury--;
            hideCursor();
            gotoxy(sizx - 1, 0);
            cout << spaces;
            gotoxy(curx, cury);
            showCursor();
        }
    }
    else if (ch == enter) {
        if (readonly) {
            if (cmd == "w" || cmd == "wq") {
                error = true;
                errortp = string(RED) + "It's a read only document   " + string(RESET);
            }
            else if (cmd == "q") //quit if not edited
            {
                if (vs == init_vs) //没有修改改过
                {
                    quit = true;
                }
                else {
                    error = true;
                    errortp = string(RED) + "You have already editted this document   " + string(RESET);
                }
            }
            else if (cmd == "q!") {
                quit = true;
            }

            else {
                error = true;
                errortp = string(RED) + "No such command                         " + string(RESET);
            }
        }
        else {
            if (cmd == "w") {
                isSave = true;
                vs_save = merge_string();
                cmd.clear();
                curState = 0;
                hideCursor();

                gotoxy(sizx - 1, 0);
                cout << spaces;
                curx = 0, cury = dy;
                gotoxy(curx, cury);
                showCursor();

            }
            else if (cmd == "wq") {
                isSave = true;
                vs_save = merge_string();
                quit = true;
            }
            else if (cmd == "q!") {
                quit = true;
            }
            else if (cmd == "q") {
                if (vs == init_vs) //没有修改改过
                {
                    quit = true;
                }
                else {
                    error = true;
                    errortp = string(RED) + "You have already editted this document   " + string(RESET);
                }
            }
            else {
                error = true;
                errortp = string(RED) + "No such command                         " + string(RESET);
            }
        }

        cmd.clear();
    }

    else if (0 <= ch && ch <= 127 && cmd.size() < 2) {
        cury++;
        cmd += ch;
    }

}

void Vim::fresh() {
    gotoxy(0, 0);
    for (int i = 0; i < sizx - 1; i++)cout << spaces << '\n';
    gotoxy(sizx - 1, sizy - 30);
    gotoxy(0, 0);
}

void Vim::printduc() {
    for (int i = l; i < l + sizx - 1; i++) {
        //            cout.flush();
        if (i <= r) {
            cout << std::setw(4) << std::setfill(' ') << (i + 1) << " >";
            //if(i>=vs.size())exit(0);
            cout << vs[i];
        }
        cout << endl;
    }
    gotoxy(curx, cury);
}

void Vim::printcmd(bool error, const string& errortp) { //分别于左下角，右下角输出当前命令，行列号等信息
    hideCursor();
    gotoxy(sizx - 1, 0);
    //cout<<spaces;
    if (error) {
        //hideCursor();//先隐藏光标，等到随便按了一个按键之后就回到正常模式
        //cout<<spaces;
        gotoxy(sizx - 1, 0);
        cout << errortp;
        //然后输出坐标
        gotoxy(sizx - 1, sizy - 35);

        //cout<<curx<<','<<cury<<"      ";
        cout << "                                          ";
        return;
    }

    if (curState == 1) {
        cout << "-- INSERT --                 ";
    }
    else if (curState == 2) {
        cout << ":                           ";
        gotoxy(sizx - 1, 1);
        cout << cmd;
    }

    //然后输出坐标
    gotoxy(sizx - 1, sizy - 35);

    if (curState == 2)
        cout << "coordinate:" << "(--" << ',' << cury << ") begin:" << l + 1 << " end:" << r + 1 << " total:" << vs.size() << "      ";
    else
        cout << "coordinate:" << '(' << curx + 1 << ',' << cury - dy + 1 << ") begin:" << l + 1 << " end:" << r + 1 << " total:" << vs.size() << "      ";
}

void Vim::print(bool error, const string& errortp) {
    //system("cls");
    hideCursor();
    fresh();
    printduc();

    printcmd(error, errortp);
    gotoxy(curx, cury);
    if (!error) showCursor();
}

void Vim::getsize() { //获取当前控制台大小(行，列)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    sizx = rows;
    sizy = columns - 20;
}