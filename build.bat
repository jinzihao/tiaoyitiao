@echo off
mkdir screenshots
g++ C:\Users\jinzihao\Desktop\tiaoyitiao.cpp -lgdi32 -std=c++11 -O2 -DOUTPUT_SCREENSHOT -o tiaoyitiao.exe
