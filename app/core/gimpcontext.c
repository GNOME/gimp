/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontext.c
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

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimpbrush.h"
#include "gimpbuffer.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpdisplay.h"
#include "gimpdynamics.h"
#include "gimpimagefile.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimplineart.h"
#include "gimpmybrush.h"
#include "gimppaintinfo.h"
#include "gimppalette.h"
#include "gimppattern.h"
#include "gimptemplate.h"
#include "gimptoolinfo.h"
#include "gimptoolpreset.h"

#include "text/gimpfont.h"

#include "gimp-intl.h"


typedef void (* GimpContextCopyPropFunc) (GimpContext *src,
                                          GimpContext *dest);

#define context_find_defined(context, prop)                              \
  while (!(((context)->defined_props) & (1 << (prop))) && (context)->parent) \
    (context) = (context)->parent

#define COPY_NAME(src, dest, member)            \
  g_free (dest->member);                        \
  dest->member = g_strdup (src->member)


/*  local function prototypes  */

static void    gimp_context_config_iface_init (GimpConfigInterface   *iface);

static void       gimp_context_constructed    (GObject               *object);
static void       gimp_context_dispose        (GObject               *object);
static void       gimp_context_finalize       (GObject               *object);
static void       gimp_context_set_property   (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void       gimp_context_get_property   (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);
static gint64     gimp_context_get_memsize    (GimpObject            *object,
                                               gint64                *gui_size);

static gboolean   gimp_context_serialize            (GimpConfig       *config,
                                                     GimpConfigWriter *writer,
                                                     gpointer          data);
static gboolean   gimp_context_deserialize          (GimpConfig       *config,
                                                     GScanner         *scanner,
                                                     gint              nest_level,
                                                     gpointer          data);
static gboolean   gimp_context_serialize_property   (GimpConfig       *config,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec,
                                                     GimpConfigWriter *writer);
static gboolean   gimp_context_deserialize_property (GimpConfig       *config,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec,
                                                     GScanner         *scanner,
                                                     GTokenType       *expected);
static GimpConfig * gimp_context_duplicate          (GimpConfig       *config);
static gboolean     gimp_context_copy               (GimpConfig       *src,
                                                     GimpConfig       *dest,
                                                     GParamFlags       flags);

/*  image  */
static void gimp_context_image_disconnect    (GimpImage        *image,
                                              GimpContext      *context);
static void gimp_context_image_removed       (GimpContainer    *container,
                                              GimpImage        *image,
                                              GimpContext      *context);
static void gimp_context_real_set_image      (GimpContext      *context,
                                              GimpImage        *image);

/*  display  */
static void gimp_context_display_removed     (GimpContainer    *container,
                                              GimpDisplay      *display,
                                              GimpContext      *context);
static void gimp_context_real_set_display    (GimpContext      *context,
                                              GimpDisplay      *display);

/*  tool  */
static void gimp_context_tool_dirty          (GimpToolInfo     *tool_info,
                                              GimpContext      *context);
static void gimp_context_tool_removed        (GimpContainer    *container,
                                              GimpToolInfo     *tool_info,
                                              GimpContext      *context);
static void gimp_context_tool_list_thaw      (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_tool       (GimpContext      *context,
                                              GimpToolInfo     *tool_info);

/*  paint info  */
static void gimp_context_paint_info_dirty    (GimpPaintInfo    *paint_info,
                                              GimpContext      *context);
static void gimp_context_paint_info_removed  (GimpContainer    *container,
                                              GimpPaintInfo    *paint_info,
                                              GimpContext      *context);
static void gimp_context_paint_info_list_thaw(GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_paint_info (GimpContext      *context,
                                              GimpPaintInfo    *paint_info);

/*  foreground  */
static void gimp_context_real_set_foreground (GimpContext      *context,
                                              GeglColor        *color);

/*  background  */
static void gimp_context_real_set_background (GimpContext      *context,
                                              GeglColor        *color);

/*  opacity  */
static void gimp_context_real_set_opacity    (GimpContext      *context,
                                              gdouble           opacity);

/*  paint mode  */
static void gimp_context_real_set_paint_mode (GimpContext      *context,
                                              GimpLayerMode     paint_mode);

/*  brush  */
static void gimp_context_brush_dirty         (GimpBrush        *brush,
                                              GimpContext      *context);
static void gimp_context_brush_removed       (GimpContainer    *brush_list,
                                              GimpBrush        *brush,
                                              GimpContext      *context);
static void gimp_context_brush_list_thaw     (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_brush      (GimpContext      *context,
                                              GimpBrush        *brush);

/*  dynamics  */

static void gimp_context_dynamics_dirty      (GimpDynamics     *dynamics,
                                              GimpContext      *context);
static void gimp_context_dynamics_removed    (GimpContainer    *container,
                                              GimpDynamics     *dynamics,
                                              GimpContext      *context);
static void gimp_context_dynamics_list_thaw  (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_dynamics   (GimpContext      *context,
                                              GimpDynamics     *dynamics);

/*  mybrush  */
static void gimp_context_mybrush_dirty       (GimpMybrush      *brush,
                                              GimpContext      *context);
static void gimp_context_mybrush_removed     (GimpContainer    *brush_list,
                                              GimpMybrush      *brush,
                                              GimpContext      *context);
static void gimp_context_mybrush_list_thaw   (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_mybrush    (GimpContext      *context,
                                              GimpMybrush      *brush);

/*  pattern  */
static void gimp_context_pattern_dirty       (GimpPattern      *pattern,
                                              GimpContext      *context);
static void gimp_context_pattern_removed     (GimpContainer    *container,
                                              GimpPattern      *pattern,
                                              GimpContext      *context);
static void gimp_context_pattern_list_thaw   (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_pattern    (GimpContext      *context,
                                              GimpPattern      *pattern);

/*  gradient  */
static void gimp_context_gradient_dirty      (GimpGradient     *gradient,
                                              GimpContext      *context);
static void gimp_context_gradient_removed    (GimpContainer    *container,
                                              GimpGradient     *gradient,
                                              GimpContext      *context);
static void gimp_context_gradient_list_thaw  (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_gradient   (GimpContext      *context,
                                              GimpGradient     *gradient);

/*  palette  */
static void gimp_context_palette_dirty       (GimpPalette      *palette,
                                              GimpContext      *context);
static void gimp_context_palette_removed     (GimpContainer    *container,
                                              GimpPalette      *palette,
                                              GimpContext      *context);
static void gimp_context_palette_list_thaw   (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_palette    (GimpContext      *context,
                                              GimpPalette      *palette);

/*  font  */
static void gimp_context_font_dirty          (GimpFont         *font,
                                              GimpContext      *context);
static void gimp_context_font_removed        (GimpContainer    *container,
                                              GimpFont         *font,
                                              GimpContext      *context);
static void gimp_context_font_list_thaw      (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_font       (GimpContext      *context,
                                              GimpFont         *font);

/*  tool preset  */
static void gimp_context_tool_preset_dirty     (GimpToolPreset   *tool_preset,
                                                GimpContext      *context);
static void gimp_context_tool_preset_removed   (GimpContainer    *container,
                                                GimpToolPreset   *tool_preset,
                                                GimpContext      *context);
static void gimp_context_tool_preset_list_thaw (GimpContainer    *container,
                                                GimpContext      *context);
static void gimp_context_real_set_tool_preset  (GimpContext      *context,
                                                GimpToolPreset   *tool_preset);

/*  buffer  */
static void gimp_context_buffer_dirty        (GimpBuffer       *buffer,
                                              GimpContext      *context);
static void gimp_context_buffer_removed      (GimpContainer    *container,
                                              GimpBuffer       *buffer,
                                              GimpContext      *context);
static void gimp_context_buffer_list_thaw    (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_buffer     (GimpContext      *context,
                                              GimpBuffer       *buffer);

/*  imagefile  */
static void gimp_context_imagefile_dirty     (GimpImagefile    *imagefile,
                                              GimpContext      *context);
static void gimp_context_imagefile_removed   (GimpContainer    *container,
                                              GimpImagefile    *imagefile,
                                              GimpContext      *context);
static void gimp_context_imagefile_list_thaw (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_imagefile  (GimpContext      *context,
                                              GimpImagefile    *imagefile);

/*  template  */
static void gimp_context_template_dirty      (GimpTemplate     *template,
                                              GimpContext      *context);
static void gimp_context_template_removed    (GimpContainer    *container,
                                              GimpTemplate     *template,
                                              GimpContext      *context);
static void gimp_context_template_list_thaw  (GimpContainer    *container,
                                              GimpContext      *context);
static void gimp_context_real_set_template   (GimpContext      *context,
                                              GimpTemplate     *template);


/*  line art  */
static gboolean gimp_context_free_line_art   (GimpContext      *context);


/*  utilities  */
static gpointer gimp_context_find_object     (GimpContext      *context,
                                              GimpContainer    *container,
                                              const gchar      *object_name,
                                              gpointer          standard_object);


/*  properties & signals  */

enum
{
  GIMP_CONTEXT_PROP_0,
  GIMP_CONTEXT_PROP_GIMP

  /*  remaining values are in core-enums.h  (GimpContextPropType)  */
};

enum
{
  DUMMY_0,
  DUMMY_1,
  IMAGE_CHANGED,
  DISPLAY_CHANGED,
  TOOL_CHANGED,
  PAINT_INFO_CHANGED,
  FOREGROUND_CHANGED,
  BACKGROUND_CHANGED,
  OPACITY_CHANGED,
  PAINT_MODE_CHANGED,
  BRUSH_CHANGED,
  DYNAMICS_CHANGED,
  MYBRUSH_CHANGED,
  PATTERN_CHANGED,
  GRADIENT_CHANGED,
  PALETTE_CHANGED,
  FONT_CHANGED,
  TOOL_PRESET_CHANGED,
  BUFFER_CHANGED,
  IMAGEFILE_CHANGED,
  TEMPLATE_CHANGED,
  PROP_NAME_CHANGED,
  LAST_SIGNAL
};

static const gchar * const gimp_context_prop_names[] =
{
  NULL, /* PROP_0 */
  "gimp",
  "image",
  "display",
  "tool",
  "paint-info",
  "foreground",
  "background",
  "opacity",
  "paint-mode",
  "brush",
  "dynamics",
  "mybrush",
  "pattern",
  "gradient",
  "palette",
  "font",
  "tool-preset",
  "buffer",
  "imagefile",
  "template"
};

static GType gimp_context_prop_types[] =
{
  G_TYPE_NONE, /* PROP_0    */
  G_TYPE_NONE, /* PROP_GIMP */
  0,
  G_TYPE_NONE,
  0,
  0,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


G_DEFINE_TYPE_WITH_CODE (GimpContext, gimp_context, GIMP_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_context_config_iface_init))

#define parent_class gimp_context_parent_class

static GimpConfigInterface *parent_config_iface = NULL;

static guint gimp_context_signals[LAST_SIGNAL] = { 0 };


static void
gimp_context_class_init (GimpContextClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GeglColor       *black             = gegl_color_new ("black");
  GeglColor       *white             = gegl_color_new ("white");

  gimp_context_signals[IMAGE_CHANGED] =
    g_signal_new ("image-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, image_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_IMAGE);

  gimp_context_signals[DISPLAY_CHANGED] =
    g_signal_new ("display-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, display_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DISPLAY);

  gimp_context_signals[TOOL_CHANGED] =
    g_signal_new ("tool-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, tool_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_TOOL_INFO);

  gimp_context_signals[PAINT_INFO_CHANGED] =
    g_signal_new ("paint-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, paint_info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PAINT_INFO);

  gimp_context_signals[FOREGROUND_CHANGED] =
    g_signal_new ("foreground-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, foreground_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  gimp_context_signals[BACKGROUND_CHANGED] =
    g_signal_new ("background-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, background_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  gimp_context_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, opacity_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    g_signal_new ("paint-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, paint_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_LAYER_MODE);

  gimp_context_signals[BRUSH_CHANGED] =
    g_signal_new ("brush-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, brush_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_BRUSH);

  gimp_context_signals[DYNAMICS_CHANGED] =
    g_signal_new ("dynamics-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, dynamics_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DYNAMICS);

  gimp_context_signals[MYBRUSH_CHANGED] =
    g_signal_new ("mybrush-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, mybrush_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_MYBRUSH);

  gimp_context_signals[PATTERN_CHANGED] =
    g_signal_new ("pattern-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, pattern_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PATTERN);

  gimp_context_signals[GRADIENT_CHANGED] =
    g_signal_new ("gradient-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, gradient_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GRADIENT);

  gimp_context_signals[PALETTE_CHANGED] =
    g_signal_new ("palette-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, palette_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PALETTE);

  gimp_context_signals[FONT_CHANGED] =
    g_signal_new ("font-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, font_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_FONT);

  gimp_context_signals[TOOL_PRESET_CHANGED] =
    g_signal_new ("tool-preset-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, tool_preset_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_TOOL_PRESET);

  gimp_context_signals[BUFFER_CHANGED] =
    g_signal_new ("buffer-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, buffer_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_BUFFER);

  gimp_context_signals[IMAGEFILE_CHANGED] =
    g_signal_new ("imagefile-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, imagefile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_IMAGEFILE);

  gimp_context_signals[TEMPLATE_CHANGED] =
    g_signal_new ("template-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, template_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_TEMPLATE);

  gimp_context_signals[PROP_NAME_CHANGED] =
    g_signal_new ("prop-name-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContextClass, prop_name_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  object_class->constructed      = gimp_context_constructed;
  object_class->set_property     = gimp_context_set_property;
  object_class->get_property     = gimp_context_get_property;
  object_class->dispose          = gimp_context_dispose;
  object_class->finalize         = gimp_context_finalize;

  gimp_object_class->get_memsize = gimp_context_get_memsize;

  klass->image_changed           = NULL;
  klass->display_changed         = NULL;
  klass->tool_changed            = NULL;
  klass->paint_info_changed      = NULL;
  klass->foreground_changed      = NULL;
  klass->background_changed      = NULL;
  klass->opacity_changed         = NULL;
  klass->paint_mode_changed      = NULL;
  klass->brush_changed           = NULL;
  klass->dynamics_changed        = NULL;
  klass->mybrush_changed         = NULL;
  klass->pattern_changed         = NULL;
  klass->gradient_changed        = NULL;
  klass->palette_changed         = NULL;
  klass->font_changed            = NULL;
  klass->tool_preset_changed     = NULL;
  klass->buffer_changed          = NULL;
  klass->imagefile_changed       = NULL;
  klass->template_changed        = NULL;
  klass->prop_name_changed       = NULL;

  gimp_context_prop_types[GIMP_CONTEXT_PROP_IMAGE]       = GIMP_TYPE_IMAGE;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_TOOL]        = GIMP_TYPE_TOOL_INFO;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_PAINT_INFO]  = GIMP_TYPE_PAINT_INFO;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_BRUSH]       = GIMP_TYPE_BRUSH;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_DYNAMICS]    = GIMP_TYPE_DYNAMICS;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_MYBRUSH]     = GIMP_TYPE_MYBRUSH;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_PATTERN]     = GIMP_TYPE_PATTERN;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_GRADIENT]    = GIMP_TYPE_GRADIENT;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_PALETTE]     = GIMP_TYPE_PALETTE;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_FONT]        = GIMP_TYPE_FONT;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_TOOL_PRESET] = GIMP_TYPE_TOOL_PRESET;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_BUFFER]      = GIMP_TYPE_BUFFER;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_IMAGEFILE]   = GIMP_TYPE_IMAGEFILE;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_TEMPLATE]    = GIMP_TYPE_TEMPLATE;

  g_object_class_install_property (object_class, GIMP_CONTEXT_PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, GIMP_CONTEXT_PROP_IMAGE,
                                   g_param_spec_object (gimp_context_prop_names[GIMP_CONTEXT_PROP_IMAGE],
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, GIMP_CONTEXT_PROP_DISPLAY,
                                   g_param_spec_object (gimp_context_prop_names[GIMP_CONTEXT_PROP_DISPLAY],
                                                        NULL, NULL,
                                                        GIMP_TYPE_DISPLAY,
                                                        GIMP_PARAM_READWRITE));

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_TOOL,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_TOOL],
                           NULL, NULL,
                           GIMP_TYPE_TOOL_INFO,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_PAINT_INFO,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_PAINT_INFO],
                           NULL, NULL,
                           GIMP_TYPE_PAINT_INFO,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_COLOR (object_class, GIMP_CONTEXT_PROP_FOREGROUND,
                          gimp_context_prop_names[GIMP_CONTEXT_PROP_FOREGROUND],
                          _("Foreground"),
                          _("Foreground color"),
                          FALSE, black,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_COLOR (object_class, GIMP_CONTEXT_PROP_BACKGROUND,
                          gimp_context_prop_names[GIMP_CONTEXT_PROP_BACKGROUND],
                          _("Background"),
                          _("Background color"),
                          FALSE, white,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, GIMP_CONTEXT_PROP_OPACITY,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_OPACITY],
                           _("Opacity"),
                           _("Opacity"),
                           GIMP_OPACITY_TRANSPARENT,
                           GIMP_OPACITY_OPAQUE,
                           GIMP_OPACITY_OPAQUE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, GIMP_CONTEXT_PROP_PAINT_MODE,
                         gimp_context_prop_names[GIMP_CONTEXT_PROP_PAINT_MODE],
                         _("Paint Mode"),
                         _("Paint Mode"),
                         GIMP_TYPE_LAYER_MODE,
                         GIMP_LAYER_MODE_NORMAL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_BRUSH,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_BRUSH],
                           _("Brush"),
                           _("Brush"),
                           GIMP_TYPE_BRUSH,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_DYNAMICS,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_DYNAMICS],
                           _("Dynamics"),
                           _("Paint dynamics"),
                           GIMP_TYPE_DYNAMICS,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_MYBRUSH,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_MYBRUSH],
                           _("MyPaint Brush"),
                           _("MyPaint Brush"),
                           GIMP_TYPE_MYBRUSH,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_PATTERN,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_PATTERN],
                           _("Pattern"),
                           _("Pattern"),
                           GIMP_TYPE_PATTERN,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_GRADIENT,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_GRADIENT],
                           _("Gradient"),
                           _("Gradient"),
                           GIMP_TYPE_GRADIENT,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_PALETTE,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_PALETTE],
                           _("Palette"),
                           _("Palette"),
                           GIMP_TYPE_PALETTE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_FONT,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_FONT],
                           _("Font"),
                           _("Font"),
                           GIMP_TYPE_FONT,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, GIMP_CONTEXT_PROP_TOOL_PRESET,
                           gimp_context_prop_names[GIMP_CONTEXT_PROP_TOOL_PRESET],
                           _("Tool Preset"),
                           _("Tool Preset"),
                           GIMP_TYPE_TOOL_PRESET,
                           GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, GIMP_CONTEXT_PROP_BUFFER,
                                   g_param_spec_object (gimp_context_prop_names[GIMP_CONTEXT_PROP_BUFFER],
                                                        NULL, NULL,
                                                        GIMP_TYPE_BUFFER,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, GIMP_CONTEXT_PROP_IMAGEFILE,
                                   g_param_spec_object (gimp_context_prop_names[GIMP_CONTEXT_PROP_IMAGEFILE],
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGEFILE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, GIMP_CONTEXT_PROP_TEMPLATE,
                                   g_param_spec_object (gimp_context_prop_names[GIMP_CONTEXT_PROP_TEMPLATE],
                                                        NULL, NULL,
                                                        GIMP_TYPE_TEMPLATE,
                                                        GIMP_PARAM_READWRITE));

  g_object_unref (black);
  g_object_unref (white);
}

static void
gimp_context_init (GimpContext *context)
{
  context->gimp            = NULL;

  context->parent          = NULL;

  context->defined_props   = GIMP_CONTEXT_PROP_MASK_ALL;
  context->serialize_props = GIMP_CONTEXT_PROP_MASK_ALL;

  context->image           = NULL;
  context->display         = NULL;

  context->tool_info       = NULL;
  context->tool_name       = NULL;

  context->paint_info      = NULL;
  context->paint_name      = NULL;

  context->brush           = NULL;
  context->brush_name      = NULL;

  context->dynamics        = NULL;
  context->dynamics_name   = NULL;

  context->mybrush         = NULL;
  context->mybrush_name    = NULL;

  context->pattern         = NULL;
  context->pattern_name    = NULL;

  context->gradient        = NULL;
  context->gradient_name   = NULL;

  context->palette         = NULL;
  context->palette_name    = NULL;

  context->font            = NULL;
  context->font_name       = NULL;

  context->tool_preset      = NULL;
  context->tool_preset_name = NULL;

  context->buffer          = NULL;
  context->buffer_name     = NULL;

  context->imagefile       = NULL;
  context->imagefile_name  = NULL;

  context->template        = NULL;
  context->template_name   = NULL;

  context->line_art            = NULL;
  context->line_art_timeout_id = 0;

  context->foreground      = gegl_color_new ("black");
  context->background      = gegl_color_new ("white");
}

static void
gimp_context_config_iface_init (GimpConfigInterface *iface)
{
  parent_config_iface = g_type_interface_peek_parent (iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (GIMP_TYPE_CONFIG);

  iface->serialize            = gimp_context_serialize;
  iface->deserialize          = gimp_context_deserialize;
  iface->serialize_property   = gimp_context_serialize_property;
  iface->deserialize_property = gimp_context_deserialize_property;
  iface->duplicate            = gimp_context_duplicate;
  iface->copy                 = gimp_context_copy;
}

static void
gimp_context_constructed (GObject *object)
{
  Gimp          *gimp;
  GimpContainer *container;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp = GIMP_CONTEXT (object)->gimp;

  gimp_assert (GIMP_IS_GIMP (gimp));

  gimp->context_list = g_list_prepend (gimp->context_list, object);

  g_signal_connect_object (gimp->images, "remove",
                           G_CALLBACK (gimp_context_image_removed),
                           object, 0);
  g_signal_connect_object (gimp->displays, "remove",
                           G_CALLBACK (gimp_context_display_removed),
                           object, 0);

  g_signal_connect_object (gimp->tool_info_list, "remove",
                           G_CALLBACK (gimp_context_tool_removed),
                           object, 0);
  g_signal_connect_object (gimp->tool_info_list, "thaw",
                           G_CALLBACK (gimp_context_tool_list_thaw),
                           object, 0);

  g_signal_connect_object (gimp->paint_info_list, "remove",
                           G_CALLBACK (gimp_context_paint_info_removed),
                           object, 0);
  g_signal_connect_object (gimp->paint_info_list, "thaw",
                           G_CALLBACK (gimp_context_paint_info_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->brush_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_brush_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_brush_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->dynamics_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_dynamics_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_dynamics_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->mybrush_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_mybrush_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_mybrush_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->pattern_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_pattern_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_pattern_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->gradient_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_gradient_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_gradient_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->palette_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_palette_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_palette_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->font_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_font_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_font_list_thaw),
                           object, 0);

  container = gimp_data_factory_get_container (gimp->tool_preset_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_context_tool_preset_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_context_tool_preset_list_thaw),
                           object, 0);

  g_signal_connect_object (gimp->named_buffers, "remove",
                           G_CALLBACK (gimp_context_buffer_removed),
                           object, 0);
  g_signal_connect_object (gimp->named_buffers, "thaw",
                           G_CALLBACK (gimp_context_buffer_list_thaw),
                           object, 0);

  g_signal_connect_object (gimp->documents, "remove",
                           G_CALLBACK (gimp_context_imagefile_removed),
                           object, 0);
  g_signal_connect_object (gimp->documents, "thaw",
                           G_CALLBACK (gimp_context_imagefile_list_thaw),
                           object, 0);

  g_signal_connect_object (gimp->templates, "remove",
                           G_CALLBACK (gimp_context_template_removed),
                           object, 0);
  g_signal_connect_object (gimp->templates, "thaw",
                           G_CALLBACK (gimp_context_template_list_thaw),
                           object, 0);

  gimp_context_set_paint_info (GIMP_CONTEXT (object),
                               gimp_paint_info_get_standard (gimp));
}

static void
gimp_context_dispose (GObject *object)
{
  GimpContext *context = GIMP_CONTEXT (object);

  gimp_context_set_parent (context, NULL);

  if (context->gimp)
    {
      context->gimp->context_list = g_list_remove (context->gimp->context_list,
                                                   context);
      context->gimp = NULL;
    }

  g_clear_object (&context->tool_info);
  g_clear_object (&context->paint_info);
  g_clear_object (&context->brush);
  g_clear_object (&context->dynamics);
  g_clear_object (&context->mybrush);
  g_clear_object (&context->pattern);
  g_clear_object (&context->gradient);
  g_clear_object (&context->palette);
  g_clear_object (&context->font);
  g_clear_object (&context->tool_preset);
  g_clear_object (&context->buffer);
  g_clear_object (&context->imagefile);
  g_clear_object (&context->template);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_context_finalize (GObject *object)
{
  GimpContext *context = GIMP_CONTEXT (object);

  context->parent  = NULL;
  context->image   = NULL;
  context->display = NULL;

  g_clear_pointer (&context->tool_name,        g_free);
  g_clear_pointer (&context->paint_name,       g_free);
  g_clear_pointer (&context->brush_name,       g_free);
  g_clear_pointer (&context->dynamics_name,    g_free);
  g_clear_pointer (&context->mybrush_name,     g_free);
  g_clear_pointer (&context->pattern_name,     g_free);
  g_clear_pointer (&context->gradient_name,    g_free);
  g_clear_pointer (&context->palette_name,     g_free);
  g_clear_pointer (&context->font_name,        g_free);
  g_clear_pointer (&context->tool_preset_name, g_free);
  g_clear_pointer (&context->buffer_name,      g_free);
  g_clear_pointer (&context->imagefile_name,   g_free);
  g_clear_pointer (&context->template_name,    g_free);

  g_clear_object (&context->line_art);
  g_clear_object (&context->foreground);
  g_clear_object (&context->background);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_context_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpContext *context = GIMP_CONTEXT (object);

  switch (property_id)
    {
    case GIMP_CONTEXT_PROP_GIMP:
      context->gimp = g_value_get_object (value);
      break;
    case GIMP_CONTEXT_PROP_IMAGE:
      gimp_context_set_image (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_DISPLAY:
      gimp_context_set_display (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_TOOL:
      gimp_context_set_tool (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_PAINT_INFO:
      gimp_context_set_paint_info (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_FOREGROUND:
      gimp_context_set_foreground (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_BACKGROUND:
      gimp_context_set_background (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_OPACITY:
      gimp_context_set_opacity (context, g_value_get_double (value));
      break;
    case GIMP_CONTEXT_PROP_PAINT_MODE:
      gimp_context_set_paint_mode (context, g_value_get_enum (value));
      break;
    case GIMP_CONTEXT_PROP_BRUSH:
      gimp_context_set_brush (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_DYNAMICS:
      gimp_context_set_dynamics (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_MYBRUSH:
      gimp_context_set_mybrush (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_PATTERN:
      gimp_context_set_pattern (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_GRADIENT:
      gimp_context_set_gradient (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_PALETTE:
      gimp_context_set_palette (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_FONT:
      gimp_context_set_font (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_TOOL_PRESET:
      gimp_context_set_tool_preset (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_BUFFER:
      gimp_context_set_buffer (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_IMAGEFILE:
      gimp_context_set_imagefile (context, g_value_get_object (value));
      break;
    case GIMP_CONTEXT_PROP_TEMPLATE:
      gimp_context_set_template (context, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_context_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpContext *context = GIMP_CONTEXT (object);

  switch (property_id)
    {
    case GIMP_CONTEXT_PROP_GIMP:
      g_value_set_object (value, context->gimp);
      break;
    case GIMP_CONTEXT_PROP_IMAGE:
      g_value_set_object (value, gimp_context_get_image (context));
      break;
    case GIMP_CONTEXT_PROP_DISPLAY:
      g_value_set_object (value, gimp_context_get_display (context));
      break;
    case GIMP_CONTEXT_PROP_TOOL:
      g_value_set_object (value, gimp_context_get_tool (context));
      break;
    case GIMP_CONTEXT_PROP_PAINT_INFO:
      g_value_set_object (value, gimp_context_get_paint_info (context));
      break;
    case GIMP_CONTEXT_PROP_FOREGROUND:
      g_value_take_object (value, gegl_color_duplicate (gimp_context_get_foreground (context)));
      break;
    case GIMP_CONTEXT_PROP_BACKGROUND:
      g_value_take_object (value, gegl_color_duplicate (gimp_context_get_background (context)));
      break;
    case GIMP_CONTEXT_PROP_OPACITY:
      g_value_set_double (value, gimp_context_get_opacity (context));
      break;
    case GIMP_CONTEXT_PROP_PAINT_MODE:
      g_value_set_enum (value, gimp_context_get_paint_mode (context));
      break;
    case GIMP_CONTEXT_PROP_BRUSH:
      g_value_set_object (value, gimp_context_get_brush (context));
      break;
    case GIMP_CONTEXT_PROP_DYNAMICS:
      g_value_set_object (value, gimp_context_get_dynamics (context));
      break;
    case GIMP_CONTEXT_PROP_MYBRUSH:
      g_value_set_object (value, gimp_context_get_mybrush (context));
      break;
    case GIMP_CONTEXT_PROP_PATTERN:
      g_value_set_object (value, gimp_context_get_pattern (context));
      break;
    case GIMP_CONTEXT_PROP_GRADIENT:
      g_value_set_object (value, gimp_context_get_gradient (context));
      break;
    case GIMP_CONTEXT_PROP_PALETTE:
      g_value_set_object (value, gimp_context_get_palette (context));
      break;
    case GIMP_CONTEXT_PROP_FONT:
      g_value_set_object (value, gimp_context_get_font (context));
      break;
    case GIMP_CONTEXT_PROP_TOOL_PRESET:
      g_value_set_object (value, gimp_context_get_tool_preset (context));
      break;
    case GIMP_CONTEXT_PROP_BUFFER:
      g_value_set_object (value, gimp_context_get_buffer (context));
      break;
    case GIMP_CONTEXT_PROP_IMAGEFILE:
      g_value_set_object (value, gimp_context_get_imagefile (context));
      break;
    case GIMP_CONTEXT_PROP_TEMPLATE:
      g_value_set_object (value, gimp_context_get_template (context));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_context_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpContext *context = GIMP_CONTEXT (object);
  gint64       memsize = 0;

  memsize += gimp_string_get_memsize (context->tool_name);
  memsize += gimp_string_get_memsize (context->paint_name);
  memsize += gimp_string_get_memsize (context->brush_name);
  memsize += gimp_string_get_memsize (context->dynamics_name);
  memsize += gimp_string_get_memsize (context->mybrush_name);
  memsize += gimp_string_get_memsize (context->pattern_name);
  memsize += gimp_string_get_memsize (context->palette_name);
  memsize += gimp_string_get_memsize (context->font_name);
  memsize += gimp_string_get_memsize (context->tool_preset_name);
  memsize += gimp_string_get_memsize (context->buffer_name);
  memsize += gimp_string_get_memsize (context->imagefile_name);
  memsize += gimp_string_get_memsize (context->template_name);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_context_serialize (GimpConfig       *config,
                        GimpConfigWriter *writer,
                        gpointer          data)
{
  return gimp_config_serialize_changed_properties (config, writer);
}

static gboolean
gimp_context_deserialize (GimpConfig *config,
                          GScanner   *scanner,
                          gint        nest_level,
                          gpointer    data)
{
  GimpContext   *context        = GIMP_CONTEXT (config);
  GimpLayerMode  old_paint_mode = context->paint_mode;
  gboolean       success;

  success = gimp_config_deserialize_properties (config, scanner, nest_level);

  if (context->paint_mode != old_paint_mode)
    {
      if (context->paint_mode == GIMP_LAYER_MODE_OVERLAY_LEGACY)
        g_object_set (context,
                      "paint-mode", GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,
                      NULL);
    }

  return success;
}

static gboolean
gimp_context_serialize_property (GimpConfig       *config,
                                 guint             property_id,
                                 const GValue     *value,
                                 GParamSpec       *pspec,
                                 GimpConfigWriter *writer)
{
  GimpContext *context = GIMP_CONTEXT (config);
  GimpObject  *serialize_obj;

  /*  serialize nothing if the property is not in serialize_props  */
  if (! ((1 << property_id) & context->serialize_props))
    return TRUE;

  switch (property_id)
    {
    case GIMP_CONTEXT_PROP_TOOL:
    case GIMP_CONTEXT_PROP_PAINT_INFO:
    case GIMP_CONTEXT_PROP_BRUSH:
    case GIMP_CONTEXT_PROP_DYNAMICS:
    case GIMP_CONTEXT_PROP_MYBRUSH:
    case GIMP_CONTEXT_PROP_PATTERN:
    case GIMP_CONTEXT_PROP_GRADIENT:
    case GIMP_CONTEXT_PROP_PALETTE:
    case GIMP_CONTEXT_PROP_FONT:
    case GIMP_CONTEXT_PROP_TOOL_PRESET:
      serialize_obj = g_value_get_object (value);
      break;

    default:
      return FALSE;
    }

  gimp_config_writer_open (writer, pspec->name);

  if (serialize_obj)
    gimp_config_writer_string (writer, gimp_object_get_name (serialize_obj));
  else
    gimp_config_writer_print (writer, "NULL", 4);

  gimp_config_writer_close (writer);

  return TRUE;
}

static gboolean
gimp_context_deserialize_property (GimpConfig *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec,
                                   GScanner   *scanner,
                                   GTokenType *expected)
{
  GimpContext   *context = GIMP_CONTEXT (object);
  GimpContainer *container;
  gpointer       standard;
  gchar        **name_loc;
  gchar         *object_name;

  switch (property_id)
    {
    case GIMP_CONTEXT_PROP_TOOL:
      container = context->gimp->tool_info_list;
      standard  = gimp_tool_info_get_standard (context->gimp);
      name_loc  = &context->tool_name;
      break;

    case GIMP_CONTEXT_PROP_PAINT_INFO:
      container = context->gimp->paint_info_list;
      standard  = gimp_paint_info_get_standard (context->gimp);
      name_loc  = &context->paint_name;
      break;

    case GIMP_CONTEXT_PROP_BRUSH:
      container = gimp_data_factory_get_container (context->gimp->brush_factory);
      standard  = gimp_brush_get_standard (context);
      name_loc  = &context->brush_name;
      break;

    case GIMP_CONTEXT_PROP_DYNAMICS:
      container = gimp_data_factory_get_container (context->gimp->dynamics_factory);
      standard  = gimp_dynamics_get_standard (context);
      name_loc  = &context->dynamics_name;
      break;

    case GIMP_CONTEXT_PROP_MYBRUSH:
      container = gimp_data_factory_get_container (context->gimp->mybrush_factory);
      standard  = gimp_mybrush_get_standard (context);
      name_loc  = &context->mybrush_name;
      break;

    case GIMP_CONTEXT_PROP_PATTERN:
      container = gimp_data_factory_get_container (context->gimp->pattern_factory);
      standard  = gimp_pattern_get_standard (context);
      name_loc  = &context->pattern_name;
      break;

    case GIMP_CONTEXT_PROP_GRADIENT:
      container = gimp_data_factory_get_container (context->gimp->gradient_factory);
      standard  = gimp_gradient_get_standard (context);
      name_loc  = &context->gradient_name;
      break;

    case GIMP_CONTEXT_PROP_PALETTE:
      container = gimp_data_factory_get_container (context->gimp->palette_factory);
      standard  = gimp_palette_get_standard (context);
      name_loc  = &context->palette_name;
      break;

    case GIMP_CONTEXT_PROP_FONT:
      container = gimp_data_factory_get_container (context->gimp->font_factory);
      standard  = gimp_font_get_standard ();
      name_loc  = &context->font_name;
      break;

    case GIMP_CONTEXT_PROP_TOOL_PRESET:
      container = gimp_data_factory_get_container (context->gimp->tool_preset_factory);
      standard  = NULL;
      name_loc  = &context->tool_preset_name;
      break;

    default:
      return FALSE;
    }

  if (gimp_scanner_parse_identifier (scanner, "NULL"))
    {
      g_value_set_object (value, NULL);
    }
  else if (gimp_scanner_parse_string (scanner, &object_name))
    {
      GimpObject *deserialize_obj;

      if (! object_name)
        object_name = g_strdup ("");

      deserialize_obj = gimp_container_get_child_by_name (container,
                                                          object_name);

      if (! deserialize_obj)
        {
          g_value_set_object (value, standard);

          g_free (*name_loc);
          *name_loc = g_strdup (object_name);
        }
      else
        {
          g_value_set_object (value, deserialize_obj);
        }

      g_free (object_name);
    }
  else
    {
      *expected = G_TOKEN_STRING;
    }

  return TRUE;
}

static GimpConfig *
gimp_context_duplicate (GimpConfig *config)
{
  GimpContext *context = GIMP_CONTEXT (config);
  GimpContext *new;

  new = GIMP_CONTEXT (parent_config_iface->duplicate (config));

  COPY_NAME (context, new, tool_name);
  COPY_NAME (context, new, paint_name);
  COPY_NAME (context, new, brush_name);
  COPY_NAME (context, new, dynamics_name);
  COPY_NAME (context, new, mybrush_name);
  COPY_NAME (context, new, pattern_name);
  COPY_NAME (context, new, gradient_name);
  COPY_NAME (context, new, palette_name);
  COPY_NAME (context, new, font_name);
  COPY_NAME (context, new, tool_preset_name);
  COPY_NAME (context, new, buffer_name);
  COPY_NAME (context, new, imagefile_name);
  COPY_NAME (context, new, template_name);

  return GIMP_CONFIG (new);
}

static gboolean
gimp_context_copy (GimpConfig  *src,
                   GimpConfig  *dest,
                   GParamFlags  flags)
{
  GimpContext *src_context  = GIMP_CONTEXT (src);
  GimpContext *dest_context = GIMP_CONTEXT (dest);
  gboolean     success      = parent_config_iface->copy (src, dest, flags);

  COPY_NAME (src_context, dest_context, tool_name);
  COPY_NAME (src_context, dest_context, paint_name);
  COPY_NAME (src_context, dest_context, brush_name);
  COPY_NAME (src_context, dest_context, dynamics_name);
  COPY_NAME (src_context, dest_context, mybrush_name);
  COPY_NAME (src_context, dest_context, pattern_name);
  COPY_NAME (src_context, dest_context, gradient_name);
  COPY_NAME (src_context, dest_context, palette_name);
  COPY_NAME (src_context, dest_context, font_name);
  COPY_NAME (src_context, dest_context, tool_preset_name);
  COPY_NAME (src_context, dest_context, buffer_name);
  COPY_NAME (src_context, dest_context, imagefile_name);
  COPY_NAME (src_context, dest_context, template_name);

  return success;
}


/*****************************************************************************/
/*  public functions  ********************************************************/

GimpContext *
gimp_context_new (Gimp        *gimp,
                  const gchar *name,
                  GimpContext *template)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (template == NULL || GIMP_IS_CONTEXT (template), NULL);

  context = g_object_new (GIMP_TYPE_CONTEXT,
                          "name", name,
                          "gimp", gimp,
                          NULL);

  if (template)
    {
      context->defined_props = template->defined_props;

      gimp_context_copy_properties (template, context,
                                    GIMP_CONTEXT_PROP_MASK_ALL);
    }

  return context;
}

GimpContext *
gimp_context_get_parent (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->parent;
}

static void
gimp_context_parent_notify (GimpContext *parent,
                            GParamSpec  *pspec,
                            GimpContext *context)
{
  if (pspec->owner_type == GIMP_TYPE_CONTEXT)
    {
      GimpContextPropType prop = pspec->param_id;

      /*  copy from parent if the changed property is undefined;
       *  ignore properties that are not context properties, for
       *  example notifications on the context's "gimp" property
       */
      if ((prop >= GIMP_CONTEXT_PROP_FIRST) &&
          (prop <= GIMP_CONTEXT_PROP_LAST)  &&
          ! ((1 << prop) & context->defined_props))
        {
          gimp_context_copy_property (parent, context, prop);
        }
    }
}

void
gimp_context_set_parent (GimpContext *context,
                         GimpContext *parent)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (parent == NULL || GIMP_IS_CONTEXT (parent));
  g_return_if_fail (parent == NULL || parent->parent != context);
  g_return_if_fail (context != parent);

  if (context->parent == parent)
    return;

  if (context->parent)
    {
      g_signal_handlers_disconnect_by_func (context->parent,
                                            gimp_context_parent_notify,
                                            context);
    }

  g_set_weak_pointer (&context->parent, parent);

  if (parent)
    {
      /*  copy all undefined properties from the new parent  */
      gimp_context_copy_properties (parent, context,
                                    ~context->defined_props &
                                    GIMP_CONTEXT_PROP_MASK_ALL);

      g_signal_connect_object (parent, "notify",
                               G_CALLBACK (gimp_context_parent_notify),
                               context,
                               0);
    }
}


/*  define / undefinine context properties  */

void
gimp_context_define_property (GimpContext         *context,
                              GimpContextPropType  prop,
                              gboolean             defined)
{
  GimpContextPropMask mask;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((prop >= GIMP_CONTEXT_PROP_FIRST) &&
                    (prop <= GIMP_CONTEXT_PROP_LAST));

  mask = (1 << prop);

  if (defined)
    {
      if (! (context->defined_props & mask))
        {
          context->defined_props |= mask;
        }
    }
  else
    {
      if (context->defined_props & mask)
        {
          context->defined_props &= ~mask;

          if (context->parent)
            gimp_context_copy_property (context->parent, context, prop);
        }
    }
}

gboolean
gimp_context_property_defined (GimpContext         *context,
                               GimpContextPropType  prop)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  return (context->defined_props & (1 << prop)) ? TRUE : FALSE;
}

void
gimp_context_define_properties (GimpContext         *context,
                                GimpContextPropMask  prop_mask,
                                gboolean             defined)
{
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  for (prop = GIMP_CONTEXT_PROP_FIRST; prop <= GIMP_CONTEXT_PROP_LAST; prop++)
    if ((1 << prop) & prop_mask)
      gimp_context_define_property (context, prop, defined);
}


/*  specify which context properties will be serialized  */

void
gimp_context_set_serialize_properties (GimpContext         *context,
                                       GimpContextPropMask  props_mask)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  context->serialize_props = props_mask;
}

GimpContextPropMask
gimp_context_get_serialize_properties (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), 0);

  return context->serialize_props;
}


/*  copying context properties  */

void
gimp_context_copy_property (GimpContext         *src,
                            GimpContext         *dest,
                            GimpContextPropType  prop)
{
  g_return_if_fail (GIMP_IS_CONTEXT (src));
  g_return_if_fail (GIMP_IS_CONTEXT (dest));
  g_return_if_fail ((prop >= GIMP_CONTEXT_PROP_FIRST) &&
                    (prop <= GIMP_CONTEXT_PROP_LAST));

  switch (prop)
    {
    case GIMP_CONTEXT_PROP_IMAGE:
      gimp_context_real_set_image (dest, src->image);
      break;

    case GIMP_CONTEXT_PROP_DISPLAY:
      gimp_context_real_set_display (dest, src->display);
      break;

    case GIMP_CONTEXT_PROP_TOOL:
      gimp_context_real_set_tool (dest, src->tool_info);
      COPY_NAME (src, dest, tool_name);
      break;

    case GIMP_CONTEXT_PROP_PAINT_INFO:
      gimp_context_real_set_paint_info (dest, src->paint_info);
      COPY_NAME (src, dest, paint_name);
      break;

    case GIMP_CONTEXT_PROP_FOREGROUND:
      gimp_context_real_set_foreground (dest, src->foreground);
      break;

    case GIMP_CONTEXT_PROP_BACKGROUND:
      gimp_context_real_set_background (dest, src->background);
      break;

    case GIMP_CONTEXT_PROP_OPACITY:
      gimp_context_real_set_opacity (dest, src->opacity);
      break;

    case GIMP_CONTEXT_PROP_PAINT_MODE:
      gimp_context_real_set_paint_mode (dest, src->paint_mode);
      break;

    case GIMP_CONTEXT_PROP_BRUSH:
      gimp_context_real_set_brush (dest, src->brush);
      COPY_NAME (src, dest, brush_name);
      break;

    case GIMP_CONTEXT_PROP_DYNAMICS:
      gimp_context_real_set_dynamics (dest, src->dynamics);
      COPY_NAME (src, dest, dynamics_name);
      break;

    case GIMP_CONTEXT_PROP_MYBRUSH:
      gimp_context_real_set_mybrush (dest, src->mybrush);
      COPY_NAME (src, dest, mybrush_name);
      break;

    case GIMP_CONTEXT_PROP_PATTERN:
      gimp_context_real_set_pattern (dest, src->pattern);
      COPY_NAME (src, dest, pattern_name);
      break;

    case GIMP_CONTEXT_PROP_GRADIENT:
      gimp_context_real_set_gradient (dest, src->gradient);
      COPY_NAME (src, dest, gradient_name);
      break;

    case GIMP_CONTEXT_PROP_PALETTE:
      gimp_context_real_set_palette (dest, src->palette);
      COPY_NAME (src, dest, palette_name);
      break;

    case GIMP_CONTEXT_PROP_FONT:
      gimp_context_real_set_font (dest, src->font);
      COPY_NAME (src, dest, font_name);
      break;

    case GIMP_CONTEXT_PROP_TOOL_PRESET:
      gimp_context_real_set_tool_preset (dest, src->tool_preset);
      COPY_NAME (src, dest, tool_preset_name);
      break;

    case GIMP_CONTEXT_PROP_BUFFER:
      gimp_context_real_set_buffer (dest, src->buffer);
      COPY_NAME (src, dest, buffer_name);
      break;

    case GIMP_CONTEXT_PROP_IMAGEFILE:
      gimp_context_real_set_imagefile (dest, src->imagefile);
      COPY_NAME (src, dest, imagefile_name);
      break;

    case GIMP_CONTEXT_PROP_TEMPLATE:
      gimp_context_real_set_template (dest, src->template);
      COPY_NAME (src, dest, template_name);
      break;

    default:
      break;
    }
}

void
gimp_context_copy_properties (GimpContext         *src,
                              GimpContext         *dest,
                              GimpContextPropMask  prop_mask)
{
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (src));
  g_return_if_fail (GIMP_IS_CONTEXT (dest));

  for (prop = GIMP_CONTEXT_PROP_FIRST; prop <= GIMP_CONTEXT_PROP_LAST; prop++)
    if ((1 << prop) & prop_mask)
      gimp_context_copy_property (src, dest, prop);
}


/*  attribute access functions  */

/*****************************************************************************/
/*  manipulate by GType  *****************************************************/

GimpContextPropType
gimp_context_type_to_property (GType type)
{
  GimpContextPropType prop;

  for (prop = GIMP_CONTEXT_PROP_FIRST; prop <= GIMP_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, gimp_context_prop_types[prop]))
        return prop;
    }

  return -1;
}

const gchar *
gimp_context_type_to_prop_name (GType type)
{
  GimpContextPropType prop;

  for (prop = GIMP_CONTEXT_PROP_FIRST; prop <= GIMP_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, gimp_context_prop_types[prop]))
        return gimp_context_prop_names[prop];
    }

  return NULL;
}

const gchar *
gimp_context_type_to_signal_name (GType type)
{
  GimpContextPropType prop;

  for (prop = GIMP_CONTEXT_PROP_FIRST; prop <= GIMP_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, gimp_context_prop_types[prop]))
        return g_signal_name (gimp_context_signals[prop]);
    }

  return NULL;
}

GimpObject *
gimp_context_get_by_type (GimpContext *context,
                          GType        type)
{
  GimpContextPropType  prop;
  GimpObject          *object = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  prop = gimp_context_type_to_property (type);

  g_return_val_if_fail (prop != -1, NULL);

  g_object_get (context,
                gimp_context_prop_names[prop], &object,
                NULL);

  /*  g_object_get() refs the object, this function however is a getter,
   *  which usually doesn't ref it's return value
   */
  if (object)
    g_object_unref (object);

  return object;
}

void
gimp_context_set_by_type (GimpContext *context,
                          GType        type,
                          GimpObject  *object)
{
  GimpContextPropType  prop;
  GParamSpec          *pspec;
  GValue               value = G_VALUE_INIT;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (object == NULL || G_IS_OBJECT (object));

  prop = gimp_context_type_to_property (type);

  g_return_if_fail (prop != -1);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (context),
                                        gimp_context_prop_names[prop]);

  g_return_if_fail (pspec != NULL);

  g_value_init (&value, pspec->value_type);
  g_value_set_object (&value, object);

  /*  we use gimp_context_set_property() (which in turn only calls
   *  gimp_context_set_foo() functions) instead of the much more obvious
   *  g_object_set(); this avoids g_object_freeze_notify()/thaw_notify()
   *  around the g_object_set() and makes GimpContext callbacks being
   *  called in a much more predictable order. See bug #731279.
   */
  gimp_context_set_property (G_OBJECT (context),
                             pspec->param_id,
                             (const GValue *) &value,
                             pspec);

  g_value_unset (&value);
}

void
gimp_context_changed_by_type (GimpContext *context,
                              GType        type)
{
  GimpContextPropType  prop;
  GimpObject          *object;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  prop = gimp_context_type_to_property (type);

  g_return_if_fail (prop != -1);

  object = gimp_context_get_by_type (context, type);

  g_signal_emit (context,
                 gimp_context_signals[prop], 0,
                 object);
}


/*****************************************************************************/
/*  image  *******************************************************************/

GimpImage *
gimp_context_get_image (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->image;
}

void
gimp_context_set_image (GimpContext *context,
                        GimpImage   *image)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  context_find_defined (context, GIMP_CONTEXT_PROP_IMAGE);

  gimp_context_real_set_image (context, image);
}

void
gimp_context_image_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[IMAGE_CHANGED], 0,
                 context->image);
}

/* This is a utility function to share, across the program, some common logic of
 * "which format to use when you are not sure and you need to give generic color
 * information" in RGBA.
 * @babl_type must be a valid babl type name such as "u8", "double", "float".
 * If @space_image is not NULL, it will be used to return the GimpImage
 * associated to the returned format (if the returned format is indeed using the
 * image's space).
 *
 * The logic for the format to use in RGB color actions is as follows:
 * - The space we navigate through is the active image's space.
 * - Increasing/decreasing follows the image TRC (in particular, if the image is
 *   linear or perceptual, we care about chromaticities yet don't follow the
 *   space TRC).
 * - If there is no active image or if its space is non-sRGB, we use the context
 *   color's space (if set).
 * - We discard non-RGB spaces and fallback to sRGB.
 */
const Babl *
gimp_context_get_rgba_format (GimpContext  *context,
                              GeglColor    *color,
                              const gchar  *babl_type,
                              GimpImage   **space_image)
{
  GimpImage            *image  = NULL;
  const Babl           *format = NULL;
  const Babl           *space  = NULL;
  gchar                *format_name;
  GimpTRCType           trc = GIMP_TRC_NON_LINEAR;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (babl_type != NULL , NULL);
  g_return_val_if_fail (space_image == NULL || *space_image == NULL, NULL);

  image = gimp_context_get_image (context);
  if (image)
    {
      format = gimp_image_get_layer_format (image, FALSE);
      space  = babl_format_get_space (format);
      if (space_image)
        *space_image = image;
    }

  if (color != NULL && (space == NULL || ! babl_space_is_rgb (space)))
    {
      format = gegl_color_get_format (color);
      space  = babl_format_get_space (format);
      if (space_image)
        *space_image = NULL;
    }

  if (! babl_space_is_rgb (space))
    {
      format = NULL;
      space  = NULL;
      if (space_image)
        *space_image = NULL;
    }

  if (format != NULL)
    {
      if (image != NULL)
        {
          GimpPrecision precision;

          precision = gimp_image_get_precision (image);
          trc       = gimp_babl_trc (precision);
        }
      else
        {
          trc = gimp_babl_format_get_trc (format);
        }
    }

  switch (trc)
    {
    case GIMP_TRC_LINEAR:
      format_name = g_strdup_printf ("RGBA %s", babl_type);
      break;
    case GIMP_TRC_NON_LINEAR:
      format_name = g_strdup_printf ("R'G'B'A %s", babl_type);
      break;
    case GIMP_TRC_PERCEPTUAL:
      format_name = g_strdup_printf ("R~G~B~A %s", babl_type);
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  format = babl_format_with_space (format_name, space);
  g_free (format_name);

  return format;
}

static void
gimp_context_image_disconnect (GimpImage   *image,
                               GimpContext *context)
{
  if (image == context->image)
    gimp_context_real_set_image (context, NULL);
}

static void
gimp_context_image_removed (GimpContainer *container,
                            GimpImage     *image,
                            GimpContext   *context)
{
  if (context->image == image)
    gimp_context_real_set_image (context, NULL);
}

static void
gimp_context_real_set_image (GimpContext *context,
                             GimpImage   *image)
{
  if (context->image == image)
    return;

  if (context->image)
    g_signal_handlers_disconnect_by_func (context->image,
                                          G_CALLBACK (gimp_context_image_disconnect),
                                          context);

  context->image = image;

  if (image)
    g_signal_connect_object (image, "disconnect",
                             G_CALLBACK (gimp_context_image_disconnect),
                             context, 0);

  g_object_notify (G_OBJECT (context), "image");
  gimp_context_image_changed (context);
}


/*****************************************************************************/
/*  display  *****************************************************************/

GimpDisplay *
gimp_context_get_display (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->display;
}

void
gimp_context_set_display (GimpContext *context,
                          GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (display == NULL || GIMP_IS_DISPLAY (display));

  context_find_defined (context, GIMP_CONTEXT_PROP_DISPLAY);

  gimp_context_real_set_display (context, display);
}

void
gimp_context_display_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[DISPLAY_CHANGED], 0,
                 context->display);
}

static void
gimp_context_display_removed (GimpContainer *container,
                              GimpDisplay   *display,
                              GimpContext   *context)
{
  if (context->display == display)
    gimp_context_real_set_display (context, NULL);
}

static void
gimp_context_real_set_display (GimpContext *context,
                               GimpDisplay *display)
{
  GimpDisplay *old_display;

  if (context->display == display)
    {
      /*  make sure that setting a display *always* sets the image
       *  to that display's image, even if the display already
       *  matches
       */
      if (display)
        {
          GimpImage *image;

          g_object_get (display, "image", &image, NULL);

          gimp_context_real_set_image (context, image);

          if (image)
            g_object_unref (image);
        }

      return;
    }

  old_display = context->display;

  context->display = display;

  if (context->display)
    {
      GimpImage *image;

      g_object_get (display, "image", &image, NULL);

      gimp_context_real_set_image (context, image);

      if (image)
        g_object_unref (image);
    }
  else if (old_display)
    {
      gimp_context_real_set_image (context, NULL);
    }

  g_object_notify (G_OBJECT (context), "display");
  gimp_context_display_changed (context);
}


/*****************************************************************************/
/*  tool  ********************************************************************/

GimpToolInfo *
gimp_context_get_tool (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->tool_info;
}

void
gimp_context_set_tool (GimpContext  *context,
                       GimpToolInfo *tool_info)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (tool_info == NULL || GIMP_IS_TOOL_INFO (tool_info));

  context_find_defined (context, GIMP_CONTEXT_PROP_TOOL);

  gimp_context_real_set_tool (context, tool_info);
}

void
gimp_context_tool_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[TOOL_CHANGED], 0,
                 context->tool_info);
}

static void
gimp_context_tool_dirty (GimpToolInfo *tool_info,
                         GimpContext  *context)
{
  g_free (context->tool_name);
  context->tool_name = g_strdup (gimp_object_get_name (tool_info));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_TOOL);
}

static void
gimp_context_tool_list_thaw (GimpContainer *container,
                             GimpContext   *context)
{
  GimpToolInfo *tool_info;

  if (! context->tool_name)
    context->tool_name = g_strdup ("gimp-paintbrush-tool");

  tool_info = gimp_context_find_object (context, container,
                                        context->tool_name,
                                        gimp_tool_info_get_standard (context->gimp));

  gimp_context_real_set_tool (context, tool_info);
}

static void
gimp_context_tool_removed (GimpContainer *container,
                           GimpToolInfo  *tool_info,
                           GimpContext   *context)
{
  if (tool_info == context->tool_info)
    {
      g_signal_handlers_disconnect_by_func (context->tool_info,
                                            gimp_context_tool_dirty,
                                            context);
      g_clear_object (&context->tool_info);

      if (! gimp_container_frozen (container))
        gimp_context_tool_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_tool (GimpContext  *context,
                            GimpToolInfo *tool_info)
{
  if (context->tool_info == tool_info)
    return;

  if (context->tool_name &&
      tool_info != gimp_tool_info_get_standard (context->gimp))
    {
      g_clear_pointer (&context->tool_name, g_free);
    }

  if (context->tool_info)
    g_signal_handlers_disconnect_by_func (context->tool_info,
                                          gimp_context_tool_dirty,
                                          context);

  g_set_object (&context->tool_info, tool_info);

  if (tool_info)
    {
      g_signal_connect_object (tool_info, "name-changed",
                               G_CALLBACK (gimp_context_tool_dirty),
                               context,
                               0);

      if (tool_info != gimp_tool_info_get_standard (context->gimp))
        context->tool_name = g_strdup (gimp_object_get_name (tool_info));

      if (tool_info->paint_info)
        gimp_context_real_set_paint_info (context, tool_info->paint_info);
    }

  g_object_notify (G_OBJECT (context), "tool");
  gimp_context_tool_changed (context);
}


/*****************************************************************************/
/*  paint info  **************************************************************/

GimpPaintInfo *
gimp_context_get_paint_info (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->paint_info;
}

void
gimp_context_set_paint_info (GimpContext   *context,
                             GimpPaintInfo *paint_info)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (paint_info == NULL || GIMP_IS_PAINT_INFO (paint_info));

  context_find_defined (context, GIMP_CONTEXT_PROP_PAINT_INFO);

  gimp_context_real_set_paint_info (context, paint_info);
}

void
gimp_context_paint_info_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[PAINT_INFO_CHANGED], 0,
                 context->paint_info);
}

static void
gimp_context_paint_info_dirty (GimpPaintInfo *paint_info,
                               GimpContext   *context)
{
  g_set_str (&context->paint_name, gimp_object_get_name (paint_info));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_PAINT_INFO);
}

/*  the global paint info list is there again after refresh  */
static void
gimp_context_paint_info_list_thaw (GimpContainer *container,
                                   GimpContext   *context)
{
  GimpPaintInfo *paint_info;

  if (! context->paint_name)
    context->paint_name = g_strdup ("gimp-paintbrush");

  paint_info = gimp_context_find_object (context, container,
                                         context->paint_name,
                                         gimp_paint_info_get_standard (context->gimp));

  gimp_context_real_set_paint_info (context, paint_info);
}

static void
gimp_context_paint_info_removed (GimpContainer *container,
                                 GimpPaintInfo *paint_info,
                                 GimpContext   *context)
{
  if (paint_info == context->paint_info)
    {
      g_signal_handlers_disconnect_by_func (context->paint_info,
                                            gimp_context_paint_info_dirty,
                                            context);
      g_clear_object (&context->paint_info);

      if (! gimp_container_frozen (container))
        gimp_context_paint_info_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_paint_info (GimpContext   *context,
                                  GimpPaintInfo *paint_info)
{
  if (context->paint_info == paint_info)
    return;

  if (context->paint_name &&
      paint_info != gimp_paint_info_get_standard (context->gimp))
    {
      g_clear_pointer (&context->paint_name, g_free);
    }

  if (context->paint_info)
    g_signal_handlers_disconnect_by_func (context->paint_info,
                                          gimp_context_paint_info_dirty,
                                          context);

  g_set_object (&context->paint_info, paint_info);

  if (paint_info)
    {
      g_signal_connect_object (paint_info, "name-changed",
                               G_CALLBACK (gimp_context_paint_info_dirty),
                               context,
                               0);

      if (paint_info != gimp_paint_info_get_standard (context->gimp))
        context->paint_name = g_strdup (gimp_object_get_name (paint_info));
    }

  g_object_notify (G_OBJECT (context), "paint-info");
  gimp_context_paint_info_changed (context);
}


/*****************************************************************************/
/*  foreground color  ********************************************************/

GeglColor *
gimp_context_get_foreground (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->foreground;
}

void
gimp_context_set_foreground (GimpContext *context,
                             GeglColor   *color)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GEGL_IS_COLOR (color));

  context_find_defined (context, GIMP_CONTEXT_PROP_FOREGROUND);

  gimp_context_real_set_foreground (context, color);
}

void
gimp_context_foreground_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[FOREGROUND_CHANGED], 0,
                 context->foreground);
}

static void
gimp_context_real_set_foreground (GimpContext *context,
                                  GeglColor   *color)
{
  g_clear_object (&context->foreground);
  context->foreground = gegl_color_duplicate (color);
  gimp_color_set_alpha (context->foreground, GIMP_OPACITY_OPAQUE);

  g_object_notify (G_OBJECT (context), "foreground");
  gimp_context_foreground_changed (context);
}


/*****************************************************************************/
/*  background color  ********************************************************/

GeglColor *
gimp_context_get_background (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->background;
}

void
gimp_context_set_background (GimpContext *context,
                             GeglColor   *color)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GEGL_IS_COLOR (color));

  context_find_defined (context, GIMP_CONTEXT_PROP_BACKGROUND);

  gimp_context_real_set_background (context, color);
}

void
gimp_context_background_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[BACKGROUND_CHANGED], 0,
                 context->background);
}

static void
gimp_context_real_set_background (GimpContext *context,
                                  GeglColor   *color)
{
  g_clear_object (&context->background);
  context->background = gegl_color_duplicate (color);
  gimp_color_set_alpha (context->background, GIMP_OPACITY_OPAQUE);

  g_object_notify (G_OBJECT (context), "background");
  gimp_context_background_changed (context);
}


/*****************************************************************************/
/*  color utility functions  *************************************************/

void
gimp_context_set_default_colors (GimpContext *context)
{
  GimpContext *bg_context;
  GeglColor   *fg;
  GeglColor   *bg;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  bg_context = context;

  context_find_defined (context, GIMP_CONTEXT_PROP_FOREGROUND);
  context_find_defined (bg_context, GIMP_CONTEXT_PROP_BACKGROUND);

  fg = gegl_color_new ("black");
  bg = gegl_color_new ("white");

  gimp_context_real_set_foreground (context, fg);
  gimp_context_real_set_background (bg_context, bg);

  g_object_unref (fg);
  g_object_unref (bg);
}

void
gimp_context_swap_colors (GimpContext *context)
{
  GimpContext *bg_context;
  GeglColor   *fg;
  GeglColor   *bg;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  bg_context = context;

  context_find_defined (context, GIMP_CONTEXT_PROP_FOREGROUND);
  context_find_defined (bg_context, GIMP_CONTEXT_PROP_BACKGROUND);

  fg = g_object_ref (context->foreground);
  bg = g_object_ref (context->background);

  gimp_context_real_set_foreground (context, bg);
  gimp_context_real_set_background (bg_context, fg);

  g_object_unref (fg);
  g_object_unref (bg);
}


/*****************************************************************************/
/*  opacity  *****************************************************************/

gdouble
gimp_context_get_opacity (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), GIMP_OPACITY_OPAQUE);

  return context->opacity;
}

void
gimp_context_set_opacity (GimpContext *context,
                          gdouble      opacity)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  context_find_defined (context, GIMP_CONTEXT_PROP_OPACITY);

  gimp_context_real_set_opacity (context, opacity);
}

void
gimp_context_opacity_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[OPACITY_CHANGED], 0,
                 context->opacity);
}

static void
gimp_context_real_set_opacity (GimpContext *context,
                               gdouble      opacity)
{
  if (context->opacity == opacity)
    return;

  context->opacity = opacity;

  g_object_notify (G_OBJECT (context), "opacity");
  gimp_context_opacity_changed (context);
}


/*****************************************************************************/
/*  paint mode  **************************************************************/

GimpLayerMode
gimp_context_get_paint_mode (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), GIMP_LAYER_MODE_NORMAL);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext   *context,
                             GimpLayerMode  paint_mode)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  context_find_defined (context, GIMP_CONTEXT_PROP_PAINT_MODE);

  gimp_context_real_set_paint_mode (context, paint_mode);
}

void
gimp_context_paint_mode_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[PAINT_MODE_CHANGED], 0,
                 context->paint_mode);
}

static void
gimp_context_real_set_paint_mode (GimpContext   *context,
                                  GimpLayerMode  paint_mode)
{
  if (context->paint_mode == paint_mode)
    return;

  context->paint_mode = paint_mode;

  g_object_notify (G_OBJECT (context), "paint-mode");
  gimp_context_paint_mode_changed (context);
}


/*****************************************************************************/
/*  brush  *******************************************************************/

GimpBrush *
gimp_context_get_brush (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->brush;
}

void
gimp_context_set_brush (GimpContext *context,
                        GimpBrush   *brush)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (brush == NULL || GIMP_IS_BRUSH (brush));

  context_find_defined (context, GIMP_CONTEXT_PROP_BRUSH);

  gimp_context_real_set_brush (context, brush);
}

void
gimp_context_brush_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[BRUSH_CHANGED], 0,
                 context->brush);
}

static void
gimp_context_brush_dirty (GimpBrush   *brush,
                          GimpContext *context)
{
  g_set_str (&context->brush_name, gimp_object_get_name (brush));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_BRUSH);
}

static void
gimp_context_brush_list_thaw (GimpContainer *container,
                              GimpContext   *context)
{
  GimpBrush *brush;

  if (! context->brush_name)
    context->brush_name = g_strdup (context->gimp->config->default_brush);

  brush = gimp_context_find_object (context, container,
                                    context->brush_name,
                                    gimp_brush_get_standard (context));

  gimp_context_real_set_brush (context, brush);
}

/*  the active brush disappeared  */
static void
gimp_context_brush_removed (GimpContainer *container,
                            GimpBrush     *brush,
                            GimpContext   *context)
{
  if (brush == context->brush)
    {
      g_signal_handlers_disconnect_by_func (context->brush,
                                            gimp_context_brush_dirty,
                                            context);
      g_clear_object (&context->brush);

      if (! gimp_container_frozen (container))
        gimp_context_brush_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_brush (GimpContext *context,
                             GimpBrush   *brush)
{
  if (context->brush == brush)
    return;

  if (context->brush_name &&
      brush != GIMP_BRUSH (gimp_brush_get_standard (context)))
    {
      g_clear_pointer (&context->brush_name, g_free);
    }

  if (context->brush)
    g_signal_handlers_disconnect_by_func (context->brush,
                                          gimp_context_brush_dirty,
                                          context);

  g_set_object (&context->brush, brush);

  if (brush)
    {
      g_signal_connect_object (brush, "name-changed",
                               G_CALLBACK (gimp_context_brush_dirty),
                               context,
                               0);

      if (brush != GIMP_BRUSH (gimp_brush_get_standard (context)))
        context->brush_name = g_strdup (gimp_object_get_name (brush));
    }

  g_object_notify (G_OBJECT (context), "brush");
  gimp_context_brush_changed (context);
}


/*****************************************************************************/
/*  dynamics *****************************************************************/

GimpDynamics *
gimp_context_get_dynamics (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->dynamics;
}

void
gimp_context_set_dynamics (GimpContext  *context,
                           GimpDynamics *dynamics)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (dynamics == NULL || GIMP_IS_DYNAMICS (dynamics));

  context_find_defined (context, GIMP_CONTEXT_PROP_DYNAMICS);

  gimp_context_real_set_dynamics (context, dynamics);
}

void
gimp_context_dynamics_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[DYNAMICS_CHANGED], 0,
                 context->dynamics);
}

static void
gimp_context_dynamics_dirty (GimpDynamics *dynamics,
                             GimpContext  *context)
{
  g_set_str (&context->dynamics_name, gimp_object_get_name (dynamics));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_DYNAMICS);
}

static void
gimp_context_dynamics_removed (GimpContainer *container,
                               GimpDynamics  *dynamics,
                               GimpContext   *context)
{
  if (dynamics == context->dynamics)
    {
      g_signal_handlers_disconnect_by_func (context->dynamics,
                                            gimp_context_dynamics_dirty,
                                            context);
      g_clear_object (&context->dynamics);

      if (! gimp_container_frozen (container))
        gimp_context_dynamics_list_thaw (container, context);
    }
}

static void
gimp_context_dynamics_list_thaw (GimpContainer *container,
                                 GimpContext   *context)
{
  GimpDynamics *dynamics;

  if (! context->dynamics_name)
    context->dynamics_name = g_strdup (context->gimp->config->default_dynamics);

  dynamics = gimp_context_find_object (context, container,
                                       context->dynamics_name,
                                       gimp_dynamics_get_standard (context));

  gimp_context_real_set_dynamics (context, dynamics);
}

static void
gimp_context_real_set_dynamics (GimpContext  *context,
                                GimpDynamics *dynamics)
{
  if (context->dynamics == dynamics)
    return;

  if (context->dynamics_name &&
      dynamics != GIMP_DYNAMICS (gimp_dynamics_get_standard (context)))
    {
      g_clear_pointer (&context->dynamics_name, g_free);
    }

  if (context->dynamics)
    g_signal_handlers_disconnect_by_func (context->dynamics,
                                          gimp_context_dynamics_dirty,
                                          context);

  g_set_object (&context->dynamics, dynamics);

  if (dynamics)
    {
      g_signal_connect_object (dynamics, "name-changed",
                               G_CALLBACK (gimp_context_dynamics_dirty),
                               context,
                               0);

      if (dynamics != GIMP_DYNAMICS (gimp_dynamics_get_standard (context)))
        context->dynamics_name = g_strdup (gimp_object_get_name (dynamics));
    }

  g_object_notify (G_OBJECT (context), "dynamics");
  gimp_context_dynamics_changed (context);
}


/*****************************************************************************/
/*  mybrush  *****************************************************************/

GimpMybrush *
gimp_context_get_mybrush (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->mybrush;
}

void
gimp_context_set_mybrush (GimpContext *context,
                          GimpMybrush *brush)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (brush == NULL || GIMP_IS_MYBRUSH (brush));

  context_find_defined (context, GIMP_CONTEXT_PROP_MYBRUSH);

  gimp_context_real_set_mybrush (context, brush);
}

void
gimp_context_mybrush_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[MYBRUSH_CHANGED], 0,
                 context->mybrush);
}

static void
gimp_context_mybrush_dirty (GimpMybrush *brush,
                            GimpContext *context)
{
  g_set_str (&context->mybrush_name, gimp_object_get_name (brush));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_MYBRUSH);
}

static void
gimp_context_mybrush_list_thaw (GimpContainer *container,
                                GimpContext   *context)
{
  GimpMybrush *brush;

  if (! context->mybrush_name)
    context->mybrush_name = g_strdup (context->gimp->config->default_mypaint_brush);

  brush = gimp_context_find_object (context, container,
                                    context->mybrush_name,
                                    gimp_mybrush_get_standard (context));

  gimp_context_real_set_mybrush (context, brush);
}

static void
gimp_context_mybrush_removed (GimpContainer *container,
                              GimpMybrush   *brush,
                              GimpContext   *context)
{
  if (brush == context->mybrush)
    {
      g_signal_handlers_disconnect_by_func (context->mybrush,
                                            gimp_context_mybrush_dirty,
                                            context);
      g_clear_object (&context->mybrush);

      if (! gimp_container_frozen (container))
        gimp_context_mybrush_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_mybrush (GimpContext *context,
                               GimpMybrush *brush)
{
  if (context->mybrush == brush)
    return;

  if (context->mybrush_name &&
      brush != GIMP_MYBRUSH (gimp_mybrush_get_standard (context)))
    {
      g_clear_pointer (&context->mybrush_name, g_free);
    }

  if (context->mybrush)
    g_signal_handlers_disconnect_by_func (context->mybrush,
                                          gimp_context_mybrush_dirty,
                                          context);

  g_set_object (&context->mybrush, brush);

  if (brush)
    {
      g_signal_connect_object (brush, "name-changed",
                               G_CALLBACK (gimp_context_mybrush_dirty),
                               context,
                               0);

      if (brush != GIMP_MYBRUSH (gimp_mybrush_get_standard (context)))
        context->mybrush_name = g_strdup (gimp_object_get_name (brush));
    }

  g_object_notify (G_OBJECT (context), "mybrush");
  gimp_context_mybrush_changed (context);
}


/*****************************************************************************/
/*  pattern  *****************************************************************/

GimpPattern *
gimp_context_get_pattern (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->pattern;
}

void
gimp_context_set_pattern (GimpContext *context,
                          GimpPattern *pattern)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (pattern == NULL || GIMP_IS_PATTERN (pattern));

  context_find_defined (context, GIMP_CONTEXT_PROP_PATTERN);

  gimp_context_real_set_pattern (context, pattern);
}

void
gimp_context_pattern_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[PATTERN_CHANGED], 0,
                 context->pattern);
}

static void
gimp_context_pattern_dirty (GimpPattern *pattern,
                            GimpContext *context)
{
  g_set_str (&context->pattern_name, gimp_object_get_name (pattern));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_PATTERN);
}

static void
gimp_context_pattern_list_thaw (GimpContainer *container,
                                GimpContext   *context)
{
  GimpPattern *pattern;

  if (! context->pattern_name)
    context->pattern_name = g_strdup (context->gimp->config->default_pattern);

  pattern = gimp_context_find_object (context, container,
                                      context->pattern_name,
                                      gimp_pattern_get_standard (context));

  gimp_context_real_set_pattern (context, pattern);
}

static void
gimp_context_pattern_removed (GimpContainer *container,
                              GimpPattern   *pattern,
                              GimpContext   *context)
{
  if (pattern == context->pattern)
    {
      g_signal_handlers_disconnect_by_func (context->pattern,
                                            gimp_context_pattern_dirty,
                                            context);
      g_clear_object (&context->pattern);

      if (! gimp_container_frozen (container))
        gimp_context_pattern_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_pattern (GimpContext *context,
                               GimpPattern *pattern)
{
  if (context->pattern == pattern)
    return;

  if (context->pattern_name &&
      pattern != GIMP_PATTERN (gimp_pattern_get_standard (context)))
    {
      g_clear_pointer (&context->pattern_name, g_free);
    }

  if (context->pattern)
    g_signal_handlers_disconnect_by_func (context->pattern,
                                          gimp_context_pattern_dirty,
                                          context);

  g_set_object (&context->pattern, pattern);

  if (pattern)
    {
      g_signal_connect_object (pattern, "name-changed",
                               G_CALLBACK (gimp_context_pattern_dirty),
                               context,
                               0);

      if (pattern != GIMP_PATTERN (gimp_pattern_get_standard (context)))
        context->pattern_name = g_strdup (gimp_object_get_name (pattern));
    }

  g_object_notify (G_OBJECT (context), "pattern");
  gimp_context_pattern_changed (context);
}


/*****************************************************************************/
/*  gradient  ****************************************************************/

GimpGradient *
gimp_context_get_gradient (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->gradient;
}

void
gimp_context_set_gradient (GimpContext  *context,
                           GimpGradient *gradient)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gradient == NULL || GIMP_IS_GRADIENT (gradient));

  context_find_defined (context, GIMP_CONTEXT_PROP_GRADIENT);

  gimp_context_real_set_gradient (context, gradient);
}

void
gimp_context_gradient_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[GRADIENT_CHANGED], 0,
                 context->gradient);
}

static void
gimp_context_gradient_dirty (GimpGradient *gradient,
                             GimpContext  *context)
{
  g_set_str (&context->gradient_name, gimp_object_get_name (gradient));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_GRADIENT);
}

static void
gimp_context_gradient_list_thaw (GimpContainer *container,
                                 GimpContext   *context)
{
  GimpGradient *gradient;

  if (! context->gradient_name)
    context->gradient_name = g_strdup (context->gimp->config->default_gradient);

  gradient = gimp_context_find_object (context, container,
                                       context->gradient_name,
                                       gimp_gradient_get_standard (context));

  gimp_context_real_set_gradient (context, gradient);
}

static void
gimp_context_gradient_removed (GimpContainer *container,
                               GimpGradient  *gradient,
                               GimpContext   *context)
{
  if (gradient == context->gradient)
    {
      g_signal_handlers_disconnect_by_func (context->gradient,
                                            gimp_context_gradient_dirty,
                                            context);
      g_clear_object (&context->gradient);

      if (! gimp_container_frozen (container))
        gimp_context_gradient_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_gradient (GimpContext  *context,
                                GimpGradient *gradient)
{
  if (context->gradient == gradient)
    return;

  if (context->gradient_name &&
      gradient != GIMP_GRADIENT (gimp_gradient_get_standard (context)))
    {
      g_clear_pointer (&context->gradient_name, g_free);
    }

  if (context->gradient)
    g_signal_handlers_disconnect_by_func (context->gradient,
                                          gimp_context_gradient_dirty,
                                          context);

  g_set_object (&context->gradient, gradient);

  if (gradient)
    {
      g_signal_connect_object (gradient, "name-changed",
                               G_CALLBACK (gimp_context_gradient_dirty),
                               context,
                               0);

      if (gradient != GIMP_GRADIENT (gimp_gradient_get_standard (context)))
        context->gradient_name = g_strdup (gimp_object_get_name (gradient));
    }

  g_object_notify (G_OBJECT (context), "gradient");
  gimp_context_gradient_changed (context);
}


/*****************************************************************************/
/*  palette  *****************************************************************/

GimpPalette *
gimp_context_get_palette (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->palette;
}

void
gimp_context_set_palette (GimpContext *context,
                          GimpPalette *palette)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (palette == NULL || GIMP_IS_PALETTE (palette));

  context_find_defined (context, GIMP_CONTEXT_PROP_PALETTE);

  gimp_context_real_set_palette (context, palette);
}

void
gimp_context_palette_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[PALETTE_CHANGED], 0,
                 context->palette);
}

static void
gimp_context_palette_dirty (GimpPalette *palette,
                            GimpContext *context)
{
  g_set_str (&context->palette_name, gimp_object_get_name (palette));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_PALETTE);
}

static void
gimp_context_palette_list_thaw (GimpContainer *container,
                                GimpContext   *context)
{
  GimpPalette *palette;

  if (! context->palette_name)
    context->palette_name = g_strdup (context->gimp->config->default_palette);

  palette = gimp_context_find_object (context, container,
                                      context->palette_name,
                                      gimp_palette_get_standard (context));

  gimp_context_real_set_palette (context, palette);
}

static void
gimp_context_palette_removed (GimpContainer *container,
                              GimpPalette   *palette,
                              GimpContext   *context)
{
  if (palette == context->palette)
    {
      g_signal_handlers_disconnect_by_func (context->palette,
                                            gimp_context_palette_dirty,
                                            context);
      g_clear_object (&context->palette);

      if (! gimp_container_frozen (container))
        gimp_context_palette_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_palette (GimpContext *context,
                               GimpPalette *palette)
{
  if (context->palette == palette)
    return;

  if (context->palette_name &&
      palette != GIMP_PALETTE (gimp_palette_get_standard (context)))
    {
      g_clear_pointer (&context->palette_name, g_free);
    }

  if (context->palette)
    g_signal_handlers_disconnect_by_func (context->palette,
                                          gimp_context_palette_dirty,
                                          context);

  g_set_object (&context->palette, palette);

  if (palette)
    {
      g_signal_connect_object (palette, "name-changed",
                               G_CALLBACK (gimp_context_palette_dirty),
                               context,
                               0);

      if (palette != GIMP_PALETTE (gimp_palette_get_standard (context)))
        context->palette_name = g_strdup (gimp_object_get_name (palette));
    }

  g_object_notify (G_OBJECT (context), "palette");
  gimp_context_palette_changed (context);
}


/*****************************************************************************/
/*  font     *****************************************************************/

GimpFont *
gimp_context_get_font (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->font;
}

void
gimp_context_set_font (GimpContext *context,
                       GimpFont    *font)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (font == NULL || GIMP_IS_FONT (font));

  context_find_defined (context, GIMP_CONTEXT_PROP_FONT);

  gimp_context_real_set_font (context, font);
}

const gchar *
gimp_context_get_font_name (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->font_name;
}

void
gimp_context_set_font_name (GimpContext *context,
                            const gchar *name)
{
  GimpContainer *container;
  GimpObject    *font;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  container = gimp_data_factory_get_container (context->gimp->font_factory);
  font      = gimp_container_search (container,
                                     (GimpContainerSearchFunc) gimp_font_match_by_lookup_name,
                                     (gpointer) name);

  if (font)
    {
      gimp_context_set_font (context, GIMP_FONT (font));
    }
  else
    {
      /* No font with this name exists, use the standard font, but
       * keep the intended name around
       */
      gimp_context_set_font (context, GIMP_FONT (gimp_font_get_standard ()));

      g_set_str (&context->font_name, name);
    }
}

void
gimp_context_font_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[FONT_CHANGED], 0,
                 context->font);
}

static void
gimp_context_font_dirty (GimpFont    *font,
                         GimpContext *context)
{
  g_set_str (&context->font_name,
             gimp_object_get_name (font));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_FONT);
}

static void
gimp_context_font_list_thaw (GimpContainer *container,
                             GimpContext   *context)
{
  GimpFont *font;

  if (! context->font_name)
    context->font_name = g_strdup (context->gimp->config->default_font);

  font = gimp_context_find_object (context, container,
                                   context->font_name,
                                   gimp_font_get_standard ());

  gimp_context_real_set_font (context, font);
}

static void
gimp_context_font_removed (GimpContainer *container,
                           GimpFont      *font,
                           GimpContext   *context)
{
  if (font == context->font)
    {
      g_signal_handlers_disconnect_by_func (context->font,
                                            gimp_context_font_dirty,
                                            context);
      g_clear_object (&context->font);

      if (! gimp_container_frozen (container))
        gimp_context_font_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_font (GimpContext *context,
                            GimpFont    *font)
{
  if (context->font == font)
    return;

  if (context->font_name &&
      font != GIMP_FONT (gimp_font_get_standard ()))
    {
      g_clear_pointer (&context->font_name, g_free);
    }

  if (context->font)
    g_signal_handlers_disconnect_by_func (context->font,
                                          gimp_context_font_dirty,
                                          context);

  g_set_object (&context->font, font);

  if (font)
    {
      g_signal_connect_object (font, "name-changed",
                               G_CALLBACK (gimp_context_font_dirty),
                               context,
                               0);

      if (font != GIMP_FONT (gimp_font_get_standard ()))
        context->font_name = g_strdup (gimp_font_get_lookup_name (font));
    }

  g_object_notify (G_OBJECT (context), "font");
  gimp_context_font_changed (context);
}


/********************************************************************************/
/*  tool preset *****************************************************************/

GimpToolPreset *
gimp_context_get_tool_preset (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->tool_preset;
}

void
gimp_context_set_tool_preset (GimpContext    *context,
                              GimpToolPreset *tool_preset)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (tool_preset == NULL || GIMP_IS_TOOL_PRESET (tool_preset));

  context_find_defined (context, GIMP_CONTEXT_PROP_TOOL_PRESET);

  gimp_context_real_set_tool_preset (context, tool_preset);
}

void
gimp_context_tool_preset_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[TOOL_PRESET_CHANGED], 0,
                 context->tool_preset);
}

static void
gimp_context_tool_preset_dirty (GimpToolPreset *tool_preset,
                                GimpContext    *context)
{
  g_set_str (&context->tool_preset_name, gimp_object_get_name (tool_preset));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_TOOL_PRESET);
}

static void
gimp_context_tool_preset_removed (GimpContainer  *container,
                                  GimpToolPreset *tool_preset,
                                  GimpContext    *context)
{
  if (tool_preset == context->tool_preset)
    {
      g_signal_handlers_disconnect_by_func (context->tool_preset,
                                            gimp_context_tool_preset_dirty,
                                            context);
      g_clear_object (&context->tool_preset);

      if (! gimp_container_frozen (container))
        gimp_context_tool_preset_list_thaw (container, context);
    }
}

static void
gimp_context_tool_preset_list_thaw (GimpContainer *container,
                                    GimpContext   *context)
{
  GimpToolPreset *tool_preset;

  tool_preset = gimp_context_find_object (context, container,
                                          context->tool_preset_name,
                                          NULL);

  gimp_context_real_set_tool_preset (context, tool_preset);
}

static void
gimp_context_real_set_tool_preset (GimpContext    *context,
                                   GimpToolPreset *tool_preset)
{
  if (context->tool_preset == tool_preset)
    return;

  if (context->tool_preset_name)
    {
      g_clear_pointer (&context->tool_preset_name, g_free);
    }

  if (context->tool_preset)
    g_signal_handlers_disconnect_by_func (context->tool_preset,
                                          gimp_context_tool_preset_dirty,
                                          context);

  g_set_object (&context->tool_preset, tool_preset);

  if (tool_preset)
    {
      g_signal_connect_object (tool_preset, "name-changed",
                               G_CALLBACK (gimp_context_tool_preset_dirty),
                               context,
                               0);

      context->tool_preset_name = g_strdup (gimp_object_get_name (tool_preset));
    }

  g_object_notify (G_OBJECT (context), "tool-preset");
  gimp_context_tool_preset_changed (context);
}


/*****************************************************************************/
/*  buffer  ******************************************************************/

GimpBuffer *
gimp_context_get_buffer (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->buffer;
}

void
gimp_context_set_buffer (GimpContext *context,
                         GimpBuffer *buffer)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (buffer == NULL || GIMP_IS_BUFFER (buffer));

  context_find_defined (context, GIMP_CONTEXT_PROP_BUFFER);

  gimp_context_real_set_buffer (context, buffer);
}

void
gimp_context_buffer_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[BUFFER_CHANGED], 0,
                 context->buffer);
}

static void
gimp_context_buffer_dirty (GimpBuffer  *buffer,
                           GimpContext *context)
{
  g_set_str (&context->buffer_name, gimp_object_get_name (buffer));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_BUFFER);
}

static void
gimp_context_buffer_list_thaw (GimpContainer *container,
                               GimpContext   *context)
{
  GimpBuffer *buffer;

  buffer = gimp_context_find_object (context, container,
                                     context->buffer_name,
                                     NULL);

  if (buffer)
    {
      gimp_context_real_set_buffer (context, buffer);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "buffer");
      gimp_context_buffer_changed (context);
    }
}

static void
gimp_context_buffer_removed (GimpContainer *container,
                             GimpBuffer    *buffer,
                             GimpContext   *context)
{
  if (buffer == context->buffer)
    {
      g_signal_handlers_disconnect_by_func (context->buffer,
                                            gimp_context_buffer_dirty,
                                            context);
      g_clear_object (&context->buffer);

      if (! gimp_container_frozen (container))
        gimp_context_buffer_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_buffer (GimpContext *context,
                              GimpBuffer  *buffer)
{
  if (context->buffer == buffer)
    return;

  if (context->buffer_name)
    {
      g_clear_pointer (&context->buffer_name, g_free);
    }

  if (context->buffer)
    g_signal_handlers_disconnect_by_func (context->buffer,
                                          gimp_context_buffer_dirty,
                                          context);

  g_set_object (&context->buffer, buffer);

  if (buffer)
    {
      g_signal_connect_object (buffer, "name-changed",
                               G_CALLBACK (gimp_context_buffer_dirty),
                               context,
                               0);

      context->buffer_name = g_strdup (gimp_object_get_name (buffer));
    }

  g_object_notify (G_OBJECT (context), "buffer");
  gimp_context_buffer_changed (context);
}


/*****************************************************************************/
/*  imagefile  ***************************************************************/

GimpImagefile *
gimp_context_get_imagefile (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->imagefile;
}

void
gimp_context_set_imagefile (GimpContext   *context,
                            GimpImagefile *imagefile)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (imagefile == NULL || GIMP_IS_IMAGEFILE (imagefile));

  context_find_defined (context, GIMP_CONTEXT_PROP_IMAGEFILE);

  gimp_context_real_set_imagefile (context, imagefile);
}

void
gimp_context_imagefile_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[IMAGEFILE_CHANGED], 0,
                 context->imagefile);
}

static void
gimp_context_imagefile_dirty (GimpImagefile *imagefile,
                              GimpContext   *context)
{
  g_set_str (&context->imagefile_name, gimp_object_get_name (imagefile));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_IMAGEFILE);
}

static void
gimp_context_imagefile_list_thaw (GimpContainer *container,
                                  GimpContext   *context)
{
  GimpImagefile *imagefile;

  imagefile = gimp_context_find_object (context, container,
                                        context->imagefile_name,
                                        NULL);

  if (imagefile)
    {
      gimp_context_real_set_imagefile (context, imagefile);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "imagefile");
      gimp_context_imagefile_changed (context);
    }
}

static void
gimp_context_imagefile_removed (GimpContainer *container,
                                GimpImagefile *imagefile,
                                GimpContext   *context)
{
  if (imagefile == context->imagefile)
    {
      g_signal_handlers_disconnect_by_func (context->imagefile,
                                            gimp_context_imagefile_dirty,
                                            context);
      g_clear_object (&context->imagefile);

      if (! gimp_container_frozen (container))
        gimp_context_imagefile_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_imagefile (GimpContext   *context,
                                 GimpImagefile *imagefile)
{
  if (context->imagefile == imagefile)
    return;

  if (context->imagefile_name)
    {
      g_clear_pointer (&context->imagefile_name, g_free);
    }

  if (context->imagefile)
    g_signal_handlers_disconnect_by_func (context->imagefile,
                                          gimp_context_imagefile_dirty,
                                          context);

  g_set_object (&context->imagefile, imagefile);

  if (imagefile)
    {
      g_signal_connect_object (imagefile, "name-changed",
                               G_CALLBACK (gimp_context_imagefile_dirty),
                               context,
                               0);

      context->imagefile_name = g_strdup (gimp_object_get_name (imagefile));
    }

  g_object_notify (G_OBJECT (context), "imagefile");
  gimp_context_imagefile_changed (context);
}


/*****************************************************************************/
/*  template  ***************************************************************/

GimpTemplate *
gimp_context_get_template (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->template;
}

void
gimp_context_set_template (GimpContext  *context,
                           GimpTemplate *template)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (template == NULL || GIMP_IS_TEMPLATE (template));

  context_find_defined (context, GIMP_CONTEXT_PROP_TEMPLATE);

  gimp_context_real_set_template (context, template);
}

void
gimp_context_template_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (context,
                 gimp_context_signals[TEMPLATE_CHANGED], 0,
                 context->template);
}

static void
gimp_context_template_dirty (GimpTemplate *template,
                             GimpContext  *context)
{
  g_set_str (&context->template_name, gimp_object_get_name (template));

  g_signal_emit (context, gimp_context_signals[PROP_NAME_CHANGED], 0,
                 GIMP_CONTEXT_PROP_TEMPLATE);
}

static void
gimp_context_template_list_thaw (GimpContainer *container,
                                 GimpContext   *context)
{
  GimpTemplate *template;

  template = gimp_context_find_object (context, container,
                                       context->template_name,
                                       NULL);

  if (template)
    {
      gimp_context_real_set_template (context, template);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "template");
      gimp_context_template_changed (context);
    }
}

static void
gimp_context_template_removed (GimpContainer *container,
                               GimpTemplate  *template,
                               GimpContext   *context)
{
  if (template == context->template)
    {
      g_signal_handlers_disconnect_by_func (context->template,
                                            gimp_context_template_dirty,
                                            context);
      g_clear_object (&context->template);

      if (! gimp_container_frozen (container))
        gimp_context_template_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_template (GimpContext  *context,
                                GimpTemplate *template)
{
  if (context->template == template)
    return;

  if (context->template_name)
    {
      g_clear_pointer (&context->template_name, g_free);
    }

  if (context->template)
    g_signal_handlers_disconnect_by_func (context->template,
                                          gimp_context_template_dirty,
                                          context);

  g_set_object (&context->template, template);

  if (template)
    {
      g_signal_connect_object (template, "name-changed",
                               G_CALLBACK (gimp_context_template_dirty),
                               context,
                               0);

      context->template_name = g_strdup (gimp_object_get_name (template));
    }

  g_object_notify (G_OBJECT (context), "template");
  gimp_context_template_changed (context);
}


/*****************************************************************************/
/*  Line Art  ****************************************************************/

GimpLineArt *
gimp_context_take_line_art (GimpContext *context)
{
  GimpLineArt *line_art;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (context->line_art)
    {
      g_source_remove (context->line_art_timeout_id);
      context->line_art_timeout_id = 0;

      line_art = context->line_art;
      context->line_art = NULL;
    }
  else
    {
      line_art = gimp_line_art_new ();
    }

  return line_art;
}

/*
 * gimp_context_store_line_art:
 * @context:
 * @line_art:
 *
 * The @context takes ownership of @line_art until the next time it is
 * requested with gimp_context_take_line_art() or until 3 minutes have
 * passed.
 * This function allows to temporarily store the computed line art data
 * in case it is needed very soon again, so that not to free and
 * recompute all the time the data when quickly switching tools.
 */
void
gimp_context_store_line_art (GimpContext *context,
                             GimpLineArt *line_art)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GIMP_IS_LINE_ART (line_art));

  if (context->line_art)
    {
      g_source_remove (context->line_art_timeout_id);
      context->line_art_timeout_id = 0;
    }

  context->line_art            = line_art;
  context->line_art_timeout_id = g_timeout_add (180000,
                                                (GSourceFunc) gimp_context_free_line_art,
                                                context);
}

static gboolean
gimp_context_free_line_art (GimpContext *context)
{
  g_clear_object (&context->line_art);

  context->line_art_timeout_id = 0;

  return G_SOURCE_REMOVE;
}


/*****************************************************************************/
/*  utility functions  *******************************************************/

static gpointer
gimp_context_find_object (GimpContext   *context,
                          GimpContainer *container,
                          const gchar   *object_name,
                          gpointer       standard_object)
{
  GimpObject *object = NULL;

  if (object_name)
    object = gimp_container_get_child_by_name (container, object_name);

  if (! object && ! gimp_container_is_empty (container))
    object = gimp_container_get_child_by_index (container, 0);

  if (! object)
    object = standard_object;

  return object;
}
