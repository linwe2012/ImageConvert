#pragma once
#include <map>
#include <sstream>
#include "def_utils.h"
#ifdef _WIN32
#include <Windows.h>
#endif
/*
* Premature Debugger
*/
enum ConsoleTextColor {
	CTC_RED = FOREGROUND_INTENSITY | FOREGROUND_RED,
	CTC_GREEN = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
	CTC_BLUE = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
	CTC_YELLOW = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
	CTC_PURPLE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
	CTC_CYAN = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CTC_WHITE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CTC_GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CTC_DARK_GRAY = BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CTC_BLACK = 0
};

enum ConsoleBackGroundColor {
	CBC_RED = BACKGROUND_INTENSITY | BACKGROUND_RED,
	CBC_GREEN = BACKGROUND_INTENSITY | BACKGROUND_GREEN,
	CBC_BLUE = BACKGROUND_INTENSITY | BACKGROUND_BLUE,
	CBC_YELLOW = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
	CBC_PURPLE = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
	CBC_CYAN = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
	CBC_WHITE = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	CBC_BLACK = 0
};

class KDebugger {
	class LineInfo {
		int line;

	};
public:
	KDebugger() 
		:color_warn_head(CTC_DARK_GRAY), color_warn_body(CTC_GRAY)
	{

	}
	int debug_output_limit;
	std::map<std::stringstream*, int> lines;
	int color_warn_head, color_warn_body;
	void ShowDebugMsg(int level, const char *msg, ...) {

	}
	void throwWarning(const char *initiater, const char *msg, ...) {
#ifdef _WIN32
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, color_warn_head);
#endif
		GLIM_LOG_OUTPUT("[WARNING] %s::", initiater);
#ifdef _WIN32
		SetConsoleTextAttribute(handle, color_warn_body);
#endif
		va_list args;
		va_start(args, msg);
		GLIM_LOG_OUTPUT_V(msg, args);
		va_end(args);

		GLIM_LOG_OUTPUT("\n");

	}
};