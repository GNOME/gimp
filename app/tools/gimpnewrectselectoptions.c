/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimprectangleoptions.h"
#include "gimpnewrectselectoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HIGHLIGHT,
  PROP_FIXED_WIDTH,
  PROP_WIDTH,
  PROP_FIXED_HEIGHT,
  PROP_HEIGHT,
  PROP_FIXED_ASPECT,
  PROP_ASPECT,
  PROP_FIXED_CENTER,
  PROP_CENTER_X,
  PROP_CENTER_Y,
  PROP_UNIT
};


static void   gimp_new_rect_select_options_class_init                   (GimpNewRectSelectOptionsClass *klass);
static void   gimp_new_rect_select_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *rectangle_iface);

static void   gimp_new_rect_select_options_set_property (GObject      *object,
                                                         guint         property_id,
                                                         const GValue *value,
                                                         GParamSpec   *pspec);
static void   gimp_new_rect_select_options_get_property (GObject      *object,
                                                         guint         property_id,
                                                         GValue       *value,
                                                         GParamSpec   *pspec);

void          gimp_new_rect_select_options_set_highlight    (GimpRectangleOptions *options,
                                                             gboolean              highlight);
gboolean      gimp_new_rect_select_options_get_highlight    (GimpRectangleOptions *options);

void          gimp_new_rect_select_options_set_fixed_width  (GimpRectangleOptions *options,
                                                             gboolean              fixed_width);
gboolean      gimp_new_rect_select_options_get_fixed_width  (GimpRectangleOptions *options);
void          gimp_new_rect_select_options_set_width        (GimpRectangleOptions *options,
                                                             gdouble               width);
gdouble       gimp_new_rect_select_options_get_width        (GimpRectangleOptions *options);

void          gimp_new_rect_select_options_set_fixed_height (GimpRectangleOptions *options,
                                                             gboolean              fixed_height);
gboolean      gimp_new_rect_select_options_get_fixed_height (GimpRectangleOptions *options);
void          gimp_new_rect_select_options_set_height       (GimpRectangleOptions *options,
                                                             gdouble               height);
gdouble       gimp_new_rect_select_options_get_height       (GimpRectangleOptions *options);

void          gimp_new_rect_select_options_set_fixed_aspect (GimpRectangleOptions *options,
                                                             gboolean              fixed_aspect);
gboolean      gimp_new_rect_select_options_get_fixed_aspect (GimpRectangleOptions *options);
void          gimp_new_rect_select_options_set_aspect       (GimpRectangleOptions *options,
                                                             gdouble               aspect);
gdouble       gimp_new_rect_select_options_get_aspect       (GimpRectangleOptions *options);

void          gimp_new_rect_select_options_set_fixed_center (GimpRectangleOptions *options,
                                                             gboolean              fixed_center);
gboolean      gimp_new_rect_select_options_get_fixed_center (GimpRectangleOptions *options);
void          gimp_new_rect_select_options_set_center_x     (GimpRectangleOptions *options,
                                                             gdouble               center_x);
gdouble       gimp_new_rect_select_options_get_center_x     (GimpRectangleOptions *options);
void          gimp_new_rect_select_options_set_center_y     (GimpRectangleOptions *options,
                                                             gdouble               center_y);
gdouble       gimp_new_rect_select_options_get_center_y     (GimpRectangleOptions *options);

void          gimp_new_rect_select_options_set_unit         (GimpRectangleOptions *options,
                                                             GimpUnit              unit);
GimpUnit      gimp_new_rect_select_options_get_unit         (GimpRectangleOptions *options);



static GimpSelectionOptionsClass *parent_class = NULL;


GType
gimp_new_rect_select_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpNewRectSelectOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_new_rect_select_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpNewRectSelectOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL
      };
      static const GInterfaceInfo rectangle_info =
      {
        (GInterfaceInitFunc) gimp_new_rect_select_options_rectangle_options_iface_init,           /* interface_init */
        NULL,           /* interface_finalize */
        NULL            /* interface_data */
      };

      type = g_type_register_static (GIMP_TYPE_SELECTION_OPTIONS,
                                     "GimpNewRectSelectOptions",
                                     &info, 0);
      g_type_add_interface_static (type,
                                   GIMP_TYPE_RECTANGLE_OPTIONS,
                                   &rectangle_info);
     }

  return type;
}

static void
gimp_new_rect_select_options_class_init (GimpNewRectSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_new_rect_select_options_set_property;
  object_class->get_property = gimp_new_rect_select_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HIGHLIGHT,
                                    "highlight",
                                    N_("Highlight rectangle"),
                                    TRUE,
                                    0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_WIDTH,
                                    "fixed-width", N_("Fixed width"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_WIDTH,
                                   "width", N_("Width"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_HEIGHT,
                                    "fixed-height", N_("Fixed height"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HEIGHT,
                                   "height", N_("Height"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_ASPECT,
                                    "fixed-aspect", N_("Fixed aspect"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_ASPECT,
                                   "aspect", N_("Aspect"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_CENTER,
                                    "fixed-center", N_("Fixed center"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CENTER_X,
                                   "center-x", N_("Center X"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CENTER_Y,
                                   "center-y", N_("Center Y"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT,
                                 "unit", NULL,
                                 TRUE, FALSE, GIMP_UNIT_PIXEL, 0);
}

static void
gimp_new_rect_select_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *rectangle_iface)
{
  rectangle_iface->set_highlight    = gimp_new_rect_select_options_set_highlight;
  rectangle_iface->get_highlight    = gimp_new_rect_select_options_get_highlight;
  rectangle_iface->set_fixed_width  = gimp_new_rect_select_options_set_fixed_width;
  rectangle_iface->get_fixed_width  = gimp_new_rect_select_options_get_fixed_width;
  rectangle_iface->set_width        = gimp_new_rect_select_options_set_width;
  rectangle_iface->get_width        = gimp_new_rect_select_options_get_width;
  rectangle_iface->set_fixed_height = gimp_new_rect_select_options_set_fixed_height;
  rectangle_iface->get_fixed_height = gimp_new_rect_select_options_get_fixed_height;
  rectangle_iface->set_height       = gimp_new_rect_select_options_set_height;
  rectangle_iface->get_height       = gimp_new_rect_select_options_get_height;
  rectangle_iface->set_fixed_aspect = gimp_new_rect_select_options_set_fixed_aspect;
  rectangle_iface->get_fixed_aspect = gimp_new_rect_select_options_get_fixed_aspect;
  rectangle_iface->set_aspect       = gimp_new_rect_select_options_set_aspect;
  rectangle_iface->get_aspect       = gimp_new_rect_select_options_get_aspect;
  rectangle_iface->set_fixed_center = gimp_new_rect_select_options_set_fixed_center;
  rectangle_iface->get_fixed_center = gimp_new_rect_select_options_get_fixed_center;
  rectangle_iface->set_center_x     = gimp_new_rect_select_options_set_center_x;
  rectangle_iface->get_center_x     = gimp_new_rect_select_options_get_center_x;
  rectangle_iface->set_center_y     = gimp_new_rect_select_options_set_center_y;
  rectangle_iface->get_center_y     = gimp_new_rect_select_options_get_center_y;
  rectangle_iface->set_unit         = gimp_new_rect_select_options_set_unit;
  rectangle_iface->get_unit         = gimp_new_rect_select_options_get_unit;
}

static void
gimp_new_rect_select_options_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpNewRectSelectOptions *options           = GIMP_NEW_RECT_SELECT_OPTIONS (object);
  GimpRectangleOptions     *rectangle_options = GIMP_RECTANGLE_OPTIONS (options);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      gimp_rectangle_options_set_highlight (rectangle_options, g_value_get_boolean (value));
      break;
    case PROP_FIXED_WIDTH:
      gimp_rectangle_options_set_fixed_width (rectangle_options, g_value_get_boolean (value));
      break;
    case PROP_WIDTH:
      gimp_rectangle_options_set_width (rectangle_options, g_value_get_double (value));
      break;
    case PROP_FIXED_HEIGHT:
      gimp_rectangle_options_set_fixed_height (rectangle_options, g_value_get_boolean (value));
      break;
    case PROP_HEIGHT:
      gimp_rectangle_options_set_height (rectangle_options, g_value_get_double (value));
      break;
    case PROP_FIXED_ASPECT:
      gimp_rectangle_options_set_fixed_aspect (rectangle_options, g_value_get_boolean (value));
      break;
    case PROP_ASPECT:
      gimp_rectangle_options_set_aspect (rectangle_options, g_value_get_double (value));
      break;
    case PROP_FIXED_CENTER:
      gimp_rectangle_options_set_fixed_center (rectangle_options, g_value_get_boolean (value));
      break;
    case PROP_CENTER_X:
      gimp_rectangle_options_set_center_x (rectangle_options, g_value_get_double (value));
      break;
    case PROP_CENTER_Y:
      gimp_rectangle_options_set_center_y (rectangle_options, g_value_get_double (value));
      break;
    case PROP_UNIT:
      gimp_rectangle_options_set_unit (rectangle_options, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_new_rect_select_options_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpNewRectSelectOptions *options           = GIMP_NEW_RECT_SELECT_OPTIONS (object);
  GimpRectangleOptions     *rectangle_options = GIMP_RECTANGLE_OPTIONS (options);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      g_value_set_boolean (value, gimp_rectangle_options_get_highlight (rectangle_options));
      break;
    case PROP_FIXED_WIDTH:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_width (rectangle_options));
      break;
    case PROP_WIDTH:
      g_value_set_double (value, gimp_rectangle_options_get_width (rectangle_options));
      break;
    case PROP_FIXED_HEIGHT:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_height (rectangle_options));
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, gimp_rectangle_options_get_height (rectangle_options));
      break;
    case PROP_FIXED_ASPECT:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_aspect (rectangle_options));
      break;
    case PROP_ASPECT:
      g_value_set_double (value, gimp_rectangle_options_get_aspect (rectangle_options));
      break;
    case PROP_FIXED_CENTER:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_center (rectangle_options));
      break;
    case PROP_CENTER_X:
      g_value_set_double (value, gimp_rectangle_options_get_center_x (rectangle_options));
      break;
    case PROP_CENTER_Y:
      g_value_set_double (value, gimp_rectangle_options_get_center_y (rectangle_options));
      break;
    case PROP_UNIT:
      g_value_set_int (value, gimp_rectangle_options_get_unit (rectangle_options));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_new_rect_select_options_set_highlight (GimpRectangleOptions *options,
                                            gboolean              highlight)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->highlight = highlight;
}

gboolean
gimp_new_rect_select_options_get_highlight (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->highlight; 
}

void
gimp_new_rect_select_options_set_fixed_width (GimpRectangleOptions *options,
                                              gboolean              fixed_width)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->fixed_width = fixed_width;
}

gboolean
gimp_new_rect_select_options_get_fixed_width (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->fixed_width;
}

void
gimp_new_rect_select_options_set_width (GimpRectangleOptions *options,
                                        gdouble               width)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->width = width;
}

gdouble
gimp_new_rect_select_options_get_width (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->width;
}

void
gimp_new_rect_select_options_set_fixed_height (GimpRectangleOptions *options,
                                               gboolean              fixed_height)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->fixed_height = fixed_height;
}

gboolean
gimp_new_rect_select_options_get_fixed_height (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->fixed_height;
}

void
gimp_new_rect_select_options_set_height (GimpRectangleOptions *options,
                                         gdouble               height)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->height = height;
}

gdouble
gimp_new_rect_select_options_get_height (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->height;
}

void
gimp_new_rect_select_options_set_fixed_aspect (GimpRectangleOptions *options,
                                               gboolean              fixed_aspect)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->fixed_aspect = fixed_aspect;
}

gboolean
gimp_new_rect_select_options_get_fixed_aspect (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->fixed_aspect;
}

void
gimp_new_rect_select_options_set_aspect (GimpRectangleOptions *options,
                                         gdouble               aspect)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->aspect = aspect;
}

gdouble
gimp_new_rect_select_options_get_aspect (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->aspect;
}

void
gimp_new_rect_select_options_set_fixed_center (GimpRectangleOptions *options,
                                               gboolean              fixed_center)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->fixed_center = fixed_center;
}

gboolean
gimp_new_rect_select_options_get_fixed_center (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->fixed_center;
}

void
gimp_new_rect_select_options_set_center_x (GimpRectangleOptions *options,
                                           gdouble               center_x)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->center_x = center_x;
}

gdouble
gimp_new_rect_select_options_get_center_x (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->center_x;
}

void
gimp_new_rect_select_options_set_center_y (GimpRectangleOptions *options,
                                           gdouble               center_y)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->center_y = center_y;
}

gdouble
gimp_new_rect_select_options_get_center_y (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->center_y;
}

void
gimp_new_rect_select_options_set_unit (GimpRectangleOptions *options,
                                       GimpUnit              unit)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  new_rect_select_options->unit = unit;
}

GimpUnit
gimp_new_rect_select_options_get_unit (GimpRectangleOptions *options)
{
  GimpNewRectSelectOptions *new_rect_select_options = GIMP_NEW_RECT_SELECT_OPTIONS (options);
  return new_rect_select_options->unit;
}

GtkWidget *
gimp_new_rect_select_options_gui (GimpToolOptions *tool_options)
{
  //GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox;
  GtkWidget *vbox_rectangle;
  //GtkWidget *frame;
  //GtkWidget *button;
  //gchar     *str;

  vbox = gimp_selection_options_gui (tool_options);

  /*  rectangle options  */
  vbox_rectangle = gimp_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  return vbox;
}
