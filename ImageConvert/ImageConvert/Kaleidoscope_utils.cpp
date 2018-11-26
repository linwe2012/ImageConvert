#include "Kaleidoscope_utils.h"
#include <conio.h>
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32


static int OperationOccuppied = false;
void pipeFile(const char *file, K_INPUT) {
	if (ss.eof()) ss.clear();
	std::ifstream is(file);
	if (!is.is_open()) {
		printf("Fails To Open File.\n");
	}
	else {
		ss << is.rdbuf();
		printf("Executing Script \"%s\"\n", file);
	}
	is.close();
}

void pipeStdin(K_INPUT) {
	if (ss.eof()) ss.clear();
	else if (OperationOccuppied) {
		OperationOccuppied = false;  
		return;
	}
	static char buf[1024];
	std::cin.getline(buf, 1024);
	ss << buf;
}
/*suggestOperation -- add str into stdin
* It will fail silently, exception handled by caller.
* return 0: everything is fine
* 
*/
int suggestOperation(const char *cmd, K_INPUT)
{
	printf("%s\r", cmd);
	printf(TERMINAL_INDICATOR);
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_BLUE);
	printf("\n[- Enter to execute, Press Any Other Key to Cancel. -]");
	GetConsoleScreenBufferInfo(handle, &csbi);
	csbi.dwCursorPosition.Y--;
	csbi.dwCursorPosition.X = static_cast<SHORT>(strlen(TERMINAL_INDICATOR));
	SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
	SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif // _WIN32
	char c = _getch();
	if (c == '\r' || c == '\n') {
		printf("\n");
		OperationOccuppied = true;
		ss << cmd;
	}
	else {
		printf("\r" TERMINAL_INDICATOR "[-Comman Cancelled-]                                                                                                                                                                     \n");
		printf(TERMINAL_INDICATOR);
	}
	return 0;
}
int islineend(int Ch)
{
	return (Ch == '\r' || Ch == '\n' || Ch == ';');
}
int isquotation(const int Ch) {
	return ( Ch == '\"' || Ch == '\'');
}