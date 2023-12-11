/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimpstrokeoptions.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpstrokeeditor.h"

#include "stroke-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


typedef struct _StrokeDialog StrokeDialog;

struct _StrokeDialog
{
  GList              *items;
  GList              *drawables;
  GimpContext        *context;
  GimpStrokeOptions  *options;
  GimpStrokeCallback  callback;
  gpointer            user_data;

  GtkWidget          *tool_combo;
  GtkWidget          *stack;
};


/*  local function prototypes  */

static void  stroke_dialog_free        (StrokeDialog *private);
static void  stroke_dialog_response    (GtkWidget    *dialog,
                                        gint          response_id,
                                        StrokeDialog *private);
static void  stroke_dialog_expand_tabs (GtkWidget    *widget,
                                        gpointer      data);


/*  public function  */

GtkWidget *
stroke_dialog_new (GList              *items,
                   GList              *drawables,
                   GimpContext        *context,
                   const gchar        *title,
                   const gchar        *icon_name,
                   const gchar        *help_id,
                   GtkWidget          *parent,
                   GimpStrokeOptions  *options,
                   GimpStrokeCallback  callback,
                   gpointer            user_data)
{
  StrokeDialog *private;
  GimpImage    *image;
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *switcher;
  GtkWidget    *stack;
  GtkWidget    *frame;

  g_return_val_if_fail (items, NULL);
  g_return_val_if_fail (drawables, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  image = gimp_item_get_image (items->data);

  private = g_slice_new0 (StrokeDialog);

  private->items     = g_list_copy (items);
  private->drawables = g_list_copy (drawables);
  private->context   = context;
  private->options   = gimp_stroke_options_new (context->gimp, context, TRUE);
  private->callback  = callback;
  private->user_data = user_data;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (private->options), 0);

  dialog = gimp_viewable_dialog_new (g_list_copy (items), context,
                                     title, "gimp-stroke-options",
                                     icon_name,
                                     _("Choose Stroke Style"),
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,

                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Stroke"), GTK_RESPONSE_OK,

                                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) stroke_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (stroke_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);


  /* switcher */

  switcher = gtk_stack_switcher_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), switcher, TRUE, TRUE, 0);
  gtk_widget_show (switcher);

  stack = gtk_stack_new ();
  gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher),
                                GTK_STACK (stack));
  gtk_box_pack_start (GTK_BOX (main_vbox), stack, TRUE, TRUE, 0);
  gtk_widget_show (stack);

  /*  the stroke frame  */

  frame = gimp_frame_new (NULL);
  gtk_stack_add_titled (GTK_STACK (stack),
                        frame,
                        "stroke-tool",
                        _("Line"));
  gtk_widget_show (frame);

  {
    GtkWidget *stroke_editor;
    gdouble    xres;
    gdouble    yres;

    gimp_image_get_resolution (image, &xres, &yres);

    stroke_editor = gimp_stroke_editor_new (private->options, yres, FALSE,
                                            FALSE);
    gtk_container_add (GTK_CONTAINER (frame), stroke_editor);
    gtk_widget_show (stroke_editor);

  }


  /*  the paint tool frame  */

  frame = gimp_frame_new (NULL);
  gtk_stack_add_titled (GTK_STACK (stack),
                        frame,
                        "paint-tool",
                        _("Paint tool"));
  gtk_widget_show (frame);

  {
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *button;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_widget_show (vbox);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new_with_mnemonic (_("P_aint tool:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combo = gimp_container_combo_box_new (image->gimp->paint_info_list,
                                          GIMP_CONTEXT (private->options),
                                          16, 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_widget_show (combo);

    switch (gimp_stroke_options_get_method (private->options))
      {
      case GIMP_STROKE_LINE:
        gtk_stack_set_visible_child_name (GTK_STACK (stack), "stroke-tool");
        break;
      case GIMP_STROKE_PAINT_METHOD:
        gtk_stack_set_visible_child_name (GTK_STACK (stack), "paint-tool");
        break;
      }

    private->stack      = stack;
    private->tool_combo = combo;

    button = gimp_prop_check_button_new (G_OBJECT (private->options),
                                         "emulate-brush-dynamics",
                                         _("_Emulate brush dynamics"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  }

  /* Setting hexpand property of all tabs to true */
  gtk_container_foreach (GTK_CONTAINER (switcher),
                         stroke_dialog_expand_tabs,
                         NULL);

  return dialog;
}


/*  private functions  */

static void
stroke_dialog_free (StrokeDialog *private)
{
  g_object_unref (private->options);
  g_list_free (private->drawables);
  g_list_free (private->items);

  g_slice_free (StrokeDialog, private);
}

static void
stroke_dialog_response (GtkWidget    *dialog,
                        gint          response_id,
                        StrokeDialog *private)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        GimpToolInfo *tool_info = gimp_context_get_tool (private->context);

        gimp_config_reset (GIMP_CONFIG (private->options));

        gimp_container_view_select_item (GIMP_CONTAINER_VIEW (private->tool_combo),
                                         GIMP_VIEWABLE (tool_info->paint_info));

      }
      break;

    case GTK_RESPONSE_OK:
      {
        gint        stroke_type;
        GValue      value = G_VALUE_INIT;
        GParamSpec *pspec;

        if (g_strcmp0 (gtk_stack_get_visible_child_name (GTK_STACK (private->stack)),
                       "stroke-tool") == 0)
          stroke_type = GIMP_STROKE_LINE;
        else
          stroke_type = GIMP_STROKE_PAINT_METHOD;

        pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (private->options)),
                                              "method");
        if (pspec == NULL)
          {
            gtk_widget_destroy (dialog);
            return;
          }

        g_value_init (&value, pspec->value_type);
        g_value_set_enum (&value, stroke_type);

        g_object_set_property (G_OBJECT (private->options), "method", &value);

        private->callback (dialog,
                           private->items,
                           private->drawables,
                           private->context,
                           private->options,
                           private->user_data);
      }
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
stroke_dialog_expand_tabs (GtkWidget *widget, gpointer data)
{
  gtk_widget_set_hexpand (widget, TRUE);
}
