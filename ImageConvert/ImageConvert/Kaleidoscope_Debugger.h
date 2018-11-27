#pragma once
#include <map>
#include <sstream>
#include "def_utils.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include <signal.h>
#include <setjmp.h>
#include "Kaleidoscope_utils.h"
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
	CTC_DARK_DARK_GRAY = FOREGROUND_INTENSITY,
	CTC_DARK_GREEN =  FOREGROUND_GREEN,
	CTC_DARK_CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
	CTC_DARK_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
	CTC_DARK_PURPLE = FOREGROUND_RED | FOREGROUND_BLUE,
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
	//wanring should be fixable error
	enum warning_level {
		_1_worth_attention,
		_2_mild,
		_3_less_severe,
		_4_severe, //approximate to error
	};
	enum note_level {
		_1_unimportant,
		_2_may_be_useful,
		_3_valuable_info,
		_4_worth_attention, //approximate to error
	};
	enum thrower_level {
		_1 = 1,
		_2 = 2,
		_3 = 2,
	};
	KDebugger() 
		:color_warn_head(CTC_DARK_DARK_GRAY), color_warn_body(CTC_GRAY),
		color_note_head(CTC_DARK_DARK_GRAY), color_note_body(CTC_GRAY)
	{
		// signal(SIGSEGV, sig_segmentfault);
	}
	int debug_output_limit;
	std::map<std::stringstream*, int> lines;
	std::stringstream content;
	char parsing[1024];
	int color_warn_head, color_warn_body;
	int color_note_head, color_note_body;
	void ShowDebugMsg(int level, const char *msg, ...) {

	}
	void pipeContent(const std::stringstream &ss) {
		content.clear();
		content.str("");
		content << ss.str();
		content.getline(parsing, 1024);
	}
	void nextLine() {
		content.getline(parsing, 1024);
	}
	void throwWarning(int level, int thrower_level, const char *initiater, const char *msg, ...) {
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
#ifdef _WIN32
		SetConsoleTextAttribute(handle, CTC_GRAY);
#endif
	}
	void throwNote(int level, int thrower_level, const char *initiater, const char *msg, ...) {
#ifdef _WIN32
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, color_note_head);
#endif
		GLIM_LOG_OUTPUT("[Note] %s::", initiater);
#ifdef _WIN32
		SetConsoleTextAttribute(handle, color_note_body);
#endif
		va_list args;
		va_start(args, msg);
		GLIM_LOG_OUTPUT_V(msg, args);
		va_end(args);

		GLIM_LOG_OUTPUT("\n");
#ifdef _WIN32
		SetConsoleTextAttribute(handle, CTC_GRAY);
#endif
	}
	char* getCurrentLine() {
		return parsing;
	}
	void printcode(const char*code)
	{
#ifdef _WIN32
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
		std::string IdentifierStr;
		int flag;
		const char *s = code;
		enum {
			def = 0x0,
			key = 0x1,
			comment = 0x2,
			str = 0x4,
			num = 0x8,
			built_in = 0x10,
		};
		std::map<int, int> color_table = {
			{def, CTC_GRAY },
			{key, CTC_DARK_CYAN },
			{comment, CTC_DARK_GREEN},
			{str, CTC_DARK_YELLOW },
			{num, CTC_YELLOW },
			{built_in, CTC_CYAN },
		};
		std::map<std::string, int> key_words{
		{"def", key},
		{ "extern", key},
		{ "return", key },
		{ "import", key },
		{ "delete", key },
		{ "if", key },
		{ "else", key },
		{ "script", key },
		{"imscale2d",built_in },
		{ "imflip2d",built_in },
		{ "imshear2d",built_in },
		{ "imrotate2d",built_in },
		{ "imshow",built_in },
		{ "figure",built_in },
		{ "imtranslate2d",built_in },
		{ "imwrite",built_in },
		{ "imread",built_in },
		};

		while (*s) {
			flag = def;
			if (isalpha(*s) || *s == '_') { // identifier: [a-zA-Z][a-zA-Z0-9]*
				IdentifierStr = *s++;
				while ((isalnum(*s) || *s == '_') && *s)
					IdentifierStr += *s++;
				std::map<std::string, int>::iterator itr = key_words.find(IdentifierStr);
				if (itr != key_words.end()) {
					flag = itr->second;
				}
				/*
				if (IdentifierStr == "def") flag = key;
				if (IdentifierStr == "extern") flag = key;
				if (IdentifierStr == "return") flag = key;
				if (IdentifierStr == "import") flag = key;
				if (IdentifierStr == "delete") flag = key;
				if (IdentifierStr == "if") flag = key;
				if (IdentifierStr == "else") flag = key;
				*/
			}
			else if (*s == '#' || *s == '/' && *(s + 1) == '/') {
				flag = comment;
				IdentifierStr = *s++;
				do { IdentifierStr += *s++;  } 
				while (*s != EOF && *s != '\n' && *s != '\r' && *s);
			}
			else if (isquotation(*s)) {
				flag = str;
				int end_quote = *s;
				IdentifierStr = *s++;
				do {
					IdentifierStr += *s++;
				} while (*s != end_quote && *s);
				IdentifierStr += *s++;
			}
			else if (isdigit(*s) || *s == '.') {   // Number: [0-9.]+
				flag = num;
				IdentifierStr = *s++;
				do {
					IdentifierStr += *s++;
				} while (isdigit(*s) || *s == '.');
			}
			else {
				// flag = def;
				IdentifierStr = *s++;
				while (*s && !isquotation(*s) && !(isalpha(*s) || *s == '_') && !(*s == '#' || *s == '/' && *(s + 1) == '/') && !(isdigit(*s) || *s == '.')) {
					IdentifierStr += *s++;
				}
			}
#ifdef _WIN32
			SetConsoleTextAttribute(handle, color_table[flag]);
#endif
			printf("%s", IdentifierStr.c_str());
		}
		
#ifdef _WIN32
		SetConsoleTextAttribute(handle, CTC_GRAY);
#endif
	}

	enum jmp_mask {
		sigseg = 0x1,
	};
	// int getjumpnum(std::string str) {
	// 	if (str == "Segment Fault") {
	// 		ifjump = ifjump | jmp_mask::sigseg;
	// 		return jmp_mask::sigseg;
	// 	}
	// }
	// static int ifjump;
	// static jmp_buf jump_buffer;
	// static void sig_segmentfault(int sig) {
	// 	printf("Debugger::Segment Fault. Will abort operation. End the program is recomanded\n");
	// 	getchar();
	// 	if (ifjump & jmp_mask::sigseg) {
	// 		longjmp(jump_buffer, jmp_mask::sigseg);
	// 	}
	// }
};
