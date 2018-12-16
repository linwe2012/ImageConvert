#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <memory.h>
#include "bmp.h"
#include "Kaleidoscope_Debugger.h"
#include "Kaleidoscope_utils.h"
#include "builtin_types.h"
#define MAP std::map
class FunctionAST;
class PrototypeAST;
class VaribaleMap;
class VariableDetail;
class VaribaleMap;
typedef void  (*Variable_Destructor)(void *);
typedef void* (*Variable_DeepCpy)(const void *);
typedef int (*Variable_IsTrue)(const void *);
typedef int(*Variable_SelfCompare)(const void *, const void *);
typedef void(*Variable_log)(VariableDetail *);
extern KDebugger GlobalDebugger;

enum var_types {
	vt_double, // pointer to double
	vt_nullptr,
	vt_Image, // Image
	vt_unkown_type, //mostly cause of error due to var is not properly assigned with value
	vt_string, //string pointer
	vt_array, //void *data = std::vector<VariableDetail*> *vars;
	vt_function,
	vt_object,
	vt_pod_ptr, //must be malloced pointer
};

void destructor_double(void *d);
void destructor_string(void *str);
void destructor_array(void *arr);
void destructor_Image(void *im);
void destructor_object(void *);

void *deepcpy_double(const void *);
void *deepcpy_string(const void *);
void *deepcpy_array(const void *);
void *deepcpy_Image(const void *);
void *deepcpy_object(const void *);

int istrue_double(const void *);
int istrue_string(const void *);
int istrue_array(const void *);
int istrue_Image(const void *);
int istrue_object(const void *);

int selfCompare_double(const void *, const void *);
int selfCompare_string(const void *, const void *);
int selfCompare_array (const void *, const void *);
int selfCompare_Image (const void *, const void *);
int selfCompare_object(const void *, const void *);

void log_double(VariableDetail *);
void log_string(VariableDetail *);
void log_array (VariableDetail *);
void log_Image (VariableDetail *);
void log_unkown_type(VariableDetail *);
void log_function(VariableDetail *);
void log_object(VariableDetail *);
void log_pod_ptr(VariableDetail *);
void log_null_ptr(VariableDetail *);

class VariableProtoType {
public:
	enum {
		cpy_deep = 0x1,
		cpy_shallow = 0x2,
		cpy_shallow_if_temp = 0x4,//shallow if temp will overide cpy_deep
	};
	enum cmp_res {
		equal = 1,
		misequal = 0,
		type_mismatch = -0x2,
		array_sub_mismatch = -0x4,
		uncomparable = -0x8,
	};
	int type;
	std::string class_name;
	std::string type_name;
	int default_cpy_method;
	Variable_DeepCpy deepCpy;
	Variable_Destructor destructor;
	Variable_IsTrue istrue;
	Variable_SelfCompare selfcmp;
	Variable_log log;
	VaribaleMap *vm;
	
	VariableProtoType(int mtype, std::string tname, VaribaleMap *_vm,
		Variable_Destructor des, Variable_DeepCpy dp, 
		Variable_IsTrue it, Variable_SelfCompare sc, Variable_log _log, int cpy_method, std::string _class_name = "@")
	:type(mtype), type_name(tname), destructor(des), deepCpy(dp), istrue(it), selfcmp(sc), 
		log(_log), default_cpy_method(cpy_method), class_name(_class_name), vm(_vm) {}
};

class VariableDetail {
public:
	std::string name;
	void *data;
	VariableProtoType *proto;
	VariableDetail(std::string _name, VariableProtoType *_proto, void *_data)
		:name(_name), proto(_proto), data(_data){}
	~VariableDetail()
	{
		clearData();
	}
	VariableDetail(const VariableDetail &vd)
		:name("@")
	{
		*this = vd;
		// copyDatafrom(&vd);
	}
	void copyDatafrom(const VariableDetail *src) {
		if (src->data != data)
			clearData();
		data = src->data;
		proto = src->proto;
	}
	
	void operator=(const VariableDetail &rhs) {
		int method = rhs.proto->default_cpy_method;
		if ((method & VariableProtoType::cpy_shallow_if_temp) && rhs.name[0] == '@' || method & VariableProtoType::cpy_shallow) {
			copyDatafrom(&rhs);
		}
		else if (method & VariableProtoType::cpy_deep) {
			if (rhs.proto->deepCpy != NULL) {
				data = rhs.proto->deepCpy(rhs.data);
			}
			proto = rhs.proto;
		}
	}
	char *auto_merge_str(char *left, const char *right) {
		left = (char *)realloc(left, strlen(left) + strlen(right) + 1);
		return strcat((char*)left, right);
	}
	int is_plus_valid(const VariableDetail &a, const VariableDetail &b) {
		int t1 = a.proto->type;
		int t2 = b.proto->type;
		return (t1 == t2 && (t1 == vt_double || t1 == vt_string || t1 == vt_array)) || \
			(t1 == vt_string && t2 == vt_double) || (t2 == vt_string && t1 == vt_double);
	}
	void operator+=(const VariableDetail &rhs) {
		if (!is_plus_valid(*this, rhs)) return;
		if (data == NULL && rhs.data != NULL) {
			*this = rhs;
			return;
		}
		else if (data == NULL && rhs.data == NULL)
			return;
		if (proto->type == vt_double && rhs.proto->type == vt_double) {
			*(double *)data += *(double*)rhs.data;
		}
		else if (proto->type == vt_string && rhs.proto->type == vt_string) {
				data = auto_merge_str((char *)data, (char *)rhs.data);
		}
		else if (proto->type == vt_string && rhs.proto->type == vt_double) {
			std::stringstream numbuf;
			numbuf << *(double *)rhs.data;
			std::string strbuf = numbuf.str();
			data = auto_merge_str((char*)data, strbuf.c_str());
		}
		else if (proto->type == vt_double && rhs.proto->type == vt_string) {
			std::stringstream numbuf;
			numbuf << *(double *)rhs.data;
			std::string strbuf = numbuf.str();
			clearData();
			char *strbuff = (char *)malloc(sizeof(strbuf.c_str()));
			strcpy(strbuff, strbuf.c_str());
			data = auto_merge_str(strbuff, (char *)rhs.data);
			proto = rhs.proto;
		}
	}
	VariableDetail* operator[](VariableDetail* vd) {
		if (vd == NULL) return NULL;
		if (vd->proto->type == vt_double) return (*this)[(int)(*(double *)vd->data)];
		if (vd->proto->type == vt_string) return (*this)[(char *)data];
		return NULL;
	}
	VariableDetail* operator[](int idx) {
		if (proto->type != vt_array) {
			return NULL;
		}
		if (data == NULL) return NULL;
		std::vector<VariableDetail*>* arr = (std::vector<VariableDetail*>*)data;
		if (arr->size() <= (size_t)idx) {
			return NULL;
		}
		return (*arr)[idx];
	}
	VariableDetail *operator[](std::string &_name);
	VariableDetail *operator[](const char *_name) {
		std::string str(_name);
		return (*this)[str];
	};
	int operator==(const VariableDetail &rhs) {
		if (proto != rhs.proto)
			return VariableProtoType::cmp_res::type_mismatch;
		if (proto->selfcmp == NULL) return VariableProtoType::cmp_res::uncomparable;
		return proto->selfcmp(this->data, rhs.data);
	}
	VariableDetail * operator+(const VariableDetail &a) {
		VariableDetail *vd = new VariableDetail(*this);
		*vd += a;
		return vd;
	}
	int istrue() {
		return (data) ? proto->istrue(data) : false;
	}
	int elem_cnt() {
		if (data == NULL) return 0;
		if (proto->type == vt_array) return ((std::vector<VariableDetail*>*)data)->size();
		if (proto->type == vt_object) return 1;/////////---------------------------------------------TODO
		return 1;
	}
	void clearData() {
		if (data && proto && proto->destructor)
			proto->destructor(data);
	}
};

class Scope {
public:
	std::string name;
	MAP<std::string, VariableDetail*> var_map;
	MAP<std::string, Scope*> subs;
	Scope *super;
	int flags;
	std::vector<VariableDetail*> * Return_Vals;
	enum {
		flag_leaving = 0x1,
		flag_detached = 0x2,
	};
	Scope(std::string nname, Scope *ssuper):name(nname), super(ssuper), flags(0), Return_Vals(NULL){}
	~Scope() {
		auto itr = var_map.begin();
		for (; itr != var_map.end(); itr++) {
			delete itr->second;
		}
		auto sub_itr = subs.begin();
		for (; sub_itr != subs.end(); itr++) {
			delete sub_itr->second;
		}
	}
	Scope *addSubScope(std::string &_name) {
		Scope *exists = findinSub(_name);
		if (exists) //already in the sub scope
			return NULL;
		else {
			exists = new Scope(_name, this);
			subs[_name] = exists;
		}
		return exists;
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
		detachFromSuper();
		MAP<std::string, VariableDetail*>::iterator itr = var_map.begin();
		for (; itr != var_map.end(); itr++) {
			delete itr->second;
		}
		delete this;
		return sus;
	}
	// do not search super scope
	VariableDetail* getVarInScope(std::string &_name)
	{
		MAP<std::string, VariableDetail*>::iterator itr = var_map.find(_name);
		if (itr == var_map.end()) return NULL;
		else return itr->second;
	}
	VariableDetail* undeclareVarInScope(std::string &_name) {
		var_map.erase(_name);
		return NULL;
	}
	// detachFromSuper - disappear in super scope,
	// still remember original super scope
	Scope *detachFromSuper() {
		flags |= flag_detached;
		if (super != NULL) {
			auto self = super->subs.find(name);
			if (self == super->subs.end()) return NULL;
			//self->second = NULL;
			subs.erase(name);
		}
		return this;
	}
	int deleteVarInScope(std::string _name) {
		auto itr = var_map.find(_name);
		if (itr == var_map.end()) return -1;
		VariableDetail *tobedel = itr->second;
		itr->second = NULL;
		var_map.erase(itr);
		delete tobedel;
		return 0;
	}
};

class VaribaleMap {
public:
	typedef unsigned int flag_t;
	enum {
		fl_seduce_scope_with_existed = 0x1,
		fl_seduce_scope_no_existed = 0x2,
	};
private:
	void __MASK_FLAG(flag_t mask, bool ifmask) {
		if (ifmask) {
			flags |= mask;
		}
		else {
			flags &= !mask;
		}
	}
	bool __CHECK_FLAG(flag_t mask) {
		return (mask & flags);
	}
public:
	VaribaleMap() 
		:global("_GLOBAL_", NULL), cur_scope(&global)
	{
		type_name_map[vt_nullptr] = "nullptr";
		type_name_map[vt_unkown_type] = "unkown_type";
#define VM_CONSTRUCT_BUILTIN_TYPE(X, method) var_proto_map[vt_##X] = new VariableProtoType( \
vt_##X, #X, this, destructor_##X, deepcpy_##X, istrue_##X, selfCompare_##X, log_##X, \
		method | VariableProtoType::cpy_shallow_if_temp)
		VM_CONSTRUCT_BUILTIN_TYPE(double, VariableProtoType::cpy_deep);
		VM_CONSTRUCT_BUILTIN_TYPE(array,  VariableProtoType::cpy_shallow);
		// VM_CONSTRUCT_BUILTIN_TYPE(Image,  VariableProtoType::cpy_shallow);
		VM_CONSTRUCT_BUILTIN_TYPE(string, VariableProtoType::cpy_deep);
		VM_CONSTRUCT_BUILTIN_TYPE(object, VariableProtoType::cpy_shallow);
		var_proto_map[vt_unkown_type] = new VariableProtoType(vt_unkown_type, "unkown_type", this, NULL, NULL, NULL, NULL, log_unkown_type, VariableProtoType::cpy_shallow_if_temp);
		var_proto_map[vt_nullptr] = new VariableProtoType(vt_nullptr, "nullptr", this, NULL, NULL, NULL, NULL, log_null_ptr, VariableProtoType::cpy_shallow_if_temp);
		var_proto_map[vt_function] = new VariableProtoType(vt_function, "function", this, NULL, NULL, NULL, NULL, log_function, VariableProtoType::cpy_shallow); // function always shallow, non-destructable
		// image is an object class of "Image"
		// var_proto_map[vt_object] = new VariableProtoType(vt_object, "object", this, destructor_Object, deepcpy_Object, istrue_Object, selfCompare_Object, log_Objecte, VariableProtoType::cpy_shallow, "@Whatever");
		var_proto_map[vt_Image] = new VariableProtoType(vt_object, "object", this, destructor_Image, deepcpy_Image, istrue_Image, selfCompare_Image, log_Image, VariableProtoType::cpy_shallow, "Image");
		var_proto_map[vt_pod_ptr] = new VariableProtoType(vt_object, "pod_ptr", this, destructor_string, deepcpy_string, istrue_string, selfCompare_string, log_pod_ptr, VariableProtoType::cpy_deep | VariableProtoType::cpy_shallow_if_temp, "Plain Old Data Pointer");
#undef VM_CONSTRUCT_BUILTIN_TYPE
	}
	MAP<int, VariableProtoType*> var_proto_map;
	MAP<int, std::string> type_name_map;
	std::vector<std::string> scope_seducer;
	Scope global;
	Scope *cur_scope; //current scope
	Scope *scope_of_var = NULL;
	int temp_var_counter = 0;
	int type_counter = 0;
	char buf[1024];
	flag_t flags = 0;
	//destroy element if it is a tem
	VariableDetail* destroyIfTemp(VariableDetail* vd);
	int updateVarList(MAP<std::string, VariableDetail*>&mmap, VariableDetail* vd);
	void deleteVariables(std::vector<VariableDetail*> *vars);
	void deleteVariable(VariableDetail* var) {
		if (var == NULL) return;
		Variable_Destructor vd = var->proto->destructor;
		if (vd != NULL && var->data != NULL) {
			vd(var->data);
		}
		delete var;
	}
	// void enterTempScope(std::string scope_name = "@") {
	// 	cur_scope = cur_scope->addSubScope(scope_name);
	// }
	
	void leaveTempScope() {
		cur_scope = cur_scope->selfDestruct(*this);
	}
	int enterScope(std::string &scope_name) {
		Scope *res = cur_scope->findinSub(scope_name);
		if (scope_name == "@") {
			// function abc {}, 'abc' will seduce '{}'
			// function already create a scope of abc
			if (__CHECK_FLAG(fl_seduce_scope_with_existed)) {
				__MASK_FLAG(fl_seduce_scope_with_existed, false);
				return 0;
			}
			// m = function {}, 'm' will seduce 'function {}'
			// but scope of 'm' not yet created
			else if(__CHECK_FLAG(fl_seduce_scope_no_existed)) {
				auto last = scope_seducer.back();
				scope_seducer.pop_back();
				return enterScope(last);
			}
		}
		if (res == NULL) {
			cur_scope = cur_scope->addSubScope(scope_name);
		}
		else cur_scope = res;
		return 0;
	}
	void leaveScope() {
		if (cur_scope->super == NULL) return;
		if (cur_scope->name[0] != '@') {
			cur_scope = cur_scope->super;
		}
		else {
			// leaving a temp scope will delete the scope.
			cur_scope = cur_scope->selfDestruct(*this);
		}
	}
	VariableDetail* newVar(double d) {
		double *dptr = new double(d);
		VariableDetail* vd = new VariableDetail("@", var_proto_map[vt_double], dptr);//new VariableDetail("@", vt_double, dptr, getDestructor(vt_double));
		return vd;
	}
	VariableDetail *newVar(std::vector<VariableDetail*> *vars) {
		return new VariableDetail("@", var_proto_map[vt_array], vars);
	}
	VariableDetail *newVar(Image im) {
		return new VariableDetail("@", var_proto_map[vt_Image], im);
	}
	// nullptr
	VariableDetail *newVar() {
		return new VariableDetail("@", var_proto_map[vt_nullptr], NULL);
	}
	template<typename T>
	VariableDetail *newVar(T unkown_type) {
		return new VariableDetail("@", var_proto_map[vt_unkown_type], unkown_type);
	}
	template<typename T>
	VariableDetail *newVar(T* pod_ptr) {
		return new VariableDetail("@", var_proto_map[vt_pod_ptr], pod_ptr);
	}
	// object -- lhs:rhs, support following format
	// "abc" : 123,
	//  efg  : 7
	VariableDetail *newVar(VariableDetail* lhs, VariableDetail*rhs) {
		if (rhs->name != "@") {
			lhs->data = rhs->data;
			lhs->proto = rhs->proto;
			cur_scope->var_map[lhs->name] = lhs;
			return lhs;
		}
		if (lhs->proto->type == vt_string) {
			rhs->name = (char *)lhs->data;
		}
		else {
			rhs->name = lhs->name;
		}
		deleteIfInCurrentScope(lhs);
		cur_scope->var_map[rhs->name] = rhs;
		return rhs;
	}
	VariableDetail *scopeToObject() {
		cur_scope->detachFromSuper();
		return new VariableDetail(cur_scope->name, var_proto_map[vt_object], cur_scope);
	}
	VariableDetail *newVar(FunctionAST *func);
	VariableDetail* insertInCurrentScope(void *data, int type, std::string name);
	VariableDetail* insertInCurrentScope(VariableDetail* vd);
	VariableDetail* newVar(std::string Name) {
		int type = vt_unkown_type;
		char *str = NULL;
		if (isquotation(Name[0])) {
			str = (char *)malloc(Name.size() + 1);
			type = vt_string;
			strcpy(str, Name.c_str() + 1);
			Name = "@"; //string should be anonymous
		}
		VariableDetail* vd = getVarByNameInScope(Name);
		if (vd != NULL) {

		}
		else if (vd == NULL) {
			vd = new VariableDetail(Name, var_proto_map[type], str);
			// We can't get it from storage if not declared before
			if(Name[0] != '@')
				insertInCurrentScope(vd);
		}
		return vd;
	}
	
	VariableDetail* getVarByNameInScope(std::string name) {
		Scope *searcher = cur_scope;
		VariableDetail* result = NULL;
		while (result == NULL && searcher != NULL) {
			scope_of_var = searcher;
			result = searcher->getVarInScope(name);
			searcher = searcher->super;
		}
		return result;
	}
	VariableDetail* updateVarListInScope(VariableDetail *vd) {
		VariableDetail *result = getVarByNameInScope(vd->name);
		if (result != NULL)
			result->copyDatafrom(vd);
		return result;
	}
	VariableDetail* deleteIfTempProtectDataIfShallow(VariableDetail* vd) {
		if (vd->name[0] == '@') {
			if (vd->proto->default_cpy_method & VariableProtoType::cpy_shallow_if_temp || vd->proto->default_cpy_method & VariableProtoType::cpy_shallow)
				vd->data = NULL;//protect data since it is shollow copied somewhere
			delete vd;
			return NULL;
		}
		return vd;
	}
	VariableDetail* deleteIfTemp(VariableDetail* vd) {
		if (vd == NULL) return NULL;
		if (vd->name[0] == '@') {
			delete vd;
			return NULL;
		}
		return vd;
	}
	VariableDetail *deleteIfInCurrentScope(VariableDetail* vd) {
		if (vd != NULL) {
			cur_scope->deleteVarInScope(vd->name);
		}
		return NULL;
	}
	void setLeavingThisScope(std::vector<VariableDetail*> * vars) {
		cur_scope->flags |= Scope::flag_leaving;
		cur_scope->Return_Vals = vars;
	}
	int checkShouldLeaveScope() 
	{
		return cur_scope->flags & Scope::flag_leaving;
	}
	std::vector<VariableDetail*> * getReturnVarThisScope() {
		return cur_scope->Return_Vals;
	}
	void bindScopeData() {

	}
	// seduce the next anonymous scope
	// namely capture the next scope if it is anonymous
	void setSeduceAnonyScope() {
		__MASK_FLAG(fl_seduce_scope_with_existed, true);
	}
	void setSeduceAnonyScope(std::string &scope_name) {
		scope_seducer.push_back(scope_name);
		__MASK_FLAG(fl_seduce_scope_with_existed, false);
		__MASK_FLAG(fl_seduce_scope_no_existed, true);
	}
	void setApathyAnonyScope() {
		__MASK_FLAG(fl_seduce_scope_with_existed, false);
	}
	Scope *swapScope(void *another) {
		Scope *last_scope = cur_scope;
		if (another == NULL) return NULL;
		cur_scope = (Scope *)another;
		return last_scope;
	}
	// void clearTemps();
	// Variable_Destructor getDestructor(int type);
};

// VariableDetail* VaribaleMap::insert(void *data, int type, std::string name)
// {
// 	VariableDetail* vd = new VariableDetail(name, type, data, getDestructor(type));
// 	insert(vd);
// 	return vd;
// }

// void VaribaleMap::insert(VariableDetail* vd) {
// 	if (!updateVarList(var_map, vd) && !updateVarList(var_temp_map, vd)) {
// 		if (vd->name == "@") {
// 			vd->name += _itoa(temp_var_counter++, buf, 16);
// 		}
// 		if (vd->name[0] == '@') {
// 			var_temp_map[vd->name] = vd;
// 		}
// 		else {
// 			var_map[vd->name] = vd;
// 		}
// 	}
// }
// VariableDetail* VaribaleMap::getVarByName(std::string name)
// {
// 	MAP<std::string, VariableDetail*>::iterator itr = var_map.find(name);
// 	// std::cout << "finding:" << name << std::endl;
// 	if (itr == var_map.end()) {
// 		return NULL;
// 	}
// 	else {
// 		return itr->second;
// 	}
// }


// void VaribaleMap::deleteVariables(std::vector<VariableDetail*> *vars)
// {
// 	std::vector<VariableDetail*>::iterator itr = vars->begin();
// 	for (; itr < vars->end(); itr++) {
// 		if (*itr != NULL) {
// 			var_map.erase((*itr)->name);
// 			int type = (*itr)->type;
// 			// 'vt_unkown_type', 'vt_nullptr' are usually temp varibale.
// 			if (type == vt_unkown_type || type == vt_nullptr) {
// 				std::string type_name = type_name_map[type];
// 				std::string name = (*itr)->name;
// 				GlobalDebugger.throwWarning("del", "The Varibale \'%s\' of type %s may not be desired var to be deleted.", name.c_str(), type_name.c_str());
// 			}
// 			delete (*itr);
// 		}
// 	}
// }
// 
// void VaribaleMap::clearTemps()
// {
// 	MAP<std::string, VariableDetail*>::iterator itr = var_temp_map.begin();
// 	VariableDetail* tobeDel = NULL;
// 	for (; itr != var_temp_map.end();) {
// 		tobeDel = itr->second;
// 		itr = var_temp_map.erase(itr);
// 
// 		delete tobeDel;
// 	}
// }
// 
// Variable_Destructor VaribaleMap::getDestructor(int type) 
// {
// 	MAP<int, VariableType*>::iterator itr = type_map.find(type);
// 	if (itr == type_map.end()) return NULL;
// 	else return itr->second->default_vd;
// }
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
