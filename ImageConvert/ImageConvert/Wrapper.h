#pragma once
#include <vector>
#include "def_utils.h"
#include "bmp.h"
#include "VariablesManagementl.h"
#include "Kaleidoscope_utils.h"
#include "glim.h"
#include "config_macros.h"

//////////////////
// dirty but quick implement, will delete after 'import' is availble//---------------TODO
/////////////////

typedef VariableDetail* (*__FUNPTR_KERNEL_IMPL)(std::vector<VariableDetail*> *, VaribaleMap &);
typedef Image(*__FUNPTR_CALL_IMAGE_TARNSFORM)(Image, Vector3, STConfig);

VariableDetail* __call_TransforFunc__wrap_kernel(__FUNPTR_CALL_IMAGE_TARNSFORM func, std::vector<VariableDetail*> *vars, VaribaleMap &vm)
{
	static STConfig defaultstc = newSTConfig(NULL);
	if (vars->size() < 2) {
		printf("Missing Arguments.\n");
		return NULL;
	}
	std::vector<VariableDetail*> *vv = (std::vector<VariableDetail*> *)((*vars)[1]->data);
	if ((*vars)[1]->proto->type != vt_array) {
		printf("Expected Array for second Argument.\n");
		return NULL;
	}
	Vector3 v = {
		*(double *)((*vv)[0]->data),
		*(double *)((*vv)[1]->data),
		0.0,
	};
	STConfig stc;
	if (vars->size() == 2)
		stc = defaultstc;
	else
		stc = (STConfig)(*vars)[2]->data;

	Image res = func((Image)((*vars)[0]->data), v, stc);
	// return new VariableDetail("@", vt_Image, res, destructor_Image);
	return vm.newVar(res);
}

VariableDetail* imtranslate2d_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm)
{
	return __call_TransforFunc__wrap_kernel(imtranslate2d, vars, vm);
}
// EXTERNC Image imscale2d(Image src, Vector3 scaler, STConfig stc);
VariableDetail* imscale2d_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm)
{
	return __call_TransforFunc__wrap_kernel(imscale2d, vars, vm);
}


// EXTERNC Image imflip2d(Image src, Vector3 mirror, STConfig stc);
VariableDetail* imflip2d_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm)
{
	return __call_TransforFunc__wrap_kernel(imflip2d, vars, vm);
}

// EXTERNC Image imshear2d(Image src, Vector3 factor, STConfig stc);
VariableDetail* imshear2d_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm)
{
	return __call_TransforFunc__wrap_kernel(imshear2d, vars, vm);
}

//imrotate2d(Image src, double theta, Vector3 *axis, STConfig stc)
VariableDetail* imrotate2d_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm) {
	static STConfig stc = newSTConfig(NULL);
	if (vars->size() < 2) {
		printf("Missing Arguments.\n");
		return NULL;
	}
	Image res = imrotate2d((Image)((*vars)[0]->data), *(double *)((*vars)[1]->data), NULL, stc);
	return vm.newVar(res);//new VariableDetail("@", vt_Image, res, destructor_Image);
}


VariableDetail* imshow_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm) {
	if ((*vars)[0]->data != NULL)
		imshow((Image)(*vars)[0]->data);
	while (remainingWindows()) {
		refreshWindows(NULL);
	}
	return NULL;
}
VariableDetail* figure_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm) {
	static int inited = 0;
	if (!inited) {
		glimInit();
		inited = 1;
	}
	const char *title = "OpenGL";
	if ((*vars)[0]->data != NULL)
		title = (char *)(*vars)[0]->data;
	figure(title, "cs", CAMERA_HORIZONTAL, SHADER_TEXTURE| SHADER_CAMERA);
	return NULL;
}
template <typename Src, typename Dst>
Dst* vector_to_carr(void *vec) {
	if (vec == NULL) return NULL;
	VariableDetail* mvec = (VariableDetail*)vec;
	if (mvec->proto->type != vt_array)
		return NULL;

	std::vector<VariableDetail*> *arr = (std::vector<VariableDetail*> *)mvec->data;
	if (arr != NULL) {
		Dst* res = (Dst*)malloc(arr->size() * sizeof(Dst));
		int idx = 0;
		for (auto itr = arr->begin(); itr != arr->end(); itr++) {
			res[idx] = (Dst)(*(Src *)(*arr)[idx]->data);
			idx++;
		}
		return res;
	}
	return NULL;
}

#define VAR(X) ((*vars)[X]?(*vars)[X]->data:NULL)
#define VAR_T(X) ((*vars)[X]->proto->type)
//weak var
#define WVAR(X) ((vars->size() >= X+1)?VAR(X):NULL)
// image_t* fspecial(const char *name, ...);
VariableDetail* fspecial_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm) {
	static STConfig stc = newSTConfig(NULL);
	int *arr = NULL;
	if (vars->size() >= 2) {
		VariableDetail* vd = (*vars)[1];
		arr = vector_to_carr<double, int>((*vars)[1]);
	}
	image_t *res = NULL;
	if (vars->size() == 0)
		res = fspecial(NULL);
	else if (vars->size() == 1)
		res = fspecial((const char *)VAR(0));
	else if (vars->size() == 2)
		res = fspecial((const char *)VAR(0), arr);
	else if (vars->size() == 3) {
		if (VAR_T(2) == vt_double)
			res = fspecial((const char *)VAR(0), arr, *(double *)VAR(2));
		else
			res = fspecial((const char *)VAR(0), arr, VAR(2));
	}
		
	

	// Image res = imrotate2d((Image)((*vars)[0]->data), *(double *)((*vars)[1]->data), NULL, stc);
	return vm.newVar(res);//new VariableDetail("@", vt_Image, res, destructor_Image);
}

int extract_data(VariableDetail*from, double *data) {
	if (from == NULL || data == NULL || from->proto->type != vt_double)
		return -1; //failed
	(*data) = *(double *)(from->data);
	return 0;
}
// Image imfilter(Image src, image_t *kernel, int *filtersz);
VariableDetail* imfilter_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm) {
	Image src = (Image)WVAR(0);
	image_t *kernel = (image_t *)WVAR(1);
	int filtersz[2] = { 3 };
	int filtersz_idx = 0;
	Vector3 *bounds = NULL;
	VariableDetail* filter_factor = NULL;
	std::vector<VariableDetail*> *arr = (std::vector<VariableDetail*> *)WVAR(2);
	std::vector<VariableDetail*> *arr_bounds = (std::vector<VariableDetail*> *)WVAR(3);
	VariableDetail* obj_bounds = NULL;
	bool delKernel = false;
	if ((*vars)[0]->proto->type == vt_object && (*vars)[0]->proto->class_name != "Image") {
		VariableDetail* obj = (*vars)[0];
		Scope *scope = (Scope *)obj->data;
		src = (Image)(*obj)["src"]->data;
		kernel = (image_t *)(*obj)["filter"]->data;
		VariableDetail* arr_wrap = (*obj)["filter_size"];
		VariableDetail* bounds_wrap = (*obj)["bounds"];
		if (arr_wrap != NULL) {
			arr = (std::vector<VariableDetail*> *)arr_wrap->data;
		}
		if (bounds_wrap != NULL && bounds_wrap->proto->type == vt_array) {
			arr_bounds = (std::vector<VariableDetail*> *)bounds_wrap->data;
		}
		else if(bounds_wrap != NULL && bounds_wrap->proto->type == vt_object)
		{
			obj_bounds = bounds_wrap;
		}
		filter_factor = (*obj)["filter_factor"];
		
	}
	if (arr != NULL) {
		for (auto itr = arr->begin(); itr != arr->end() && filtersz_idx < 2; itr++) {
			filtersz[filtersz_idx] = CAST_INT(*(double *)(*arr)[filtersz_idx]->data);
			filtersz_idx++;
		}
	}
	if (filter_factor != NULL && filter_factor->data != NULL) {
		image_t factor = (image_t)(*(double *)filter_factor->data);
		int len = filtersz[0] * filtersz[1];
		delKernel = true;
		// we must create a new kernel, or else we may muitiply the factor by many times
		image_t *new_kernel = new image_t[len];
		for (--len; len >= 0; --len) {
			new_kernel[len] = kernel[len] * factor;
		}
		kernel = new_kernel;
	}
	if (arr_bounds != NULL || obj_bounds != NULL) {
		bounds = new Vector3[2];
		bounds[0].x = 0;
		bounds[0].y = 0;
		bounds[1].x = 1;
		bounds[1].y = 1;
	}
	if (arr_bounds != NULL) {
		int which_one = 0;
		for (auto itr = arr_bounds->begin(); itr != arr_bounds->end() && which_one < 2; itr++) {
			// VariableDetail* inner = (**itr)[0];
			// if(inner != NULL && inner->data != NULL) bounds[which_one].x = CAST_INT(*(double *)inner->data);
			// inner = (**itr)[1];
			// if (inner != NULL && inner->data != NULL) bounds[which_one].y = CAST_INT(*(double *)inner->data);
			extract_data((**itr)[0], &(bounds[which_one].x));
			extract_data((**itr)[1], &(bounds[which_one].y));
			which_one++;
		}
	}
	if (obj_bounds != NULL) {
		extract_data((*obj_bounds)["left"], &(bounds[0].x));
		extract_data((*obj_bounds)["top"],  &(bounds[0].y));
		extract_data((*obj_bounds)["right"],  &(bounds[1].x));
		extract_data((*obj_bounds)["bottom"], &(bounds[1].y));
	}
	Image res = imfilter(src, kernel, filtersz, bounds);
	delete[] bounds;
	if (delKernel)
		delete[] kernel;
	return vm.newVar(res);
}
// void imfuse(Image lhs, const Image rhs, image_t alpha, image_t beta, image_t delta, int *layers)
VariableDetail* imfuse_wrap(std::vector<VariableDetail*> *vars, VaribaleMap &vm) {
	Image lhs = (Image)WVAR(0);
	Image rhs = (Image)WVAR(1);
	image_t params[] = { 0.42f, 0.99f, 0.0f };
	int param = 0;
	int*layers = NULL;
	if (vars->size() >= 3) {
		if ((*vars)[2]->proto->type != vt_array) {
			printf("Expected Array for second Argument.\n");
		}
		else {
			std::vector<VariableDetail*> *arr = (std::vector<VariableDetail*> *)(*vars)[2]->data;
			for (auto itr = arr->begin(); itr != arr->end() && param < 3; itr++) {
				params[param] = CAST_IMAGE_T(*(double *)(*arr)[param]->data);
				param++;
			}
		}
	}
	imfuse(lhs, rhs, params[0], params[1], params[2], layers);
	return NULL;
}
#undef VAR