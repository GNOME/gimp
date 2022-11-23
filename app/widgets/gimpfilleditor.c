/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmafilleditor.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmafilloptions.h"

#include "ligmacolorpanel.h"
#include "ligmafilleditor.h"
#include "ligmapropwidgets.h"
#include "ligmaviewablebox.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_OPTIONS,
  PROP_EDIT_CONTEXT
};


static void   ligma_fill_editor_constructed  (GObject      *object);
static void   ligma_fill_editor_finalize     (GObject      *object);
static void   ligma_fill_editor_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   ligma_fill_editor_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaFillEditor, ligma_fill_editor, GTK_TYPE_BOX)

#define parent_class ligma_fill_editor_parent_class


static void
ligma_fill_editor_class_init (LigmaFillEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_fill_editor_constructed;
  object_class->finalize     = ligma_fill_editor_finalize;
  object_class->set_property = ligma_fill_editor_set_property;
  object_class->get_property = ligma_fill_editor_get_property;

  g_object_class_install_property (object_class, PROP_OPTIONS,
                                   g_param_spec_object ("options",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_FILL_OPTIONS,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_EDIT_CONTEXT,
                                   g_param_spec_boolean ("edit-context",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_fill_editor_init (LigmaFillEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static void
ligma_fill_editor_constructed (GObject *object)
{
  LigmaFillEditor *editor = LIGMA_FILL_EDITOR (object);
  GtkWidget      *box;
  GtkWidget      *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_FILL_OPTIONS (editor->options));

  box = ligma_prop_enum_radio_box_new (G_OBJECT (editor->options), "style",
                                      0, 0);
  gtk_box_pack_start (GTK_BOX (editor), box, FALSE, FALSE, 0);

  if (editor->edit_context)
    {
      GtkWidget *color_button;
      GtkWidget *pattern_box;

      color_button = ligma_prop_color_button_new (G_OBJECT (editor->options),
                                                 "foreground",
                                                 _("Fill Color"),
                                                 1, 24,
                                                 LIGMA_COLOR_AREA_SMALL_CHECKS);
      ligma_color_panel_set_context (LIGMA_COLOR_PANEL (color_button),
                                    LIGMA_CONTEXT (editor->options));
      ligma_enum_radio_box_add (GTK_BOX (box), color_button,
                               LIGMA_FILL_STYLE_SOLID, FALSE);

      pattern_box = ligma_prop_pattern_box_new (NULL,
                                               LIGMA_CONTEXT (editor->options),
                                               NULL, 2,
                                               "pattern-view-type",
                                               "pattern-view-size");
      ligma_enum_radio_box_add (GTK_BOX (box), pattern_box,
                               LIGMA_FILL_STYLE_PATTERN, FALSE);
    }

  button = ligma_prop_check_button_new (G_OBJECT (editor->options),
                                       "antialias",
                                       _("_Antialiasing"));
  gtk_box_pack_start (GTK_BOX (editor), button, FALSE, FALSE, 0);
}

static void
ligma_fill_editor_finalize (GObject *object)
{
  LigmaFillEditor *editor = LIGMA_FILL_EDITOR (object);

  g_clear_object (&editor->options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_fill_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaFillEditor *editor = LIGMA_FILL_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      if (editor->options)
        g_object_unref (editor->options);
      editor->options = g_value_dup_object (value);
      break;

    case PROP_EDIT_CONTEXT:
      editor->edit_context = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_fill_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaFillEditor *editor = LIGMA_FILL_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_value_set_object (value, editor->options);
      break;

    case PROP_EDIT_CONTEXT:
      g_value_set_boolean (value, editor->edit_context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_fill_editor_new (LigmaFillOptions *options,
                      gboolean         edit_context)
{
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), NULL);

  return g_object_new (LIGMA_TYPE_FILL_EDITOR,
                       "options",      options,
                       "edit-context", edit_context ? TRUE : FALSE,
                       NULL);
}
