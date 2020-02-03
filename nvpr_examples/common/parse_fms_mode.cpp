
// parse_fms_mode.cpp - common parsing for mixed samples mode strings

#include <stdio.h>
#include <string.h>

extern const char *program_name;

static bool isPowerOfTwo (int x)
{
  // First x in the below expression is for the case when x is 0.
  return x && (!(x&(x-1)));
}

bool parseMixedSamplesModeString(const char *mode,
                                 int &stencil_samples,
                                 int &color_samples,
                                 bool &supersample)
{
  int s = 0, c = 0;
  int chars_parsed = 0;  // for %n
  size_t mode_strlen = strlen(mode);
  int rc = sscanf(mode, "%d:%ds%n", &s, &c, &chars_parsed);
  if (rc == 2 && size_t(chars_parsed) == mode_strlen && isPowerOfTwo(s) && isPowerOfTwo(c)) {
    stencil_samples = s;
    color_samples = c;
    supersample = true;
    return true;
  }
  rc = sscanf(mode, "%d:%d%n", &s, &c, &chars_parsed);
  if (rc == 2 && size_t(chars_parsed) == mode_strlen && isPowerOfTwo(s) && isPowerOfTwo(c)) {
    stencil_samples = s;
    color_samples = c;
    supersample = false;
    return true;
  }
  rc = sscanf(mode, "%dxs%n", &c, &chars_parsed);
  if (rc == 1 && size_t(chars_parsed) == mode_strlen && isPowerOfTwo(c)) {
    stencil_samples = c;
    color_samples = c;
    supersample = true;
    return true;
  }
  rc = sscanf(mode, "%dx%n", &c, &chars_parsed);
  if (rc == 1 && size_t(chars_parsed) == mode_strlen && isPowerOfTwo(c)) {
    stencil_samples = c;
    color_samples = c;
    supersample = false;
    return true;
  }
  // No valid mode format matched.
  return false;
}

void modeOptionHelp(const char *program_name)
{
  printf("%s: -mode expects a framebuffer mode\n",
    program_name);
  printf("EXAMPLES\n"
    "  8:1  ==> 8 stencil samples and 1 color sample per pixel\n"
    "           (requires NV_framebuffer_mixed_samples support)\n"
    "  8x   ==> 8 stencil samples and 8 color sample per pixel\n"
    "           (conventional multisampling)\n"
    "  4:2s ==> 2x2 supersampling of 4 & 2 stencil & color samples per pixel\n"
    "           (effectively quadruples quality)\n"
    "DETAILS\n"
    "  M:N  ==> M stencil samples, N color samples\n"
    "           Valid M values: 1, 2, 4, 8, 16\n"
    "           Valid N values: 1, 2, 4, 8\n"
    "           M must be >= N\n"
    "  M:Ns ==> optional 's' suffix requests 2x2 supersampling\n"
    "  Nx   ==> Same as N:N\n"
    "  Nxs  ==> Same as N:Ns\n");
}
