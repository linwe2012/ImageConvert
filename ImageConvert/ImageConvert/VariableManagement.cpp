#include "VariablesManagementl.h"
#include "Kaleidoscope.h"

VariableDetail* VariableDetail::operator[](std::string &_name) {
	Scope *obj = (Scope*)data;
	VariableDetail*res = NULL;
	if (data == NULL) {
		proto = proto->vm->var_proto_map[vt_unkown_type];
	}
	else
		res = obj->getVarInScope(_name);
	if (res == NULL) {
		if (proto->type == vt_nullptr || proto->type == vt_unkown_type) {
			proto->vm->enterScope(name);
			res = proto->vm->newVar(_name);
			proto = proto->vm->var_proto_map[vt_object];
			data = proto->vm->cur_scope;
			proto->vm->leaveScope();
		}
	}
	return res;
}

VariableDetail *VaribaleMap::newVar(FunctionAST *func) {
	VariableDetail *vd = cur_scope->getVarInScope(func->Proto->Name);
	VariableDetail *res = new VariableDetail(func->Proto->Name, var_proto_map[vt_function], func);
	if (vd == NULL)
		vd = cur_scope->var_map[func->Proto->Name] = res;
	else {
		*vd = *res;
		delete res;
	}
	return vd;
}

VariableDetail* VaribaleMap::insertInCurrentScope(void *data, int type, std::string name) {
	VariableDetail* result = new VariableDetail(name, var_proto_map[type], data);
	insertInCurrentScope(result);
	return result;
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
void VaribaleMap::deleteVariables(std::vector<VariableDetail*> *vars)
{
	std::vector<VariableDetail*>::iterator itr = vars->begin();
	VariableDetail* res;
	for (; itr < vars->end(); itr++) {
		if (*itr != NULL) {
			res = getVarByNameInScope((*itr)->name);
			if (res != NULL)
				scope_of_var->undeclareVarInScope((*itr)->name);
			delete res;

		}
	}
}

VariableDetail* VaribaleMap::destroyIfTemp(VariableDetail* vd) {
	VariableDetail* vd_tmp = getVarByNameInScope(vd->name);
	if (vd_tmp == NULL) {
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

void destructor_double(void *d)
{
	double *dd = (double *)d;
	delete dd;
}

void destructor_string(void *str)
{
	free((char*)str);
	// delete[](char *)str;
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

void* deepcpy_double(const void *d)
{
	double *dd = new double(*(double *)d);
	return (void *)dd;
}

void* deepcpy_string(const void *str)
{
	char *res = (char *)malloc(strlen((char*)str) + 1);
	return strcpy(res, (char*)str);
}

void* deepcpy_array(const void *str) {
	return NULL;
}

void* deepcpy_Image(const void *str) {
	return NULL;
}

int istrue_double(const void *d) {
	if (*(double *)d)
		return 1;
	else return 0;
}
int istrue_string(const void *str) {
	if (str && *(char*)str != '\0')
		return 1;
	else
		return 0;
}
int istrue_array(const void * arr) {
	return (arr) ? 1 : 0;
}
int istrue_Image(const void *im) {
	return (im) ? 1 : 0;
}

int selfCompare_double(const void *lhs, const void *rhs) {
	return *(double *)lhs == *(double *)rhs;
}
int selfCompare_string(const void *lhs, const void *rhs) {
	return (!strcmp((char *)lhs, (char *)rhs));
}
int selfCompare_array(const void *lhs, const void *rhs) {
	int flag = VariableProtoType::cmp_res::equal;
	int size;
	int thisterm;
	if (lhs == rhs) return true;
	if (lhs == NULL || rhs == NULL) return VariableProtoType::cmp_res::misequal;
	std::vector<VariableDetail*> *vsl = (std::vector<VariableDetail*> *)lhs;
	std::vector<VariableDetail*> *vsr = (std::vector<VariableDetail*> *)rhs;
	std::vector<VariableDetail*>::iterator itl = vsl->begin();
	std::vector<VariableDetail*>::iterator itr = vsr->begin();
	size = vsl->size();
	if (size != vsr->size()) return VariableProtoType::cmp_res::misequal;
	for (; itl < vsl->end(); itl++, itr++) {
		thisterm = (**itl == **itr);
		if (thisterm != VariableProtoType::cmp_res::equal) {
			return VariableProtoType::cmp_res::array_sub_mismatch | thisterm;
		}
	}
	return VariableProtoType::cmp_res::equal;
}
int selfCompare_Image(const void *lhs, const void *rhs) {
	return lhs == rhs;
}

void destructor_object(void *scope) { delete (Scope *)scope; };
void *deepcpy_object(const void *) { return NULL; };
int istrue_object(const void *) { return 0; };
int selfCompare_object(const void *, const void *) { return 0; };
//---------------------------------------------------TODO
void log_double(VariableDetail *){};
void log_string(VariableDetail *){};
void log_array (VariableDetail *){};
void log_Image (VariableDetail *){};
void log_unkown_type(VariableDetail *){};
void log_function   (VariableDetail *){};
void log_object     (VariableDetail *){};
void log_pod_ptr    (VariableDetail *){};
void log_null_ptr   (VariableDetail *){};