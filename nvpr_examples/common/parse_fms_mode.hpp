
// parse_fms_mode.hpp - common parsing for mixed samples mode strings

/* Copyright NVIDIA Corporation, 2015. */

#ifndef __PARSE_FMS_MODE_H__
#define __PARSE_FMS_MODE_H__

bool parseMixedSamplesModeString(const char *mode, int &stencil_samples, int &color_samples, bool &supersample);
void modeOptionHelp(const char *program_name);

#endif // __PARSE_FMS_MODE_H__
