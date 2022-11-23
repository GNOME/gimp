/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontext.h
 * Copyright (C) 1999-2010 Michael Natterer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_CONTEXT_H__
#define __LIGMA_CONTEXT_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_CONTEXT            (ligma_context_get_type ())
#define LIGMA_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTEXT, LigmaContext))
#define LIGMA_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, LIGMA_TYPE_CONTEXT, LigmaContextClass))
#define LIGMA_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTEXT))
#define LIGMA_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTEXT))
#define LIGMA_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS (obj, LIGMA_TYPE_CONTEXT, LigmaContextClass))


typedef struct _LigmaContextClass LigmaContextClass;

/**
 * LigmaContext:
 *
 * Holds state such as the active image, active display, active brush,
 * active foreground and background color, and so on. There can many
 * instances of contexts. The user context is what the user sees and
 * interacts with but there can also be contexts for docks and for
 * plug-ins.
 */
struct _LigmaContext
{
  LigmaViewable          parent_instance;

  Ligma                 *ligma;

  LigmaContext          *parent;

  guint32               defined_props;
  guint32               serialize_props;

  LigmaImage            *image;
  LigmaDisplay          *display;

  LigmaToolInfo         *tool_info;
  gchar                *tool_name;

  LigmaPaintInfo        *paint_info;
  gchar                *paint_name;

  LigmaRGB               foreground;
  LigmaRGB               background;

  gdouble               opacity;
  LigmaLayerMode         paint_mode;

  LigmaBrush            *brush;
  gchar                *brush_name;

  LigmaDynamics         *dynamics;
  gchar                *dynamics_name;

  LigmaMybrush          *mybrush;
  gchar                *mybrush_name;

  LigmaPattern          *pattern;
  gchar                *pattern_name;

  LigmaGradient         *gradient;
  gchar                *gradient_name;

  LigmaPalette          *palette;
  gchar                *palette_name;

  LigmaFont             *font;
  gchar                *font_name;

  LigmaToolPreset       *tool_preset;
  gchar                *tool_preset_name;

  LigmaBuffer           *buffer;
  gchar                *buffer_name;

  LigmaImagefile        *imagefile;
  gchar                *imagefile_name;

  LigmaTemplate         *template;
  gchar                *template_name;

  LigmaLineArt          *line_art;
  guint                 line_art_timeout_id;
};

struct _LigmaContextClass
{
  LigmaViewableClass  parent_class;

  void (* image_changed)      (LigmaContext          *context,
                               LigmaImage            *image);
  void (* display_changed)    (LigmaContext          *context,
                               LigmaDisplay          *display);

  void (* tool_changed)       (LigmaContext          *context,
                               LigmaToolInfo         *tool_info);
  void (* paint_info_changed) (LigmaContext          *context,
                               LigmaPaintInfo        *paint_info);

  void (* foreground_changed) (LigmaContext          *context,
                               LigmaRGB              *color);
  void (* background_changed) (LigmaContext          *context,
                               LigmaRGB              *color);
  void (* opacity_changed)    (LigmaContext          *context,
                               gdouble               opacity);
  void (* paint_mode_changed) (LigmaContext          *context,
                               LigmaLayerMode         paint_mode);
  void (* brush_changed)      (LigmaContext          *context,
                               LigmaBrush            *brush);
  void (* dynamics_changed)   (LigmaContext          *context,
                               LigmaDynamics         *dynamics);
  void (* mybrush_changed)    (LigmaContext          *context,
                               LigmaMybrush          *brush);
  void (* pattern_changed)    (LigmaContext          *context,
                               LigmaPattern          *pattern);
  void (* gradient_changed)   (LigmaContext          *context,
                               LigmaGradient         *gradient);
  void (* palette_changed)    (LigmaContext          *context,
                               LigmaPalette          *palette);
  void (* font_changed)       (LigmaContext          *context,
                               LigmaFont             *font);
  void (* tool_preset_changed)(LigmaContext          *context,
                               LigmaToolPreset       *tool_preset);
  void (* buffer_changed)     (LigmaContext          *context,
                               LigmaBuffer           *buffer);
  void (* imagefile_changed)  (LigmaContext          *context,
                               LigmaImagefile        *imagefile);
  void (* template_changed)   (LigmaContext          *context,
                               LigmaTemplate         *template);

  void (* prop_name_changed)  (LigmaContext          *context,
                               LigmaContextPropType   prop);
};


GType         ligma_context_get_type          (void) G_GNUC_CONST;

LigmaContext * ligma_context_new               (Ligma                *ligma,
                                              const gchar         *name,
                                              LigmaContext         *template);

LigmaContext * ligma_context_get_parent        (LigmaContext         *context);
void          ligma_context_set_parent        (LigmaContext         *context,
                                              LigmaContext         *parent);

/*  define / undefinine context properties
 *
 *  the value of an undefined property will be taken from the parent context.
 */
void          ligma_context_define_property   (LigmaContext         *context,
                                              LigmaContextPropType  prop,
                                              gboolean             defined);

gboolean      ligma_context_property_defined  (LigmaContext         *context,
                                              LigmaContextPropType  prop);

void          ligma_context_define_properties (LigmaContext         *context,
                                              LigmaContextPropMask  props_mask,
                                              gboolean             defined);


/*  specify which context properties will be serialized
 */
void   ligma_context_set_serialize_properties (LigmaContext         *context,
                                              LigmaContextPropMask  props_mask);

LigmaContextPropMask
       ligma_context_get_serialize_properties (LigmaContext         *context);


/*  copying context properties
 */
void          ligma_context_copy_property     (LigmaContext         *src,
                                              LigmaContext         *dest,
                                              LigmaContextPropType  prop);

void          ligma_context_copy_properties   (LigmaContext         *src,
                                              LigmaContext         *dest,
                                              LigmaContextPropMask  props_mask);


/*  manipulate by GType  */
LigmaContextPropType   ligma_context_type_to_property    (GType     type);
const gchar         * ligma_context_type_to_prop_name   (GType     type);
const gchar         * ligma_context_type_to_signal_name (GType     type);

LigmaObject     * ligma_context_get_by_type         (LigmaContext     *context,
                                                   GType            type);
void             ligma_context_set_by_type         (LigmaContext     *context,
                                                   GType            type,
                                                   LigmaObject      *object);
void             ligma_context_changed_by_type     (LigmaContext     *context,
                                                   GType            type);


/*  image  */
LigmaImage      * ligma_context_get_image           (LigmaContext     *context);
void             ligma_context_set_image           (LigmaContext     *context,
                                                   LigmaImage       *image);
void             ligma_context_image_changed       (LigmaContext     *context);


/*  display  */
LigmaDisplay    * ligma_context_get_display         (LigmaContext     *context);
void             ligma_context_set_display         (LigmaContext     *context,
                                                   LigmaDisplay     *display);
void             ligma_context_display_changed     (LigmaContext     *context);


/*  tool  */
LigmaToolInfo   * ligma_context_get_tool            (LigmaContext     *context);
void             ligma_context_set_tool            (LigmaContext     *context,
                                                   LigmaToolInfo    *tool_info);
void             ligma_context_tool_changed        (LigmaContext     *context);


/*  paint info  */
LigmaPaintInfo  * ligma_context_get_paint_info      (LigmaContext     *context);
void             ligma_context_set_paint_info      (LigmaContext     *context,
                                                   LigmaPaintInfo   *paint_info);
void             ligma_context_paint_info_changed  (LigmaContext     *context);


/*  foreground color  */
void             ligma_context_get_foreground       (LigmaContext     *context,
                                                    LigmaRGB         *color);
void             ligma_context_set_foreground       (LigmaContext     *context,
                                                    const LigmaRGB   *color);
void             ligma_context_foreground_changed   (LigmaContext     *context);


/*  background color  */
void             ligma_context_get_background       (LigmaContext     *context,
                                                    LigmaRGB         *color);
void             ligma_context_set_background       (LigmaContext     *context,
                                                    const LigmaRGB   *color);
void             ligma_context_background_changed   (LigmaContext     *context);


/*  color utility functions  */
void             ligma_context_set_default_colors  (LigmaContext     *context);
void             ligma_context_swap_colors         (LigmaContext     *context);


/*  opacity  */
gdouble          ligma_context_get_opacity         (LigmaContext     *context);
void             ligma_context_set_opacity         (LigmaContext     *context,
                                                   gdouble          opacity);
void             ligma_context_opacity_changed     (LigmaContext     *context);


/*  paint mode  */
LigmaLayerMode    ligma_context_get_paint_mode      (LigmaContext     *context);
void             ligma_context_set_paint_mode      (LigmaContext     *context,
                                                   LigmaLayerMode    paint_mode);
void             ligma_context_paint_mode_changed  (LigmaContext     *context);


/*  brush  */
LigmaBrush      * ligma_context_get_brush           (LigmaContext     *context);
void             ligma_context_set_brush           (LigmaContext     *context,
                                                   LigmaBrush       *brush);
void             ligma_context_brush_changed       (LigmaContext     *context);


/*  dynamics  */
LigmaDynamics   * ligma_context_get_dynamics        (LigmaContext     *context);
void             ligma_context_set_dynamics        (LigmaContext     *context,
                                                   LigmaDynamics    *dynamics);
void             ligma_context_dynamics_changed    (LigmaContext     *context);


/*  mybrush  */
LigmaMybrush    * ligma_context_get_mybrush         (LigmaContext     *context);
void             ligma_context_set_mybrush         (LigmaContext     *context,
                                                   LigmaMybrush     *brush);
void             ligma_context_mybrush_changed     (LigmaContext     *context);


/*  pattern  */
LigmaPattern    * ligma_context_get_pattern         (LigmaContext     *context);
void             ligma_context_set_pattern         (LigmaContext     *context,
                                                   LigmaPattern     *pattern);
void             ligma_context_pattern_changed     (LigmaContext     *context);


/*  gradient  */
LigmaGradient   * ligma_context_get_gradient        (LigmaContext     *context);
void             ligma_context_set_gradient        (LigmaContext     *context,
                                                   LigmaGradient    *gradient);
void             ligma_context_gradient_changed    (LigmaContext     *context);


/*  palette  */
LigmaPalette    * ligma_context_get_palette         (LigmaContext     *context);
void             ligma_context_set_palette         (LigmaContext     *context,
                                                   LigmaPalette     *palette);
void             ligma_context_palette_changed     (LigmaContext     *context);


/*  font  */
LigmaFont       * ligma_context_get_font            (LigmaContext     *context);
void             ligma_context_set_font            (LigmaContext     *context,
                                                   LigmaFont        *font);
const gchar    * ligma_context_get_font_name       (LigmaContext     *context);
void             ligma_context_set_font_name       (LigmaContext     *context,
                                                   const gchar     *name);
void             ligma_context_font_changed        (LigmaContext     *context);


/*  tool_preset  */
LigmaToolPreset * ligma_context_get_tool_preset     (LigmaContext     *context);
void             ligma_context_set_tool_preset     (LigmaContext     *context,
                                                   LigmaToolPreset  *tool_preset);
void             ligma_context_tool_preset_changed (LigmaContext     *context);


/*  buffer  */
LigmaBuffer     * ligma_context_get_buffer          (LigmaContext     *context);
void             ligma_context_set_buffer          (LigmaContext     *context,
                                                   LigmaBuffer      *palette);
void             ligma_context_buffer_changed      (LigmaContext     *context);


/*  imagefile  */
LigmaImagefile  * ligma_context_get_imagefile       (LigmaContext     *context);
void             ligma_context_set_imagefile       (LigmaContext     *context,
                                                   LigmaImagefile   *imagefile);
void             ligma_context_imagefile_changed   (LigmaContext     *context);


/*  template  */
LigmaTemplate   * ligma_context_get_template        (LigmaContext     *context);
void             ligma_context_set_template        (LigmaContext     *context,
                                                   LigmaTemplate    *template);
void             ligma_context_template_changed    (LigmaContext     *context);

/*  line art  */
LigmaLineArt    * ligma_context_take_line_art       (LigmaContext     *context);
void             ligma_context_store_line_art      (LigmaContext     *context,
                                                   LigmaLineArt     *line_art);


#endif /* __LIGMA_CONTEXT_H__ */
