#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "bmp.h"
#include "VariablesManagementl.h"
#include "Wrapper.h"
#include "Kaleidoscope_utils.h"

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
	tok_eof = -1,

	// commands
	tok_def = -2, tok_extern = -3, tok_return = -99, 

	// primary
	tok_identifier = -4, tok_number = -5, tok_array = -6, tok_import = -7, tok_del = -8,tok_if=-9, tok_else=-10, 
	tok_blockstart = -11,
};

static std::string IdentifierStr;  // Filled in if tok_identifier
static double NumVal;              // Filled in if tok_number
static std::stringstream CurTopExpr; //record current line of expression
static int CurLine = 0;
KDebugger GlobalDebugger;
/// gettok - Return the next token from standard input.
static int gettok(K_INPUT) {
	static int LastChar = ' ';
	// Skip any whitespace.
	while (isspace(LastChar)) {
		if (islineend(LastChar)) CurLine++;
		LastChar = ss.get();
	}
	if (isalpha(LastChar) || LastChar == '_') { // identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;
		while (isalnum((LastChar = ss.get())) || LastChar == '_')
			IdentifierStr += LastChar;

		if (IdentifierStr == "def") return tok_def;
		if (IdentifierStr == "extern") return tok_extern;
		if (IdentifierStr == "return") return tok_return;
		if (IdentifierStr == "import") return tok_import;
		if (IdentifierStr == "delete") return tok_del;
		if (IdentifierStr == "if") return tok_if;
		if (IdentifierStr == "else") return tok_else;
		return tok_identifier;
	}
	if (LastChar == '[') { //Array will be seen as a function call returning 
		LastChar = ss.get();
		return tok_array;
	}
	if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = ss.get();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}
	if (LastChar == '#' || LastChar == '/' && (ss.peek() == '/')) { //Comment: '#' or '//'
		// Comment until end of line.
		do LastChar = ss.get();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok(ss);
	}
	if (LastChar == '{')
		return tok_blockstart;
	if (isquotation(LastChar)) {
		int end_quote = LastChar;
		IdentifierStr = LastChar;
		LastChar = ss.get();
		do {
			IdentifierStr += LastChar;
			LastChar = ss.get();
		} while (LastChar != end_quote);
		LastChar = ss.get();
		return tok_identifier;
	}
	// Check for end of file.  Don't eat the EOF.
	if (LastChar == EOF) {
		LastChar = ss.get();
		return tok_eof;
	}
	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;
	LastChar = ss.get();
	return ThisChar;
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
#define EXE_INPUT VaribaleMap &vm
#define EXE_INPUT_NAME vm
class ExprAST;

static ExprAST* ParseExpression(K_INPUT);

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
	virtual ~ExprAST() {}
	virtual VariableDetail *execute(EXE_INPUT, K_INPUT) = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
	double Val;
public:
	NumberExprAST(double val) : Val(val) {}
	VariableDetail *execute(EXE_INPUT, K_INPUT) override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
	std::string Name;
public:
	VariableExprAST(const std::string &name) : Name(name) {}
	VariableDetail *execute(EXE_INPUT, K_INPUT) override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
	char Op;
	ExprAST *LHS, *RHS;
public:
	BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
		: Op(op), LHS(lhs), RHS(rhs) {}
	VariableDetail *execute(EXE_INPUT, K_INPUT) override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
	std::string Callee;
	std::vector<ExprAST*> Args;
public:
	CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
		: Callee(callee), Args(args) {}
	VariableDetail *execute(EXE_INPUT, K_INPUT) override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
	std::unique_ptr<ExprAST> Cond, Then, Else;

public:
	IfExprAST(ExprAST* Cond, ExprAST* Then, ExprAST* Else)
		: Cond(Cond), Then(Then), Else(Else) {}

	VariableDetail *execute(EXE_INPUT, K_INPUT) override;
};

/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
public:
	std::string Name;
	std::vector<std::string> Args;
	PrototypeAST(const std::string &name, const std::vector<std::string> &args)
		: Name(name), Args(args) {}
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
public:
	PrototypeAST *Proto;
	ExprAST *Body;
	ExprAST *Return_Val;
	FunctionAST(PrototypeAST *proto, ExprAST *body, ExprAST *return_Val)
		: Proto(proto), Body(body), Return_Val(return_Val) {}
	VariableDetail* execute(EXE_INPUT, K_INPUT) {
		VariableDetail* vd = NULL;
		if (Body != NULL) vd = Body->execute(EXE_INPUT_NAME, K_INPUT_PASSON);
		if(Return_Val != NULL) vd = Return_Val->execute(EXE_INPUT_NAME, K_INPUT_PASSON);
		return vd;
	}
};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

static ExprAST* ParseBinOpRHS(int ExprPrec, ExprAST* LHS, K_INPUT);

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken(K_INPUT) {
	return CurTok = gettok(ss);
}

/// Error* - These are little helper functions for error handling.
ExprAST *LogError(const char *Str) { fprintf(stderr, "Error: %s\n", Str); return 0; }
PrototypeAST *LogErrorP(const char *Str) { LogError(Str); return 0; }
FunctionAST *LogErrorF(const char *Str) { LogError(Str); return 0; }

/// numberexpr ::= number
static ExprAST *ParseNumberExpr(std::stringstream &ss) {
	ExprAST *Result = new NumberExprAST(NumVal);
	getNextToken(ss); // consume the number
	return Result;
}

/// parenexpr ::= '(' expression ')'
static ExprAST *ParseParenExpr(K_INPUT) {
	getNextToken(ss);  // eat (.
	ExprAST *V = ParseExpression(K_INPUT_PASSON);
	if (!V) return 0;

	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(K_INPUT_PASSON);  // eat ).
	return V;
}



/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static ExprAST *ParseIdentifierExpr(K_INPUT) {
	std::string IdName = IdentifierStr;

	getNextToken(K_INPUT_PASSON);  // eat identifier.

	if (CurTok != '(') // Simple variable ref.
		return new VariableExprAST(IdName);

	// Call.
	getNextToken(K_INPUT_PASSON);  // eat (
	std::vector<ExprAST*> Args;
	if (CurTok != ')') {
		while (1) {
			ExprAST *Arg = ParseExpression(K_INPUT_PASSON);
			if (!Arg) return 0;
			Args.push_back(Arg);

			if (CurTok == ')') break;

			if (CurTok != ',')
				return LogError("Expected ')' or ',' in argument list");
			getNextToken(K_INPUT_PASSON);
		}
	}
	// Eat the ')'.
	getNextToken(K_INPUT_PASSON);
	
	return new CallExprAST(IdName, Args);
}

static ExprAST *ParseArrayExpr(K_INPUT)//----------------------------------------TODO:Merge with ParseIdentifierExpr
{
	getNextToken(K_INPUT_PASSON);  // eat '['
	std::vector<ExprAST*> Args;
	if (CurTok != ']') {
		while (1) {
			ExprAST *Arg = ParseExpression(K_INPUT_PASSON);
			if (!Arg) return 0;
			Args.push_back(Arg);

			if (CurTok == ']') break;

			if (CurTok != ',')
				return LogError("Expected ']' or ',' in argument list");
			getNextToken(K_INPUT_PASSON);
		}
	}
	// Eat the ']'.
	getNextToken(K_INPUT_PASSON);
	return new CallExprAST("__std_func_Array", Args); //array is defined by function returning all it's params
}

static ExprAST *ParseDelExpr(K_INPUT)//----------------------------------------TODO:Merge with ParseIdentifierExpr
{
	int lineFlag = CurLine;
	getNextToken(K_INPUT_PASSON);  // eat 'del'
	std::vector<ExprAST*> Args;
	if (lineFlag == CurLine) {
		while (1) {
			ExprAST *Arg = ParseExpression(K_INPUT_PASSON);
			if (!Arg) return 0;
			Args.push_back(Arg);

			if (lineFlag != CurLine) break; //stop on change of line

			if (CurTok != ',')
				return LogError("Expected ';' or ',' in argument list");
			getNextToken(K_INPUT_PASSON);
		}
	}
	return new CallExprAST("__std_func_Del", Args); //array is defined by function returning all it's params
}

static ExprAST* ParseBlockExpr(K_INPUT) {
	getNextToken(K_INPUT_PASSON);  // eat '{'
	std::vector<ExprAST*> Args;
	if (CurTok != '}') {
		while (1) {
			ExprAST *Arg = ParseExpression(K_INPUT_PASSON);
			if (!Arg) return 0;
			Args.push_back(Arg);

			if (CurTok == '}') break; //stop on change of line

			getNextToken(K_INPUT_PASSON);
		}
	}
	getNextToken(K_INPUT_PASSON);  // eat '}'
	return new CallExprAST("__std_func_Block", Args);
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static ExprAST* ParseIfExpr(K_INPUT) {
	getNextToken(K_INPUT_PASSON);  // eat the if.

	if (CurTok != '(')
		return LogError("Expected '(' after 'if'.");
	getNextToken(K_INPUT_PASSON);  // eat the '('.
					 // condition.
	auto Cond = ParseExpression(K_INPUT_PASSON);

	if (CurTok != ')')
		return LogError("Expected ')' for 'if'.");

	getNextToken(K_INPUT_PASSON);  // eat the ')'

	if (!Cond)
		return nullptr;

	
	auto Then = ParseExpression(K_INPUT_PASSON);
	
	// if (!Then) means do nothing in then

	ExprAST* Else = NULL;
	if (CurTok == tok_else) //if there is an else expression
	{
		getNextToken(K_INPUT_PASSON); //eat else
		Else = ParseExpression(K_INPUT_PASSON);
	}

	return new IfExprAST(Cond, Then, Else);
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static ExprAST *ParsePrimary(K_INPUT) {
	switch (CurTok) {
	default: 
		throwWarn("Line?: %s. [err token] %c.", IdentifierStr.c_str(), char(CurTok));
		return LogError("unknown token when expecting an expression");
	case tok_identifier: return ParseIdentifierExpr(K_INPUT_PASSON);
	case tok_array:      return ParseArrayExpr(K_INPUT_PASSON);
	case tok_del:        return ParseDelExpr(K_INPUT_PASSON);
	case tok_number:     return ParseNumberExpr(K_INPUT_PASSON);
	case '(':            return ParseParenExpr(K_INPUT_PASSON);
	}
}

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
	if (!isascii(CurTok))
		return -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

/// binoprhs
///   ::= ('+' primary)*
static ExprAST* ParseBinOpRHS(int ExprPrec, ExprAST* LHS, K_INPUT) {
	// If this is a binop, find its precedence.
	while (1) {
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken(K_INPUT_PASSON);  // eat binop

		// Parse the primary expression after the binary operator.
		auto RHS = ParsePrimary(K_INPUT_PASSON);
		if (!RHS)
			return nullptr;
		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS), K_INPUT_PASSON);
			if (!RHS)
				return nullptr;
		}

		// Merge LHS/RHS.
		LHS = new BinaryExprAST(BinOp, std::move(LHS), std::move(RHS));
	}
}

/// expression
///   ::= primary binoprhs
///
static ExprAST* ParseExpression(K_INPUT) {
	auto LHS = ParsePrimary(K_INPUT_PASSON);
	if (!LHS)
		return nullptr;

	return ParseBinOpRHS(0, std::move(LHS), K_INPUT_PASSON);
}

/// prototype
///   ::= id '(' id* ')'
static PrototypeAST* ParsePrototype(K_INPUT) {
	if (CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken(K_INPUT_PASSON);

	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	std::vector<std::string> ArgNames;
	while (getNextToken(K_INPUT_PASSON) == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// success.
	getNextToken(K_INPUT_PASSON); // eat ')'.

	return new PrototypeAST(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static FunctionAST* ParseDefinition(K_INPUT) {
	getNextToken(K_INPUT_PASSON); // eat def.
	auto Proto = ParsePrototype(K_INPUT_PASSON);
	if (!Proto)
		return nullptr;

	//if (auto E = ParseExpression(K_INPUT_PASSON))
	auto E = ParseExpression(K_INPUT_PASSON);
	
	if (getNextToken(K_INPUT_PASSON) != tok_return) {
		LogError("Expected Return value of a function definition");
		return new FunctionAST(Proto, E, NULL);
	}
	auto R = ParseExpression(K_INPUT_PASSON);
	return new FunctionAST(Proto, E, R); //---------------------------------------------TODO: Function Return An
	return nullptr;
}

/// toplevelexpr ::= expression
static FunctionAST* ParseTopLevelExpr(K_INPUT) {
	if (auto E = ParseExpression(K_INPUT_PASSON)) {
		// Make an anonymous proto.
		auto Proto = new PrototypeAST("__anon_expr",
			std::vector<std::string>());
		return new FunctionAST(std::move(Proto), NULL, E);
	}
	return nullptr;
}

/// external ::= 'extern' prototype
static PrototypeAST* ParseExtern(K_INPUT) {
	getNextToken(K_INPUT_PASSON); // eat extern.
	return ParsePrototype(K_INPUT_PASSON);
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleDefinition(K_INPUT) {
	FunctionAST *fun_def = ParseDefinition(K_INPUT_PASSON);
	if (fun_def) {
		fprintf(stderr, "Parsed a function definition.\n");

	}
	else {
		// Skip token for error recovery.
		getNextToken(K_INPUT_PASSON);
	}
}

static void HandleExtern(K_INPUT) {
	if (ParseExtern(K_INPUT_PASSON)) {
		fprintf(stderr, "Parsed an extern\n");
	}
	else {
		// Skip token for error recovery.
		getNextToken(K_INPUT_PASSON);
	}
}

static void HandleTopLevelExpression(K_INPUT, EXE_INPUT) {
	// Evaluate a top-level expression into an anonymous function.
	FunctionAST *func = ParseTopLevelExpr(K_INPUT_PASSON);
	if (func) {
		fprintf(stderr, "Parsed a top-level expr\n");
		func->execute(EXE_INPUT_NAME, K_INPUT_PASSON);//---------------------------------------------TODO: Possible Mem Leak
	}
	else {
		// Skip token for error recovery.
		getNextToken(K_INPUT_PASSON);
	}
}

static void MainLoop(K_INPUT, EXE_INPUT);
static void HandleImport(K_INPUT, EXE_INPUT) {//----------------------------------TODO: Handle Import
	static char buf[1024];
	ss.getline(buf, 1024);
	std::stringstream defs;
	std::ifstream is(buf);
	if (is.good()) {
		defs << is.rdbuf();
		MainLoop(defs, vm);
	}
	else {
		printf("Unexpected Error Opening File.");
	}

	//start a new line
	getNextToken(K_INPUT_PASSON);
}

/// top ::= definition | external | expression | ';'
static void MainLoop(K_INPUT, EXE_INPUT) {
	while (true) {
		switch (CurTok) {
		case tok_eof:
			getNextToken(K_INPUT_PASSON); // eat the eof
			getNextToken(K_INPUT_PASSON); //check if there is file input
			if (CurTok != tok_eof) break;
			pipeStdin(K_INPUT_PASSON);
			getNextToken(K_INPUT_PASSON); // eat the eof
			getNextToken(K_INPUT_PASSON);
			break;
			//return;
		case ';': // ignore top-level semicolons.
			getNextToken(K_INPUT_PASSON);
			break;
		case tok_def:
			HandleDefinition(K_INPUT_PASSON);
			break;
		case tok_extern:
			HandleExtern(K_INPUT_PASSON);
			break;
		default:
			HandleTopLevelExpression(K_INPUT_PASSON, EXE_INPUT_NAME);
			break;
		}
		fprintf(stderr, TERMINAL_INDICATOR);
	}
}

//===----------------------------------------------------------------------===//
// Execute Script
//===----------------------------------------------------------------------===//

VariableDetail* NumberExprAST::execute(EXE_INPUT, K_INPUT) {
	VariableDetail* vd = vm.newVar(Val);
	delete this; //commit suicide
	return vd;
}

VariableDetail* IfExprAST::execute(EXE_INPUT, K_INPUT) {
	VariableDetail* vd = Cond->execute(EXE_INPUT_NAME, K_INPUT_PASSON);
	double flag = 0;
	if (vd->type == vt_double) {
		flag = *(double *)(vd->data);
	}
	
	return NULL;/////////////////////////////////////////////////////////////////////////////////////////////////////
}

VariableDetail* CallExprAST::execute(EXE_INPUT, K_INPUT) {
	int do_not_clean_vars = false;
	std::vector<ExprAST*>::iterator itr = Args.begin();
	std::vector<VariableDetail*> *vars = new std::vector<VariableDetail*>();
	VariableDetail* vd = NULL;
	for (; itr < Args.end(); itr++) {
		VariableDetail *vvd = (*itr)->execute(vm, K_INPUT_PASSON);//--------------------------------------TODO:CLean Up Debug Code
		/// failure executing function will clean up the mass
		/// The function that fails should thow exception
		/// If it fails, all procedure depending on it will fail silently, 
		/// otherwise we may get countless warnings.
		if(vvd == NULL) goto cleanup; 
		vars->push_back(vvd);
	}
	
	if (Callee == "__std_func_Array") {
		vd = new VariableDetail("@", vt_array, vars, vm.getDestructor(vt_array));
		do_not_clean_vars = true;
	}
	else if (Callee == "__std_func_Del") {
		vm.deleteVariables(vars);
		do_not_clean_vars = true;
	}
	else if (Callee == "imwrite") {
		std::vector<VariableDetail*> &vvvv = *vars;
		if (vars->size() == 1) vars->push_back(new VariableDetail("@", vt_nullptr, NULL, NULL));
		imwrite((char *)(*vars)[0]->data, (Image)(*vars)[1]->data);
		vd = new VariableDetail("@", vt_nullptr, NULL, NULL);
	}
	else if (Callee == "imread") {
		if ((*vars).size() == 1) (*vars).push_back(new VariableDetail("@", vt_nullptr, NULL, NULL));
		Image im = imread((char *)(*vars)[0]->data, (Image)(*vars)[1]->data);
		vd = new VariableDetail("@", vt_Image, im, vm.getDestructor(vt_Image));
	}
	else if (Callee == "imtranslate2d") {
		vd = imtranslate2d_wrap(vars);
	}
	else if (Callee == "imrotate2d") {
		vd = imrotate2d_wrap(vars);
	}
	else if (Callee == "imscale2d") {
		vd = imscale2d_wrap(vars);
	}
	else if (Callee == "imflip2d") {
		vd = imflip2d_wrap(vars);
	}
	else if (Callee == "imshear2d") {
		vd = imshear2d_wrap(vars);
	}
	else if (Callee == "figure") {
		vd = figure_wrap(vars);
	}
	else if (Callee == "imshow") {
		vd = imshow_wrap(vars);
	}
	else if (Callee == "script") {
		if ((*vars)[0]->type != vt_string) {
			printf("Expected script be a string of file path\n");
		}
		else {
			pipeFile((char *)((*vars)[0]->data), K_INPUT_PASSON);
		}
	}
	else if (Callee == "exit") {
		printf("-----------------------ENDS------------------------\n");
		printf("Press ant key to exit...");
		fflush(stdin);
		getchar();
		exit(0);
	}
cleanup:
	for (std::vector<VariableDetail*>::iterator it = vars->begin(); it < vars->end() && !do_not_clean_vars; it++) {
		vm.destroyIfTemp(*it);
	}
	delete this; //commit suicide
	return vd;
}

char *auto_strmerge(const char *left, const char *right) {
	char *res = (char *)malloc(strlen(left) + strlen(right) + 1);
	strcpy(res, left);
	return strcat(res, right);
}

VariableDetail *BinaryExprAST::execute(EXE_INPUT, K_INPUT) {
	VariableDetail *lhs = LHS->execute(EXE_INPUT_NAME, K_INPUT_PASSON);
	VariableDetail *rhs = RHS->execute(EXE_INPUT_NAME, K_INPUT_PASSON);
	VariableDetail *res = NULL;
	switch (Op)
	{
	case '=':
		lhs->shallowCpy(rhs);
		rhs->data = NULL; //protect data transfered to lhs
		delete rhs;
		vm.insert(lhs);
		res = lhs;
		break;
	case '+':
		// connect 2 string
		//if (lhs->type == vt_string && rhs->type == vt_string) {//----------------------TODO: Type check is crrently crippled cuz type is not passed currectly
		{
			char *data = auto_strmerge((char*)lhs->data, (char *)rhs->data);
			res = new VariableDetail("@", vt_string, data, vm.getDestructor(vt_string));
		}
		//}
		//else {
			//--------------------------------------------------------------------------TODO
		//}
		break;
	default:
		break;
	}
	delete this; //commit suicide
	return res;
}

VariableDetail *VariableExprAST::execute(EXE_INPUT, K_INPUT) {
	int type = vt_unkown_type;
	char *str = NULL;
	if (isquotation(Name[0])) {
		str = new char[Name.size() + 1];
		type = vt_string;
		strcpy(str, Name.c_str() + 1);
		Name = "@"; //string should be anonymous
	}
	VariableDetail* vd = vm.getVarByName(Name);
	if (vd != NULL) {
		// We get it from storage since it is a declared varibale
	}
	else {
		vd = vm.insert(str, type, Name);
	}
	delete this; //commit suicide	
	return vd;
}


#if 1
int main()
{
	BinopPrecedence['='] = 5;
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40;  // highest.

	VaribaleMap VarMap;
	std::stringstream ss;
	printf("Use script(\"./script.js\") to execute a script.\n");
	printf(TERMINAL_INDICATOR);
	suggestOperation("script(\"./script.js\")", ss);
	//fire the program
	pipeStdin(ss);
	getNextToken(ss);
	MainLoop(ss, VarMap);
	return 0;
}
#endif
#if 0

int c = 1;
while (c > 0) {
	c = ss.get();
	if (c <= 0) {
		printf("\nedl=%d\n", c);
	}
	printf("%c", c);
}
is.close();
is.open("./script.py");
ss << is.rdbuf();

#endif