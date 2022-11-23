/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_RECTANGLE_OPTIONS_H__
#define __LIGMA_RECTANGLE_OPTIONS_H__


typedef enum
{
  LIGMA_RECTANGLE_OPTIONS_PROP_0,

  LIGMA_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK,
  LIGMA_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED,
  LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
  LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY,
  LIGMA_RECTANGLE_OPTIONS_PROP_GUIDE,

  LIGMA_RECTANGLE_OPTIONS_PROP_X,
  LIGMA_RECTANGLE_OPTIONS_PROP_Y,
  LIGMA_RECTANGLE_OPTIONS_PROP_WIDTH,
  LIGMA_RECTANGLE_OPTIONS_PROP_HEIGHT,
  LIGMA_RECTANGLE_OPTIONS_PROP_POSITION_UNIT,
  LIGMA_RECTANGLE_OPTIONS_PROP_SIZE_UNIT,

  LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE,
  LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE,
  LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH,
  LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT,
  LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH,
  LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT,
  LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH,
  LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT,
  LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE,
  LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
  LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
  LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR,
  LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR,
  LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT,
  LIGMA_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT,
  LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_UNIT,
  LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,

  LIGMA_RECTANGLE_OPTIONS_PROP_LAST = LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_CENTER
} LigmaRectangleOptionsProp;


#define LIGMA_TYPE_RECTANGLE_OPTIONS               (ligma_rectangle_options_get_type ())
#define LIGMA_IS_RECTANGLE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_RECTANGLE_OPTIONS))
#define LIGMA_RECTANGLE_OPTIONS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_RECTANGLE_OPTIONS, LigmaRectangleOptions))
#define LIGMA_RECTANGLE_OPTIONS_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), LIGMA_TYPE_RECTANGLE_OPTIONS, LigmaRectangleOptionsInterface))

#define LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE(obj)   (ligma_rectangle_options_get_private (LIGMA_RECTANGLE_OPTIONS (obj)))


typedef struct _LigmaRectangleOptions          LigmaRectangleOptions;
typedef struct _LigmaRectangleOptionsInterface LigmaRectangleOptionsInterface;
typedef struct _LigmaRectangleOptionsPrivate   LigmaRectangleOptionsPrivate;

struct _LigmaRectangleOptionsInterface
{
  GTypeInterface base_iface;
};

struct _LigmaRectangleOptionsPrivate
{
  gboolean                auto_shrink;
  gboolean                shrink_merged;

  gboolean                highlight;
  gdouble                 highlight_opacity;
  LigmaGuidesType          guide;

  gdouble                 x;
  gdouble                 y;
  gdouble                 width;
  gdouble                 height;

  LigmaUnit                position_unit;
  LigmaUnit                size_unit;

  gboolean                fixed_rule_active;
  LigmaRectangleFixedRule  fixed_rule;

  gdouble                 desired_fixed_width;
  gdouble                 desired_fixed_height;

  gdouble                 desired_fixed_size_width;
  gdouble                 desired_fixed_size_height;

  gdouble                 default_fixed_size_width;
  gdouble                 default_fixed_size_height;
  gboolean                overridden_fixed_size;

  gdouble                 aspect_numerator;
  gdouble                 aspect_denominator;

  gdouble                 default_aspect_numerator;
  gdouble                 default_aspect_denominator;
  gboolean                overridden_fixed_aspect;

  gboolean                fixed_center;

  /* This gboolean is not part of the actual rectangle tool options,
   * and should be refactored out along with the pointers to widgets.
   */
  gboolean                use_string_current;

  LigmaUnit                fixed_unit;

  /* options gui */

  GtkWidget              *auto_shrink_button;

  GtkWidget              *fixed_width_entry;
  GtkWidget              *fixed_height_entry;

  GtkWidget              *fixed_aspect_hbox;
  GtkWidget              *aspect_button_box;
  GtkListStore           *aspect_history;

  GtkWidget              *fixed_size_hbox;
  GtkWidget              *size_button_box;
  GtkListStore           *size_history;

  GtkWidget              *position_entry;
  GtkWidget              *size_entry;
};


GType       ligma_rectangle_options_get_type           (void) G_GNUC_CONST;

GtkWidget * ligma_rectangle_options_gui                (LigmaToolOptions      *tool_options);

void        ligma_rectangle_options_connect            (LigmaRectangleOptions *options,
                                                       LigmaImage            *image,
                                                       GCallback             shrink_callback,
                                                       gpointer              shrink_object);
void        ligma_rectangle_options_disconnect         (LigmaRectangleOptions *options,
                                                       GCallback             shrink_callback,
                                                       gpointer              shrink_object);

gboolean    ligma_rectangle_options_fixed_rule_active  (LigmaRectangleOptions *rectangle_options,
                                                       LigmaRectangleFixedRule fixed_rule);

LigmaRectangleOptionsPrivate *
            ligma_rectangle_options_get_private        (LigmaRectangleOptions *options);


/*  convenience functions  */

void        ligma_rectangle_options_install_properties (GObjectClass *klass);
void        ligma_rectangle_options_set_property       (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
void        ligma_rectangle_options_get_property       (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);


/*  testing helper functions  */

GtkWidget * ligma_rectangle_options_get_size_entry     (LigmaRectangleOptions *rectangle_options);


#endif  /* __LIGMA_RECTANGLE_OPTIONS_H__ */
