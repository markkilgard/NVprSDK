/*
 * Copyright © 2007,2008,2009  Red Hat, Inc.
 * Copyright © 2012  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_LAYOUT_PRIVATE_HH
#define HB_OT_LAYOUT_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-layout.h"

#include "hb-font-private.hh"
#include "hb-buffer-private.hh"
#include "hb-set-private.hh"


/* buffer var allocations, used during the GSUB/GPOS processing */
#define glyph_props()		var1.u16[0] /* GDEF glyph properties */
#define syllable()		var1.u8[2] /* GSUB/GPOS shaping boundaries */
#define lig_props()		var1.u8[3] /* GSUB/GPOS ligature tracking */

/* buffer var allocations, used during the entire shaping process */
#define unicode_props0()	var2.u8[0]
#define unicode_props1()	var2.u8[1]


inline void
_hb_glyph_info_set_unicode_props (hb_glyph_info_t *info, hb_unicode_funcs_t *unicode)
{
  info->unicode_props0() = ((unsigned int) unicode->general_category (info->codepoint)) |
			   (unicode->is_default_ignorable (info->codepoint) ? 0x80 : 0) |
			   (info->codepoint == 0x200C ? 0x40 : 0) |
			   (info->codepoint == 0x200D ? 0x20 : 0);
  info->unicode_props1() = unicode->modified_combining_class (info->codepoint);
}

inline void
_hb_glyph_info_set_general_category (hb_glyph_info_t *info, hb_unicode_general_category_t gen_cat)
{
  info->unicode_props0() = (unsigned int) gen_cat | ((info->unicode_props0()) & ~0x1F);
}

inline hb_unicode_general_category_t
_hb_glyph_info_get_general_category (const hb_glyph_info_t *info)
{
  return (hb_unicode_general_category_t) (info->unicode_props0() & 0x1F);
}

inline void
_hb_glyph_info_set_modified_combining_class (hb_glyph_info_t *info, unsigned int modified_class)
{
  info->unicode_props1() = modified_class;
}

inline unsigned int
_hb_glyph_info_get_modified_combining_class (const hb_glyph_info_t *info)
{
  return info->unicode_props1();
}

inline hb_bool_t
_hb_glyph_info_is_default_ignorable (const hb_glyph_info_t *info)
{
  return !!(info->unicode_props0() & 0x80);
}

inline hb_bool_t
_hb_glyph_info_is_zwnj (const hb_glyph_info_t *info)
{
  return !!(info->unicode_props0() & 0x40);
}

inline hb_bool_t
_hb_glyph_info_is_zwj (const hb_glyph_info_t *info)
{
  return !!(info->unicode_props0() & 0x20);
}


#define hb_ot_layout_from_face(face) ((hb_ot_layout_t *) face->shaper_data.ot)

/*
 * GDEF
 */

typedef enum {
  HB_OT_LAYOUT_GLYPH_PROPS_UNCLASSIFIED	= 1 << HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED,
  HB_OT_LAYOUT_GLYPH_PROPS_BASE_GLYPH	= 1 << HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH,
  HB_OT_LAYOUT_GLYPH_PROPS_LIGATURE	= 1 << HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE,
  HB_OT_LAYOUT_GLYPH_PROPS_MARK		= 1 << HB_OT_LAYOUT_GLYPH_CLASS_MARK,
  HB_OT_LAYOUT_GLYPH_PROPS_COMPONENT	= 1 << HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT
} hb_ot_layout_glyph_class_mask_t;



/*
 * GSUB/GPOS
 */

/* lig_id / lig_comp
 *
 * When a ligature is formed:
 *
 *   - The ligature glyph and any marks in between all the same newly allocated
 *     lig_id,
 *   - The ligature glyph will get lig_num_comps set to the number of components
 *   - The marks get lig_comp > 0, reflecting which component of the ligature
 *     they were applied to.
 *   - This is used in GPOS to attach marks to the right component of a ligature
 *     in MarkLigPos.
 *
 * When a multiple-substitution is done:
 *
 *   - All resulting glyphs will have lig_id = 0,
 *   - The resulting glyphs will have lig_comp = 0, 1, 2, ... respectively.
 *   - This is used in GPOS to attach marks to the first component of a
 *     multiple substitution in MarkBasePos.
 *
 * The numbers are also used in GPOS to do mark-to-mark positioning only
 * to marks that belong to the same component of a ligature in MarkMarPos.
 */
#define IS_LIG_BASE 0x10
static inline void
set_lig_props_for_ligature (hb_glyph_info_t &info, unsigned int lig_id, unsigned int lig_num_comps)
{
  info.lig_props() = (lig_id << 5) | IS_LIG_BASE | (lig_num_comps & 0x0F);
}
static inline void
set_lig_props_for_mark (hb_glyph_info_t &info, unsigned int lig_id, unsigned int lig_comp)
{
  info.lig_props() = (lig_id << 5) | (lig_comp & 0x0F);
}
static inline void
set_lig_props_for_component (hb_glyph_info_t &info, unsigned int comp)
{
  set_lig_props_for_mark (info, 0, comp);
}

static inline unsigned int
get_lig_id (const hb_glyph_info_t &info)
{
  return info.lig_props() >> 5;
}
static inline bool
is_a_ligature (const hb_glyph_info_t &info)
{
  return !!(info.lig_props() & IS_LIG_BASE);
}
static inline unsigned int
get_lig_comp (const hb_glyph_info_t &info)
{
  if (is_a_ligature (info))
    return 0;
  else
    return info.lig_props() & 0x0F;
}
static inline unsigned int
get_lig_num_comps (const hb_glyph_info_t &info)
{
  if ((info.glyph_props() & HB_OT_LAYOUT_GLYPH_PROPS_LIGATURE) && is_a_ligature (info))
    return info.lig_props() & 0x0F;
  else
    return 1;
}

static inline uint8_t allocate_lig_id (hb_buffer_t *buffer) {
  uint8_t lig_id = buffer->next_serial () & 0x07;
  if (unlikely (!lig_id))
    lig_id = allocate_lig_id (buffer); /* in case of overflow */
  return lig_id;
}


HB_INTERNAL hb_bool_t
hb_ot_layout_lookup_would_substitute_fast (hb_face_t            *face,
					   unsigned int          lookup_index,
					   const hb_codepoint_t *glyphs,
					   unsigned int          glyphs_length,
					   hb_bool_t             zero_context);


/* Should be called before all the substitute_lookup's are done. */
HB_INTERNAL void
hb_ot_layout_substitute_start (hb_font_t    *font,
			       hb_buffer_t  *buffer);


struct hb_ot_layout_lookup_accelerator_t;

namespace OT {
  struct hb_apply_context_t;
  struct SubstLookup;
}

HB_INTERNAL void
hb_ot_layout_substitute_lookup (OT::hb_apply_context_t *c,
				const OT::SubstLookup &lookup,
				const hb_ot_layout_lookup_accelerator_t &accel);


/* Should be called after all the substitute_lookup's are done */
HB_INTERNAL void
hb_ot_layout_substitute_finish (hb_font_t    *font,
				hb_buffer_t  *buffer);


/* Should be called before all the position_lookup's are done.  Resets positions to zero. */
HB_INTERNAL void
hb_ot_layout_position_start (hb_font_t    *font,
			     hb_buffer_t  *buffer);

/* Should be called after all the position_lookup's are done */
HB_INTERNAL void
hb_ot_layout_position_finish (hb_font_t    *font,
			      hb_buffer_t  *buffer);



/*
 * hb_ot_layout_t
 */

namespace OT {
  struct GDEF;
  struct GSUB;
  struct GPOS;
}

struct hb_ot_layout_lookup_accelerator_t
{
  template <typename TLookup>
  inline void init (const TLookup &lookup)
  {
    digest.init ();
    lookup.add_coverage (&digest);
  }

  template <typename TLookup>
  inline void fini (const TLookup &lookup)
  {
  }

  hb_set_digest_t digest;
};

struct hb_ot_layout_t
{
  hb_blob_t *gdef_blob;
  hb_blob_t *gsub_blob;
  hb_blob_t *gpos_blob;

  const struct OT::GDEF *gdef;
  const struct OT::GSUB *gsub;
  const struct OT::GPOS *gpos;

  unsigned int gsub_lookup_count;
  unsigned int gpos_lookup_count;

  hb_ot_layout_lookup_accelerator_t *gsub_accels;
  hb_ot_layout_lookup_accelerator_t *gpos_accels;
};


HB_INTERNAL hb_ot_layout_t *
_hb_ot_layout_create (hb_face_t *face);

HB_INTERNAL void
_hb_ot_layout_destroy (hb_ot_layout_t *layout);



#endif /* HB_OT_LAYOUT_PRIVATE_HH */
