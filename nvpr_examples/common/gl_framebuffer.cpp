
// gl_framebuffer.cpp - simple/powerful OpenGL framebuffer class

// Copyright (c) NVIDIA Corporation. All rights reserved.

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1  // Suppress Visusl Studio deprecation warnings about sscanf
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>

#include "countof.h"
#include "gl_framebuffer.hpp"

bool GLFramebufferCapabilities::extensionSupported(const char *extension)
{
  const GLubyte *start;
  const GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *)strchr(extension, ' ');
  if (where || *extension == '\0') {
    return false;
  }

  if (!gl_extensions) {
    gl_extensions = glGetString(GL_EXTENSIONS);
  }
  if (!gl_extensions) {
    return false;
  }
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string.  Don't be fooled by sub-strings,
     etc. */
  start = gl_extensions;
  for (;;) {
    /* If your application crashes in the strstr routine below,
       you are probably calling this without
       having a current window.  Calling glGetString without
       a current OpenGL context has unpredictable results.
       Please fix your program. */
    where = (const GLubyte *)strstr((const char *)start, extension);
    if (!where) {
      break;
    }
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return true;
      }
    }
    start = terminator;
  }
  return false;
}

# define GOT_EXTENSION(x) (extensionSupported("GL_" #x) ? true : false)

void GLFramebufferCapabilities::decide_gl_version()
{
  assert(gl_minor_version == 0);
  const char *version_string = reinterpret_cast<const char *>(glGetString(GL_VERSION));
  if (version_string) {
    int got_major, got_minor;
    if (sscanf(version_string, "%d.%d", &got_major, &got_minor) == 2) {
      gl_major_version = got_major;
      gl_minor_version = got_minor;
    }
  }
}

void GLFramebufferCapabilities::decide_framebuffer_object() {
  if (hasOpenGL_3_0()) {
    has_framebuffer_object = Yes;
  }
  else if (GOT_EXTENSION(ARB_framebuffer_object)) {
    has_framebuffer_object = Yes;
  }
  else {
    has_framebuffer_object = No;
  }
}

void GLFramebufferCapabilities::decide_layered_framebuffer() {
  if (hasOpenGL_3_0()) {
    has_layered_framebuffer = Yes;
  }
  else if (GOT_EXTENSION(GL_ARB_geometry_shader4)) {
    has_layered_framebuffer = Yes;  // Would have to use glFramebufferTextureARB
  }
  else if (GOT_EXTENSION(GL_EXT_geometry_shader4)) {
    has_layered_framebuffer = Yes;  // Would have to use glFramebufferTextureEXT
  }
  else {
    has_layered_framebuffer = No;
  }
}

void GLFramebufferCapabilities::decide_framebuffer_mixed_samples() {
  if (GOT_EXTENSION(NV_framebuffer_mixed_samples)) {
    has_framebuffer_mixed_samples = Yes;
  } else {
    has_framebuffer_mixed_samples = No;
  }
}

void GLFramebufferCapabilities::decide_explicit_multisample() {
  if (GOT_EXTENSION(NV_explicit_multisample)) {
    has_explicit_multisample = Yes;
  } else {
    has_explicit_multisample = No;
  }
}

void GLFramebufferCapabilities::decide_sample_locations() {
  if (GOT_EXTENSION(NV_sample_locations)) {
    has_sample_locations = Yes;
  } else {
    has_sample_locations = No;
  }
}

void GLFramebufferCapabilities::decide_framebuffer_sRGB()
{
  if (hasOpenGL_3_0()) {
    has_framebuffer_sRGB = Yes;
  }
  else if (GOT_EXTENSION(ARB_framebuffer_sRGB)) {
    has_framebuffer_sRGB = Yes;
  }
  else {
    has_framebuffer_sRGB = No;
  }
}

void GLFramebufferCapabilities::decide_texture_multisample()
{
  if (hasOpenGL_3_2()) {
    has_texture_multisample = Yes;
  }
  else if (GOT_EXTENSION(ARB_texture_multisample)) {
    has_texture_multisample = Yes;
  }
  else {
    has_texture_multisample = No;
  }
}

void GLFramebufferCapabilities::decide_texture_storage()
{
  if (hasOpenGL_4_2()) {
    has_texture_storage = Yes;
  }
  else if (GOT_EXTENSION(ARB_texture_storage)) {
    has_texture_storage = Yes;
  }
  else {
    has_texture_storage = No;
  }
}

void GLFramebufferCapabilities::decide_texture_storage_multisample()
{
  if (hasOpenGL_4_3()) {
    has_texture_storage_multisample = Yes;
  }
  else if (GOT_EXTENSION(ARB_texture_storage_multisample)) {
    has_texture_storage_multisample = Yes;
  }
  else {
    has_texture_storage_multisample = No;
  }
}

void GLFramebufferCapabilities::decide_texture_view()
{
  if (hasOpenGL_4_3()) {
    has_texture_view = Yes;
  }
  else if (GOT_EXTENSION(ARB_texture_view)) {
    has_texture_view = Yes;
  }
  else if (GOT_EXTENSION(OES_texture_view)) {
    has_texture_view = Yes;
  }
  else if (GOT_EXTENSION(EXT_texture_view)) {
    has_texture_view = Yes;
  }
  else {
    has_texture_view = No;
  }
}

void GLFramebufferCapabilities::decide_ARB_direct_state_access()
{
  if (hasOpenGL_4_5()) {
    has_ARB_direct_state_access = Yes;
  } else if (GOT_EXTENSION(ARB_direct_state_access)) {
    has_ARB_direct_state_access = Yes;
  } else {
    has_ARB_direct_state_access = No;
  }
}

void GLFramebufferCapabilities::decide_ARB_texture_storage()
{
  if (GOT_EXTENSION(ARB_texture_storage)) {
    has_ARB_texture_storage = Yes;
  } else {
    has_ARB_texture_storage = No;
  }
}

void GLFramebufferCapabilities::decide_ARB_texture_storage_multisample()
{
  if (GOT_EXTENSION(ARB_texture_storage_multisample)) {
    has_ARB_texture_storage_multisample = Yes;
  } else {
    has_ARB_texture_storage_multisample = No;
  }
}

void GLFramebufferCapabilities::decide_ARB_texture_view()
{
  if (GOT_EXTENSION(ARB_texture_view)) {
    has_ARB_texture_view = Yes;
  } else {
    has_ARB_texture_view = No;
  }
}

void GLFramebufferCapabilities::decide_ARB_framebuffer_no_attachments()
{
  if (GOT_EXTENSION(ARB_framebuffer_no_attachments)) {
    has_ARB_framebuffer_no_attachments = Yes;
  } else {
    has_ARB_framebuffer_no_attachments = No;
  }
}

void GLFramebufferCapabilities::decide_EXT_direct_state_access()
{
  if (GOT_EXTENSION(EXT_direct_state_access)) {
    has_EXT_direct_state_access = Yes;
  } else {
    has_EXT_direct_state_access = No;
  }
}

void GLFramebufferCapabilities::decide_EXT_texture_storage()
{
  if (GOT_EXTENSION(EXT_texture_storage)) {
    has_EXT_texture_storage = Yes;
  } else {
    has_EXT_texture_storage = No;
  }
}

void GLFramebufferCapabilities::decide_EXT_texture_view()
{
  if (GOT_EXTENSION(EXT_texture_view)) {
    has_EXT_texture_view = Yes;
  } else {
    has_EXT_texture_view = No;
  }
}

void GLFramebufferCapabilities::decide_EXT_window_rectangles()
{
  if (GOT_EXTENSION(EXT_window_rectangles)) {
    has_EXT_window_rectangles = Yes;
  } else {
    has_EXT_window_rectangles = No;
  }
}

void GLFramebufferCapabilities::decide_ARB_geometry_shader4()
{
  if (GOT_EXTENSION(ARB_geometry_shader4)) {
    has_ARB_geometry_shader4 = Yes;
  } else {
    has_ARB_geometry_shader4 = No;
  }
}

void GLFramebufferCapabilities::decide_EXT_geometry_shader4()
{
  if (GOT_EXTENSION(EXT_geometry_shader4)) {
    has_EXT_geometry_shader4 = Yes;
  } else {
    has_EXT_geometry_shader4 = No;
  }
}

void GLFramebufferCapabilities::decide_OES_texture_view()
{
  if (GOT_EXTENSION(OES_texture_view)) {
    has_OES_texture_view = Yes;
  } else {
    has_OES_texture_view = No;
  }
}

void GLFramebufferCapabilities::decide_GL_MAX_COLOR_ATTACHMENTS_value()
{
  GL_MAX_COLOR_ATTACHMENTS_value = 0;
  if (hasOpenGL_3_0() || GOT_EXTENSION(ARB_framebuffer_object) || GOT_EXTENSION(EXT_framebuffer_object)) {
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &GL_MAX_COLOR_ATTACHMENTS_value);
  }
}

void GLFramebufferCapabilities::decide_default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value()
{
  GLuint default_framebuffer = 0;
  // XXX use GL_BACK_LEFT instead of simply GL_BACK or GL_FRONT to
  // workaround a bug in pre-OpenGL 4.5 NVIDIA drivers where GL_BACK
  // (and GL_FRONT) generate errors.
  const GLenum query_attachment = GL_BACK_LEFT;
#ifdef GL_VERSION_4_5
  if (hasARBDirectStateAccess()) {
    GLint v = GL_INVALID_VALUE;
    glGetNamedFramebufferAttachmentParameteriv(default_framebuffer,
      query_attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &v);
    default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value = GLenum(v);
  }
  else
#endif
#ifdef GL_EXT_direct_state_access
  if (hasEXTDirectStateAccess()) {
    GLint v = GL_INVALID_VALUE;
    glGetNamedFramebufferAttachmentParameterivEXT(default_framebuffer,
      query_attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &v);
    default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value = GLenum(v);
  }
  else
#endif
  {
    // Lacks ARB_direct_state_access or EXT_direct_state_access so must
    // save/restore the current FBO.
    GLint save_framebuffer_object;  // Technically GLuint
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &save_framebuffer_object); {
      const GLuint default_framebuffer_binding = 0;
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, default_framebuffer_binding);
      GLint v = GL_INVALID_VALUE;
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
          query_attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &v);
      default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value = GLenum(v);
    } glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(save_framebuffer_object));
  }
}

GLFramebufferCapabilities::GLFramebufferCapabilities()
  : gl_extensions(NULL)
  , gl_major_version(0)
  , gl_minor_version(0)
  , GL_MAX_COLOR_ATTACHMENTS_value(-1)  // Undetermined yet.
  , default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value(GL_INVALID_VALUE)
{
  // GLFramebufferCapabilities relies on CapStatusInit default constructor to set CapStatus values to Undetermined.
}

// Declare smart_-prefixed functions early to avoid non-smart usage.

void GLFramebuffer::smart_glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
  if (capabilities.hasOpenGL_3_2()) {
    glFramebufferTexture(target, attachment, texture, level);
    return;
  }
  if (capabilities.hasARBGeometryShader4()) {
    glFramebufferTextureARB(target, attachment, texture, level);
    return;
  }
  assert(capabilities.hasEXTGeometryShader4());
  glFramebufferTextureEXT(target, attachment, texture, level);
}

#undef glFramebufferTexture
#undef glFramebufferTextureARB
#undef glFramebufferTextureEXT
#define glFramebufferTexture @use smart_glFramebufferTexture
#define glFramebufferTextureARB @use smart_glFramebufferTexture
#define glFramebufferTextureEXT @use smart_glFramebufferTexture

void GLFramebuffer::smart_glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers)
{
  if (capabilities.hasOpenGL_4_3() || capabilities.hasARBTextureView()) {
    glTextureView(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
    return;
  }
#ifdef GL_OES_texture_view  // GLEW lacks this extension currently.
  if (capabilities.hasOESTextureView()) {
    glTextureViewOES(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
    return;
  }
#endif
#if defined(GL_EXT_texture_storage)
  assert(capabilities.hasEXTTextureView());
  glTextureViewEXT(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
#else
  assert(!"no glTextureView available");
#endif
}

#undef glTextureView
#undef glTextureViewOES
#undef glTextureViewEXT
#define glTextureView @use smart_glTextureView
#define glTextureViewOES @use smart_glTextureView
#define glTextureViewEXT @use smart_glTextureView

void GLFramebuffer::smart_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
  if (capabilities.hasOpenGL_4_2() || capabilities.hasARBTextureStorage()) {
    glTexStorage2D(target, levels, internalformat, width, height);
    return;
  }
#if defined(GL_EXT_texture_storage)
  assert(capabilities.hasEXTTextureStorage());
  glTexStorage2DEXT(target, levels, internalformat, width, height);
#else
  assert(!"no glTexStorage2D available");
#endif
}

#undef glTexStorage2D
#undef glTexStorage2DEXT
#define glTexStorage2D @use smart_glTexStorage2D
#define glTexStorage2DEXT @use smart_glTexStorage2D

void GLFramebuffer::smart_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
  if (capabilities.hasOpenGL_4_2() || capabilities.hasARBTextureStorage()) {
    glTexStorage3D(target, levels, internalformat, width, height, depth);
    return;
  }
#if defined(GL_EXT_texture_storage)
  assert(capabilities.hasEXTTextureStorage());
  glTexStorage3DEXT(target, levels, internalformat, width, height, depth);
#else
  assert(!"no glTexStorage3D available");
#endif
}

#undef glTexStorage3D
#undef glTexStorage3DEXT
#define glTexStorage3D @use smart_glTexStorage3D
#define glTexStorage3DEXT @use smart_glTexStorage3D

GLFramebuffer::GLFramebuffer(GLFramebufferCapabilities &caps,
                             GLenum cf, GLenum dsf,
                             int w, int h, int s,
                             int cov_samples, int col_samples, BufferMode m)
  : capabilities(caps)
  , color_format(cf)
  , depth_stencil_format(dsf)
  , alternate_color_format(GL_NONE)
  , width(w)
  , height(h)
  , slices(s)
  , coverage_samples(cov_samples)
  , color_samples(col_samples)
  , buffer_mode(m)
  , alpha_handling(StraightAlpha)
  , coverage_modulation(true)
  , layer_mode(AllSlices)
  , bound_layered_slice(BogusLayer)  // bogus value
  , fbo_render(fbos[0])
  , fbo_downsample(fbos[1])
  , tex_color(textures[0])
  , tex_downsample(textures[1])
  , tex_depth_stencil(textures[2])
  , alt_tex_color_view(textures[3])
  , tex_color_target(GL_NONE)
  , tex_downsample_target(GL_NONE)
  , direct_state_access(false)
  , depth_attach(false)
  , stencil_attach(false)
  , dirty(true)
  , invalidate_after_blit(false) // XXX making true can slow down rendering :-(
{
  assert(s >= 1);
  // Hard requirement for GLFramebuffer: GL implementation must support framebuffer
  // object (OpenGL 3.0 or ARB_framebuffer_object extension).
  assert(caps.hasFramebufferObject());

  for (size_t i = 0; i < countof(fbos); i++) {
      fbos[i] = 0;
  }
  assert(fbo_render == 0);
  assert(fbo_downsample == 0);

  for (size_t i = 0; i < countof(textures); i++) {
      textures[i] = 0;
  }
  assert(tex_color == 0);
  assert(tex_depth_stencil == 0);
  assert(tex_downsample == 0);
  assert(alt_tex_color_view == 0);

}

const GLenum *GLFramebuffer::drawBuffersList() {
  static const GLenum draw_buffers_list[] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4,
    GL_COLOR_ATTACHMENT5,
    GL_COLOR_ATTACHMENT6,
    GL_COLOR_ATTACHMENT7
  };
  return draw_buffers_list;
}

void GLFramebuffer::deallocateSliceFBOs()
{
  if (slice_fbo.size() > 0) {
    glDeleteFramebuffers(GLsizei(slice_fbo.size()), &slice_fbo[0]);
    slice_fbo.clear();
  }
}

inline void GLFramebuffer::markDirty()
{
  dirty = true;
}

void GLFramebuffer::deallocate()
{
  deallocateSliceFBOs();

  // Deallocate textures and framebuffer objects as group.
  glDeleteTextures(countof(textures), textures);
  tex_color = 0;
  tex_downsample = 0;
  tex_depth_stencil = 0;
  alt_tex_color_view = 0;
  glDeleteFramebuffers(countof(fbos), fbos);
  fbo_render = 0;
  fbo_downsample = 0;

  markDirty();
}

void GLFramebuffer::resize(int w, int h)
{
  if (width != w || height != h) {
    width = w;
    height = h;
    markDirty();
  }
}
void GLFramebuffer::setQuality(int cov_samples, int col_samples, BufferMode m)
{
  if ((cov_samples != coverage_samples) ||
      (col_samples != color_samples) ||
      (m != buffer_mode)) {
    coverage_samples = cov_samples;
    color_samples = col_samples;
    if (coverage_samples != color_samples) {
      // Downgrade depth/stencil format to drop the depth buffer.
      switch (depth_stencil_format) {
      case GL_DEPTH_COMPONENT24:
      case GL_DEPTH_COMPONENT32F:
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT32F_NV:  // NV_depth_buffer_float
        depth_stencil_format = GL_NONE;
        break;
      case GL_DEPTH24_STENCIL8:
      case GL_DEPTH32F_STENCIL8:
      case GL_DEPTH32F_STENCIL8_NV:  // NV_depth_buffer_float
        depth_stencil_format = GL_STENCIL_INDEX8;
        break;
      }
    }
    buffer_mode = m;
    markDirty();
  }
}

void GLFramebuffer::setDepthStencilFormat(GLenum dsf)
{
  if (dsf != depth_stencil_format) {
    depth_stencil_format = dsf;
    if (capabilities.hasFramebufferMixedSamples()) {
      if (dsf == GL_STENCIL_INDEX8) {
        assert(coverage_samples <= 16);
      } else {
        // Maxmimum of 8 depth samples per pixel.
        if (coverage_samples > 8) {
          coverage_samples = 8;
        }
      }
    }
    markDirty();
  }
}

void GLFramebuffer::setSupersampled(bool ss, BufferMode mode)
{
  if (ss) {
    setBufferMode(Supersample);
  } else {
    setBufferMode(mode);
  }
}

void GLFramebuffer::setSlices(int s)
{
  assert(s >= 1);
  if (s != slices) {
    slices = s;
    markDirty();
  }
}

void GLFramebuffer::setBufferMode(BufferMode m)
{
  if (m != buffer_mode) {
      buffer_mode = m;
      markDirty();
  }
}

void GLFramebuffer::setAlphaHandling(AlphaHandling ah)
{
  alpha_handling = ah;
}

void GLFramebuffer::setLayerMode(LayerMode lm)
{
  if (lm == Layered) {
    assert(capabilities.hasLayeredFramebuffer());
  }
  // If the layer mode changes...
  if (lm != layer_mode) {
    // Update layer mode and dirty state.
    layer_mode = lm;
    markDirty();
    bound_layered_slice = BogusLayer;
  }
}

// Decoder ring for color component variables:
//   Rca  = result premultiplied color
//   Ra   = result alpha
//   Sf   = source fractional coverage
//   Sc   = source straight color
//   Sa   = source alpha
//   Sa'  = source alpha modulated by source fractional coverage
//   Sca  = source pre-multiplied color
//   Sca' = source pre-multiplied color modulated by source fractional coverage
//   Dca  = destination premultiplied color
//   Da   = destination alpha

void GLFramebuffer::coverageModulationConfiguration()
{
  // If mixed samples...
  if (isMixedSamples()) {
    if (capabilities.hasFramebufferMixedSamples()) {
      if (coverage_modulation) {
        // If straight alpha (non-premultiplied alpha)...
        if (alpha_handling == StraightAlpha) {
          glCoverageModulationNV(GL_ALPHA);  // Sa' = Sa*Sf
        } else {
          assert(alpha_handling == PremultipliedAlpha);
          glCoverageModulationNV(GL_RGBA);  // Sca' = Sca*Sf
        }
      } else {
        glCoverageModulationNV(GL_NONE);  // Sca' = Sca*1
      }
    } else {
      assert(!"framebuffer claims mixed samples without mixed samples GL support");
    }
  } else {
    // Only need to configurate coverage modulation for mixed samples.
  }
}

void GLFramebuffer::enableCoverageModulation()
{
  if (!coverage_modulation) {
    coverage_modulation = true;
    coverageModulationConfiguration();
  }
}

void GLFramebuffer::disableCoverageModulation()
{
  if (coverage_modulation) {
    coverage_modulation = false;
    coverageModulationConfiguration();
  }
}

void GLFramebuffer::blendConfiguration()
{
  coverageModulationConfiguration();

  // If straight alpha (non-premultiplied alpha)...
  if (alpha_handling == StraightAlpha) {
    // Blend "over" formula:
    //   Rca = Sc*(Sf*Sa) + (1-Sa*Sf)*Dca
    //   Ra  = Sa*Sf      + (1-Sa*Sf)*Da
    glBlendFuncSeparate(
      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,  // Rca = ...
      GL_ONE, GL_ONE_MINUS_SRC_ALPHA);       // Ra  = ...
  } else {
    assert(alpha_handling == PremultipliedAlpha);
    // Blend "over" formula:
    //   Rca = Sca*Sf + (1-Sa*Sf)*Dca
    //   Ra  = Sa*Sf  + (1-Sa*Sf)*Da
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Rca,Ra = ...
  }
  glEnable(GL_BLEND);
}

GLenum GLFramebuffer::requestColorEncodingConfiguration(GLenum encoding)
{
  if (capabilities.hasFramebufferSRGB()) {
    if (encoding == GL_SRGB && sRGBColorEncoding()) {
      glEnable(GL_FRAMEBUFFER_SRGB);
      return GL_SRGB;
    } else { 
      glDisable(GL_FRAMEBUFFER_SRGB);
      return GL_LINEAR;
    }
  } else {
    // Only allow GL_LINEAR (not GL_SRGB) if sRGB framebuffer support not available.
    assert(encoding == GL_LINEAR);
    return GL_LINEAR;
  }
}

#ifndef NDEBUG
static bool framebufferIsBoundToDraw(GLuint fbo)
{
  GLint current_draw_fbo = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &current_draw_fbo);
  return GLuint(current_draw_fbo) == fbo;
}
#endif

void GLFramebuffer::colorEncodingConfiguration(GLenum encoding)
{
  (void) requestColorEncodingConfiguration(encoding);
}

void GLFramebuffer::validateLayerModeAllSlices()
{
  assert(layer_mode == AllSlices);
  assert(framebufferIsBoundToDraw(fbo_render));
  if (color_format != GL_NONE) {
    if (slices > 1) {
      // Slices in AllSlices mode is limited to the maximum number of color attachments.
      assert(slices <= capabilities.maxColorAttachments());
      // Using a multisample 2D texture array?
      if (color_samples > 1) {
        // Yes, use glFramebufferTexture3D.
        assert(capabilities.hasTextureMultisample());
        for (int slice=0; slice<slices; slice++) {
          glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+slice,
            GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_color, base_level, slice);
        }
      } else {
        for (int slice=0; slice<slices; slice++) {
          glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+slice,
            tex_color, base_level, slice);
        }
      }
    } else {
      if (capabilities.hasTextureMultisample() && color_samples > 1) {
        assert(tex_color_target == GL_TEXTURE_2D_MULTISAMPLE);
      } else {
        assert(tex_color_target == GL_TEXTURE_2D);
      }
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        tex_color_target, tex_color, base_level);
    }
  }
  bound_layered_slice = AllLayers;  // Bound to "all slices" at once.
}

void GLFramebuffer::validateLayerModePerSlice()
{
  assert(layer_mode == PerSlice);
  assert(framebufferIsBoundToDraw(fbo_render));
  glDrawBuffers(1, drawBuffersList());
  bound_layered_slice = BogusLayer;
}

void GLFramebuffer::validateLayerModeLayered()
{
  assert(capabilities.hasLayeredFramebuffer());
  assert(layer_mode == Layered);
  assert(framebufferIsBoundToDraw(fbo_render));

  if (color_format != GL_NONE) {
    smart_glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      tex_color, base_level);       
  }
  if (depth_stencil_format != GL_NONE) {
    if (depth_attach) {
      smart_glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
        tex_depth_stencil, base_level);
    }
    if (stencil_attach) {
      smart_glFramebufferTexture(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
        tex_depth_stencil, base_level);
    }
  }
  glDrawBuffers(1, drawBuffersList());
  bound_layered_slice = AllLayers;  // Bound to "all slices" at once.
}

void GLFramebuffer::validateLayerMode()
{
  switch (layer_mode) {
  case PerSlice:
    validateLayerModePerSlice();
    break;
  case AllSlices:
    validateLayerModeAllSlices();
    break;
  case Layered:
    validateLayerModeLayered();
    allocateSliceFBOs();
    break;
  }
}

void GLFramebuffer::allocateSliceFBO(const GLenum target, const GLint slice)
{
  assert(layer_mode == Layered);
  const bool has_texture_multisample = capabilities.hasTextureMultisample();
  assert(slice_fbo[slice] != 0);  // glGenFramebuffers should have happened already.
  glBindFramebuffer(GL_FRAMEBUFFER, slice_fbo[slice]);
  if (color_format != GL_NONE) {
    if (color_samples > 1) {
      glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_color, base_level, slice);
    } else {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        tex_color, base_level, slice);
    }
  }

  // Layered mode needs to bind the appropriate slice's depth/stencil buffer.
  if (depth_attach) {
    if (capabilities.hasTextureMultisample() && coverage_samples > 1) {
      glFramebufferTexture3D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_depth_stencil, base_level, slice);
    } else {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        tex_depth_stencil, base_level, slice);
    }
  }
  if (stencil_attach) {
    if (capabilities.hasTextureMultisample() && coverage_samples > 1) {
      glFramebufferTexture3D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
        GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_depth_stencil, base_level, slice);
    } else {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
        tex_depth_stencil, base_level, slice);
    }
  }
}

void GLFramebuffer::allocateSliceFBOs()
{
  assert(layer_mode == Layered);
  if (slices > 1) {
    if (slice_fbo.size() > 0) {
      glDeleteFramebuffers(GLsizei(slice_fbo.size()), &slice_fbo[0]);
    }
    slice_fbo.resize(slices);
    glGenFramebuffers(GLsizei(slice_fbo.size()), &slice_fbo[0]);

    GLenum target = GL_TEXTURE_2D;
    const bool has_texture_multisample = capabilities.hasTextureMultisample();
    if (has_texture_multisample && coverage_samples > 1) {
      target = GL_TEXTURE_2D_MULTISAMPLE;
    }

    for (size_t slice=0; slice<slice_fbo.size(); slice++) {
      allocateSliceFBO(target, GLint(slice));
    }
  } else {
    // No need to create per-slice FBOs if there is only one slice; just use
    // the main FBO when binding to a slice.
  }
}

void GLFramebuffer::setDownsampleTextureParameters()
{
  glTexParameteri(tex_downsample_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(tex_downsample_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(tex_downsample_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(tex_downsample_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void GLFramebuffer::allocateDownsampleFBO(int w, int h)
{
  assert(buffer_mode == Supersample || buffer_mode == Resolve);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_downsample);
  // Does the downsample FBO needs multiple slices?
  if (layer_mode != PerSlice && slices > 1) {
    assert(layer_mode == AllSlices || layer_mode == Layered);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex_downsample);
    if (capabilities.hasTextureStorage()) {
      const GLsizei one_level = 1;  // No mipmaps
      smart_glTexStorage3D(GL_TEXTURE_2D_ARRAY, one_level, color_format, w, h, slices);
    } else {
      const GLint level_zero = 0;
      const GLint no_border = 0;
      const GLenum irrelevant_type = GL_UNSIGNED_BYTE;
      const void* without_image_data = NULL;
      glTexImage3D(GL_TEXTURE_2D_ARRAY, level_zero, color_format, w, h, slices, no_border, GL_RGBA, irrelevant_type, without_image_data);
    }
    if (layer_mode == AllSlices) {
      assert(slices <= capabilities.maxColorAttachments());
      for (int slice = 0; slice < slices; slice++) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slice,
          tex_downsample, base_level, slice);
      }
    }
    tex_downsample_target = GL_TEXTURE_2D_ARRAY;
  } else {
    glBindTexture(GL_TEXTURE_2D, tex_downsample);
    if (capabilities.hasTextureStorage()) {
      const GLsizei one_level = 1;  // No mipmaps
      smart_glTexStorage2D(GL_TEXTURE_2D, one_level, color_format, w, h);
    } else {
      const GLint level_zero = 0;
      const GLint no_border = 0;
      const GLenum irrelevant_type = GL_UNSIGNED_BYTE;
      const void* without_image_data = NULL;
      glTexImage2D(GL_TEXTURE_2D, level_zero, color_format, w, h, no_border, GL_RGBA, irrelevant_type, without_image_data);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, tex_downsample, base_level);
    tex_downsample_target = GL_TEXTURE_2D;
  }
  setDownsampleTextureParameters();
}

void GLFramebuffer::allocateRenderFBOColorAttachmentOneSlice(int w, int h)
{
  if (capabilities.hasTextureMultisample() && color_samples > 1) {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex_color);
    const GLboolean  fixed_sample_locations = GL_TRUE;
    // Favor texture storage if supported so texture views are supported.
    if (capabilities.hasTextureStorageMultisample()) {
      glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
        color_samples, color_format, w, h, fixed_sample_locations);
    } else {
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
        color_samples, color_format, w, h, fixed_sample_locations);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D_MULTISAMPLE, tex_color, base_level);
    tex_color_target = GL_TEXTURE_2D_MULTISAMPLE;
  } else {
    glBindTexture(GL_TEXTURE_2D, tex_color);
    if (capabilities.hasTextureStorage()) {
      const GLsizei one_level = 1;  // No mipmaps
      smart_glTexStorage2D(GL_TEXTURE_2D, one_level, color_format, w, h);
    } else {
      const GLint level_zero = 0;
      const GLint no_border = 0;
      const GLenum irrelevant_type = GL_UNSIGNED_BYTE;
      const void* without_image_data = NULL;
      glTexImage2D(GL_TEXTURE_2D, level_zero, color_format, w, h, no_border, GL_RGBA, irrelevant_type, without_image_data);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, tex_color, base_level);
    tex_color_target = GL_TEXTURE_2D;
  }
}

void GLFramebuffer::allocateRenderFBOColorAttachmentMultiSlice(int w, int h)
{
  if (color_samples > 1) {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_color);
    const GLboolean  fixed_sample_locations = GL_TRUE;
    // Favor texture storage if supported so texture views are supported.
    if (capabilities.hasTextureStorageMultisample()) {
      glTexStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
        color_samples, color_format, w, h, slices, fixed_sample_locations);
    } else {
      glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
        color_samples, color_format, w, h, slices, fixed_sample_locations);
    }
    if (layer_mode == AllSlices) {
      assert(slices <= capabilities.maxColorAttachments());
      for (int slice = 0; slice < slices; slice++) {
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slice,
          GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_color, base_level, slice);
      }
    }
    tex_color_target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
  } else {
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex_color);
    // Favor texture storage if supported so texture views are supported.
    if (capabilities.hasTextureStorage()) {
      const GLsizei one_level = 1;  // No mipmaps
      smart_glTexStorage3D(GL_TEXTURE_2D_ARRAY, one_level, color_format, w, h, slices);
    } else {
      const GLint level_zero = 0;
      const GLint no_border = 0;
      const GLenum irrelevant_type = GL_UNSIGNED_BYTE;
      const void* without_image_data = NULL;
      glTexImage3D(GL_TEXTURE_2D_ARRAY, level_zero, color_format, w, h, slices, no_border, GL_RGBA, irrelevant_type, without_image_data);
    }
    if (layer_mode == AllSlices) {
      assert(slices <= capabilities.maxColorAttachments());
      for (int slice = 0; slice < slices; slice++) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slice,
          tex_color, base_level, slice);
      }
    }
    tex_color_target = GL_TEXTURE_2D_ARRAY;
  }
  const int num_attachments = (layer_mode == AllSlices) ? slices : 1;
#if 0 // OpenGL 4.5
  glNamedFramebufferDrawBuffers(fbo_render, num_attachments, drawBuffersList());
#else
  glDrawBuffers(num_attachments, drawBuffersList());
#endif
}

void GLFramebuffer::allocateRenderFBOColorAttachment(int w, int h)
{
  // Does this framebuffer lack a color buffer?
  if (color_format == GL_NONE) {
    // Yes, return without allocating color attachments.
    return;
  }

  if (slices > 1 || layer_mode == Layered) {
    allocateRenderFBOColorAttachmentMultiSlice(w, h);
  } else {
    allocateRenderFBOColorAttachmentOneSlice(w, h);
  }
}

void GLFramebuffer::applyDepthStencilFormat()
{
  switch (depth_stencil_format) {
  default:
    // XXX please add support for missing future extension formats if additional depth or stencil formats get added.
    assert(!"unknown depth_stencil_format");
  case GL_NONE:
    stencil_attach = depth_attach = false;  // both false
    break;
  case GL_STENCIL_INDEX1:
  case GL_STENCIL_INDEX4:
  case GL_STENCIL_INDEX8:
  case GL_STENCIL_INDEX16:
    stencil_attach = true;
    depth_attach = false;
    break;
  case GL_DEPTH_COMPONENT24:
  case GL_DEPTH_COMPONENT32:
  case GL_DEPTH_COMPONENT16:
  case GL_DEPTH_COMPONENT32F:
  case GL_DEPTH_COMPONENT32F_NV:  // legacy NV_depth_buffer_float enumerant
    depth_attach = true;
    stencil_attach = false;
    break;
  case GL_DEPTH24_STENCIL8:
  case GL_DEPTH32F_STENCIL8:
  case GL_DEPTH32F_STENCIL8_NV:  // legacy NV_depth_buffer_float enumerant
    stencil_attach = depth_attach = true;  // both true
    break;
  }
}

GLenum GLFramebuffer::externalDepthStencilFormat()
{
  if (depth_attach && stencil_attach) {
    return GL_DEPTH_STENCIL;
  } else {
    if (depth_attach) {
      return GL_DEPTH_COMPONENT;
    } else {
      assert(stencil_attach);
      return  GL_STENCIL_INDEX;
    }
  }
}

void GLFramebuffer::allocateRenderFBODepthStencilAttachmentLayered(int w, int h)
{
  // Layered needs layered depth/stencil buffer.
  if (capabilities.hasTextureMultisample() && coverage_samples > 1) {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_depth_stencil);
    const GLboolean  fixed_sample_locations = GL_TRUE;
    // Favor texture storage if supported so texture views are supported.
    if (capabilities.hasTextureStorageMultisample()) {
      glTexStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
        coverage_samples, depth_stencil_format, w, h, slices, fixed_sample_locations);
    } else {
      glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
        coverage_samples, depth_stencil_format, w, h, slices, fixed_sample_locations);
    }
  } else {
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex_depth_stencil);
    // Favor texture storage if supported so texture views are supported.
    if (capabilities.hasTextureStorage()) {
      const GLsizei one_level = 1;  // No mipmaps
      smart_glTexStorage3D(GL_TEXTURE_2D_ARRAY, one_level, depth_stencil_format, w, h, slices);
    } else {
      const GLint level_zero = 0;
      const GLint no_border = 0;
      const GLenum external_format = externalDepthStencilFormat();
      const GLenum irrelevant_type = GL_UNSIGNED_BYTE;
      const void* without_image_data = NULL;
      glTexImage3D(GL_TEXTURE_2D_ARRAY, level_zero, depth_stencil_format, w, h, slices, no_border, external_format, irrelevant_type, without_image_data);
    }
  }
}

void GLFramebuffer::allocateRenderFBODepthStencilAttachmentNonLayered(int w, int h)
{
  assert(layer_mode == AllSlices || layer_mode == PerSlice);
  GLenum target = GL_TEXTURE_2D;
  const bool has_texture_multisample = capabilities.hasTextureMultisample();
  if (has_texture_multisample && coverage_samples > 1) {
    target = GL_TEXTURE_2D_MULTISAMPLE;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex_depth_stencil);
    const GLboolean  fixed_sample_locations = GL_TRUE;
    // Favor texture storage if supported so texture views are supported.
    if (capabilities.hasTextureStorageMultisample()) {
      glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
        coverage_samples, depth_stencil_format, w, h, fixed_sample_locations);
    } else {
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
        coverage_samples, depth_stencil_format, w, h, fixed_sample_locations);
    }
  } else {
    if (has_texture_multisample) {
      glBindTexture(GL_TEXTURE_2D, tex_depth_stencil);
      // Favor texture storage if supported so texture views are supported.
      if (capabilities.hasTextureStorage()) {
        const GLsizei one_level = 1;  // No mipmaps
        smart_glTexStorage2D(GL_TEXTURE_2D, one_level, depth_stencil_format, w, h);
      } else {
        const GLint level_zero = 0;
        const GLint no_border = 0;
        const GLenum external_format = externalDepthStencilFormat();
        const GLenum irrelevant_type = GL_UNSIGNED_BYTE;
        const void* without_image_data = NULL;
        glTexImage2D(GL_TEXTURE_2D, level_zero, depth_stencil_format, w, h, no_border, external_format, irrelevant_type, without_image_data);
      }
    } else {
      glBindRenderbuffer(GL_RENDERBUFFER, tex_depth_stencil);
      glRenderbufferStorage(GL_RENDERBUFFER, depth_stencil_format, w, h);
    }
  }
  if (depth_attach) {
    if (has_texture_multisample) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        target, tex_depth_stencil, base_level);
    } else {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, tex_depth_stencil);
    }
  }
  if (stencil_attach) {
    if (has_texture_multisample) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
        target, tex_depth_stencil, base_level);
    } else {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, tex_depth_stencil);
    }
  }
}

void GLFramebuffer::allocateRenderFBODepthStencilAttachment(int w, int h)
{
  applyDepthStencilFormat();

  // Does this framebuffer lack a depth and stencil buffer?
  if (!stencil_attach && !depth_attach) {
    // Yes, return without allocating depth or stencil attachments.
    return;
  }
  
  if (layer_mode == Layered) {
    allocateRenderFBODepthStencilAttachmentLayered(w, h);
  } else {
    allocateRenderFBODepthStencilAttachmentNonLayered(w, h);
  }
}

void GLFramebuffer::allocateRenderFBO(int w, int h)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_render);
  allocateRenderFBOColorAttachment(w, h);
  allocateRenderFBODepthStencilAttachment(w, h);
}

void GLFramebuffer::allocate()
{
  if (!dirty) {
    return;
  }
  deallocate();
  const bool supersampled = isSupersampled();
  const int w = supersampled ? 2*width : width;
  const int h = supersampled ? 2*height : height;
  
  // Initialize FBOs: fbo_render & fbo_downsample
  glGenFramebuffers(countof(fbos), fbos);
  assert(fbo_render);
  assert(fbo_downsample);

  // Initialize textures/renderbuffers: tex_color, tex_depth_stencil, and tex_downsample
  if (capabilities.hasTextureMultisample()) {
    glGenTextures(3, textures);
  } else {
    // Use a renderbuffer for the depth-stencil buffer.
    glGenTextures(2, &tex_color);  // and tex_downsample that follows it!
    glGenRenderbuffers(1, &tex_depth_stencil);
  }
  assert(tex_color);
  assert(tex_depth_stencil);
  assert(tex_downsample);

  if (buffer_mode != Normal) {
    allocateDownsampleFBO(w, h);
  }
  allocateRenderFBO(w, h);

  validateLayerMode();
  allocateAlternateColorTexture();

  blendConfiguration();
  glViewport(0, 0, w, h);
  dirty = false;
#ifndef NDEBUG
  GLenum error = glGetError();
  if (error != GL_NONE) {
    fprintf(stderr, "GL error: 0x%x\n", error);
  }
#endif
}

bool GLFramebuffer::readyRenderFBO(GLenum target)
{
  const GLenum status = glCheckNamedFramebufferStatus(fbo_render, target);
  if (status == GL_FRAMEBUFFER_COMPLETE) {
    return true;
  } else {
    return false;
  }
}

bool GLFramebuffer::readyDownsampleFBO(GLenum target)
{
  const GLenum status = glCheckNamedFramebufferStatus(fbo_downsample, target);
  if (status == GL_FRAMEBUFFER_COMPLETE) {
    return true;
  } else {
    return false;
  }
}

bool GLFramebuffer::readyToRender()
{
  const bool fbo_render_ok = readyRenderFBO(GL_FRAMEBUFFER);
  if (fbo_render_ok) {
    if (buffer_mode == Normal) {
      // Render FBO ok without need for a downsample buffer.
      return true;
    }
    const bool fbo_downsample_ok = readyDownsampleFBO(GL_FRAMEBUFFER);
    if (fbo_downsample_ok) {
      // Both render FBO and downsample FBO ok.
      return true;
    }
  }
  return false;
}

bool GLFramebuffer::supportsAlternateColorFormat()
{
  if (capabilities.hasTextureView()) {
    if (color_samples > 1 || coverage_samples > 1) {
      if (capabilities.hasTextureStorageMultisample()) {
        return true;
      } else {
        return false;
      }
    } else {
      if (capabilities.hasTextureStorage()) {
        return true;
      } else {
        return false;
      }
    }
  } else {
    return false;
  }
}

// GL_SRGB* and GL_SLUMINANCE* textures always perform sRGB-decode for texelFetch, irrespective
// of the sampler object or texture object's GL_TEXTURE_SRGB_DECODE_EXT state.  This is becausse
// texelFetch ignores sampler state.
//
// So the only way to get a GL_SRGB* and GL_SLUMINANCE* texture to skip sRGB decode is to create
// a texture view to a non-sRGB version of the texture image and use texelFetch on this alternative
// texture object.
//
// This means if you use an sRGB texture internal format such as GL_SRGB8_ALPHA8_EXT and you want
// texelFetch access to the texture image that skips the sRGB-decode, you should call:
//
//   fb->setAlternateColorFormat(GL_RGBA8);  // if the format is GL_SRGB8_ALPHA8_EXT
//
// And then bind to fb->AlternateColorTexture() instead of fb->ColorTexture().
//
// No return value because this routine expects the proper extension support (see assertion).
// 
void GLFramebuffer::setAlternateColorFormat(GLenum internalformat)
{
  assert(supportsAlternateColorFormat());
  if (internalformat != alternate_color_format) {
    alternate_color_format = internalformat;
    if (alt_tex_color_view) {
      glDeleteTextures(1, &alt_tex_color_view);
      alt_tex_color_view = 0;
    }
  }
}

// Like setAlternateColorFormat but works when neither OpenGL 4.3 nor ARB_texture_view
// are available by having the alterantive color format simply match the
// internal format.
GLenum GLFramebuffer::requestAlternateColorFormat(GLenum internalformat)
{
  // The texture view functionality requires the immutability of the texture
  // storage functionality.
  if (supportsAlternateColorFormat()) {
    setAlternateColorFormat(internalformat);
    return internalformat;
  } else {
    assert(alternate_color_format == GL_NONE);
    return color_format;
  }
}

void GLFramebuffer::allocateAlternateColorTexture()
{
  if (alt_tex_color_view == 0 && alternate_color_format != GL_NONE) {
    const GLuint minlevel = 0;
    const GLuint numlevels = 1;
    const GLuint minlayer = 0;
    const GLuint numlayers = slices;
    glGenTextures(1, &alt_tex_color_view);
    smart_glTextureView(alt_tex_color_view, tex_color_target, tex_color, alternate_color_format,
      minlevel, numlevels, minlayer, numlayers);
  }
}

GLuint GLFramebuffer::AlternateColorTexture()
{
  allocate();
  if (alternate_color_format == GL_NONE || alternate_color_format == color_format) {
    return tex_color;
  } else {
    allocateAlternateColorTexture();
    return alt_tex_color_view;
  }
}

void GLFramebuffer::bind()
{
  if (dirty) {
    allocate();
  }
  assert(!dirty);
  assert(fbo_render);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_render);
}

void GLFramebuffer::bindAllSlices()
{
  assert(layer_mode == Layered);
  bind();
  if (bound_layered_slice != AllLayers) {
    validateLayerModeLayered();
  }
}

void GLFramebuffer::bindSliceBySliceFBO(int slice)
{
  assert(layer_mode == Layered);
  if (dirty) {
    allocate();
  }
  assert(!dirty);
  if (slices > 1) {
    assert(slice_fbo.size() <= size_t(slices));
    const GLuint fbo_for_slice = slice_fbo[slice];
    assert(fbo_for_slice != 0);  // glGenFramebuffers should have happened already.
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_for_slice);
  } else {
    // No per-slice FBOs when just a single slice; just bind to the master FBO.
    bind();
  }
}

void GLFramebuffer::bindSliceByMasterFBO(int slice)
{
  assert(slice >= 0);
  assert(layer_mode == AllSlices || layer_mode == PerSlice);
  bind();
  // Is the requested slice already
  if (slice == bound_layered_slice) {
    // Confirm color attachment zero is really configured as we expect.
    assert(GL_TEXTURE == GetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE));
    assert(tex_color == GetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME));
    assert(base_level == GetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL));
    // Early return.
    return;
  }

  if (slices > 1) {
    if (color_format != GL_NONE) {
      if (layer_mode == AllSlices) {
        if (bound_layered_slice < 0) {
          glDrawBuffers(1, drawBuffersList());
          // For every one but the slice to bind, bind the color attachment to zero.
          for (int slice_to_disable=0; slice_to_disable<slices; slice_to_disable++) {
            if (slice_to_disable != slice) {
              const GLint ignored_slice_value = 0;
              glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+slice_to_disable,
                GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0, base_level, ignored_slice_value);
            }
          }
        }
      }
      // Now bind the indicated slice to bind...
      if (color_samples > 1) {
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D_MULTISAMPLE_ARRAY, tex_color, base_level, slice);
      } else {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
          tex_color, base_level, slice);
      }
    }
  }
  bound_layered_slice = slice;
}

void GLFramebuffer::bindSlice(int slice)
{
  assert(slice >= 0);
  assert(slice < slices);

  // Two choices for binding to a slice:
  // 1) reconfigure the master FBO to change the color buffer's slice
  // 2) bind to a pre-configured FBO per slice
  //
  // #1 is faster for AllSlices mode because chaning a single color slice is
  // basically one GL command (glFramebufferTexture3D or glFramebufferTextureLayer) while
  // calling glBindFramebuffer to bind to a completely different FBO turns out to be more
  // expensive (on 2018 NVIDIA drivers).
  //
  // #2 is probably a better choice for Layered mode because we have to change
  // both the color and depth/stencil attachments.
  //
  // XXX I never really showed that #2 is faster even for Layered mode but seems likely.
  if (layer_mode == Layered) {
    bindSliceBySliceFBO(slice);
  } else {
    assert(layer_mode == AllSlices || layer_mode == PerSlice);
    bindSliceByMasterFBO(slice);
  }
}

void GLFramebuffer::viewport()
{
  const GLsizei w = Supersampled() ? 2*width : width;
  const GLsizei h = Supersampled() ? 2*height : height;
  glViewport(0, 0, w, h);
}

void GLFramebuffer::viewport(GLint x, GLint y, GLsizei view_width, GLsizei view_height)
{
  const GLsizei w = Supersampled() ? 2*view_width : view_width;
  const GLsizei h = Supersampled() ? 2*view_height : view_height;
  glViewport(x, y, w, h);
}

void GLFramebuffer::enableScissor(GLint x, GLint y, GLsizei scissor_width, GLsizei scissor_height)
{
  const GLsizei w = Supersampled() ? 2*scissor_width : scissor_width;
  const GLsizei h = Supersampled() ? 2*scissor_height : scissor_height;
  glScissor(x, y, w, h);
  glEnable(GL_SCISSOR_TEST);
}

void GLFramebuffer::disableScissor()
{
  glDisable(GL_SCISSOR_TEST);
}

#if defined(GL_EXT_window_rectangles)
void GLFramebuffer::windowRectangles(GLenum mode, GLsizei count, const GLint rects[])
{
  assert(capabilities.hasEXTWindowRectangles());
  if (Supersampled()) {
      assert(count <= 8);
      GLint ss_rects[8*4];
      const int scalars = count*4;
      for (int i=0; i<scalars; i++) {
          ss_rects[i] = 2*rects[i];
      }
      glWindowRectanglesEXT(mode, count, ss_rects);
  } else {
      glWindowRectanglesEXT(mode, count, rects);
  }
}
#endif

void GLFramebuffer::resolveSimple(int w, int h)
{
  // Simple case for one slice.
  glBlitFramebuffer(0, 0, w, h,
    0, 0, w, h,
    GL_COLOR_BUFFER_BIT, GL_NEAREST);
  invalidateSingleSliceReadBuffer();
}

void GLFramebuffer::resolveSlices(int w, int h)
{
  if (slices > 1) {
    // For each slice...
    for (int i = 0; i<slices; i++) {
      // From multisampled render FBO attachment i...
      glReadBuffer(GL_COLOR_ATTACHMENT0 + i);  // source
                                               // ...to aliased downsample FBO attachment i...
      glDrawBuffers(1, &drawBuffersList()[i]);   // destination
                                                 // ...perform window-sized downsample blit.
      glBlitFramebuffer(0, 0, w, h,
        0, 0, w, h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    invalidateAllSlicesReadBuffer();
  } else {
    resolveSimple(w, h);
  }
}

void GLFramebuffer::resolveLayered(int w, int h)
{
  if (slices > 1) {
    // Read and write through color attachment 0.
    glReadBuffer(GL_COLOR_ATTACHMENT0);  // source
    glDrawBuffers(1, &drawBuffersList()[0]);   // destination

                                               // XXX revisit, we detach the depth and stencil buffers to ensure a consistent FBO;
                                               // otherwise "multi-layered" depth and stencil attachments could be inconsistent with
                                               // "single layer" rendering.
    const GLuint null_texture_object = 0;
    const GLint layer_zero = 0;
    if (depth_attach) {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, null_texture_object, base_level, layer_zero);
      glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, null_texture_object, base_level, layer_zero);
    }
    if (stencil_attach) {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, null_texture_object, base_level, layer_zero);
      glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, null_texture_object, base_level, layer_zero);
    }

    // For each slice...
    for (int slice = 0; slice<slices; slice++) {
      glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        tex_color, base_level, slice);
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        tex_downsample, base_level, slice);
      resolveSimple(w, h);
    }
    bound_layered_slice = BogusLayer;  // set to bogus for revalidation
  } else {
    resolveSimple(w, h);
  }
}

void GLFramebuffer::resolve()
{
  assert(color_format != GL_NONE);
  if (buffer_mode == Normal) {
    return;
  }
  assert(buffer_mode == Supersample || buffer_mode == Resolve);
  const bool supersampled = isSupersampled();
  const int w = supersampled ? 2*width : width;
  const int h = supersampled ? 2*height : height;

  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_render);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_downsample);
  if (layer_mode == Layered) {
    resolveLayered(w, h);
  } else {
    assert(layer_mode == AllSlices || layer_mode == PerSlice);
    resolveSlices(w, h);
  }
}

void GLFramebuffer::copyConfigureForSRGB(GLuint dest)
{
  // If copying to the default framebuffer when it claims to be GL_LINEAR (non-sRGB encoded)
  // disable sRGB framebuffer writing for the glBlitFramebuffer.
  // XXX I'm not confident about this
  if (dest == 0 && sRGBColorEncoding() && capabilities.defaultFramebufferColorEncoding() == GL_LINEAR) {
    glDisable(GL_FRAMEBUFFER_SRGB);
  }
}

void GLFramebuffer::copyInternalXY(GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter)
{
  assert(!dirty);  // Contents undefined if dirty so shouldn't be copying!
  assert(layer_mode == AllSlices);
  assert(slices == 1);
  if (Supersampled()) {
    const int width2 = 2*width;
    const int height2 = 2*height;
    // From multisample fbo_render FBO...
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_render);
    glReadBuffer(GL_COLOR_ATTACHMENT0);  
    // ...to aliased fbo_downsample FBO...
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_downsample);
    glDrawBuffers(1, &drawBuffersList()[0]);
    // perform multisample-downsampling copy.
    glBlitFramebuffer(0, 0, width2, height2,
      0, 0, width2, height2,
      GL_COLOR_BUFFER_BIT, GL_NEAREST);  // GL_NEAREST reduces multisampling.
    invalidateSingleSliceReadBuffer();
    // From aliased fbo_downsample FBO...
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_downsample);
    // ...to dest FBO...
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
    copyConfigureForSRGB(dest);
    // copy with 2x2 supersample
    // (FYI: copies with a supersampled framebuffers always use GL_LINEAR for their filter)
    glBlitFramebuffer(0, 0, width2, height2,
      x, y, x+blit_width, y+blit_height,
      GL_COLOR_BUFFER_BIT, GL_LINEAR);  // GL_LINEAR averages 2x2 samples.
    invalidateSingleSliceReadBuffer();
  } else {
    // From (possibly multisampled) fbo_render FBO...
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_render);
    // ...to aliased fbo_downsample FBO...
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
    copyConfigureForSRGB(dest);
    // perform (possibly multisample-downsampling) copy.
    glBlitFramebuffer(0, 0, width, height,
      x, y, x+blit_width, y+blit_height,
      GL_COLOR_BUFFER_BIT, filter);
    invalidateSingleSliceReadBuffer();
  }
}

void GLFramebuffer::copyLinearXY(GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest)
{
  copyInternalXY(x, y, w, h, dest, GL_LINEAR);
}

void GLFramebuffer::copyXY(GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest)
{
  copyInternalXY(x, y, w, h, dest, GL_NEAREST);
}

void GLFramebuffer::copyXY(GLint x, GLint y, GLuint dest)
{
  copyInternalXY(x, y, width, height, dest, GL_NEAREST);
}

void GLFramebuffer::copy(GLuint dest)
{
  copyInternalXY(0, 0, width, height, dest, GL_NEAREST);
}

void GLFramebuffer::copySliceInternalXY_AllSlices(int slice, GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter)
{
  assert(layer_mode == AllSlices);
  if (Supersampled()) {
    const int width2 = 2 * width;
    const int height2 = 2 * height;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_render);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_downsample);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + slice);  // source
    glDrawBuffers(1, &drawBuffersList()[slice]);   // destination
    glBlitFramebuffer(0, 0, width2, height2,
      0, 0, width2, height2,
      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    invalidateReadBufferSlice(slice);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_downsample);
    if (slices > 1) {
      glReadBuffer(GL_COLOR_ATTACHMENT0 + slice);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
    glBlitFramebuffer(0, 0, width2, height2,
      x, y, x + blit_width, y + blit_height,
      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    invalidateReadBufferSlice(slice);
  } else {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
    if (slices > 1) {
      glReadBuffer(GL_COLOR_ATTACHMENT0 + slice);
    }
    glBlitFramebuffer(0, 0, width, height,
      x, y, x + blit_width, y + blit_height,
      GL_COLOR_BUFFER_BIT, filter);
    invalidateReadBufferSlice(slice);
  }
}

void GLFramebuffer::copySliceInternalXY_Generic(int slice, GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter)
{
  assert(layer_mode == PerSlice || layer_mode == Layered);
  bindSlice(slice);
  if (Supersampled()) {
    const int width2 = 2 * width;
    const int height2 = 2 * height;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_render);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_downsample);
    glReadBuffer(GL_COLOR_ATTACHMENT0);  // source
    glDrawBuffers(1, &drawBuffersList()[0]);   // destination
    glBlitFramebuffer(0, 0, width2, height2,
      0, 0, width2, height2,
      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    invalidateSingleSliceReadBuffer();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_downsample);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, width2, height2,
      x, y, x + blit_width, y + blit_height,
      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    invalidateSingleSliceReadBuffer();
  } else {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, width, height,
      x, y, x + blit_width, y + blit_height,
      GL_COLOR_BUFFER_BIT, filter);
    invalidateSingleSliceReadBuffer();
  }
}

// Similar to copyInternalXY but used invalidateReadBufferSlice
// (instead of invalidateSingleSliceReadBuffer)
void GLFramebuffer::copySliceInternalXY(int slice, GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter)
{
  assert(!dirty);  // Contents undefined if dirty so shouldn't be copying!
  if (layer_mode == AllSlices) {
    copySliceInternalXY_AllSlices(slice, x, y, blit_width, blit_height, dest, filter);
  } else {
    copySliceInternalXY_Generic(slice, x, y, blit_width, blit_height, dest, filter);
  }
}

void GLFramebuffer::copySliceLinearXY(int slice, GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest)
{
  copySliceInternalXY(slice, x, y, w, h, dest, GL_LINEAR);
}

void GLFramebuffer::copySliceXY(int slice, GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest)
{
  copySliceInternalXY(slice, x, y, w, h, dest, GL_NEAREST);
}

void GLFramebuffer::copySliceXY(int slice, GLint x, GLint y, GLuint dest)
{
  copySliceInternalXY(slice, x, y, width, height, dest, GL_NEAREST);
}

void GLFramebuffer::copySlice(int slice, GLuint dest)
{
  copySliceInternalXY(slice, 0, 0, width, height, dest, GL_NEAREST);
}

bool GLFramebuffer::isSupersampled() const
{
  return buffer_mode == Supersample;
}

GLFramebuffer::BufferMode GLFramebuffer::Mode() const
{
  return buffer_mode;
}

GLFramebuffer::~GLFramebuffer()
{
  deallocate();
}

bool GLFramebuffer::sRGBColorEncoding() const
{
  switch (color_format) {
  case GL_SLUMINANCE8:
  case GL_LUMINANCE8_ALPHA8:   // XXX not typically renderable
  case GL_SRGB8_ALPHA8:
  case GL_SRGB8:
    return true;
  default:
    return false;
  }
}

unsigned int GLFramebuffer::bitsPerColorSample() const
{
  switch (color_format) {
  case GL_LUMINANCE8:
  case GL_SLUMINANCE8:
  case GL_R8:
    return 8;
  case GL_LUMINANCE8_ALPHA8:   // XXX not typically renderable
  case GL_SLUMINANCE8_ALPHA8:  // XXX not typically renderable
  case GL_RG8:
  case GL_R16F:
  case GL_LUMINANCE16:
    return 16;
  case GL_RGBA8:
  case GL_SRGB8_ALPHA8:
  case GL_RGB8:
  case GL_SRGB8:
  case GL_RGB10_A2:
  case GL_RGB10:
  case GL_RG16F:
  case GL_R32F:
    return 32;
  case GL_RGBA16F:
  case GL_RGB16F:
    return 64;
  case GL_RGBA32F:
  case GL_RGB32F:
    return 64;
  }
  assert(!"unknown color format");
  return 0;
}

unsigned int GLFramebuffer::bitsPerCoverageSample()
{
  allocate();
  switch (depth_stencil_format) {
  case GL_NONE:
    return 0;
  case GL_STENCIL_INDEX8:
    if (capabilities.hasFramebufferMixedSamples()) {
      // Assume that NV_mixed_framebuffer_samples implies an 8-bit only stencil buffer
      return 8;
    }
    return 32;
  case GL_DEPTH_COMPONENT24:
  case GL_DEPTH24_STENCIL8:
  case GL_DEPTH_COMPONENT32F:
  case GL_DEPTH_COMPONENT32F_NV:  // NV_depth_buffer_float
    return 32;
  case GL_DEPTH32F_STENCIL8:
  case GL_DEPTH32F_STENCIL8_NV:  // NV_depth_buffer_float
    return 64;
  case GL_DEPTH_COMPONENT16:
    return 16;
  }
  assert(!"unknown depth/stencil format");
  return 0;
}

unsigned int GLFramebuffer::framebufferSize()
{
  unsigned int color_bits = bitsPerColorSample();
  unsigned int coverage_bits = bitsPerCoverageSample();
  unsigned int color_pixel_bits = slices * color_samples * color_bits;
  unsigned int stencil_pixel_bits = coverage_samples * coverage_bits;
  unsigned int pixel_bits = color_pixel_bits + stencil_pixel_bits;
  unsigned int pixels = width*height;
  if (Supersampled()) {
    pixels *= 4;  // 2x2 supersampling
  }
  unsigned int framebuffer_bytes = pixels * (pixel_bits / 8);
  return framebuffer_bytes;
}

unsigned int GLFramebuffer::totalSize()
{
  unsigned int framebuffer_bytes = framebufferSize();
  unsigned int color_bits = slices * bitsPerColorSample();
  unsigned int pixels = width*height;
  unsigned int downsample_buffer_bytes = 0;
  if (Supersampled()) {
    pixels *= 4;  // 2x2 supersampling
    downsample_buffer_bytes = pixels * (color_bits / 8);
  }
  return framebuffer_bytes + downsample_buffer_bytes;
}

GLint GLFramebuffer::GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname)
{
  GLint value = GL_INVALID_VALUE;
  glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
    attachment, pname, &value);
  return value;
}

GLint GLFramebuffer::GetNamedFramebufferAttachmentParameteriv(GLuint fbo, int color_buffer_ndx, GLenum pname)
{
  GLint value = GL_INVALID_VALUE;
  const GLenum attachment = drawBuffersList()[color_buffer_ndx];
#ifdef GL_VERSION_4_5
  if (capabilities.hasARBDirectStateAccess()) {
    glGetNamedFramebufferAttachmentParameteriv(fbo,
      attachment, pname, &value);
  }
  else
#endif
#ifdef GL_EXT_direct_state_access
  if (capabilities.hasEXTDirectStateAccess()) {
    glGetNamedFramebufferAttachmentParameterivEXT(fbo,
      attachment, pname, &value);
  }
  else
#endif
  {
    // Lacks ARB_direct_state_access or EXT_direct_state_access so might need to
    // save/restore the current FBO.
    GLint current_framebuffer_object;  // Technically GLuint
    glGetIntegerv(GL_DRAW_FRAMEBUFFER, &current_framebuffer_object);
    // Is the queried buffer already bound?
    if (GLuint(current_framebuffer_object) == fbo) {
      // Yes, just do query.
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        attachment, pname, &value);
    } else {
      // No, so do save/query/restore.
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        attachment, pname, &value);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(current_framebuffer_object));
    } 
  }
  return value;
}

GLenum GLFramebuffer::getRenderFramebufferColorEncoding(int color_buffer_ndx)
{
  return GLenum(GetNamedFramebufferAttachmentParameteriv(RenderFBO(), color_buffer_ndx, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING));
}

GLenum GLFramebuffer::getDownsampleFramebufferColorEncoding(int color_buffer_ndx)
{
  return GLenum(GetNamedFramebufferAttachmentParameteriv(DownsampleFBO(), color_buffer_ndx, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING));
}
