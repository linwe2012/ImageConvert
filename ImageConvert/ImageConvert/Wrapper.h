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

typedef Image(*__FUNPTR_CALL_IMAGE_TARNSFORM)(Image, Vector3, STConfig);

VariableDetail* __call_TransforFunc__wrap_kernel(__FUNPTR_CALL_IMAGE_TARNSFORM func, std::vector<VariableDetail*> *vars)
{
	static STConfig defaultstc = newSTConfig(NULL);
	if (vars->size() < 2) {
		printf("Missing Arguments.\n");
		return NULL;
	}
	std::vector<VariableDetail*> *vv = (std::vector<VariableDetail*> *)((*vars)[1]->data);
	if ((*vars)[1]->type != vt_array) {
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
	return new VariableDetail("@", vt_Image, res, destructor_Image);
}

VariableDetail* imtranslate2d_wrap(std::vector<VariableDetail*> *vars)
{
	return __call_TransforFunc__wrap_kernel(imtranslate2d, vars);
}
// EXTERNC Image imscale2d(Image src, Vector3 scaler, STConfig stc);
VariableDetail* imscale2d_wrap(std::vector<VariableDetail*> *vars)
{
	return __call_TransforFunc__wrap_kernel(imscale2d, vars);
}


// EXTERNC Image imflip2d(Image src, Vector3 mirror, STConfig stc);
VariableDetail* imflip2d_wrap(std::vector<VariableDetail*> *vars)
{
	return __call_TransforFunc__wrap_kernel(imflip2d, vars);
}

// EXTERNC Image imshear2d(Image src, Vector3 factor, STConfig stc);
VariableDetail* imshear2d_wrap(std::vector<VariableDetail*> *vars)
{
	return __call_TransforFunc__wrap_kernel(imshear2d, vars);
}

//imrotate2d(Image src, double theta, Vector3 *axis, STConfig stc)
VariableDetail* imrotate2d_wrap(std::vector<VariableDetail*> *vars) {
	static STConfig stc = newSTConfig(NULL);
	if (vars->size() < 2) {
		printf("Missing Arguments.\n");
		return NULL;
	}
	Image res = imrotate2d((Image)((*vars)[0]->data), *(double *)((*vars)[1]->data), NULL, stc);
	return new VariableDetail("@", vt_Image, res, destructor_Image);
}


VariableDetail* imshow_wrap(std::vector<VariableDetail*> *vars) {
	if ((*vars)[0]->data != NULL)
		imshow((Image)(*vars)[0]->data);
	while (remainingWindows()) {
		refreshWindows(NULL);
	}
	return NULL;
}
VariableDetail* figure_wrap(std::vector<VariableDetail*> *vars) {
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

