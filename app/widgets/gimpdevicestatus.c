/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdevicestatus.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpdnd.h"
#include "gimpdeviceinfo.h"
#include "gimpdevices.h"
#include "gimpdevicestatus.h"
#include "gimpdialogfactory.h"
#include "gimpview.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


#define CELL_SIZE 20 /* The size of the view cells */


struct _GimpDeviceStatusEntry
{
  GdkDevice *device;

  GtkWidget *separator;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *tool;
  GtkWidget *foreground;
  GtkWidget *background;
  GtkWidget *brush;
  GtkWidget *pattern;
  GtkWidget *gradient;
};


static void gimp_device_status_class_init      (GimpDeviceStatusClass *klass);
static void gimp_device_status_init            (GimpDeviceStatus      *editor);

static void gimp_device_status_destroy         (GtkObject             *object);

static void gimp_device_status_update_entry    (GimpDeviceInfo        *device_info,
                                                GimpDeviceStatusEntry *entry);
static void gimp_device_status_save_clicked    (GtkWidget             *button,
                                                GimpDeviceStatus      *status);
static void gimp_device_status_preview_clicked (GtkWidget             *widget,
                                                GdkModifierType        state,
                                                const gchar           *identifier);


static GimpEditorClass *parent_class = NULL;


GType
gimp_device_status_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpDeviceStatusClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_device_status_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDeviceStatus),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_device_status_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                          "GimpDeviceStatus",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_device_status_class_init (GimpDeviceStatusClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_device_status_destroy;
}

static void
gimp_device_status_init (GimpDeviceStatus *status)
{
  GdkDisplay *display;
  GList      *list;
  gint        i;

  display = gtk_widget_get_display (GTK_WIDGET (status));

  status->gimp           = NULL;
  status->current_device = NULL;
  status->num_devices    = g_list_length (gdk_display_list_devices (display));
  status->entries        = g_new0 (GimpDeviceStatusEntry,
                                   status->num_devices);

  status->table = gtk_table_new (status->num_devices * 3, 7, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (status->table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (status->table), 6);
  gtk_container_add (GTK_CONTAINER (status), status->table);
  gtk_widget_show (status->table);

  for (list = gdk_display_list_devices (display), i = 0;
       list;
       list = list->next, i++)
    {
      GimpDeviceInfo        *device_info;
      GimpContext           *context;
      GimpDeviceStatusEntry *entry = &status->entries[i];
      gint                   row    = i * 3;
      gchar                 *markup;
      GClosure              *closure;

      entry->device = GDK_DEVICE (list->data);

      device_info = gimp_device_info_get_by_device (entry->device);
      context     = GIMP_CONTEXT (device_info);

      closure = g_cclosure_new (G_CALLBACK (gimp_device_status_update_entry),
                                entry, NULL);
      g_object_watch_closure (G_OBJECT (status), closure);
      g_signal_connect_closure (device_info, "changed", closure, FALSE);

      /*  the separator  */

      entry->separator = gtk_hbox_new (FALSE, 0);
      gtk_table_attach (GTK_TABLE (status->table), entry->separator,
                        0, 7, row, row + 1,
                        GTK_FILL, GTK_FILL, 0, 2);

      row++;

      /*  the device name  */

      entry->label = gtk_label_new (NULL);

      markup = g_strdup_printf ("<b>%s</b>", GIMP_OBJECT (device_info)->name);
      gtk_label_set_markup (GTK_LABEL (entry->label), markup);
      g_free (markup);

      gtk_widget_set_size_request (entry->label, -1, CELL_SIZE);
      gtk_misc_set_alignment (GTK_MISC (entry->label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (status->table), entry->label,
                        1, 7, row, row + 1,
                        GTK_FILL, GTK_FILL, 0, 2);

      entry->arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
      gtk_widget_set_size_request (entry->arrow, CELL_SIZE, CELL_SIZE);
      gtk_table_attach (GTK_TABLE (status->table), entry->arrow,
                        0, 1, row, row + 1,
                        GTK_FILL, GTK_FILL, 0, 0);

      row++;

      /*  the tool  */

      entry->tool = gimp_prop_preview_new (G_OBJECT (context),
                                            "tool", CELL_SIZE);
      GIMP_VIEW (entry->tool)->clickable = TRUE;
      gtk_table_attach (GTK_TABLE (status->table), entry->tool,
                        1, 2, row, row + 1,
                        0, 0, 0, 0);

      g_signal_connect (entry->tool, "clicked",
                        G_CALLBACK (gimp_device_status_preview_clicked),
                        "gimp-tool-list|gimp-tool-grid");

      /*  the foreground color  */

      entry->foreground = gimp_prop_color_area_new (G_OBJECT (context),
                                                     "foreground",
                                                     CELL_SIZE, CELL_SIZE,
                                                     GIMP_COLOR_AREA_FLAT);
      gtk_widget_add_events (entry->foreground,
                             GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      gtk_table_attach (GTK_TABLE (status->table), entry->foreground,
                        2, 3, row, row + 1,
                        0, 0, 0, 0);

      /*  the background color  */

      entry->background = gimp_prop_color_area_new (G_OBJECT (context),
                                                     "background",
                                                     CELL_SIZE, CELL_SIZE,
                                                     GIMP_COLOR_AREA_FLAT);
      gtk_widget_add_events (entry->background,
                             GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      gtk_table_attach (GTK_TABLE (status->table), entry->background,
                        3, 4, row, row + 1,
                        0, 0, 0, 0);

      /*  the brush  */

      entry->brush = gimp_prop_preview_new (G_OBJECT (context),
                                             "brush", CELL_SIZE);
      GIMP_VIEW (entry->brush)->clickable  = TRUE;
      GIMP_VIEW (entry->brush)->show_popup = TRUE;
      gtk_table_attach (GTK_TABLE (status->table), entry->brush,
                        4, 5, row, row + 1,
                        0, 0, 0, 0);

      g_signal_connect (entry->brush, "clicked",
                        G_CALLBACK (gimp_device_status_preview_clicked),
                        "gimp-brush-grid|gimp-brush-list");

      /*  the pattern  */

      entry->pattern = gimp_prop_preview_new (G_OBJECT (context),
                                               "pattern", CELL_SIZE);
      GIMP_VIEW (entry->pattern)->clickable  = TRUE;
      GIMP_VIEW (entry->pattern)->show_popup = TRUE;
      gtk_table_attach (GTK_TABLE (status->table), entry->pattern,
                        5, 6, row, row + 1,
                        0, 0, 0, 0);

      g_signal_connect (entry->pattern, "clicked",
                        G_CALLBACK (gimp_device_status_preview_clicked),
                        "gimp-pattern-grid|gimp-pattern-list");

      /*  the gradient  */

      entry->gradient = gimp_prop_preview_new (G_OBJECT (context),
                                                "gradient", 2 * CELL_SIZE);
      GIMP_VIEW (entry->gradient)->clickable  = TRUE;
      GIMP_VIEW (entry->gradient)->show_popup = TRUE;
      gtk_table_attach (GTK_TABLE (status->table), entry->gradient,
                        6, 7, row, row + 1,
                        0, 0, 0, 0);

      g_signal_connect (entry->gradient, "clicked",
                        G_CALLBACK (gimp_device_status_preview_clicked),
                        "gimp-gradient-list|gimp-gradient-grid");

      gimp_device_status_update_entry (device_info, entry);
    }

  status->save_button =
    gimp_editor_add_button (GIMP_EDITOR (status), GTK_STOCK_SAVE,
                            _("Save device status"), NULL,
                            G_CALLBACK (gimp_device_status_save_clicked),
                            NULL,
                            status);
}

static void
gimp_device_status_destroy (GtkObject *object)
{
  GimpDeviceStatus *status = GIMP_DEVICE_STATUS (object);

  if (status->entries)
    {
      gint i;

      for (i = 0; i < status->num_devices; i++)
        {
          GimpDeviceStatusEntry *entry = &status->entries[i];

          g_signal_handlers_disconnect_by_func (entry->device,
                                                gimp_device_status_update_entry,
                                                entry);
        }

      g_free (status->entries);
      status->entries     = NULL;
      status->num_devices = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_device_status_new (Gimp *gimp)
{
  GimpDeviceStatus *status;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  status = g_object_new (GIMP_TYPE_DEVICE_STATUS, NULL);

  status->gimp = gimp;

  gimp_device_status_update (status);

  return GTK_WIDGET (status);
}

void
gimp_device_status_update (GimpDeviceStatus *status)
{
  gint i;

  g_return_if_fail (GIMP_IS_DEVICE_STATUS (status));

  status->current_device = gimp_devices_get_current (status->gimp);

  for (i = 0; i < status->num_devices; i++)
    {
      GimpDeviceStatusEntry *entry = &status->entries[i];

      if (entry->device == status->current_device)
        gtk_widget_show (entry->arrow);
      else
        gtk_widget_hide (entry->arrow);
    }
}


/*  private functions  */

static void
gimp_device_status_update_entry (GimpDeviceInfo        *device_info,
                                 GimpDeviceStatusEntry *entry)
{
  if (device_info->device->mode == GDK_MODE_DISABLED)
    {
      gtk_widget_hide (entry->separator);
      gtk_widget_hide (entry->label);
      gtk_widget_hide (entry->tool);
      gtk_widget_hide (entry->foreground);
      gtk_widget_hide (entry->background);
      gtk_widget_hide (entry->brush);
      gtk_widget_hide (entry->pattern);
      gtk_widget_hide (entry->gradient);
    }
  else
    {
      GimpContext *context = GIMP_CONTEXT (device_info);
      GimpRGB      color;
      guchar       r, g, b;
      gchar        buf[64];

      gtk_widget_show (entry->separator);
      gtk_widget_show (entry->label);
      gtk_widget_show (entry->tool);
      gtk_widget_show (entry->foreground);
      gtk_widget_show (entry->background);
      gtk_widget_show (entry->brush);
      gtk_widget_show (entry->pattern);
      gtk_widget_show (entry->gradient);

      gimp_context_get_foreground (context, &color);
      gimp_rgb_get_uchar (&color, &r, &g, &b);
      g_snprintf (buf, sizeof (buf), _("Foreground: %d, %d, %d"), r, g, b);
      gimp_help_set_help_data (entry->foreground, buf, NULL);

      gimp_context_get_background (context, &color);
      gimp_rgb_get_uchar (&color, &r, &g, &b);
      g_snprintf (buf, sizeof (buf), _("Background: %d, %d, %d"), r, g, b);
      gimp_help_set_help_data (entry->background, buf, NULL);
    }
}

static void
gimp_device_status_save_clicked (GtkWidget        *button,
                                 GimpDeviceStatus *status)
{
  gimp_devices_save (status->gimp, TRUE);
}

static void
gimp_device_status_preview_clicked (GtkWidget       *widget,
                                    GdkModifierType  state,
                                    const gchar     *identifier)
{
  GimpDialogFactory *dialog_factory;

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  gimp_dialog_factory_dialog_raise (dialog_factory,
                                    gtk_widget_get_screen (widget),
                                    identifier, -1);
}
