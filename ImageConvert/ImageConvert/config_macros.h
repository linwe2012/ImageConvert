#ifndef _CONFIG_MACROS_H_
#define _CONFIG_MACROS_H_

// it seems cpp doesn't support TRUE & FALSE
#ifndef TRUE
#define TRUE (1 == 1) // boolean
#endif // !TRUE

#ifndef FALSE
#define FALSE !TRUE
#endif // !FALSE


#define SHADER_TEXTURE 0x1
#define SHADER_CAMERA (0x1 << 1)
#define CAMERA_DISABLE 0x0
#define CAMERA_HORIZONTAL (0x1)
#define CAMERA_3D (0x1 << 1)

#define SCRIPT_EXECUTE TRUE
#endif // !_CONFIG_MACROS_H_

