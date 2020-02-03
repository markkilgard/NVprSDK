
// gl_framebuffer.hpp - simple/powerful OpenGL framebuffer class

// Copyright (c) NVIDIA Corporation. All rights reserved.

#ifndef __GL_FRAMEBUFFER_H__
#define __GL_FRAMEBUFFER_H__

#include <assert.h>
#include <GL/glew.h>

#include <vector>  // for std::vector

// Small class to keep track of underlying OpenGL context capabilities
class GLFramebufferCapabilities {
private:
  enum CapStatus {  // Avoid "Status" because Xlib #define's that word
    No = 0,
    Yes = 1,
    Undetermined = 2
  };
  // Helper class to make sure CapStatus values are initialized to Undetermined.
  class CapStatusInit {
    CapStatus value;
  public:
    CapStatusInit()
      : value(Undetermined)
    {}
    operator CapStatus() {
      return value;
    }
    CapStatusInit& operator = (const CapStatus& rhs) {
      value = rhs;
      return *this;
    }
  };
  const GLubyte* gl_extensions;
  int gl_major_version;
  int gl_minor_version;
  CapStatusInit has_framebuffer_object;
  CapStatusInit has_layered_framebuffer;          // OpenGL 3.2 (4.4.7 Layered Framebuffers)
  CapStatusInit has_framebuffer_mixed_samples;
  CapStatusInit has_explicit_multisample;
  CapStatusInit has_sample_locations;
  CapStatusInit has_framebuffer_sRGB;             // OpenGL 3.0 or ARB_framebuffer_sRGB
  CapStatusInit has_texture_multisample;          // OpenGL 3.2 or ARB_texture_multisample
  CapStatusInit has_texture_storage;              // OpenGL 4.2 or ARB_texture_storage
  CapStatusInit has_texture_storage_multisample;  // OpenGL 4.3 or ARB_texture_storage_multisample
  CapStatusInit has_texture_view;                 // OpenGL 4.3 or ARB_texture_view
  CapStatusInit has_ARB_direct_state_access;
  CapStatusInit has_ARB_texture_storage;
  CapStatusInit has_ARB_texture_storage_multisample;
  CapStatusInit has_ARB_texture_view;
  CapStatusInit has_EXT_direct_state_access;
  CapStatusInit has_ARB_framebuffer_no_attachments;
  CapStatusInit has_EXT_texture_storage;
  CapStatusInit has_EXT_texture_view;
  CapStatusInit has_EXT_window_rectangles;
  CapStatusInit has_ARB_geometry_shader4;
  CapStatusInit has_EXT_geometry_shader4;
  CapStatusInit has_OES_texture_view;
  GLint GL_MAX_COLOR_ATTACHMENTS_value;
  GLenum default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value;

  void decide_gl_version();
  void decide_framebuffer_object();
  void decide_layered_framebuffer();
  void decide_framebuffer_mixed_samples();
  void decide_explicit_multisample();
  void decide_sample_locations();
  void decide_framebuffer_sRGB();
  void decide_texture_multisample();
  void decide_texture_storage();
  void decide_texture_storage_multisample();
  void decide_texture_view();
  void decide_ARB_direct_state_access();
  void decide_ARB_texture_storage();
  void decide_ARB_texture_storage_multisample();
  void decide_ARB_texture_view();
  void decide_EXT_direct_state_access();
  void decide_EXT_texture_storage();
  void decide_EXT_texture_view();
  void decide_ARB_framebuffer_no_attachments();
  void decide_EXT_window_rectangles();
  void decide_ARB_geometry_shader4();
  void decide_EXT_geometry_shader4();
  void decide_OES_texture_view();  
  void decide_GL_MAX_COLOR_ATTACHMENTS_value();
  void decide_default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value();

public:
  GLFramebufferCapabilities();

  bool extensionSupported(const char * extension);

  inline bool hasOpenGL_3_0() {
    if (gl_major_version < 1) {
      decide_gl_version();
    }
    // Request 3.0 or better...
    int major = 3;
    int minor = 0;
    return gl_major_version > major || ((gl_major_version == major) && (gl_minor_version >= minor));
  }

  inline bool hasOpenGL_3_2() {
    if (gl_major_version < 1) {
      decide_gl_version();
    }
    // Request 3.2 or better...
    int major = 3;
    int minor = 2;
    return gl_major_version > major || ((gl_major_version == major) && (gl_minor_version >= minor));
  }

  inline bool hasOpenGL_4_2() {
    if (gl_major_version < 1) {
      decide_gl_version();
    }
    // Request 4.2 or better...
    int major = 4;
    int minor = 2;
    return gl_major_version > major || ((gl_major_version == major) && (gl_minor_version >= minor));
  }

  inline bool hasOpenGL_4_3() {
    if (gl_major_version < 1) {
      decide_gl_version();
    }
    // Request 4.3 or better...
    int major = 4;
    int minor = 3;
    return gl_major_version > major || ((gl_major_version == major) && (gl_minor_version >= minor));
  }

  inline bool hasOpenGL_4_5() {
    if (gl_major_version < 1) {
      decide_gl_version();
    }
    // Request 4.5 or better...
    int major = 4;
    int minor = 5;
    return gl_major_version > major || ((gl_major_version == major) && (gl_minor_version >= minor));
  }

  inline bool hasFramebufferObject() {
    if (has_framebuffer_object == Undetermined) {
      decide_framebuffer_object();
    }
    return !!has_framebuffer_object;
  }
  inline bool hasLayeredFramebuffer() {
    if (has_layered_framebuffer == Undetermined) {
      decide_layered_framebuffer();
    }
    return !!has_layered_framebuffer;
  }
  inline bool hasFramebufferMixedSamples() {
    if (has_framebuffer_mixed_samples == Undetermined) {
      decide_framebuffer_mixed_samples();
    }
    return !!has_framebuffer_mixed_samples;
  }
  inline bool hasExplicitMultisample() {
    if (has_explicit_multisample == Undetermined) {
      decide_explicit_multisample();
    }
    return !!has_explicit_multisample;
  }
  inline bool hasSampleLocations() {
    if (has_sample_locations == Undetermined) {
      decide_sample_locations();
    }
    return !!has_sample_locations;
  }
  inline bool hasFramebufferSRGB() {
    if (has_framebuffer_sRGB == Undetermined) {
      decide_framebuffer_sRGB();
    }
    return !!has_framebuffer_sRGB;
  }
  inline bool hasTextureMultisample() {
    if (has_texture_multisample == Undetermined) {
      decide_texture_multisample();
    }
    return !!has_texture_multisample;
  }
  inline bool hasTextureStorage() {
    if (has_texture_storage == Undetermined) {
      decide_texture_storage();
    }
    return !!has_texture_storage;
  }
  inline bool hasTextureStorageMultisample() {
    if (has_texture_storage_multisample == Undetermined) {
      decide_texture_storage_multisample();
    }
    return !!has_texture_storage_multisample;
  } 
  inline bool hasTextureView() {
    if (has_texture_view == Undetermined) {
      decide_texture_view();
    }
    return !!has_texture_view;
  }
  inline bool hasARBDirectStateAccess() {
    if (has_ARB_direct_state_access == Undetermined) {
      decide_ARB_direct_state_access();
    }
    return !!has_ARB_direct_state_access;
  }
  inline bool hasARBTextureStorage() {
    if (has_ARB_texture_storage == Undetermined) {
      decide_ARB_texture_storage();
    }
    return !!has_ARB_texture_storage;
  }
  inline bool hasARBTextureStorageMultisample() {
    if (has_ARB_texture_storage_multisample == Undetermined) {
      decide_ARB_texture_storage_multisample();
    }
    return !!has_ARB_texture_storage;
  }
  inline bool hasARBTextureView() {
    if (has_ARB_texture_view == Undetermined) {
      decide_ARB_texture_view();
    }
    return !!has_ARB_texture_view;
  }
  inline bool hasEXTDirectStateAccess() {
    if (has_EXT_direct_state_access == Undetermined) {
      decide_EXT_direct_state_access();
    }
    return !!has_EXT_direct_state_access;
  }
  inline bool hasARBFramebufferNoAttachments() {
    if (has_ARB_framebuffer_no_attachments == Undetermined) {
      decide_ARB_framebuffer_no_attachments();
    }
    return !!has_ARB_framebuffer_no_attachments;
  }
  inline bool hasEXTTextureStorage() {
    if (has_EXT_texture_storage == Undetermined) {
      decide_EXT_texture_storage();
    }
    return !!has_EXT_texture_storage;
  }
  inline bool hasEXTTextureView() {
    if (has_EXT_texture_view == Undetermined) {
      decide_EXT_texture_view();
    }
    return !!has_EXT_texture_view;
  }
  inline bool hasEXTWindowRectangles() {
    if (has_EXT_window_rectangles == Undetermined) {
      decide_EXT_window_rectangles();
    }
    return !!has_EXT_window_rectangles;
  }
  inline bool hasARBGeometryShader4() {
    if (has_ARB_geometry_shader4 == Undetermined) {
      decide_ARB_geometry_shader4();
    }
    return !!has_ARB_geometry_shader4;
  }
  inline bool hasEXTGeometryShader4() {
    if (has_EXT_geometry_shader4 == Undetermined) {
      decide_EXT_geometry_shader4();
    }
    return !!has_EXT_geometry_shader4;
  }
  inline bool hasOESTextureView() {
    if (has_OES_texture_view == Undetermined) {
      decide_OES_texture_view();
    }
    return !!has_OES_texture_view;
  }
  inline GLint maxColorAttachments() {
    if (GL_MAX_COLOR_ATTACHMENTS_value == -1) {
      decide_GL_MAX_COLOR_ATTACHMENTS_value();
    }
    return GL_MAX_COLOR_ATTACHMENTS_value;
  }
  inline GLint defaultFramebufferColorEncoding() {
    if (default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value == GL_INVALID_VALUE) {
      decide_default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value();
    }
    return default_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_value;
  }
};

class GLFramebuffer {
  static const GLint base_level = 0;

public:
  enum BufferMode {
    Normal,       // No resolve buffer allocated
    Supersample,  // Resolve buffer for 2x2 supersampled rendering
    Resolve       // Resolve buffer for normal-size rendering
  };
  enum AlphaHandling {
    PremultipliedAlpha,  // shader output RGBA is (A*R, A*G, A*B, A)
                         // so ONE,ONE_MINUS_SRC_ALPHA blending 
                         //    with RGBA coverage modulation
    StraightAlpha,       // shader output RGBA is (R, G, B, A)
                         // so ONE,ONE_MINUS_SRC_ALPHA/ONE,ONE seperate blending
                         //    with ALPHA coverage modulation
  };
  enum LayerMode {
    AllSlices,    // Render to all slices at once (each slice bound to a color target)
                  // using multiple draw buffers (a.k.a. Multiple Render Targets or MRT)
                  // with the option to render to single slices.
    Layered,      // Layered rendering
    PerSlice,     // Render to one slice at a time.
  };
private:
  // Specified state
  GLFramebufferCapabilities &capabilities;

  GLenum color_format;
  GLenum depth_stencil_format;
  GLenum alternate_color_format;  // Alternative internalformat, see setAlternateColorFormat.

  int width;
  int height;
  int slices;
  int coverage_samples;
  int color_samples;
  BufferMode buffer_mode;
  AlphaHandling alpha_handling;
  bool coverage_modulation;
  LayerMode layer_mode;
  int bound_layered_slice;  // -1 (AllLayers) means "true" layered rendering, >=0 indicates slice to render, -2 (BogusLayer) 

  // Special constant values for bound_layered_slice.
  static const int AllLayers = -1;
  static const int BogusLayer = -2;

  // Generated state
  GLuint fbos[2];  // [fbo_render, fbo_downsample]
  GLuint &fbo_render;
  GLuint &fbo_downsample;
  GLuint textures[4];  // [tex_color, tex_downsample, tex_depth_stencil, alt_tex_color_view]
  GLuint &tex_color;
  GLuint &tex_downsample;
  GLuint &tex_depth_stencil;
  GLuint &alt_tex_color_view;  // Alterative texture object with a different internalformat via texture view.

  GLenum tex_color_target;
  GLenum tex_downsample_target;

  std::vector<GLenum> slice_fbo;

  bool direct_state_access;

  bool depth_attach;
  bool stencil_attach;

  bool dirty;
  bool invalidate_after_blit;

protected:
  void markDirty();

  void validateLayerModeAllSlices();
  void validateLayerModePerSlice();
  void validateLayerModeLayered();
  void validateLayerMode();
  void copyConfigureForSRGB(GLuint dest);
  static const GLenum *drawBuffersList();

  inline void invalidateSingleSliceReadBuffer() {
    if (invalidate_after_blit) {
      const GLenum single_attachment = GL_COLOR_ATTACHMENT0;
      glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, &single_attachment);
    }
  }
  inline void invalidateAllSlicesReadBuffer() {
    if (invalidate_after_blit) {
      glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, slices, drawBuffersList());
    }
  }
  inline void invalidateReadBufferSlice(int slice) {
    if (invalidate_after_blit) {
      glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, &drawBuffersList()[slice]);
    }
  }
  void copyInternalXY(GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter);
  void copySliceInternalXY_AllSlices(int slice, GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter);
  void copySliceInternalXY_Generic(int slice, GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter);
  void copySliceInternalXY(int slice, GLint x, GLint y, GLsizei blit_width, GLsizei blit_height, GLuint dest, GLenum filter);
  GLint GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname);
  GLint GetNamedFramebufferAttachmentParameteriv(GLuint fbo, int color_buffer_ndx, GLenum pname);

  // Helpers to bindSlice.
  void bindSliceBySliceFBO(int slice);
  void bindSliceByMasterFBO(int slice);

  // Helpers to allocate.
  void allocateSliceFBO(GLenum target, GLint slice);
  void allocateSliceFBOs();
  void allocateRenderFBOColorAttachment(int w, int h);
  GLenum externalDepthStencilFormat();
  void applyDepthStencilFormat();
  void allocateRenderFBODepthStencilAttachmentLayered(int w, int h);
  void allocateRenderFBODepthStencilAttachmentNonLayered(int w, int h);
  void allocateRenderFBODepthStencilAttachment(int w, int h);
  void setDownsampleTextureParameters();
  void allocateDownsampleFBO(int w, int h);
  void allocateRenderFBOColorAttachmentOneSlice(int w, int h);
  void allocateRenderFBOColorAttachmentMultiSlice(int w, int h);
  void allocateRenderFBO(int w, int h);
  void allocateAlternateColorTexture();

  bool readyRenderFBO(GLenum target);
  bool readyDownsampleFBO(GLenum target);
  
public:
  GLFramebuffer(GLFramebufferCapabilities &capabilities,
                GLenum cf, GLenum dsf,
                int w, int h, int s,
                int cov_samples, int col_samples, BufferMode mode);
  void deallocateSliceFBOs();
  void deallocate();
  void resize(int w, int h);
  void setQuality(int cov_samples, int col_samples, BufferMode mode);
  void setDepthStencilFormat(GLenum dsf);
  void setSupersampled(bool ss, BufferMode mode = Normal);
  void setSlices(int s);
  void setBufferMode(BufferMode m);
  void setAlphaHandling(AlphaHandling ah);
  void setLayerMode(LayerMode lm);
  bool supportsAlternateColorFormat();
  void setAlternateColorFormat(GLenum internalformat);  // Fails if neither OpenGL 4.3 nor ARB_texture_view are supported.
  GLenum requestAlternateColorFormat(GLenum internalformat);  // Returns effective alternative color format.
  void coverageModulationConfiguration();
  void enableCoverageModulation();
  void disableCoverageModulation();
  void blendConfiguration();
  GLenum requestColorEncodingConfiguration(GLenum encoding);  // returns GL_SRGB or GL_LINEAR
  void colorEncodingConfiguration(GLenum encoding);
  void allocate();
  bool readyToRender();
  void bind();
  void bindSlice(int slice);
  void bindAllSlices();
  void viewport();
  void viewport(GLint x, GLint y, GLsizei width, GLsizei height);
  void enableScissor(GLint x, GLint y, GLsizei scissor_width, GLsizei scissor_height);
  void disableScissor();
#if defined(GL_EXT_window_rectangles)
  void windowRectangles(GLenum mode, GLsizei count, const GLint rects[]);
#endif
  void resolveSimple(int w, int h);
  void resolveSlices(int w, int h);
  void resolveLayered(int w, int h);
  void resolve();

  void copy(GLuint dest = 0);
  void copyXY(GLint x, GLint y, GLuint dest = 0);
  void copyXY(GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest = 0);
  void copyLinearXY(GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest = 0);
  // No sense having void GLFramebuffer::copyLinearXY(GLint x, GLint y, GLuint dest)
  // since width and height of source and destination are identical so no resampling occurs.

  // Copy by the slice.
  void copySlice(int slice, GLuint dest = 0);
  void copySliceXY(int slice, GLint x, GLint y, GLuint dest = 0);
  void copySliceXY(int slice, GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest = 0);
  void copySliceLinearXY(int slice, GLint x, GLint y, GLsizei w, GLsizei h, GLuint dest = 0);
  // No sense having void GLFramebuffer::copySliceLinearXY(int slice, GLint x, GLint y, GLuint dest)
  // since width and height of source and destination are identical so no resampling occurs.

  bool isSupersampled() const;
  BufferMode Mode() const;
  bool sRGBColorEncoding() const;
  unsigned int bitsPerColorSample() const;
  unsigned int bitsPerCoverageSample();
  unsigned int framebufferSize();
  unsigned int totalSize();  // includes size of downsample buffer
  ~GLFramebuffer();

  // Inline state queries
  inline GLuint RenderFBO() const {
      assert(fbo_render);
      return fbo_render;
  }
  inline GLuint DownsampleFBO() const {
      assert(fbo_downsample);
      return fbo_downsample;
  }
  inline GLuint RenderFBO() {
      if (dirty) {
        allocate();
      }
      assert(fbo_render);
      return fbo_render;
  }
  inline GLuint DownsampleFBO() {
      if (dirty) {
        allocate();
      }
      assert(fbo_downsample);
      return fbo_downsample;
  }
  inline GLenum ColorFormat() const {
    return color_format;
  }
  inline GLenum AlternateColorFormat() const {
    return (alternate_color_format != GL_NONE) ? alternate_color_format : color_format;
  }
  inline GLenum DepthStencilFormat() const {
    return depth_stencil_format;
  }
  inline int Width() const {
    return width;
  }
  inline int Height() const {
    return height;
  }
  inline int RenderWidth() const {
    return isSupersampled() ? 2*width : width;
  }
  inline int RenderHeight() const {
    return isSupersampled() ? 2*height : height;
  }
  inline int RenderScale() const {
    return isSupersampled() ? 2 : 1;
  }
  inline int Slices() const {
    return slices;
  }
  inline int ColorSamples() const {
    return color_samples;
  }
  inline int CoverageSamples() const {
    return coverage_samples;
  }
  inline bool Supersampled() const {
    return buffer_mode == Supersample;
  }
  inline bool isMixedSamples() const {
      return color_samples != coverage_samples;
  }

  inline void useDirectStateAccess(bool use) {
    direct_state_access = use;
  }

  // When binding a texture to the color texture.
  inline GLenum ColorTarget() {
    allocate();
    return tex_color_target;
  }
  inline GLuint ColorTexture() {
    allocate();
    return tex_color;
  }
  GLuint AlternateColorTexture();

  // When binding a texture to the downsampled texture (the color texture when not supersampled).
  inline GLenum DownsampleTarget() {
    allocate();
    return buffer_mode==Normal ? tex_color_target : tex_downsample_target;
  }
  inline GLuint DownsampleTexture() {
    allocate();
    return buffer_mode==Normal ? tex_color : tex_downsample;
  }

  // Various OpenGL commands (glPointSize, glLineWidth, glLineStipple) expect pixel-space
  // units.  Provide methods to account for pixel-size scaling due to supersampling.
  // See also: viewport, scissor, enableScissor, windowRectangles, and copy* methods.
  inline void pointSize(GLfloat size) const {
      if (isSupersampled()) {
          size *= 2;
      }
      glPointSize(size);
  }
  inline void lineWidth(GLfloat width) const {
      if (isSupersampled()) {
          width *= 2;
      }
      glLineWidth(width);
  }
  inline void lineStipple(GLint factor, GLushort pattern) const {
      if (isSupersampled()) {
          factor *= 2;
      }
      glLineStipple(factor, pattern);
  }

  GLenum getRenderFramebufferColorEncoding(int color_buffer_ndx);
  GLenum getDownsampleFramebufferColorEncoding(int color_buffer_ndx);

  inline void setInvalidateAfterBlit(bool state) {
    invalidate_after_blit = state;
  }

  // OpenGL has accumulated multiple essentially duplicate entrypoint names as
  // OpenGL has evolved by extensions and core standard updates.  So that GLFramebuffer
  // can work with various OpenGL driver versions, provide "smart" functions that
  // fallback to an older entry points when newer standard functions are not available.
  //
  // smart_-prefixed methods call supported core or extension function appropriately.
  void smart_glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
  void smart_glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
  void smart_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
  void smart_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
};

#endif // __GL_FRAMEBUFFER_H__
