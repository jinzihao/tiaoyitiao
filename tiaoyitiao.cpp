#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <queue>
#include <time.h>
#include <algorithm>
#include <cmath> 

using namespace std;

void MouseMove(int x, int y) {
    double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN) - 1;
    double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN) - 1;
    double fx = x*(65535.0f / fScreenWidth);
    double fy = y*(65535.0f / fScreenHeight);
    INPUT  Input = { 0 };
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    Input.mi.dx = fx;
    Input.mi.dy = fy;
    SendInput(1, &Input, sizeof(INPUT));
}

void MouseLeftDown() {
    INPUT  Input = { 0 };
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &Input, sizeof(INPUT));
}

void MouseLeftUp() {
    INPUT  Input = { 0 };
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &Input, sizeof(INPUT));

}

void MouseRightDown() {
    INPUT  Input = { 0 };
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &Input, sizeof(INPUT));
}

void MouseRightUp() {
    INPUT  Input = { 0 };
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &Input, sizeof(INPUT));
}

class LeastSquare {
public:
    double a, b;
    LeastSquare(const vector<double>& x, const vector<double>& y) {
        double t1 = 0, t2 = 0, t3 = 0, t4 = 0;
        for (int i = 0; i < x.size(); ++i) {
            t1 += x[i] * x[i];
            t2 += x[i];
            t3 += x[i] * y[i];
            t4 += y[i];
        }
        a = (t3 * x.size() - t2 * t4) / (t1 * x.size() - t2 * t2);
        b = (t1 * t4 - t2 * t3) / (t1 * x.size() - t2 * t2);
    }

    double getY(const double x) const {
        return a * x + b;
    }

    void print() const {
        cout << "y = " << a << "x + " << b << endl;
    }
};

static const int xMin = 694 + 1;
static const int xMax = 1224;
static const int yMin = 88 + 1;
static const int yMax = 1032;

static const int COLOR_DISTANCE_THRESHOLD = 6;
static const double X_FACTOR = 1.74;
static const int MINOR_ADJUSTMENT = 10;
static const int UNUSABLE_AREA_HEIGHT = 160; 
//static const int RESTART_POSITION_X = 960;
//static const int RESTART_POSITION_Y = 800;
// anti-anti-cheat
#define RESTART_POSITION_X (955 + rand() % 10)
#define RESTART_POSITION_Y (796 + rand() % 8)

int DIRECTION_X[] = {-1, 0, 1, 0};
int DIRECTION_Y[] = {0, 1, 0, -1};

int ScrWidth = 0;
int ScrHeight = 0;
char *buf;
char *alternative_buf;

int last_x_inc = 1;
int x_inc = 1;
int last_distance = 0;
int wait = 0;
int last_border_top_size = 0;
int iteration_count = 0;

//double A = 5.59876;
//double B = 13.6041;
//double A = 5.35825;
//double B = 37.5679;
//double A = 5.16558;
//double B = 44.2517;
//double A = 5.23295;
//double B = 40.5638;
//double A = 5.2;
//double B = 49;
double A = 5.17137;
double B = 59.5253;
//double A = 5.1929;
//double B = 60.655;

vector<double> distances;
vector<int> errors;
vector<double> Ys;

class Color {
public:
	Color() : r(0), g(0), b(0) {}
	Color(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {}

	bool operator==(const Color& color2) {
		if (r == color2.r && g == color2.g && b == color2.b) {
			return true;
		}
		else {
			return false;
		}
	}
	
	bool operator!=(const Color& color2) {
		return !(*this == color2);
	}
	
	friend ostream& operator<<(ostream& stream, const Color& color);

	unsigned char r;
	unsigned char g;
	unsigned char b;
};

ostream& operator<<(ostream& stream, const Color& color) {
	stream << "r = " << (unsigned int)color.r << ", g = " << (unsigned int)color.g << ", b = " << (unsigned int)color.b;
	return stream;
}

static const Color CENTER_COLOR(56, 58, 100);
static const Color PROCESSED_BACKGROUND(0, 0, 255);
static const Color OBJECT_COLOR(255, 0, 0);
static const Color MEDICINE_BOTTLE_LABEL_COLOR(95, 126, 153);
static const Color MEDICINE_BOTTLE_LABEL_REVERSED_COLOR(108, 144, 174);
//static const Color CENTER_COLOR(56, 59, 102);

int manhattanDistance(const Color& color1, const Color& color2) {
	return abs(color1.r - color2.r) + abs(color1.g - color2.g) + abs(color1.b - color2.b);
}

Color getColor(int x, int y, char *target = buf) {
	return Color((unsigned char)target[(y * ScrWidth + x) * 4 + 2], (unsigned char)target[(y * ScrWidth + x) * 4 + 1], (unsigned char)target[(y * ScrWidth + x) * 4]);
}

void setColor(int x, int y, unsigned char r, unsigned char g, unsigned char b, char *target = buf) {
	target[(y * ScrWidth + x) * 4 + 2] = r;
	target[(y * ScrWidth + x) * 4 + 1] = g;
	target[(y * ScrWidth + x) * 4] = b;
}

void setColor(int x, int y, const Color& color, char *target = buf) {
	setColor(x, y, color.r, color.g, color.b, target);
}

void ScanScreen() {
	if (wait) {
		--wait;
		return;
	}
	
    HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
    RECT rect = { 0 };

    ScrWidth = GetDeviceCaps(hdc, HORZRES);
    ScrHeight = GetDeviceCaps(hdc, VERTRES);
    HDC hmdc = CreateCompatibleDC(hdc);

    HBITMAP hBmpScreen = CreateCompatibleBitmap(hdc, ScrWidth, ScrHeight);
    HBITMAP holdbmp = (HBITMAP)SelectObject(hmdc, hBmpScreen);

    BITMAP bm;
    GetObject(hBmpScreen, sizeof(bm), &bm);

    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = bm.bmPlanes;
    bi.biBitCount = bm.bmBitsPixel;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = bm.bmHeight * bm.bmWidthBytes;

    buf = new char[bi.biSizeImage];

    BitBlt(hmdc, 0, 0, ScrWidth, ScrHeight, hdc, rect.left, rect.top, SRCCOPY);
    GetDIBits(hmdc, hBmpScreen, 0L, (DWORD)ScrHeight, buf, (LPBITMAPINFO)&bi, (DWORD)DIB_RGB_COLORS);
    
#ifndef USE_CLASSICAL_IMAGE_CAPTURE
    int color_count[2][3][256] = {{{0}}};
    for (int x = xMin + 1; x < xMax - 1; ++x) {
    	Color bottom_color = getColor(x, yMin + 2);
    	Color top_color = getColor(x, yMax - 2);
    	++color_count[0][0][bottom_color.r];
    	++color_count[0][1][bottom_color.g];
    	++color_count[0][2][bottom_color.b];
    	++color_count[1][0][top_color.r];
    	++color_count[1][1][top_color.g];
    	++color_count[1][2][top_color.b];
	}
	Color background_color_bottom(max_element(color_count[0][0], color_count[0][0] + 256) - color_count[0][0],
		max_element(color_count[0][1], color_count[0][1] + 256) - color_count[0][1],
		max_element(color_count[0][2], color_count[0][2] + 256) - color_count[0][2]);
		
	Color background_color_top(max_element(color_count[1][0], color_count[1][0] + 256) - color_count[1][0],
		max_element(color_count[1][1], color_count[1][1] + 256) - color_count[1][1],
		max_element(color_count[1][2], color_count[1][2] + 256) - color_count[1][2]);
		
//	cout << "background_color_bottom = " << background_color_bottom << endl;
//	cout << "background_color_top = " << background_color_top << endl;
	
	double r_step = double(background_color_top.r - background_color_bottom.r) / double(yMax - yMin - 4);
	double g_step = double(background_color_top.g - background_color_bottom.g) / double(yMax - yMin - 4);
	double b_step = double(background_color_top.b - background_color_bottom.b) / double(yMax - yMin - 4);
#endif 
	
//	for (int y = yMin + 2; y < yMax - 1; ++y) {
//		int color_distance = manhattanDistance(getColor(xMax - 1, y), Color(round(background_color_bottom.r + r_step * (y - yMin - 2)), round(background_color_bottom.g + g_step * (y - yMin - 2)), round(background_color_bottom.b + b_step * (y - yMin - 2))));
//		if (color_distance) {
//			cout << y << " " << color_distance << endl;
//		}
//	}

    int center_x = -1;
	int center_y = -1;
	int center_count = 0;
	for (int y = yMin; y < yMax; ++y) {
		for (int x = xMin; x < xMax; ++x) {	
    		Color current_color = getColor(x, y);
#ifndef USE_CLASSICAL_IMAGE_CAPTURE
    		Color background_color(round(background_color_bottom.r + r_step * (y - yMin - 2)), round(background_color_bottom.g + g_step * (y - yMin - 2)), round(background_color_bottom.b + b_step * (y - yMin - 2)));
#endif
			if (current_color == CENTER_COLOR) {
//    			cout << "found center: " << x << " " << y << " " << endl;
    			center_x += x;
    			center_y += y;
    			++center_count;
//    			break;
			}
#ifndef USE_CLASSICAL_IMAGE_CAPTURE
			else if (manhattanDistance(background_color, current_color) <= 3) {
				setColor(x, y, PROCESSED_BACKGROUND);
			}
			else if (background_color.r && background_color.g && background_color.b) {
				double r_factor = double(current_color.r) / double(background_color.r);
				double g_factor = double(current_color.g) / double(background_color.g);
				double b_factor = double(current_color.b) / double(background_color.b);
				
				if (abs(r_factor - g_factor) < 0.01 && abs(r_factor - b_factor) < 0.01 && abs(g_factor - b_factor) < 0.01) {
					setColor(x, y, PROCESSED_BACKGROUND);
				}
			}
#endif
		}
	}
	
#ifndef USE_CLASSICAL_IMAGE_CAPTURE
	memcpy(alternative_buf, buf, 1920 * 1080 * 4);
	int object_left_border = -1;
	for (int y = yMin + 1; y < yMax - 1; ++y) {
		for (int x = xMin + 1; x < xMax - 1; ++x) {	
    		Color current_color = getColor(x, y);
    		if (getColor(x, y) == PROCESSED_BACKGROUND && getColor(x + 1, y) != PROCESSED_BACKGROUND) {
				object_left_border = x;
			}
			else if (getColor(x, y) != PROCESSED_BACKGROUND && getColor(x + 1, y) == PROCESSED_BACKGROUND) {
				for (int i = object_left_border + 1; i <= x; ++i) {
					setColor(i, y, OBJECT_COLOR, alternative_buf);
				}
			}
    	}
    }
#endif
	
	if (center_count) {
		center_x /= center_count;
		center_y /= center_count;
		cout << "center_x = " << center_x << ", center_y = " << center_y << endl; 
	}
	
//	for (int x = xMin; x < xMax; ++x) {
//		Color currentColor = getColor(x, center_y);
//		if (manhattanDistance(getColor(x, center_y), CENTER_COLOR) <= COLOR_DISTANCE_THRESHOLD) {
//			cout << x << " " << center_y << endl;
//		}
//	}
//	return;

#ifdef OUTPUT_SCREENSHOT
	BITMAPFILEHEADER bfh = { 0 };
    bfh.bfType = ((WORD)('M' << 8) | 'B');
    bfh.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+bi.biSizeImage;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    char string_buffer[20] = {0};
    sprintf(string_buffer, "%d", ++iteration_count);
    string filename = string("screenshots/") + string_buffer + string(".bmp");
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    DWORD dwWrite;
    WriteFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &dwWrite, NULL);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwWrite, NULL);
    WriteFile(hFile, buf, bi.biSizeImage, &dwWrite, NULL);
    CloseHandle(hFile);
    hBmpScreen = (HBITMAP)SelectObject(hmdc, holdbmp);
#endif

	if (center_x >= 0 && center_y >= 0) {
//		Color head_color = getColor(center_x, center_y + 90);
//		if (head_color.r < 128 || head_color.g < 128 || head_color.b < 128) {
		cout << "found center at " << center_x << " " << center_y << endl;

		vector<pair<int, int> > border_top;
		// scan top
		for (int y = center_y + 12; y < yMax; ++y) {
			if (manhattanDistance(getColor(center_x, y), getColor(center_x, y + 1)) > COLOR_DISTANCE_THRESHOLD) {
				border_top.push_back(pair<int, int>(center_x, y));
			}
		}
		
		if (border_top.size() != last_border_top_size) {
			last_border_top_size = border_top.size();
			return;
		}
		
		vector<pair<int, int> > border_top_left;
		// scan top-left
		for (int y = center_y + 12; y < yMax; ++y) {
			int x = center_x - (y - center_y) * X_FACTOR;
			if (x < xMin + 2) {
				break;
			}
			if (manhattanDistance(getColor(x, y), getColor(x - 1, y + 1)) > COLOR_DISTANCE_THRESHOLD) {
				cout << "t-l: " << x << " " << y << endl;
				border_top_left.push_back(pair<int, int>(x, y));
			}
		}

		vector<pair<int, int> > border_top_right;
		// scan top-right
		for (int y = center_y + 12; y < yMax; ++y) {
			int x = center_x + (y - center_y) * X_FACTOR;
			if (x > xMax - 2) {
				break;
			}
			if (manhattanDistance(getColor(x, y), getColor(x + 1, y + 1)) > COLOR_DISTANCE_THRESHOLD) {
				cout << "t-r: " << x << " " << y << endl;
				border_top_right.push_back(pair<int, int>(x, y));
			}
		}

//		cout << "top-left: " << border_top_left.size() << endl;
//		cout << "top-right: " << border_top_right.size() << endl;

		int max_distance_top_left = 0;
		int max_distance_top_right = 0;

		if (border_top_left.size() > 1) {
			max_distance_top_left = border_top_left.back().second - border_top_left.front().second;
		}

		if (border_top_right.size() > 1) {
			max_distance_top_right = border_top_right.back().second - border_top_right.front().second;
		}

//		cout << "top-left max distance: " << max_distance_top_left << endl;
//		cout << "top-right max distance: " << max_distance_top_right << endl;

		if (max_distance_top_left || max_distance_top_right) {
			vector<pair<int, int> > *direction_to_jump;

			if (max_distance_top_left < max_distance_top_right) {
				direction_to_jump = &border_top_right;
	//			int distance_to_jump = (border_top_right.back().second + border_top_right[border_top_right.size() - 2].second) / 2 - center_y;
				cout << "next: top-right" << endl;
				last_x_inc = x_inc;
				x_inc = 1;
			}
			else {
				direction_to_jump = &border_top_left;
	//			int distance_to_jump = (border_top_left.back().second + border_top_left[border_top_left.size() - 2].second) / 2 - center_y;
				cout << "next: top-left" << endl;
				last_x_inc = x_inc;
				x_inc = -1;
			}
			
#ifndef USE_CLASSICAL_OBJECT_DETECTION
			int object_left_border = -1;
			int last_object_width = -1;
			int object_center_x = -1;
			int object_center_y = -1;
			int count = 0;
			bool flag = false;
			for (int y = yMax - UNUSABLE_AREA_HEIGHT; y > yMin; --y) {
				int border_count = 0;
				for (int x = ((x_inc == -1) ? (xMin + 2) : (xMax - 2)); ((x_inc == -1) ? (x < center_x - 20) : (x > center_x + 20)); x -= x_inc) {
					if (getColor(x, y, alternative_buf) == PROCESSED_BACKGROUND && getColor(x - x_inc, y, alternative_buf) != PROCESSED_BACKGROUND && abs(x - center_x) > 20) {
						++border_count;
						object_left_border = x;
					}
					else if (getColor(x, y, alternative_buf) != PROCESSED_BACKGROUND && getColor(x - x_inc, y, alternative_buf) == PROCESSED_BACKGROUND && border_count == 1) {
						++border_count;
//						int object_width = abs(x - object_left_border);
//						if (last_object_width >= object_width) {
							if (++count > 3) {	
								object_center_x = (x + object_left_border) / 2;
//								object_center_y = y;
								flag = true;
								break;
							}
//						}
//						else {
//							count = 0;
//							last_object_width = object_width;
//						}
						break;
					}
					if (border_count == 2) {
						break;
					}
				}
				if (flag) {
					break;
				}
			}
			
			count = 0;
			flag = false;
			for (int x = ((x_inc == -1) ? (xMin + 2) : (xMax - 2)); ((x_inc == -1) ? (x < xMax - 2) : (x > xMin - 2)); x -= x_inc) {
				for (int y = yMax - UNUSABLE_AREA_HEIGHT; y > center_y; --y) {
					if (getColor(x - 1, y) != PROCESSED_BACKGROUND && getColor(x + 1, y) != PROCESSED_BACKGROUND) {
						bool fail = false;
						for (int i = 1; i < 12; ++i) {
							if (getColor(x, y - i) == PROCESSED_BACKGROUND) {
								fail = true;
								break;
							}
						}
						if (!fail) {
							object_center_y = y - 1;
							if (getColor(x - x_inc, y - 20) == ((x_inc == -1) ? MEDICINE_BOTTLE_LABEL_COLOR : MEDICINE_BOTTLE_LABEL_REVERSED_COLOR)) {
								cout << "Detected medicine bottle" << endl;
								object_center_y += 20;
							}
							flag = true;
							break;
						}
						else {
							break;
						}
					}
				}
				if (flag) {
					break;
				}
			}
#endif

			while (direction_to_jump->back().second - (*direction_to_jump)[direction_to_jump->size() - 2].second == 1) {
				direction_to_jump->pop_back();
			}


			pair<int, int> border_back(-1, -1);
			// scan bottom-left/right
			for (int y = center_y - 12; y > yMin; --y) {
				int x = center_x + last_x_inc * (y - center_y) * X_FACTOR;
//				cout << "Scanning " << x << " " << y << endl;
				if (x < xMin + 2 || x > xMax - 2) {
					break;
				}
				if (manhattanDistance(getColor(x, y), getColor(x - last_x_inc * 2, y - 2)) > COLOR_DISTANCE_THRESHOLD) {
					border_back.first = x;
					border_back.second = y;
					break;
				}
			}

			int end_x = direction_to_jump->back().first;
			int end_y = direction_to_jump->back().second;

			int begin_x = -1;
			int begin_y = -1;
//			for (vector<pair<int, int> >::iterator it = direction_to_jump->begin() + 1; it != direction_to_jump->end(); ++it) {
			for (vector<pair<int, int> >::iterator it = direction_to_jump->begin() + 2; it != direction_to_jump->end(); ++it) {
				if (manhattanDistance(getColor(it->first + x_inc * 2, it->second + 2), getColor(end_x - x_inc * 2, end_y - 2)) < COLOR_DISTANCE_THRESHOLD) {
					begin_x = it->first;
					begin_y = it->second;
					break;
				}
			}
			
			int target_y = -1;
			int target_x = -1;

			if (begin_x >= 0 && begin_y >= 0) {
				target_y = (end_y + begin_y) / 2;
				target_x = (end_x + begin_x) / 2;
			}
#ifndef USE_CLASSICAL_OBJECT_DETECTION
			cout << "object_center_x = " << object_center_x << endl;
			cout << "target_x = " << target_x << endl;
			cout << "object_center_y = " << object_center_y << endl;
			cout << "target_y = " << target_y << endl;
			int target_y_from_object_center_x = double(abs(object_center_x - center_x)) / X_FACTOR + center_y;
			cout << "target_y_from_object_center_x = " << target_y_from_object_center_x << endl;
			
			if (object_center_y < 0) {
				target_y = target_y_from_object_center_x;
			}
			else if (target_y >= 0 && target_x >= 0) {
				int diff12 = abs(object_center_y - target_y);
				int diff13 = abs(object_center_y - target_y_from_object_center_x);
				int diff23 = abs(target_y - target_y_from_object_center_x);
				if (diff13 > diff12 && diff23 > diff12 && diff13 > 10 && diff12 > 10) {
#ifdef ALLOW_HUMAN_INTERVENTION
					cout << "Need human intervention..." << endl;
					exit(0);
#endif
					target_y = (object_center_y + target_y) / 2;
				}
				else {
					target_y = target_y_from_object_center_x;
				}
			}
			else {
				target_y = target_y_from_object_center_x;
			}

//				target_y = double(abs(object_center_x - center_x)) / X_FACTOR + center_y;
#endif
			int distance = target_y - center_y;
			cout << "(" << begin_x << "," << begin_y << ") (" << end_x << "," << end_y << ")" << endl;
			cout << "distance: " << target_y - center_y << endl;

#ifdef LEARNING 
			if (distances.size()) {
				errors.push_back((direction_to_jump->front().second - center_y) - (center_y - border_back.second));
				Ys.push_back(A * distances.back() + B + errors.back());
			}

			if (distances.size() > 3 && errors.size() > 2) {
				LeastSquare lsq(distances, Ys);
				A = lsq.a;
				B = lsq.b;
				lsq.print();
			}
			distances.push_back(target_y - center_y);
#endif

			MouseMove(RESTART_POSITION_X, RESTART_POSITION_Y);
			MouseLeftDown();
//				Sleep(distance * 5.7 + 15 + ((direction_to_jump->size() > 20) ? (-20) : 0));
			Sleep(A * distance + B - (distance < 70 ? (70 - distance) * 2 : 0));
			MouseLeftUp();
			Sleep(1000);
		}
		else {
			Sleep(500);
			MouseMove(RESTART_POSITION_X, RESTART_POSITION_Y);
			MouseLeftDown();
			Sleep(MINOR_ADJUSTMENT);
			MouseLeftUp();
		}
//		else {
//			Sleep(500);
//			MouseMove(RESTART_POSITION_X, RESTART_POSITION_Y);
//			MouseLeftDown();
//			Sleep(MINOR_ADJUSTMENT);
//			MouseLeftUp();
//		}
	}
	else {
		Sleep(500);
		MouseMove(RESTART_POSITION_X, RESTART_POSITION_Y);
		MouseLeftDown();
		Sleep(MINOR_ADJUSTMENT);
		MouseLeftUp();
	}
}

int main() {
	srand(time(0));
#ifndef USE_CLASSICAL_IMAGE_CAPTURE
	alternative_buf = new char[1920 * 1080 * 4];
#endif
    while (true) {
    	ScanScreen();
    	Sleep(500);
	}
    return 0;
}
