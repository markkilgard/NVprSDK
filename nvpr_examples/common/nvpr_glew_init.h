
/* nvpr_glew_init.h - initialize NV_path_rendering API */

/* Copyright NVIDIA Corporation, 2015. */

#ifndef __NVPR_GLEW_INIT_H__
#define __NVPR_GLEW_INIT_H__

#include <stdio.h>
#include <GL/glew.h>

#ifdef  __cplusplus
extern "C" {
#endif

extern int has_NV_path_rendering;

int initialize_NVPR_GLEW_emulation(FILE *output, const char *programName, int quiet);

#ifdef  __cplusplus
}
#endif

#endif /* __NVPR_GLEW_INIT_H__ */
