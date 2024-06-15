#ifndef CODES_VIM_H
#define CODES_VIM_H

#include <bits/stdc++.h>
#include <conio.h>
#include <windows.h>
#include "../Tools.h"

using std::cout, std::endl, std::string, std::vector, std::pair;

class Vim {
private:
    //上下左右箭头的ascii码
    const int upArrow = 72;
    const int downArrow = 80;
    const int leftArrow = 75;
    const int rightArrow = 77;
    const int esc = 27;
    const int backspace = 8;
    const int enter = 13;
    const int del = 83;
    const int dy = 6; //行号+空格+'>'共六个字符位置

    string s; //整个文件的内容
    vector<string> vs; //每一行分别存储，以'\n'为分界符号
    string vs_save; //保存的内容
    vector<string> init_vs; //初始的文件内容；
    vector<bool> isPre; //是否与前面的是同一行。

    //当前模式
    int curState; //0：普通，1：insert,2：高级指令

    string cutBoard; //剪切板
    string spaces = ""; //全为空格，用于覆盖之前的信息

    bool readonly; //只读文件
    bool isSave; //是否被修改过
    bool error; //是否报错
    bool quit; //是否退出

    string errortp; //报错信息；

    string cmd = ""; //高级指令

    int l, r; //输出第几行到第几行
    int curx, cury; //当前光标的坐标；
    //    int vsx,vsy; //vector对应的索引
    int sizx, sizy; //窗口的大小；

    int cnt = 0;

    struct P { int x, y; };
    P dir[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

public:
    Vim(bool readonly = false, const string s = ""); //构造函数
    Vim(); //构造函数
    void hideCursor(); //隐藏光标
    void showCursor(); //显示光标

    void gotoxy(int x, int y); //建立函数 移动光标

    void changePos(int p);

    string merge_string();

    pair<bool, string> exec(); //从键盘中读取一个字母
    void normal_cmd_mode(int ch); //普通命令模式
    void insert_cmd_mode(int ch, bool fg = false); //插入模式

    void advanced_cmd_mode(int ch);

    void fresh();
    void printduc();

    void printcmd(bool error = false, const string& errortp = ""); //分别于左下角，右下角输出当前命令，行列号等信息
    void print(bool error = false, const string& errortp = "");

    void getsize(); //获取当前控制台大小(行，列)

};

#endif //CODES_VIM_H
