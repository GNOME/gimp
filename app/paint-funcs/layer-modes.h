/* GIMP - The GNU Image Manipulation Program
 * Copyright (C); 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option); any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __LAYER_MODES_H__
#define __LAYER_MODES_H__

/* FIXME: This function should not be called directly but
 * through layer_dissolve_mode function
 */
void dissolve_pixels (const guchar *src,
                      const guchar *mask,
                      guchar       *dest,
                      gint          x,
                      gint          y,
                      gint          opacity,
                      gint          length,
                      gint          sb,
                      gint          db,
                      gboolean      has_alpha);

struct apply_layer_mode_struct
{
  guint              bytes1 : 3;
  guint              bytes2 : 3;
  guchar            *src1;
  guchar            *src2;
  const guchar      *mask;
  guchar           **dest;
  gint               x;
  gint               y;
  guint              opacity;
  guint              length;
  CombinationMode    combine;
};

void layer_modes_setup        (void);

void layer_normal_mode        (struct apply_layer_mode_struct *alms);
void layer_dissolve_mode      (struct apply_layer_mode_struct *alms);
void layer_multiply_mode      (struct apply_layer_mode_struct *alms);
void layer_divide_mode        (struct apply_layer_mode_struct *alms);
void layer_screen_mode        (struct apply_layer_mode_struct *alms);
void layer_overlay_mode       (struct apply_layer_mode_struct *alms);
void layer_difference_mode    (struct apply_layer_mode_struct *alms);
void layer_addition_mode      (struct apply_layer_mode_struct *alms);
void layer_subtract_mode      (struct apply_layer_mode_struct *alms);
void layer_darken_only_mode   (struct apply_layer_mode_struct *alms);
void layer_lighten_only_mode  (struct apply_layer_mode_struct *alms);
void layer_hue_mode           (struct apply_layer_mode_struct *alms);
void layer_saturation_mode    (struct apply_layer_mode_struct *alms);
void layer_value_mode         (struct apply_layer_mode_struct *alms);
void layer_color_mode         (struct apply_layer_mode_struct *alms);
void layer_behind_mode        (struct apply_layer_mode_struct *alms);
void layer_replace_mode       (struct apply_layer_mode_struct *alms);
void layer_erase_mode         (struct apply_layer_mode_struct *alms);
void layer_anti_erase_mode    (struct apply_layer_mode_struct *alms);
void layer_color_erase_mode   (struct apply_layer_mode_struct *alms);
void layer_dodge_mode         (struct apply_layer_mode_struct *alms);
void layer_burn_mode          (struct apply_layer_mode_struct *alms);
void layer_hardlight_mode     (struct apply_layer_mode_struct *alms);
void layer_softlight_mode     (struct apply_layer_mode_struct *alms);
void layer_grain_extract_mode (struct apply_layer_mode_struct *alms);
void layer_grain_merge_mode   (struct apply_layer_mode_struct *alms);

#endif

