#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include "bmp.h"
#include "Kaleidoscope_Debugger.h"
#define MAP std::map
class FunctionAST;
class PrototypeAST;
class VaribaleMap;
typedef void  (*Variable_Destructor)(void *);
typedef void* (*Variable_DeepCpy)(void *);
extern KDebugger GlobalDebugger;

enum var_types {
	vt_double, // pointer to double
	vt_nullptr,
	vt_Image, // Image
	vt_unkown_type,
	vt_string, //string pointer
	vt_array, //void *data = std::vector<VariableDetail*> *vars;
};

void destructor_double(void *d);
void destructor_string(void *str);
void destructor_array(void *arr);
void destructor_Image(void *im);

void *deepcpy_double(void *);
void *deepcpy_string(void *);
void *deepcpy_array(void *);
void *deepcpy_Image(void *);

class VariableProtoType {
public:
	enum {
		cpy_deep = 0x1,
		cpy_shallow = 0x2,
		cpy_shallow_if_temp = 0x4,
	};
	int type;
	std::string name;
	int default_cpy_method;
	Variable_DeepCpy deepCpy;
	Variable_Destructor destructor;
	VariableProtoType(int mtype, std::string mname, Variable_Destructor des, Variable_DeepCpy dp, int cpy_method) 
	:type(mtype), name(mname), destructor(des), deepCpy(dp) {}
};

class VariableDetail {
public:
	std::string name;
	void *data;
	VariableProtoType *proto;
	int type;
	Variable_Destructor vd = NULL;
	Variable_DeepCpy vdcpy = NULL; //deep copy
	VariableDetail(std::string name, int Type, void *data, Variable_Destructor mvd)
		:name(name), type(Type), data(data), vd(mvd) {}
	VariableDetail(std::string _name, VariableProtoType *_proto, void *_data)
		:name(_name), proto(_proto), data(_data){}
	~VariableDetail()
	{
		deleteData();
	}
	void copyDatafrom(VariableDetail *src) {
		if (src->data != data)
			clearData();
		data = src->data;
		proto = src->proto;
	}
	void clearData() {
		if (data && proto && proto->destructor)
			proto->destructor(data);
	}
	//do not copy name
	void shallowCpy(VariableDetail *vd_src) {
		if (vd_src->data != data) {
			deleteData();
		}
		data = vd_src->data;
		type = vd_src->type;
		vd = vd_src->vd;
	}
	void deepCopy(VariableDetail *vd_src) {
		if (vdcpy != NULL) {
			data = vdcpy(vd_src);
		}
	}
	void deleteData() {
		if (vd != NULL && data != NULL)
			vd(data);
		data = NULL;
	}
};

class Scope {
public:
	std::string name;
	MAP<std::string, VariableDetail*> var_map;
	MAP<std::string, Scope*> subs;
	Scope *super;
	Scope(std::string nname, Scope *ssuper):name(nname), super(ssuper){}
	Scope *addSubScope(std::string _name) {
		Scope *exists = findinSub(_name);
		if (exists) //already in the sub scope
			return NULL;
		else {
			exists = new Scope(_name, this);
			subs[_name] = exists;
		}
		return exists;
	}
	VariableDetail* addVar(VariableDetail* vd) {
		
	}
	Scope *findinSub(std::string name) {
		MAP<std::string, Scope*>::iterator itr = subs.find(name);
		if (itr == subs.end()) {
			return NULL;
		}
		else
			return itr->second;
	}
	Scope *selfDestruct(VaribaleMap &vm) {
		Scope *sus = super; //super scope
		sus->subs.erase(name);
		MAP<std::string, VariableDetail*>::iterator itr = var_map.begin();
		for (; itr != var_map.end(); itr++) {
			vm.deleteVariable(itr->second);
		}
		delete this;
		return sus;
	}
	VariableDetail* getVarInScope(std::string &_name)
	{
		MAP<std::string, VariableDetail*>::iterator itr = var_map.find(_name);
		if (itr == var_map.end()) return NULL;
		else return itr->second;
	}
};
class VaribaleMap {
	class VariableType {
	public:
		int Type;
		Variable_Destructor default_vd;
		Variable_DeepCpy vdcpy; //deep copy
		VariableType(int type, Variable_Destructor de_vd)
			:Type(type), default_vd(de_vd) {}
	};

public:
	VaribaleMap() 
		:global("_GLOBAL_", NULL), cur_scope(&global)
	{
#define VM_CONSTRUCT_TYPE(X) do{\
	type_map[vt_##X] = new VariableType(vt_##X, destructor_##X); \
	type_name_map[vt_##X] = #X; \
} while(0)
		VM_CONSTRUCT_TYPE(double);
		VM_CONSTRUCT_TYPE(array);
		VM_CONSTRUCT_TYPE(string);
		VM_CONSTRUCT_TYPE(Image);
#undef VM_CONSTRUCT_TYPE
		type_name_map[vt_nullptr] = "nullptr";
		type_name_map[vt_unkown_type] = "unkown_type";
		enum var_types {
			vt_double, // pointer to double
			vt_nullptr,
			vt_Image, // Image
			vt_unkown_type,
			vt_string, //string pointer
			vt_array, //void *data = std::vector<VariableDetail*> *vars;
		};
#define VM_CONSTRUCT_BUILTIN_TYPE(X, method) var_proto_map[vt_##X] = new VariableProtoType(vt_##X, #X, destructor_##X, deepcpy_##X, method | VariableProtoType::cpy_shallow_if_temp)
		VM_CONSTRUCT_BUILTIN_TYPE(double, VariableProtoType::cpy_deep);
		VM_CONSTRUCT_BUILTIN_TYPE(array,  VariableProtoType::cpy_shallow);
		VM_CONSTRUCT_BUILTIN_TYPE(Image,  VariableProtoType::cpy_shallow);
		VM_CONSTRUCT_BUILTIN_TYPE(string, VariableProtoType::cpy_deep);
		var_proto_map[vt_unkown_type] = new VariableProtoType(vt_unkown_type, "unkown_type", NULL, NULL, VariableProtoType::cpy_shallow_if_temp);
		var_proto_map[vt_nullptr] = new VariableProtoType(vt_nullptr, "nullptr", NULL, NULL, VariableProtoType::cpy_shallow_if_temp);
#undef VM_CONSTRUCT_BUILTIN_TYPE
	}
	MAP<std::string, VariableDetail*> var_map; 
	MAP<std::string, VariableDetail*> var_temp_map;
	MAP<std::string, FunctionAST*> func_map;
	MAP<int, VariableType*> type_map;
	MAP<int, VariableProtoType*> var_proto_map;
	MAP<int, std::string> type_name_map;
	Scope global;
	Scope *cur_scope; //current scope
	Scope *scope_of_var = NULL;
	int temp_var_counter = 0;
	int type_counter = 0;
	char buf[1024];

	VariableDetail* insert(void *data, int type, std::string name = "@");
	void insert(VariableDetail* vd);

	VariableDetail* getVarByName(std::string name);
	//destroy element if it is a tem
	VariableDetail* destroyIfTemp(VariableDetail* vd);
	int updateVarList(MAP<std::string, VariableDetail*>&mmap, VariableDetail* vd);
	void deleteVariables(std::vector<VariableDetail*> *vars);
	void deleteVariable(VariableDetail* var) {
		if (var == NULL) return;
		Variable_Destructor vd = type_map[var->type]->default_vd;
		if (vd != NULL && var->data != NULL) {
			vd(var->data);
		}
		delete var;
	}
	void enterScope(std::string scope_name = "@") {
		cur_scope = cur_scope->addSubScope(scope_name);
	}
	void leaveScope() {
		cur_scope = cur_scope->selfDestruct(*this);
	}

	VariableDetail* newVar(double d) {
		double *dptr = new double(d);
		VariableDetail* vd = new VariableDetail("@", var_proto_map[vt_double]);//new VariableDetail("@", vt_double, dptr, getDestructor(vt_double));
		return vd;
	}
	VariableDetail* insertInCurrentScope(void *data, int type, std::string name);
	VariableDetail* insertInCurrentScope(VariableDetail* vd);
	VariableDetail* newVar(std::string Name) {
		int type = vt_unkown_type;
		char *str = NULL;
		if (isquotation(Name[0])) {
			str = new char[Name.size() + 1];
			type = vt_string;
			strcpy(str, Name.c_str() + 1);
			Name = "@"; //string should be anonymous
		}
		VariableDetail* vd = getVarByNameInScope(Name);
		if (vd == NULL) {
			// We can't get it from storage if not declared before
			vd = insertInCurrentScope(str, type, Name);
		}
		return vd;
	}
	VariableDetail* getVarByNameInScope(std::string name) {
		Scope *searcher = cur_scope;
		VariableDetail* result;
		while (result == NULL && searcher != NULL) {
			scope_of_var = searcher;
			result = searcher->getVarInScope(name);
			searcher = searcher->super;
		}
		return result;
	}
	VariableDetail* updateVarListInScope(VariableDetail* vd) {
		VariableDetail *result = getVarByNameInScope(vd->name);
		if (result != NULL)
			result->copyDatafrom(vd);
		return result;
	}
	/*
	void registerType(std::string t_name, Variable_Destructor default_vd = NULL)
	{
		VariableType vt = {
			type_counter++,
			default_vd
		};
		type_map[t_name] = vt;
	}
	void registerFunc(PrototypeAST *proto, const char *comment = NULL) {

	}*/
	// FunctionAST* registerFunction(FunctionAST* func) {
	// 	MAP<std::string, FunctionAST*>::iterator itr = func_map.find(func->Proto->Name);
	// 	
	// }
	/*TO DO -- Variable Type Security Check
	int checkFuncDataMatch(std::string funcname, std::vector<VariableDetail*> &args) {
	PrototypeAST *proto = func_map[funcname];
	}*/
	void clearTemps();
	Variable_Destructor getDestructor(int type);
};
VariableDetail* VaribaleMap::insertInCurrentScope(void *data, int type, std::string name) {
	VariableDetail* result = new VariableDetail(name, )
}
VariableDetail* VaribaleMap::insertInCurrentScope(VariableDetail* vd) {
	VariableDetail* result = updateVarListInScope(vd);
	if (!result) {
		if (vd->name == "@") {
			vd->name += _itoa(temp_var_counter++, buf, 16);
		}
		//we have already checked for duplicate name senario
		if (vd->name[0] != '@') {
			scope_of_var->var_map[vd->name] = vd;
		}
	}
	return result;
}
VariableDetail* VaribaleMap::insert(void *data, int type, std::string name)
{
	VariableDetail* vd = new VariableDetail(name, type, data, getDestructor(type));
	insert(vd);
	return vd;
}

void VaribaleMap::insert(VariableDetail* vd) {
	if (!updateVarList(var_map, vd) && !updateVarList(var_temp_map, vd)) {
		if (vd->name == "@") {
			vd->name += _itoa(temp_var_counter++, buf, 16);
		}
		if (vd->name[0] == '@') {
			var_temp_map[vd->name] = vd;
		}
		else {
			var_map[vd->name] = vd;
		}
	}
}
VariableDetail* VaribaleMap::getVarByName(std::string name)
{
	MAP<std::string, VariableDetail*>::iterator itr = var_map.find(name);
	// std::cout << "finding:" << name << std::endl;
	if (itr == var_map.end()) {
		return NULL;
	}
	else {
		return itr->second;
	}
}

VariableDetail* VaribaleMap::destroyIfTemp(VariableDetail* vd) {
	VariableDetail* vd_tmp = getVarByName(vd->name);
	if (vd_tmp == NULL ) {
		delete vd;
		return NULL;
	}
	return vd_tmp;
}

int VaribaleMap::updateVarList(MAP<std::string, VariableDetail*>&mmap, VariableDetail* vd) {
	MAP<std::string, VariableDetail*>::iterator itr = mmap.find(vd->name);
	VariableDetail* vd_tmp;
	if (itr != mmap.end())
	{
		if (vd != itr->second) {
			vd_tmp = itr->second;
			itr->second = vd;
			delete vd_tmp;
		}
		return 1; //find the element
	}
	return 0; //not found
}
void VaribaleMap::deleteVariables(std::vector<VariableDetail*> *vars)
{
	std::vector<VariableDetail*>::iterator itr = vars->begin();
	for (; itr < vars->end(); itr++) {
		if (*itr != NULL) {
			var_map.erase((*itr)->name);
			int type = (*itr)->type;
			// 'vt_unkown_type', 'vt_nullptr' are usually temp varibale.
			if (type == vt_unkown_type || type == vt_nullptr) {
				std::string type_name = type_name_map[type];
				std::string name = (*itr)->name;
				GlobalDebugger.throwWarning("del", "The Varibale \'%s\' of type %s may not be desired var to be deleted.", name.c_str(), type_name.c_str());
			}
			delete (*itr);
		}
	}
}

void VaribaleMap::clearTemps()
{
	MAP<std::string, VariableDetail*>::iterator itr = var_temp_map.begin();
	VariableDetail* tobeDel = NULL;
	for (; itr != var_temp_map.end();) {
		tobeDel = itr->second;
		itr = var_temp_map.erase(itr);

		delete tobeDel;
	}
}

Variable_Destructor VaribaleMap::getDestructor(int type) 
{
	MAP<int, VariableType*>::iterator itr = type_map.find(type);
	if (itr == type_map.end()) return NULL;
	else return itr->second->default_vd;
}

void destructor_double(void *d)
{
	double *dd = (double *)d;
	delete dd;
}

void destructor_string(void *str)
{
	//free(str);
	delete[](char *)str;
}

void destructor_array(void *arr)
{
	std::vector<VariableDetail*> *arrptr = (std::vector<VariableDetail*> *)arr;
	std::vector<VariableDetail*>::iterator itr = arrptr->begin();
	for (; itr < arrptr->end(); itr++) {
		delete (*itr);
	}
}

void destructor_Image(void *im)
{
	destroyImage((Image)im);
}

void* deepcpy_double(void *d)
{
	double *dd = new double(*(double *)d);
	return (void *)dd;
}

void* deepcpy_string(void *d)
{
	return NULL;
}

void* deepcpy_array(void *str) {
	return NULL;
}

void* deepcpy_Image(void *str) {
	return NULL;
}