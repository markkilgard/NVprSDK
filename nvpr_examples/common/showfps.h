#ifndef SHOWFPS_H
#define SHOWFPS_H

/* showfps.h - OpenGL code for rendering frames per second */

/* Call handleFPS in your GLUT display callback every frame. */

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FPS_QUADS (12*4)

typedef enum {
    FPS_USAGE_TEXTURE,
    FPS_USAGE_BITMAP
} FPSusage;

typedef struct _FPScontext {
    FPSusage usage;
    int width, height;
    GLuint fps_text_texture;
    double last_fpsRate;
    float last_scale;
    GLint count;
    GLfloat varray[4*MAX_FPS_QUADS];
} FPScontext;

extern void initFPScontext(FPScontext *, FPSusage);
extern void reshapeFPScontext(FPScontext *ctx, int w, int h);
extern void releaseFPScontext(FPScontext *ctx);

extern double just_handleFPS(void);
extern double handleFPS(FPScontext *);
extern void toggleFPSunits(void);
extern void reportFPSinMS(void);
extern void reportFPSinFPS(void);
extern void toggleFPS();
extern void enableFPS();
extern void disableFPS();
extern void colorFPS(float r, float g, float b);
extern void scaleFPS(float new_scale);
extern double getElapsedTime();
extern void invalidateFPS();

#ifdef __cplusplus
}
#endif

#endif /* SHOWFPS_H */
