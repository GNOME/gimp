/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpfilleditor.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpfilloptions.h"

#include "gimpcolorpanel.h"
#include "gimpfilleditor.h"
#include "gimppropwidgets.h"
#include "gimpviewablebox.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPTIONS,
  PROP_EDIT_CONTEXT,
  PROP_USE_CUSTOM_STYLE
};


static void   gimp_fill_editor_constructed  (GObject      *object);
static void   gimp_fill_editor_finalize     (GObject      *object);
static void   gimp_fill_editor_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   gimp_fill_editor_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (GimpFillEditor, gimp_fill_editor, GTK_TYPE_BOX)

#define parent_class gimp_fill_editor_parent_class


static void
gimp_fill_editor_class_init (GimpFillEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_fill_editor_constructed;
  object_class->finalize     = gimp_fill_editor_finalize;
  object_class->set_property = gimp_fill_editor_set_property;
  object_class->get_property = gimp_fill_editor_get_property;

  g_object_class_install_property (object_class, PROP_OPTIONS,
                                   g_param_spec_object ("options",
                                                        NULL, NULL,
                                                        GIMP_TYPE_FILL_OPTIONS,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_EDIT_CONTEXT,
                                   g_param_spec_boolean ("edit-context",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USE_CUSTOM_STYLE,
                                   g_param_spec_boolean ("use-custom-style",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_fill_editor_init (GimpFillEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static void
gimp_fill_editor_constructed (GObject *object)
{
  GimpFillEditor *editor = GIMP_FILL_EDITOR (object);
  GtkWidget      *box;
  GtkWidget      *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_FILL_OPTIONS (editor->options));

  if (editor->use_custom_style)
    box = gimp_prop_enum_radio_box_new (G_OBJECT (editor->options),
                                        "custom-style", 0, 0);
  else
    box = gimp_prop_enum_radio_box_new (G_OBJECT (editor->options), "style",
                                        0, 0);

  gtk_box_pack_start (GTK_BOX (editor), box, FALSE, FALSE, 0);

  if (editor->edit_context)
    {
      GtkWidget *color_button;
      GtkWidget *pattern_box;

      if (editor->use_custom_style)
        {
          color_button = gimp_prop_color_button_new (G_OBJECT (editor->options),
                                                     "foreground",
                                                     _("Fill Color"),
                                                     1, 24,
                                                     GIMP_COLOR_AREA_SMALL_CHECKS);
          gimp_color_panel_set_context (GIMP_COLOR_PANEL (color_button),
                                        GIMP_CONTEXT (editor->options));
          gimp_enum_radio_box_add (GTK_BOX (box), color_button,
                                   GIMP_CUSTOM_STYLE_SOLID_COLOR, FALSE);
        }
      else
        {
          color_button = gimp_prop_color_button_new (G_OBJECT (editor->options),
                                                     "foreground",
                                                     _("Fill Color"),
                                                     1, 24,
                                                     GIMP_COLOR_AREA_SMALL_CHECKS);
          gimp_color_panel_set_context (GIMP_COLOR_PANEL (color_button),
                                        GIMP_CONTEXT (editor->options));
          gimp_enum_radio_box_add (GTK_BOX (box), color_button,
                                   GIMP_FILL_STYLE_FG_COLOR, FALSE);
          gimp_color_button_set_update (GIMP_COLOR_BUTTON (color_button), TRUE);

          color_button = gimp_prop_color_button_new (G_OBJECT (editor->options),
                                                     "background",
                                                     _("Fill BG Color"),
                                                     1, 24,
                                                     GIMP_COLOR_AREA_SMALL_CHECKS);
          gimp_color_panel_set_context (GIMP_COLOR_PANEL (color_button),
                                        GIMP_CONTEXT (editor->options));
          gimp_enum_radio_box_add (GTK_BOX (box), color_button,
                                   GIMP_FILL_STYLE_BG_COLOR, FALSE);
        }
      gimp_color_button_set_update (GIMP_COLOR_BUTTON (color_button), TRUE);

      pattern_box = gimp_prop_pattern_box_new (NULL,
                                               GIMP_CONTEXT (editor->options),
                                               NULL, 2,
                                               "pattern-view-type",
                                               "pattern-view-size");
      gimp_enum_radio_box_add (GTK_BOX (box), pattern_box,
                               (editor->use_custom_style ?
                                GIMP_CUSTOM_STYLE_PATTERN :
                                GIMP_FILL_STYLE_PATTERN),
                               FALSE);
    }

  button = gimp_prop_check_button_new (G_OBJECT (editor->options),
                                       "antialias",
                                       _("_Antialiasing"));
  gtk_box_pack_start (GTK_BOX (editor), button, FALSE, FALSE, 0);
}

static void
gimp_fill_editor_finalize (GObject *object)
{
  GimpFillEditor *editor = GIMP_FILL_EDITOR (object);

  g_clear_object (&editor->options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_fill_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpFillEditor *editor = GIMP_FILL_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      editor->options = g_value_dup_object (value);
      break;

    case PROP_EDIT_CONTEXT:
      editor->edit_context = g_value_get_boolean (value);
      break;

    case PROP_USE_CUSTOM_STYLE:
      editor->use_custom_style = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fill_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpFillEditor *editor = GIMP_FILL_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_value_set_object (value, editor->options);
      break;

    case PROP_EDIT_CONTEXT:
      g_value_set_boolean (value, editor->edit_context);
      break;

    case PROP_USE_CUSTOM_STYLE:
      g_value_set_boolean (value, editor->use_custom_style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_fill_editor_new (GimpFillOptions *options,
                      gboolean         edit_context,
                      gboolean         use_custom_style)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), NULL);

  return g_object_new (GIMP_TYPE_FILL_EDITOR,
                       "options",          options,
                       "edit-context",     edit_context ? TRUE : FALSE,
                       "use-custom-style", use_custom_style ? TRUE : FALSE,
                       NULL);
}
