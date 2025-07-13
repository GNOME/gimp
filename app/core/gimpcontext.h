/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontext.h
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

#pragma once

#include "gimpviewable.h"


#define GIMP_TYPE_CONTEXT            (gimp_context_get_type ())
#define GIMP_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTEXT, GimpContext))
#define GIMP_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, GIMP_TYPE_CONTEXT, GimpContextClass))
#define GIMP_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTEXT))
#define GIMP_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTEXT))
#define GIMP_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS (obj, GIMP_TYPE_CONTEXT, GimpContextClass))


typedef struct _GimpContextClass GimpContextClass;

/**
 * GimpContext:
 *
 * Holds state such as the active image, active display, active brush,
 * active foreground and background color, and so on. There can many
 * instances of contexts. The user context is what the user sees and
 * interacts with but there can also be contexts for docks and for
 * plug-ins.
 */
struct _GimpContext
{
  GimpViewable          parent_instance;

  Gimp                 *gimp;

  GimpContext          *parent;

  guint32               defined_props;
  guint32               serialize_props;

  GimpImage            *image;
  GimpDisplay          *display;

  GimpToolInfo         *tool_info;
  gchar                *tool_name;

  GimpPaintInfo        *paint_info;
  gchar                *paint_name;

  GeglColor            *foreground;
  GeglColor            *background;

  gdouble               opacity;
  GimpLayerMode         paint_mode;

  GimpBrush            *brush;
  gchar                *brush_name;

  GimpDynamics         *dynamics;
  gchar                *dynamics_name;

  GimpMybrush          *mybrush;
  gchar                *mybrush_name;

  GimpPattern          *pattern;
  gchar                *pattern_name;

  GimpGradient         *gradient;
  gchar                *gradient_name;

  GimpPalette          *palette;
  gchar                *palette_name;

  GimpFont             *font;
  gchar                *font_name;

  GimpToolPreset       *tool_preset;
  gchar                *tool_preset_name;

  GimpBuffer           *buffer;
  gchar                *buffer_name;

  GimpImagefile        *imagefile;
  gchar                *imagefile_name;

  GimpTemplate         *template;
  gchar                *template_name;

  GimpLineArt          *line_art;
  guint                 line_art_timeout_id;
};

struct _GimpContextClass
{
  GimpViewableClass  parent_class;

  void (* image_changed)      (GimpContext          *context,
                               GimpImage            *image);
  void (* display_changed)    (GimpContext          *context,
                               GimpDisplay          *display);

  void (* tool_changed)       (GimpContext          *context,
                               GimpToolInfo         *tool_info);
  void (* paint_info_changed) (GimpContext          *context,
                               GimpPaintInfo        *paint_info);

  void (* foreground_changed) (GimpContext          *context,
                               GeglColor            *color);
  void (* background_changed) (GimpContext          *context,
                               GeglColor            *color);
  void (* opacity_changed)    (GimpContext          *context,
                               gdouble               opacity);
  void (* paint_mode_changed) (GimpContext          *context,
                               GimpLayerMode         paint_mode);
  void (* brush_changed)      (GimpContext          *context,
                               GimpBrush            *brush);
  void (* dynamics_changed)   (GimpContext          *context,
                               GimpDynamics         *dynamics);
  void (* mybrush_changed)    (GimpContext          *context,
                               GimpMybrush          *brush);
  void (* pattern_changed)    (GimpContext          *context,
                               GimpPattern          *pattern);
  void (* gradient_changed)   (GimpContext          *context,
                               GimpGradient         *gradient);
  void (* palette_changed)    (GimpContext          *context,
                               GimpPalette          *palette);
  void (* font_changed)       (GimpContext          *context,
                               GimpFont             *font);
  void (* tool_preset_changed)(GimpContext          *context,
                               GimpToolPreset       *tool_preset);
  void (* buffer_changed)     (GimpContext          *context,
                               GimpBuffer           *buffer);
  void (* imagefile_changed)  (GimpContext          *context,
                               GimpImagefile        *imagefile);
  void (* template_changed)   (GimpContext          *context,
                               GimpTemplate         *template);

  void (* prop_name_changed)  (GimpContext          *context,
                               GimpContextPropType   prop);
};


GType         gimp_context_get_type          (void) G_GNUC_CONST;

GimpContext * gimp_context_new               (Gimp                *gimp,
                                              const gchar         *name,
                                              GimpContext         *template);

GimpContext * gimp_context_get_parent        (GimpContext         *context);
void          gimp_context_set_parent        (GimpContext         *context,
                                              GimpContext         *parent);

/*  define / undefinine context properties
 *
 *  the value of an undefined property will be taken from the parent context.
 */
void          gimp_context_define_property   (GimpContext         *context,
                                              GimpContextPropType  prop,
                                              gboolean             defined);

gboolean      gimp_context_property_defined  (GimpContext         *context,
                                              GimpContextPropType  prop);

void          gimp_context_define_properties (GimpContext         *context,
                                              GimpContextPropMask  props_mask,
                                              gboolean             defined);


/*  specify which context properties will be serialized
 */
void   gimp_context_set_serialize_properties (GimpContext         *context,
                                              GimpContextPropMask  props_mask);

GimpContextPropMask
       gimp_context_get_serialize_properties (GimpContext         *context);


/*  copying context properties
 */
void          gimp_context_copy_property     (GimpContext         *src,
                                              GimpContext         *dest,
                                              GimpContextPropType  prop);

void          gimp_context_copy_properties   (GimpContext         *src,
                                              GimpContext         *dest,
                                              GimpContextPropMask  props_mask);


/*  manipulate by GType  */
GimpContextPropType   gimp_context_type_to_property    (GType     type);
const gchar         * gimp_context_type_to_prop_name   (GType     type);
const gchar         * gimp_context_type_to_signal_name (GType     type);

GimpObject     * gimp_context_get_by_type         (GimpContext     *context,
                                                   GType            type);
void             gimp_context_set_by_type         (GimpContext     *context,
                                                   GType            type,
                                                   GimpObject      *object);
void             gimp_context_changed_by_type     (GimpContext     *context,
                                                   GType            type);


/*  image  */
GimpImage      * gimp_context_get_image           (GimpContext     *context);
void             gimp_context_set_image           (GimpContext     *context,
                                                   GimpImage       *image);
void             gimp_context_image_changed       (GimpContext     *context);


const Babl *     gimp_context_get_rgba_format     (GimpContext     *context,
                                                   GeglColor       *color,
                                                   const gchar     *babl_type,
                                                   GimpImage      **space_image);


/*  display  */
GimpDisplay    * gimp_context_get_display         (GimpContext     *context);
void             gimp_context_set_display         (GimpContext     *context,
                                                   GimpDisplay     *display);
void             gimp_context_display_changed     (GimpContext     *context);


/*  tool  */
GimpToolInfo   * gimp_context_get_tool            (GimpContext     *context);
void             gimp_context_set_tool            (GimpContext     *context,
                                                   GimpToolInfo    *tool_info);
void             gimp_context_tool_changed        (GimpContext     *context);


/*  paint info  */
GimpPaintInfo  * gimp_context_get_paint_info      (GimpContext     *context);
void             gimp_context_set_paint_info      (GimpContext     *context,
                                                   GimpPaintInfo   *paint_info);
void             gimp_context_paint_info_changed  (GimpContext     *context);


/*  foreground color  */
GeglColor      * gimp_context_get_foreground       (GimpContext     *context);
void             gimp_context_set_foreground       (GimpContext     *context,
                                                    GeglColor       *color);
void             gimp_context_foreground_changed   (GimpContext     *context);


/*  background color  */
GeglColor      * gimp_context_get_background       (GimpContext     *context);
void             gimp_context_set_background       (GimpContext     *context,
                                                    GeglColor       *color);
void             gimp_context_background_changed   (GimpContext     *context);


/*  color utility functions  */
void             gimp_context_set_default_colors  (GimpContext     *context);
void             gimp_context_swap_colors         (GimpContext     *context);


/*  opacity  */
gdouble          gimp_context_get_opacity         (GimpContext     *context);
void             gimp_context_set_opacity         (GimpContext     *context,
                                                   gdouble          opacity);
void             gimp_context_opacity_changed     (GimpContext     *context);


/*  paint mode  */
GimpLayerMode    gimp_context_get_paint_mode      (GimpContext     *context);
void             gimp_context_set_paint_mode      (GimpContext     *context,
                                                   GimpLayerMode    paint_mode);
void             gimp_context_paint_mode_changed  (GimpContext     *context);


/*  brush  */
GimpBrush      * gimp_context_get_brush           (GimpContext     *context);
void             gimp_context_set_brush           (GimpContext     *context,
                                                   GimpBrush       *brush);
void             gimp_context_brush_changed       (GimpContext     *context);


/*  dynamics  */
GimpDynamics   * gimp_context_get_dynamics        (GimpContext     *context);
void             gimp_context_set_dynamics        (GimpContext     *context,
                                                   GimpDynamics    *dynamics);
void             gimp_context_dynamics_changed    (GimpContext     *context);


/*  mybrush  */
GimpMybrush    * gimp_context_get_mybrush         (GimpContext     *context);
void             gimp_context_set_mybrush         (GimpContext     *context,
                                                   GimpMybrush     *brush);
void             gimp_context_mybrush_changed     (GimpContext     *context);


/*  pattern  */
GimpPattern    * gimp_context_get_pattern         (GimpContext     *context);
void             gimp_context_set_pattern         (GimpContext     *context,
                                                   GimpPattern     *pattern);
void             gimp_context_pattern_changed     (GimpContext     *context);


/*  gradient  */
GimpGradient   * gimp_context_get_gradient        (GimpContext     *context);
void             gimp_context_set_gradient        (GimpContext     *context,
                                                   GimpGradient    *gradient);
void             gimp_context_gradient_changed    (GimpContext     *context);


/*  palette  */
GimpPalette    * gimp_context_get_palette         (GimpContext     *context);
void             gimp_context_set_palette         (GimpContext     *context,
                                                   GimpPalette     *palette);
void             gimp_context_palette_changed     (GimpContext     *context);


/*  font  */
GimpFont       * gimp_context_get_font            (GimpContext     *context);
void             gimp_context_set_font            (GimpContext     *context,
                                                   GimpFont        *font);
const gchar    * gimp_context_get_font_name       (GimpContext     *context);
void             gimp_context_set_font_name       (GimpContext     *context,
                                                   const gchar     *name);
void             gimp_context_font_changed        (GimpContext     *context);


/*  tool_preset  */
GimpToolPreset * gimp_context_get_tool_preset     (GimpContext     *context);
void             gimp_context_set_tool_preset     (GimpContext     *context,
                                                   GimpToolPreset  *tool_preset);
void             gimp_context_tool_preset_changed (GimpContext     *context);


/*  buffer  */
GimpBuffer     * gimp_context_get_buffer          (GimpContext     *context);
void             gimp_context_set_buffer          (GimpContext     *context,
                                                   GimpBuffer      *palette);
void             gimp_context_buffer_changed      (GimpContext     *context);


/*  imagefile  */
GimpImagefile  * gimp_context_get_imagefile       (GimpContext     *context);
void             gimp_context_set_imagefile       (GimpContext     *context,
                                                   GimpImagefile   *imagefile);
void             gimp_context_imagefile_changed   (GimpContext     *context);


/*  template  */
GimpTemplate   * gimp_context_get_template        (GimpContext     *context);
void             gimp_context_set_template        (GimpContext     *context,
                                                   GimpTemplate    *template);
void             gimp_context_template_changed    (GimpContext     *context);

/*  line art  */
GimpLineArt    * gimp_context_take_line_art       (GimpContext     *context);
void             gimp_context_store_line_art      (GimpContext     *context,
                                                   GimpLineArt     *line_art);
