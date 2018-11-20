/* mini terminal -- a small plugin lib making the console like linux
* author: leon lin 
*/
#include <windows.h>
#include <stdio.h>
#include <math.h>

#define MT_VAR_PROTECT static

#define MAX_BUF 256
#define WARN_LOW 0
#define WARN_VOID_FUNC_LOW 1

#define ASSERT(check, level,  msg, ...) do{ \
	if(!(check)) { \
		printf(msg, __VA_ARGS__);\
	} \
}while(0);

typedef int (*CmdCallback)(const char *, const char *, void *data);
typedef struct tagCmdCell CmdCell;
typedef struct tagOptCell OptCell;
typedef struct tagOption *Option;
typedef struct tagCmdTbl *CmdTbl;
typedef struct tagOptTbl *OptTbl;

struct tagCmdCell{
	CmdCallback func;
	CmdCallback help;
	CmdCallback init;
	CmdTbl OptionTbl;
	int calltimes;
	char *cmd;
	void *data;
	CmdCell *next;
};

struct tagOptCell{
	int key;
	char *opt;
	void *data;
	CmdCallback func;
	CmdCell *next;
};

struct tagOption{
	int key;
	char *opt;
	char *content;
	double num;
	Option next;
};

struct tagCmdTbl{
	CmdCell *tbl;
	int max;
};
struct tagOptTbl{
	OptCell *tbl;
	int max;
};



MT_VAR_PROTECT char currentLoc[MAX_BUF] = "";
MT_VAR_PROTECT char currentUsr[MAX_BUF] = "";
MT_VAR_PROTECT char currentCom[MAX_BUF] = "";
MT_VAR_PROTECT char currentCmd[MAX_BUF] = "";
MT_VAR_PROTECT char userNest  [MAX_BUF] = "";
MT_VAR_PROTECT char globalBuf [MAX_BUF] = "";
MT_VAR_PROTECT int  hide_title = 0;
MT_VAR_PROTECT CmdTbl cmdtbl = NULL;



/************************************************************
* Hash method to store cmd and its corresponding handler function
*/
unsigned int strhash(const unsigned char *cmd, unsigned int base){
	unsigned long hash = 5381;
        int c;
        while (c = *cmd++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % base;
}
void sanitizeNewCmdCell(CmdCell *cc){
	cc->func = NULL;
	cc->help = NULL;
	cc->init = NULL;
	cc->OptionTbl = NULL;
	cc->calltimes = 0;
	cc->data = NULL;
	cc->cmd = NULL;
	cc->next = NULL;	
}

CmdTbl newCmdTbl(int max){
	CmdTbl ct = (CmdTbl)malloc(sizeof(struct tagCmdTbl));
	ct->tbl = (CmdCell *)malloc(sizeof(CmdCell) * max);
	ct->max = max;
	CmdCell *cc = NULL;
	int i;
	for(i=0;i<max;i++){
		sanitizeNewCmdCell(ct->tbl + i);
	}
	return ct;
}


CmdCell *insertCmd(CmdTbl ct, const char *cmd, CmdCallback func){
	unsigned int h = strhash(cmd, ct->max);
	CmdCell *cc = ct->tbl + h;
	int cmd_len = strlen(cmd);
	while(cc->func != NULL && cc->next != NULL){
		cc = cc->next; 
	}
	if(cc != ct->tbl + h){
		cc->next = (CmdCell *)malloc(sizeof(CmdCell));
		cc = cc->next;
		sanitizeNewCmdCell(cc);
	}
	cc->cmd = (char *)malloc(sizeof(char) * cmd_len);
	strcpy(cc->cmd, cmd);
	cc->func = func;
	cc->next = NULL;
	return cc;
}

CmdCell *fetchCmdCell(CmdTbl ct, const char *cmd){
	unsigned int h = strhash(cmd, ct->max);
	CmdCell *cc = ct->tbl + h;
	while(cc && cc->func != NULL){
		if(strcmp(cc->cmd, cmd) == 0)
			return cc;
		cc = cc->next;
	}
	return NULL;
}

void sanitizeNewOptCell(OptCell *oc){
	cc->key = -1;
	cc->opt = NULL;
	cc->data = NULL;
	oc->func = NULL;
	cc->next = NULL;	
}
OptTbl *newOptTbl(int max)
{
	OptTbl ot = (OptTbl)malloc(sizeof(struct tagOptTbl));
	ot->tbl = (OptCell *)malloc(sizeof(OptCell) * max);
	ot->max = max;
	int i;
	for(i=0;i<max;i++){
		sanitizeNewOptCell(ot->tbl + i);
	}
	return ot;
}
OptCell *insertOpt(OptTbl ct, const char *opt, CmdCallback func){
	unsigned int h = strhash(cmd, ct->max);
	CmdCell *cc = ct->tbl + h;
	int cmd_len = strlen(cmd);
	while(cc->func != NULL && cc->next != NULL){
		cc = cc->next; 
	}
	if(cc != ct->tbl + h){
		cc->next = (CmdCell *)malloc(sizeof(CmdCell));
		cc = cc->next;
		sanitizeNewCmdCell(cc);
	}
	cc->cmd = (char *)malloc(sizeof(char) * cmd_len);
	strcpy(cc->cmd, cmd);
	cc->func = func;
	cc->next = NULL;
	return cc;
}


Option newOption()
{
	Option opt = (Option)malloc(sizeof(struct tagOption));
	opt->content = NULL;
	opt->key = -1;
	opt->next = NULL;
	opt->num = NAN;
	opt->opt = NULL;
	return opt;
}

/************************************************************
* APIs
*/
void registerLoc(const char *s){
	int len = strlen(s);
	ASSERT(len < MAX_BUF, WARN_LOW, "registerLoc::location too long.\n", len);
	strcpy(currentLoc, s);
}
void printTitle(){
	printf("\r");
	if(!hide_title){
		printf("%s@%s: ",currentUsr, currentCom);
	}
	if(strcmp(currentLoc, userNest) == 0)
		printf("~$ ");
	else{
		printf("%s $ ",currentLoc);
	}
}
void updateUser()
{
	DWORD dwNameLen = sizeof(currentUsr);
	GetUserName(currentUsr, &dwNameLen);
	sprintf(userNest, "C:\\Users\\%s", currentUsr);
}

void updateComputer()
{
	DWORD bufSizeP = sizeof(currentCom);
	GetComputerName(currentCom, &bufSizeP);
}

Option parseOptions(const char *s, const char *cmd){
	CmdCell * cc = fetchCmdCell(cmdtbl, cmd);
	if(cc == NULL) return NULL;
	Option opt_head = NULL;
	Option opt_cur = NULL;
	while(1){
		if(*s == '-' && *(s+1) == '-'){
			if(*(s+1) == '-'){
				while(isLetter(*s)){
					
				}
			}
		}
	}
}

/************************************************************
* Auxilary Functions
*/
inline int isLetter(char c){
	return ((c <= 'z' && c >= 'a') || c <= 'Z' && c >= 'A');
}
inline int isUpLetter(char c){
	return  c <= 'Z' && c >= 'A';
}
inline int isDnLetter(char c){
	return (c <= 'z' && c >= 'a') ;
}
void cropLoc(int left, int right){
	int pivot = 0, loc = 0;
	int cnt = 0;
	while(!isLetter(currentLoc[pivot])){
		pivot ++;
	}
	while(cnt < left){
		while(currentLoc[pivot] != '\\')
			pivot++;
		cnt++;
	}
	if(pivot)
		strcpy(currentLoc, currentLoc + pivot);
	pivot = strlen(currentLoc);
	cnt = 0;
	while(cnt < right){
		while(currentLoc[--pivot] != '\\')
			;
		cnt++;
	}
	currentLoc[pivot] = '\0';
}

int checkIllegal(char c, char *dels, int n){
	for(--n;n >= 0;--n){
		if(c == dels[n]){
			return 1;
		}
	}
	return 0;
}

void trim(char *s, char *dels, char *perserveone){
	char *s_lazy = s;
	int cnt = strlen(dels);
	int one_cnt = strlen(perserveone);
	while(*s && checkIllegal(*s, dels, cnt)){
		s++;
	}
	while(*s && !checkIllegal(*s, dels, cnt)){
		*s_lazy++ = *s++;
	}
	while(*s){
		while(*s && checkIllegal(*s, dels, cnt)){
			s++;
		}
		if(s > 0 && checkIllegal(*(s-1), perserveone, one_cnt)){
			*s_lazy++ = *(s-1);
		}
		while(*s && !checkIllegal(*s, dels, cnt)){
			*s_lazy++ = *s++;
		}
	}
	*s_lazy = '\0';
}
/************************************************************
* Built-in commands
*/
int changeDir(const char *s, const char *cur, void *data){
	int right = 0;
	
	if(s[1] == ':'){
		strcpy(currentLoc, s);
		return 0;
	}
	else if(*s == '~'){
		strcpy(currentLoc, userNest);
		return 0;
	}
	strcpy(globalBuf, currentLoc);
	while(*s == '.' && *(s+1) == '.' && (*(s+2) == '/' || *(s+2) == '\\')){
		s = s + 3;
		right ++;
	}
	cropLoc(0, right);
	if(*s == '.' && (*(s+1) == '/' || *(s+1) == '\\')){
		s+=2;
	}
	strcpy(globalBuf, currentLoc);
	int len = strlen(currentLoc);
	if(isLetter(*s)){
		currentLoc[len++] = '\\';
	}
	while(isLetter(*s) || *s=='/' || *s =='\\'){
		currentLoc[len++] = *s;
		s++;
	}
	currentLoc[len] = '\0';
	DWORD dwAttrib = GetFileAttributes(currentLoc);
	if(!(dwAttrib != INVALID_FILE_ATTRIBUTES && 
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))){
        printf("Can\'t find specified Directory.\n");
        strcpy(currentLoc, globalBuf);
	}
	return 0;
}
int list(const char *s, const char *cur, void *data){
	int n = 3;
	if(currentCmd[1] == 'l'){
		n = 6;
		sprintf(globalBuf,"ls -l ");
	}
	else{
		sprintf(globalBuf,"ls ");
	}
	strcpy(globalBuf+n, s);
	system(globalBuf);
	return 0;
}

int display(const char *s, const char *cur, void *data){
	int right = 0;
	if(strcmp(s, "-title=1") == 0 || strcmp(s, "-title=true") == 0){
		hide_title = 0;
	}
	else if(strcmp(s, "-title=0") == 0 || strcmp(s, "-title=false") == 0){
		hide_title = 1;
	}
	return 0;
}


void initConsole(char *loc){
	registerLoc(loc);
	updateUser();
	updateComputer();
	cropLoc(0,1);
	cmdtbl = newCmdTbl(100);
	insertCmd(cmdtbl, "cd", changeDir);
	insertCmd(cmdtbl, "ls", list);
	insertCmd(cmdtbl, "ll", list);
	insertCmd(cmdtbl, "display", display);
	printTitle();
}

inline int checkEnd(char c){
	return c == '\0' || c== '\r' || c == '\n';
}

void getCmd(){
	gets(currentCmd);
	
	CmdCallback callback = NULL;
	CmdCell *cc = NULL;
	int i = 0;
	int respond= 0; 
	trim(currentCmd, " \n\r", " ");
	if(checkEnd(currentCmd[0])) return;
	while(currentCmd[i] != ' ' && currentCmd[i] != '\0' && currentCmd[i] != '\n'){
		i++;
	}
	currentCmd[i] = '\0';
	cc =  fetchCmdCell(cmdtbl, currentCmd);
	if(cc->init != NULL && cc->calltimes == 0){
		callback = cc->init; 
	}
	else
		callback = cc->func;
	if(callback != NULL){
		respond = callback(currentCmd+i+1, currentLoc, cc->data);
	}
	else{
		printf("\rMiniBash::Unkown Command: %s.\n", currentCmd);
		return;
	}
	if(respond != 0){
		printf("\rMiniBash::Possible failure executing command \'%s\'\n", currentCmd);
	}
}


int main(int argc, char *argv[]){
	initConsole(argv[0]);
	while(1){
		getCmd();
		printTitle();
	}
	
}

/***********************************
* clean up macros
*/
#undef MAX_BUF

/************************************
* Code Dump, may come to useful someday
*/

#define CMD_CMP(A, do_something) do{ \
	int next = strlen(#A); \
	char mask = currentCmd[next]; \
	currentCmd[next] = '\0'; \
	if(strcmp(#A, currentCmd) == 0){\
		do_something; \
	}\
}while(0);

CmdCallback fetchCmdCallback(CmdTbl ct, const char *cmd){
	unsigned int h = strhash(cmd, ct->max);
	CmdCell *cc = ct->tbl + h;
	while(cc && cc->func != NULL){
		if(strcmp(cc->cmd, cmd) == 0)
			return cc->func;
		cc = cc->next;
	}
	return NULL; 
}
#define MT_TBL_CMD 0
#define MT_TBL_OPT 1

#define MT_TBL_FUNC_FUNC 0
#define MT_TBL_FUNC_HELP 1
#define MT_TBL_FUNC_INIT 2
