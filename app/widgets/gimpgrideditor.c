/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGridEditor
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpgrid.h"
#include "core/gimpmarshal.h"

#include "gimppropwidgets.h"
#include "gimpgrideditor.h"

#include "gimp-intl.h"


#define GRID_EDITOR_DEFAULT_RESOLUTION   72.0

#define GRID_EDITOR_COLOR_BUTTON_WIDTH   60
#define GRID_EDITOR_COLOR_BUTTON_HEIGHT  20


enum
{
  PROP_0,
  PROP_GRID,
  PROP_XRESOLUTION,
  PROP_YRESOLUTION
};


static void      gimp_grid_editor_class_init   (GimpGridEditorClass   *klass);
static GObject * gimp_grid_editor_constructor  (GType                  type,
                                                guint                  n_params,
                                                GObjectConstructParam *params);
static void      gimp_grid_editor_set_property (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void      gimp_grid_editor_get_property (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);
static void      gimp_grid_editor_finalize     (GObject               *object);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_grid_editor_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpGridEditorClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_grid_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpGridEditor),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      view_type = g_type_register_static (GTK_TYPE_VBOX,
                                          "GimpGridEditor",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_grid_editor_class_init (GimpGridEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_grid_editor_constructor;
  object_class->set_property = gimp_grid_editor_set_property;
  object_class->get_property = gimp_grid_editor_get_property;
  object_class->finalize     = gimp_grid_editor_finalize;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        GIMP_TYPE_GRID,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_XRESOLUTION,
                                   g_param_spec_double ("xresolution", NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        GRID_EDITOR_DEFAULT_RESOLUTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_YRESOLUTION,
                                   g_param_spec_double ("yresolution", NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        GRID_EDITOR_DEFAULT_RESOLUTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_grid_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpGridEditor *editor = GIMP_GRID_EDITOR (object);

  switch (property_id)
    {
    case PROP_GRID:
      editor->grid = GIMP_GRID (g_value_dup_object (value));
      break;
    case PROP_XRESOLUTION:
      editor->xresolution = g_value_get_double (value);
      break;
    case PROP_YRESOLUTION:
      editor->yresolution = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_grid_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpGridEditor *editor = GIMP_GRID_EDITOR (object);

  switch (property_id)
    {
    case PROP_GRID:
      g_value_set_object (value, editor->grid);
      break;
    case PROP_XRESOLUTION:
      g_value_set_double (value, editor->xresolution);
      break;
    case PROP_YRESOLUTION:
      g_value_set_double (value, editor->yresolution);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GObject *
gimp_grid_editor_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GimpGridEditor *editor;
  GObject        *object;
  GtkWidget      *frame;
  GtkWidget      *hbox;
  GtkWidget      *table;
  GtkWidget      *style;
  GtkWidget      *color_button;
  GtkWidget      *sizeentry;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_GRID_EDITOR (object);

  g_assert (editor->grid != NULL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  frame = gimp_frame_new (_("Appearance"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  style = gimp_prop_enum_combo_box_new (G_OBJECT (editor->grid), "style",
                                        GIMP_GRID_DOTS,
                                        GIMP_GRID_SOLID);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Line _Style:"), 0.0, 0.5,
                             style, 1, FALSE);

  color_button = gimp_prop_color_button_new (G_OBJECT (editor->grid), "fgcolor",
                                             _("Change grid foreground color"),
                                             GRID_EDITOR_COLOR_BUTTON_WIDTH,
                                             GRID_EDITOR_COLOR_BUTTON_HEIGHT,
                                             GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Foreground color:"), 0.0, 0.5,
                             color_button, 1, TRUE);

  color_button = gimp_prop_color_button_new (G_OBJECT (editor->grid), "bgcolor",
                                             _("Change grid background color"),
                                             GRID_EDITOR_COLOR_BUTTON_WIDTH,
                                             GRID_EDITOR_COLOR_BUTTON_HEIGHT,
                                             GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("_Background color:"), 0.0, 0.5,
                             color_button, 1, TRUE);

  gtk_widget_show (table);

  frame = gimp_frame_new (_("Spacing"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  sizeentry = gimp_prop_coordinates_new (G_OBJECT (editor->grid),
                                         "xspacing",
                                         "yspacing",
                                         "spacing-unit",
                                         "%a",
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         editor->xresolution,
                                         editor->yresolution,
                                         TRUE);

  gtk_table_set_col_spacings (GTK_TABLE (sizeentry), 2);
  gtk_table_set_row_spacings (GTK_TABLE (sizeentry), 2);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Pixels"), 1, 4, 0.0);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  gtk_widget_show (hbox);

  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  sizeentry = gimp_prop_coordinates_new (G_OBJECT (editor->grid),
                                         "xoffset",
                                         "yoffset",
                                         "offset-unit",
                                         "%a",
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         editor->xresolution,
                                         editor->yresolution,
                                         TRUE);

  gtk_table_set_col_spacings (GTK_TABLE (sizeentry), 2);
  gtk_table_set_row_spacings (GTK_TABLE (sizeentry), 2);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Pixels"), 1, 4, 0.0);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);

  gtk_widget_show (hbox);

  return object;
}

static void
gimp_grid_editor_finalize (GObject *object)
{
  GimpGridEditor *editor = GIMP_GRID_EDITOR (object);

  if (editor->grid)
    {
      g_object_unref (editor->grid);
      editor->grid = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
gimp_grid_editor_new (GimpGrid *grid,
                      gdouble   xresolution,
                      gdouble   yresolution)
{
  g_return_val_if_fail (GIMP_IS_GRID (grid), NULL);

  return g_object_new (GIMP_TYPE_GRID_EDITOR,
                       "grid",        grid,
                       "xresolution", xresolution,
                       "yresolution", yresolution,
                       NULL);
}
