/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdeviceinfoeditor.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcurve.h"
#include "core/gimppadactions.h"

#include "gimpactioneditor.h"
#include "gimpactionview.h"
#include "gimpcurveview.h"
#include "gimpdeviceinfo.h"
#include "gimpdeviceinfoeditor.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimpviewabledialog.h"

#include "gimp-intl.h"


#define CURVE_SIZE   256
#define CURVE_BORDER   4


enum
{
  PROP_0,
  PROP_INFO
};

enum
{
  AXIS_COLUMN_INDEX,
  AXIS_COLUMN_NAME,
  AXIS_COLUMN_INPUT_NAME,
  AXIS_N_COLUMNS
};

enum
{
  INPUT_COLUMN_INDEX,
  INPUT_COLUMN_NAME,
  INPUT_N_COLUMNS
};

enum
{
  KEY_COLUMN_INDEX,
  KEY_COLUMN_NAME,
  KEY_COLUMN_KEY,
  KEY_COLUMN_MASK,
  KEY_N_COLUMNS
};

enum
{
  PAD_COLUMN_TYPE,
  PAD_COLUMN_NUMBER,
  PAD_COLUMN_MODE,
  PAD_COLUMN_ACTION_ICON,
  PAD_COLUMN_ACTION_NAME,
  PAD_N_COLUMNS
};


typedef struct _GimpDeviceInfoEditorPrivate GimpDeviceInfoEditorPrivate;

struct _GimpDeviceInfoEditorPrivate
{
  GimpDeviceInfo *info;

  GtkWidget      *vbox;

  GtkListStore   *input_store;

  GtkListStore   *axis_store;

  GtkListStore   *pad_store;
  GtkWidget      *pad_action_view;
  GtkWidget      *pad_grab_button;
  GtkWidget      *pad_edit_button;
  GtkWidget      *pad_delete_button;
  GtkWidget      *pad_edit_dialog;

  GtkTreeSelection *pad_sel;
  GtkTreeSelection *pad_edit_sel;

  GtkWidget      *notebook;
};

#define GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE(editor) \
        ((GimpDeviceInfoEditorPrivate *) gimp_device_info_editor_get_instance_private ((GimpDeviceInfoEditor *) (editor)))


static void   gimp_device_info_editor_constructed   (GObject              *object);
static void   gimp_device_info_editor_finalize      (GObject              *object);
static void   gimp_device_info_editor_set_property  (GObject              *object,
                                                     guint                 property_id,
                                                     const GValue         *value,
                                                     GParamSpec           *pspec);
static void   gimp_device_info_editor_get_property  (GObject              *object,
                                                     guint                 property_id,
                                                     GValue               *value,
                                                     GParamSpec           *pspec);

static void   gimp_device_info_editor_set_axes      (GimpDeviceInfoEditor *editor);

static void   gimp_device_info_editor_axis_changed  (GtkCellRendererCombo *combo,
                                                     const gchar          *path_string,
                                                     GtkTreeIter          *new_iter,
                                                     GimpDeviceInfoEditor *editor);
static void   gimp_device_info_editor_axis_selected (GtkTreeSelection     *selection,
                                                     GimpDeviceInfoEditor *editor);

static void   gimp_device_info_editor_curve_reset   (GtkWidget            *button,
                                                     GimpCurve            *curve);

static gboolean gimp_device_info_editor_foreach     (GtkTreeModel         *model,
                                                     GtkTreePath          *path,
                                                     GtkTreeIter          *iter,
                                                     gpointer              data);

static void gimp_device_info_pad_name_column_func   (GtkTreeViewColumn    *column,
                                                     GtkCellRenderer      *cell,
                                                     GtkTreeModel         *tree_model,
                                                     GtkTreeIter          *iter,
                                                     gpointer              data);
static void gimp_device_info_editor_sel_changed     (GtkTreeSelection     *sel,
                                                     GimpDeviceInfoEditor *editor);
static void gimp_device_info_editor_pad_row_edit    (GtkTreeView          *view,
                                                     GtkTreePath          *path,
                                                     GtkTreeViewColumn    *column,
                                                     GimpDeviceInfoEditor *editor);
static void gimp_device_info_editor_grab_toggled    (GtkWidget            *button,
                                                     GimpDeviceInfoEditor *editor);

static void gimp_device_info_editor_edit_clicked    (GtkWidget            *button,
                                                     GimpDeviceInfoEditor *editor);
static void gimp_device_info_editor_delete_clicked  (GtkWidget            *button,
                                                     GimpDeviceInfoEditor *editor);
static void gimp_device_info_editor_edit_activated  (GtkTreeView          *tv,
                                                     GtkTreePath          *path,
                                                     GtkTreeViewColumn    *column,
                                                     GimpDeviceInfoEditor *editor);
static void gimp_device_info_editor_edit_response   (GtkWidget            *dialog,
                                                     gint                  response_id,
                                                     GimpDeviceInfoEditor *editor);
static void gimp_device_info_editor_pad_action_foreach (GimpPadActions    *pad_actions,
                                                        GimpPadActionType  action_type,
                                                        guint              number,
                                                        guint              mode,
                                                        const gchar       *action,
                                                        gpointer           data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDeviceInfoEditor, gimp_device_info_editor,
                            GTK_TYPE_BOX)

#define parent_class gimp_device_info_editor_parent_class


static void
gimp_device_info_editor_class_init (GimpDeviceInfoEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_device_info_editor_constructed;
  object_class->finalize     = gimp_device_info_editor_finalize;
  object_class->set_property = gimp_device_info_editor_set_property;
  object_class->get_property = gimp_device_info_editor_get_property;

  g_object_class_install_property (object_class, PROP_INFO,
                                   g_param_spec_object ("info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DEVICE_INFO,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_device_info_editor_init (GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  private->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (editor), private->vbox, TRUE, TRUE, 0);
  gtk_widget_show (private->vbox);

  private->axis_store = gtk_list_store_new (AXIS_N_COLUMNS,
                                            /* Axis index. */
                                            G_TYPE_INT,
                                            /* Axis name.  */
                                            G_TYPE_STRING,
                                            /* Input name. */
                                            G_TYPE_STRING);

}

static void
gimp_device_info_editor_constructed (GObject *object)
{
  GimpDeviceInfoEditor        *editor  = GIMP_DEVICE_INFO_EDITOR (object);
  GimpDeviceInfoEditorPrivate *private;
  GtkWidget                   *frame;
  GtkWidget                   *frame2;
  GtkWidget                   *grid;
  GtkWidget                   *label;
  GtkWidget                   *combo;
  GtkWidget                   *view;
  GtkTreeSelection            *sel;
  GtkTreeViewColumn           *column;
  GtkCellRenderer             *cell;
  GtkTreeIter                  axis_iter;
  gboolean                     has_axes = FALSE;

  gint                         n_axes;
  gint                         i;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_DEVICE_INFO (private->info));

  /*  the axes  */

  n_axes = gimp_device_info_get_n_axes (private->info);
  for (i = 0; i < n_axes; i++)
    {
      const gchar *axis_name;
      GdkAxisUse   use       = i + 1;
      gboolean     has_curve = FALSE;

      if (gimp_device_info_get_device (private->info, NULL))
        {
          if (gimp_device_info_ignore_axis (private->info, i))
            /* Some axis are apparently returned by the driver, yet
             * should be ignored. We just pass these.
             */
            continue;

          use = gimp_device_info_get_axis_use (private->info, i);
          has_curve = (gimp_device_info_get_curve (private->info, use) != NULL);
        }

      axis_name = gimp_device_info_get_axis_name (private->info, i);
      gtk_list_store_insert_with_values (private->axis_store,
                                         /* Set the initially selected
                                          * axis to an axis with curve
                                          * if available, or the first
                                          * row otherwise.
                                          */
                                         (! has_axes || has_curve) ? &axis_iter : NULL,
                                         -1,
                                         AXIS_COLUMN_INDEX, i,
                                         AXIS_COLUMN_NAME,  gettext (axis_name),
                                         -1);
      has_axes = TRUE;
    }

  if (has_axes)
    {
      /* The list of axes of an input device */

      frame = gimp_frame_new (_("Axes"));
      gtk_box_pack_start (GTK_BOX (private->vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      frame2 = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (frame), frame2);
      gtk_widget_show (frame2);

      view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (private->axis_store));
      g_object_unref (private->axis_store);

      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                   -1, NULL,
                                                   gtk_cell_renderer_text_new (),
                                                   "text", AXIS_COLUMN_NAME,
                                                   NULL);

      private->input_store = gtk_list_store_new (INPUT_N_COLUMNS,
                                                 /* Axis real index.    */
                                                 G_TYPE_INT,
                                                 /* Axis printed index. */
                                                 G_TYPE_STRING);

      cell = gtk_cell_renderer_combo_new ();
      g_object_set (cell,
                    "mode",        GTK_CELL_RENDERER_MODE_EDITABLE,
                    "editable",    TRUE,
                    "model",       private->input_store,
                    "text-column", INPUT_COLUMN_NAME,
                    "has-entry",   FALSE,
                    NULL);

      g_object_unref (private->input_store);

      g_signal_connect (cell, "changed",
                        G_CALLBACK (gimp_device_info_editor_axis_changed),
                        editor);

      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                   -1, NULL,
                                                   cell,
                                                   "text", AXIS_COLUMN_INPUT_NAME,
                                                   NULL);

      gtk_container_add (GTK_CONTAINER (frame2), view);
      gtk_widget_show (view);

      sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
      gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    }
  else
    {
      g_clear_object (&private->axis_store);
    }

  /*  general device information  */

  frame = gimp_frame_new (_("General"));
  gtk_box_pack_start (GTK_BOX (private->vbox), frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (private->vbox), frame, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (private->info), "mode",
                                        0, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Mode:"), 0.0, 0.5,
                            combo, 1);

  label = gimp_prop_enum_label_new (G_OBJECT (private->info), "source");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Source:"), 0.0, 0.5,
                            label, 1);

  label = gimp_prop_label_new (G_OBJECT (private->info), "vendor-id");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Vendor ID:"), 0.0, 0.5,
                            label, 1);

  label = gimp_prop_label_new (G_OBJECT (private->info), "product-id");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("Product ID:"), 0.0, 0.5,
                            label, 1);

  label = gimp_prop_enum_label_new (G_OBJECT (private->info), "tool-type");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Tool type:"), 0.0, 0.5,
                            label, 1);

  label = gimp_prop_label_new (G_OBJECT (private->info), "tool-serial");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 5,
                            _("Tool serial:"), 0.0, 0.5,
                            label, 1);

  label = gimp_prop_label_new (G_OBJECT (private->info), "tool-hardware-id");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 6,
                            _("Tool hardware ID:"), 0.0, 0.5,
                            label, 1);

  if (has_axes)
    {
      /*  the axes  */

      for (i = -1; i < n_axes; i++)
        {
          gchar name[16];

          if (i == -1)
            g_snprintf (name, sizeof (name), _("none"));
          else
            g_snprintf (name, sizeof (name), "%d", i + 1);

          gtk_list_store_insert_with_values (private->input_store, NULL, -1,
                                             INPUT_COLUMN_INDEX, i,
                                             INPUT_COLUMN_NAME,  name,
                                             -1);
        }

      gimp_device_info_editor_set_axes (editor);

      /*  the curves  */

      private->notebook = gtk_notebook_new ();
      gtk_notebook_set_show_border (GTK_NOTEBOOK (private->notebook), FALSE);
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);
      gtk_box_pack_start (GTK_BOX (editor), private->notebook, TRUE, TRUE, 0);
      gtk_widget_show (private->notebook);

      for (i = GDK_AXIS_X; i < GDK_AXIS_LAST; i++)
        {
          GimpCurve *curve;
          gchar     *title;

          /* e.g. "Pressure Curve" for mapping input device axes */
          title = g_strdup_printf (_("%s Curve"),
                                   gettext (gimp_device_info_get_axis_name (private->info, i - 1)));

          frame = gimp_frame_new (title);
          gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook), frame, NULL);
          gtk_widget_show (frame);

          g_free (title);

          curve = gimp_device_info_get_curve (private->info, i);

          if (curve)
            {
              GtkWidget *vbox;
              GtkWidget *hbox;
              GtkWidget *view;
              GtkWidget *label;
              GtkWidget *combo;
              GtkWidget *button;

              vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
              gtk_box_set_spacing (GTK_BOX (vbox), 6);
              gtk_container_add (GTK_CONTAINER (frame), vbox);
              gtk_widget_show (vbox);

              frame = gtk_frame_new (NULL);
              gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
              gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
              gtk_widget_show (frame);

              view = gimp_curve_view_new ();
              g_object_set (view,
                            "gimp",         GIMP_TOOL_PRESET (private->info)->gimp,
                            "border-width", CURVE_BORDER,
                            NULL);
              gtk_widget_set_size_request (view,
                                           CURVE_SIZE + CURVE_BORDER * 2,
                                           CURVE_SIZE + CURVE_BORDER * 2);
              gtk_container_add (GTK_CONTAINER (frame), view);
              gtk_widget_show (view);

              gimp_curve_view_set_curve (GIMP_CURVE_VIEW (view), curve, NULL);

              hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
              gtk_box_set_spacing (GTK_BOX (hbox), 6);
              gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
              gtk_widget_show (hbox);

              label = gtk_label_new_with_mnemonic (_("Curve _type:"));
              gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
              gtk_widget_show (label);

              combo = gimp_prop_enum_combo_box_new (G_OBJECT (curve),
                                                    "curve-type", 0, 0);
              gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                                   "gimp-curve");
              gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

              gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

              button = gtk_button_new_with_mnemonic (_("_Reset Curve"));
              gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_signal_connect (button, "clicked",
                                G_CALLBACK (gimp_device_info_editor_curve_reset),
                                curve);
            }
          else
            {
              GtkWidget *label;
              gchar     *string;

              string = g_strdup_printf (_("The axis '%s' has no curve"),
                                        gettext (gimp_device_info_get_axis_name (private->info, i - 1)));

              label = gtk_label_new (string);
              gtk_container_add (GTK_CONTAINER (frame), label);
              gtk_widget_show (label);

              g_free (string);
            }
        }

      /* Callback when selecting an axis. */

      g_signal_connect (sel, "changed",
                        G_CALLBACK (gimp_device_info_editor_axis_selected),
                        editor);

      if (has_axes)
        gtk_tree_selection_select_iter (sel, &axis_iter);
    }

  if (gimp_device_info_get_source (private->info) == GDK_SOURCE_TABLET_PAD)
    {
      GimpPadActions *pad_actions;
      GtkWidget      *vbox;
      GtkWidget      *hbox;
      GtkWidget      *sw;
      gboolean        sensitive_pad_actions = TRUE;

      frame = gimp_frame_new (_("Pad Actions"));
      gtk_box_pack_start (GTK_BOX (private->vbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      sw = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
      gtk_widget_show (sw);

      private->pad_action_view = gtk_tree_view_new ();
      gtk_container_add (GTK_CONTAINER (sw), private->pad_action_view);
      gtk_widget_show (private->pad_action_view);

      g_signal_connect (private->pad_action_view, "row-activated",
                        G_CALLBACK (gimp_device_info_editor_pad_row_edit),
                        editor);

      gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (private->pad_action_view),
                                                  -1, _("Event"),
                                                  gtk_cell_renderer_text_new (),
                                                  gimp_device_info_pad_name_column_func,
                                                  NULL, NULL);

      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (private->pad_action_view),
                                                   -1, _("Mode"),
                                                   gtk_cell_renderer_text_new (),
                                                   "text", PAD_COLUMN_MODE,
                                                   NULL);

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_title (column, _("Action"));
      gtk_tree_view_append_column (GTK_TREE_VIEW (private->pad_action_view),
                                   column);

      cell = gtk_cell_renderer_pixbuf_new ();
      gtk_tree_view_column_pack_start (column, cell, FALSE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "icon-name", PAD_COLUMN_ACTION_ICON,
                                           NULL);

      cell = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (column, cell, TRUE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "text", PAD_COLUMN_ACTION_NAME,
                                           NULL);

      private->pad_store = gtk_list_store_new (PAD_N_COLUMNS,
                                               /* Pad feature type      */
                                               G_TYPE_UINT,
                                               /* Number of pad feature */
                                               G_TYPE_UINT,
                                               /* Mode that applies.    */
                                               G_TYPE_UINT,
                                               /* Action icon name.     */
                                               G_TYPE_STRING,
                                               /* Action name.          */
                                               G_TYPE_STRING);

      pad_actions = gimp_device_info_get_pad_actions (private->info);
      gimp_pad_actions_foreach (pad_actions, gimp_device_info_editor_pad_action_foreach,
                                private->pad_store);
      gtk_tree_view_set_model (GTK_TREE_VIEW (private->pad_action_view),
                               GTK_TREE_MODEL (private->pad_store));

      private->pad_sel =
        gtk_tree_view_get_selection (GTK_TREE_VIEW (private->pad_action_view));

      g_signal_connect (private->pad_sel, "changed",
                        G_CALLBACK (gimp_device_info_editor_sel_changed),
                        editor);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      private->pad_grab_button = gtk_toggle_button_new_with_mnemonic (_("_Grab event"));
      gtk_box_pack_start (GTK_BOX (hbox), private->pad_grab_button, TRUE, TRUE, 0);
      gtk_widget_show (private->pad_grab_button);

      g_signal_connect (private->pad_grab_button, "toggled",
                        G_CALLBACK (gimp_device_info_editor_grab_toggled),
                        editor);

      gimp_help_set_help_data (private->pad_grab_button,
                               _("Select the next event arriving from "
                                 "the pad"),
                               NULL);

      private->pad_edit_button =
        gtk_button_new_with_mnemonic (_("_Edit event"));
      gtk_box_pack_start (GTK_BOX (hbox), private->pad_edit_button,
                          TRUE, TRUE, 0);
      gtk_widget_show (private->pad_edit_button);

      g_signal_connect (private->pad_edit_button, "clicked",
                        G_CALLBACK (gimp_device_info_editor_edit_clicked),
                        editor);

      private->pad_delete_button =
        gtk_button_new_with_mnemonic (_("_Clear event"));
      gtk_box_pack_start (GTK_BOX (hbox), private->pad_delete_button,
                          TRUE, TRUE, 0);
      gtk_widget_show (private->pad_delete_button);

      g_signal_connect (private->pad_delete_button, "clicked",
                        G_CALLBACK (gimp_device_info_editor_delete_clicked),
                        editor);

      gtk_widget_set_sensitive (private->pad_edit_button,   FALSE);
      gtk_widget_set_sensitive (private->pad_delete_button, FALSE);

#ifdef GDK_WINDOWING_X11
      /* We only deactivate the Pad Actions frame on X11 because on
       * macOS/Windows, there is likely no GDK_SOURCE_TABLET_PAD device
       * at all, to start with. Also there might be an implementation
       * eventually (whereas seeing this come to X11 seems improbable
       * and we don't have a generic way to see if pad actions would
       * work).
       * See: https://gitlab.gnome.org/GNOME/gimp/-/merge_requests/946#note_1768869
       * See also the message, 2 comments below, explaining how this
       * could be implemented on Windows with WM_POINTER API.
       */
      if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
        sensitive_pad_actions = FALSE;
#endif

      gtk_widget_set_sensitive (frame, sensitive_pad_actions);
      if (! sensitive_pad_actions)
        gtk_widget_set_tooltip_text (frame, _("Pad Actions are not supported on your platform yet"));
    }
}

static void
gimp_device_info_editor_finalize (GObject *object)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  g_clear_object (&private->info);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_device_info_editor_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      private->info = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_info_editor_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      g_value_set_object (value, private->info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_info_editor_set_axes (GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_tree_model_foreach (GTK_TREE_MODEL (private->axis_store),
                          gimp_device_info_editor_foreach, private);
}

static void
gimp_device_info_editor_axis_changed (GtkCellRendererCombo  *combo,
                                      const gchar           *path_string,
                                      GtkTreeIter           *new_iter,
                                      GimpDeviceInfoEditor  *editor)
{
  GimpDeviceInfoEditorPrivate *private;
  GtkTreePath                 *path;
  GtkTreeIter                  new_use_iter;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  path = gtk_tree_path_new_from_string (path_string);

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (private->axis_store),
                               &new_use_iter, path))
    {
      gint       axis_index;
      GdkAxisUse new_use  = GDK_AXIS_IGNORE;
      GdkAxisUse old_use  = GDK_AXIS_IGNORE;
      gint       new_axis = -1;
      gint       old_axis = -1;
      gint       n_axes;
      gint       i;

      gtk_tree_model_get (GTK_TREE_MODEL (private->axis_store), &new_use_iter,
                          AXIS_COLUMN_INDEX, &axis_index,
                          -1);
      new_use = axis_index + 1;

      gtk_tree_model_get (GTK_TREE_MODEL (private->input_store), new_iter,
                          INPUT_COLUMN_INDEX, &new_axis,
                          -1);

      n_axes = gimp_device_info_get_n_axes (private->info);

      for (i = 0; i < n_axes; i++)
        if (gimp_device_info_get_axis_use (private->info, i) == new_use)
          {
            old_axis = i;
            break;
          }

      if (new_axis == old_axis)
        goto out;

      if (new_axis != -1)
        old_use = gimp_device_info_get_axis_use (private->info, new_axis);

      /* we must always have an x and a y axis */
      if ((new_axis == -1 && (new_use == GDK_AXIS_X ||
                              new_use == GDK_AXIS_Y)) ||
          (old_axis == -1 && (old_use == GDK_AXIS_X ||
                              old_use == GDK_AXIS_Y)))
        {
          /* do nothing */
        }
      else
        {
          if (new_axis != -1)
            gimp_device_info_set_axis_use (private->info, new_axis, new_use);

          if (old_axis != -1)
            gimp_device_info_set_axis_use (private->info, old_axis, old_use);

          gimp_device_info_editor_set_axes (editor);
        }
    }

 out:
  gtk_tree_path_free (path);
}

static void
gimp_device_info_editor_axis_selected (GtkTreeSelection     *selection,
                                       GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;
  GtkTreeIter                  iter;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint axis_index;

      gtk_tree_model_get (GTK_TREE_MODEL (private->axis_store), &iter,
                          AXIS_COLUMN_INDEX, &axis_index,
                          -1);

      gtk_notebook_set_current_page (GTK_NOTEBOOK (private->notebook),
                                     axis_index);
    }
}

static void
gimp_device_info_editor_curve_reset (GtkWidget *button,
                                     GimpCurve *curve)
{
  gimp_curve_reset (curve, TRUE);
}

static gboolean
gimp_device_info_editor_foreach (GtkTreeModel *model,
                                 GtkTreePath *path,
                                 GtkTreeIter *iter,
                                 gpointer data)
{
  GimpDeviceInfoEditorPrivate *private = data;
  gchar                        input_name[16];
  gint                         n_axes;
  gint                         axe_index;
  gint                         i;

  n_axes = gimp_device_info_get_n_axes (private->info);

  gtk_tree_model_get (model, iter,
                      AXIS_COLUMN_INDEX, &axe_index,
                      -1);

  for (i = 0; i < n_axes; i++)
    {
      if (gimp_device_info_get_axis_use (private->info, i) == axe_index + 1)
        break;
    }

  if (i == n_axes)
    i = -1;

  if (i == -1)
    g_snprintf (input_name, sizeof (input_name), _("none"));
  else
    g_snprintf (input_name, sizeof (input_name), "%d", i + 1);

  gtk_list_store_set (private->axis_store,
                      iter,
                      AXIS_COLUMN_INPUT_NAME, input_name,
                      -1);

  /* Continue to walk the tree. */
  return FALSE;
}

static gchar *
gimp_device_info_get_pad_feature_name (GimpPadActionType type,
                                       guint             number)
{
  gchar *str = NULL;

  switch (type)
    {
    case GIMP_PAD_ACTION_BUTTON:
      str = g_strdup_printf (_("Button %d"), number + 1);
      break;
    case GIMP_PAD_ACTION_RING:
      str = g_strdup_printf (_("Ring %d"), number + 1);
      break;
    case GIMP_PAD_ACTION_STRIP:
      str = g_strdup_printf (_("Strip %d"), number + 1);
      break;
    default:
      break;
    }

  return str;
}

static void
gimp_device_info_pad_name_column_func (GtkTreeViewColumn *column,
                                       GtkCellRenderer   *cell,
                                       GtkTreeModel      *tree_model,
                                       GtkTreeIter       *iter,
                                       gpointer           data)
{
  GimpPadActionType  type;
  guint              number;
  gchar             *str = NULL;

  gtk_tree_model_get (tree_model, iter,
                      PAD_COLUMN_TYPE, &type,
                      PAD_COLUMN_NUMBER, &number,
                      -1);

  str = gimp_device_info_get_pad_feature_name (type, number);
  g_object_set (G_OBJECT (cell), "text", str, NULL);
  g_free (str);
}

static void
gimp_device_info_editor_sel_changed (GtkTreeSelection     *sel,
                                     GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;
  GtkTreeModel                *model;
  GtkTreeIter                  iter;
  gchar                       *edit_help        = NULL;
  gchar                       *delete_help      = NULL;
  gboolean                     edit_sensitive   = FALSE;
  gboolean                     delete_sensitive = FALSE;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GimpPadActionType  type;
      guint              number;
      gchar             *action = NULL;
      gchar             *event;

      gtk_tree_model_get (model, &iter,
                          PAD_COLUMN_TYPE,        &type,
                          PAD_COLUMN_NUMBER,      &number,
                          PAD_COLUMN_ACTION_NAME, &action,
                          -1);

      event = gimp_device_info_get_pad_feature_name (type, number);

      if (action)
        {
          g_free (action);

          delete_sensitive = TRUE;
          delete_help =
            g_strdup_printf (_("Remove the action assigned to '%s'"), event);
        }

      edit_sensitive = TRUE;
      edit_help = g_strdup_printf (_("Assign an action to '%s'"), event);
    }

  gimp_help_set_help_data (private->pad_edit_button, edit_help, NULL);
  gtk_widget_set_sensitive (private->pad_edit_button, edit_sensitive);
  g_free (edit_help);

  gimp_help_set_help_data (private->pad_delete_button, delete_help, NULL);
  gtk_widget_set_sensitive (private->pad_delete_button, delete_sensitive);
  g_free (delete_help);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->pad_grab_button),
                                FALSE);
}

static void
gimp_device_info_editor_pad_row_edit (GtkTreeView          *view,
                                      GtkTreePath          *path,
                                      GtkTreeViewColumn    *column,
                                      GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  if (gtk_widget_is_sensitive (private->pad_edit_button))
    gtk_button_clicked (GTK_BUTTON (private->pad_edit_button));
}

static void
gimp_device_info_editor_pad_row_ensure (GimpDeviceInfoEditor *editor,
                                        GimpPadActionType     type,
                                        guint                 number,
                                        guint                 mode)
{
  GimpDeviceInfoEditorPrivate *private;
  GtkTreeView                 *view;
  GtkTreeModel                *tree_model;
  GtkTreePath                 *path;
  GtkTreeIter                  iter;
  gint                         insert_pos = 0;
  gboolean                     iter_valid;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);
  tree_model = GTK_TREE_MODEL (private->pad_store);

  for (iter_valid = gtk_tree_model_get_iter_first (tree_model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (tree_model, &iter))
    {
      guint row_type;
      guint row_number;
      guint row_mode;

      gtk_tree_model_get (tree_model, &iter,
                          PAD_COLUMN_TYPE, &row_type,
                          PAD_COLUMN_NUMBER, &row_number,
                          PAD_COLUMN_MODE, &row_mode,
                          -1);
      if (type == row_type && number == row_number && mode == row_mode)
        goto select_item;

      if (row_type > type ||
          (row_type == type && row_number > number) ||
          (row_type == type && row_number == number && row_mode > mode))
        break;

      insert_pos++;
    }

  gtk_list_store_insert_with_values (private->pad_store, &iter, insert_pos,
                                     PAD_COLUMN_TYPE, type,
                                     PAD_COLUMN_NUMBER, number,
                                     PAD_COLUMN_MODE, mode,
                                     -1);

 select_item:

  view = GTK_TREE_VIEW (private->pad_action_view);
  path = gtk_tree_model_get_path (tree_model, &iter);
  gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0.0, 0.0);
  gtk_tree_view_set_cursor (view, path, NULL, FALSE);
  gtk_tree_path_free (path);

  gtk_widget_grab_focus (private->pad_action_view);
}

static gboolean
gimp_device_info_editor_pad_events (GtkWidget            *widget,
                                    GdkEvent             *event,
                                    GimpDeviceInfoEditor *editor)
{
  switch (event->type)
    {
    case GDK_PAD_BUTTON_RELEASE:
      gimp_device_info_editor_pad_row_ensure (editor,
                                              GIMP_PAD_ACTION_BUTTON,
                                              event->pad_button.button,
                                              event->pad_button.mode);
      break;
    case GDK_PAD_RING:
      gimp_device_info_editor_pad_row_ensure (editor,
                                              GIMP_PAD_ACTION_RING,
                                              event->pad_axis.index,
                                              event->pad_axis.mode);
      break;
    case GDK_PAD_STRIP:
      gimp_device_info_editor_pad_row_ensure (editor,
                                              GIMP_PAD_ACTION_STRIP,
                                              event->pad_axis.index,
                                              event->pad_axis.mode);
      break;
    default:
      break;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
gimp_device_info_editor_grab_toggled (GtkWidget            *button,
                                      GimpDeviceInfoEditor *editor)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      g_signal_connect (gtk_widget_get_toplevel (button), "event",
                        G_CALLBACK (gimp_device_info_editor_pad_events),
                        editor);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (gtk_widget_get_toplevel (button),
                                            gimp_device_info_editor_pad_events,
                                            editor);
    }
}

static void
gimp_device_info_editor_edit_clicked (GtkWidget            *button,
                                      GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;
  GtkTreeModel                *model;
  GtkTreeIter                  iter;
  GtkWidget                   *view;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->pad_grab_button),
                                FALSE);

  if (gtk_tree_selection_get_selected (private->pad_sel, &model, &iter))
    {
      GimpContext       *user_context;
      GimpPadActionType  type;
      guint              number;
      gchar             *label;
      gchar             *title;
      gchar             *action_name;

      gtk_tree_model_get (model, &iter,
                          PAD_COLUMN_TYPE,        &type,
                          PAD_COLUMN_NUMBER,      &number,
                          PAD_COLUMN_ACTION_NAME, &action_name,
                          -1);

      label = gimp_device_info_get_pad_feature_name (type, number);
      title = g_strdup_printf (_("Select Action for Event '%s'"),
                               label);
      g_free (label);

      user_context =
        gimp_get_user_context (GIMP_TOOL_PRESET (private->info)->gimp);

      g_set_weak_pointer
        (&private->pad_edit_dialog,
         gimp_viewable_dialog_new (g_list_prepend (NULL, NULL), user_context,
                                   _("Select Pad Event Action"),
                                   "gimp-pad-action-dialog",
                                   GIMP_ICON_EDIT,
                                   title,
                                   gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                                   gimp_standard_help_func,
                                   GIMP_HELP_PREFS_INPUT_DEVICES,

                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                   _("_OK"),     GTK_RESPONSE_OK,

                                   NULL));

      g_free (title);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (private->pad_edit_dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      gimp_dialog_factory_add_foreign (gimp_dialog_factory_get_singleton (),
                                       "gimp-pad-action-dialog",
                                       private->pad_edit_dialog,
                                       gimp_widget_get_monitor (button));

      g_signal_connect (private->pad_edit_dialog, "response",
                        G_CALLBACK (gimp_device_info_editor_edit_response),
                        editor);

      view = gimp_action_editor_new (GIMP_TOOL_PRESET (private->info)->gimp,
                                     action_name, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (view), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (private->pad_edit_dialog))),
                          view, TRUE, TRUE, 0);
      gtk_widget_show (view);

      g_signal_connect (GIMP_ACTION_EDITOR (view)->view, "row-activated",
                        G_CALLBACK (gimp_device_info_editor_edit_activated),
                        editor);

      g_set_weak_pointer
        (&private->pad_edit_sel,
         gtk_tree_view_get_selection (GTK_TREE_VIEW (GIMP_ACTION_EDITOR (view)->view)));

      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
      gtk_widget_show (private->pad_edit_dialog);
    }
}

static void
gimp_device_info_editor_delete_clicked (GtkWidget            *button,
                                        GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;
  GtkTreeModel                *model;
  GtkTreeIter                  iter;
  GimpPadActions              *pad_actions;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->pad_grab_button),
                                FALSE);

  if (gtk_tree_selection_get_selected (private->pad_sel, &model, &iter))
    {
      GimpPadActionType type;
      guint             number;
      guint             mode;

      gtk_tree_model_get (model, &iter,
                          PAD_COLUMN_TYPE,   &type,
                          PAD_COLUMN_NUMBER, &number,
                          PAD_COLUMN_MODE,   &mode,
                          -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          PAD_COLUMN_ACTION_ICON, NULL,
                          PAD_COLUMN_ACTION_NAME, NULL,
                          -1);

      pad_actions = gimp_device_info_get_pad_actions (private->info);
      gimp_pad_actions_clear_action (pad_actions, type, number, mode);
    }

  gtk_widget_set_sensitive (button, FALSE);
}

static void
gimp_device_info_editor_edit_activated (GtkTreeView          *tv,
                                        GtkTreePath          *path,
                                        GtkTreeViewColumn    *column,
                                        GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_dialog_response (GTK_DIALOG (private->pad_edit_dialog), GTK_RESPONSE_OK);
}

static void
gimp_device_info_editor_edit_response (GtkWidget            *dialog,
                                       gint                  response_id,
                                       GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gchar        *icon_name   = NULL;
      gchar        *action_name = NULL;

      if (gtk_tree_selection_get_selected (private->pad_edit_sel, &model, &iter))
        gtk_tree_model_get (model, &iter,
                            GIMP_ACTION_VIEW_COLUMN_ICON_NAME, &icon_name,
                            GIMP_ACTION_VIEW_COLUMN_NAME,      &action_name,
                            -1);

      if (gtk_tree_selection_get_selected (private->pad_sel, &model, &iter))
        {
          if (action_name)
            {
              GimpPadActions    *pad_actions;
              GimpPadActionType  type;
              guint              number;
              guint              mode;

              gtk_tree_model_get (model, &iter,
                                  PAD_COLUMN_TYPE,   &type,
                                  PAD_COLUMN_NUMBER, &number,
                                  PAD_COLUMN_MODE,   &mode,
                                  -1);

              gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                                  PAD_COLUMN_ACTION_ICON, icon_name,
                                  PAD_COLUMN_ACTION_NAME, action_name,
                                  -1);

              pad_actions = gimp_device_info_get_pad_actions (private->info);
              gimp_pad_actions_set_action (pad_actions, type, number, mode,
                                           action_name);
            }
        }

      g_free (icon_name);
      g_free (action_name);

      gimp_device_info_editor_sel_changed (private->pad_sel, editor);
    }

  gtk_widget_destroy (dialog);
}

static void
gimp_device_info_editor_pad_action_foreach (GimpPadActions    *pad_actions,
                                            GimpPadActionType  action_type,
                                            guint              number,
                                            guint              mode,
                                            const gchar       *action,
                                            gpointer           data)
{
  GtkTreeModel *tree_model = data;

  gtk_list_store_insert_with_values (GTK_LIST_STORE (tree_model), NULL, -1,
                                     PAD_COLUMN_TYPE, action_type,
                                     PAD_COLUMN_NUMBER, number,
                                     PAD_COLUMN_MODE, mode,
                                     PAD_COLUMN_ACTION_NAME, action,
                                     -1);
}


/*  public functions  */

GtkWidget *
gimp_device_info_editor_new (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  return g_object_new (GIMP_TYPE_DEVICE_INFO_EDITOR,
                       "info", info,
                       NULL);
}
