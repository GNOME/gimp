/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdashboard.c
 * Copyright (C) 2017 Ell
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gegl.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpdocked.h"
#include "gimpdashboard.h"
#include "gimpdialogfactory.h"
#include "gimpmeter.h"
#include "gimpsessioninfo-aux.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define DEFAULT_UPDATE_INTERVAL        GIMP_DASHBOARD_UPDATE_INTERVAL_0_25_SEC
#define DEFAULT_HISTORY_DURATION       GIMP_DASHBOARD_HISTORY_DURATION_60_SEC
#define DEFAULT_LOW_SWAP_SPACE_WARNING TRUE

#define LOW_SWAP_SPACE_WARNING_ON      /* swap occupied is above */ 0.90 /* of swap limit */
#define LOW_SWAP_SPACE_WARNING_OFF     /* swap occupied is below */ 0.85 /* of swap limit */

#define CACHE_OCCUPIED_COLOR           {0.3, 0.6, 0.3, 1.0}
#define SWAP_OCCUPIED_COLOR            {0.8, 0.2, 0.2, 1.0}
#define SWAP_SIZE_COLOR                {0.8, 0.6, 0.4, 1.0}
#define SWAP_LED_COLOR                 {0.8, 0.4, 0.4, 1.0}


static void       gimp_dashboard_docked_iface_init (GimpDockedInterface *iface);

static void       gimp_dashboard_finalize          (GObject             *object);

static void       gimp_dashboard_map               (GtkWidget           *widget);
static void       gimp_dashboard_unmap             (GtkWidget           *widget);

static void       gimp_dashboard_set_aux_info      (GimpDocked          *docked,
                                                    GList               *aux_info);
static GList    * gimp_dashboard_get_aux_info      (GimpDocked          *docked);

static gboolean   gimp_dashboard_update            (GimpDashboard       *dashboard);
static gboolean   gimp_dashboard_low_swap_space    (GimpDashboard       *dashboard);
static gpointer   gimp_dashboard_sample            (GimpDashboard       *dashboard);

static void       gimp_dashboard_label_set_text    (GtkLabel            *label,
                                                    const gchar         *text);
static void       gimp_dashboard_label_set_size    (GtkLabel            *label,
                                                    guint64              size,
                                                    guint64              limit);

static guint64    gimp_dashboard_get_swap_limit    (guint64              swap_size);


G_DEFINE_TYPE_WITH_CODE (GimpDashboard, gimp_dashboard, GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_dashboard_docked_iface_init))

#define parent_class gimp_dashboard_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


/*  private functions  */


static void
gimp_dashboard_class_init (GimpDashboardClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = gimp_dashboard_finalize;

  widget_class->map      = gimp_dashboard_map;
  widget_class->unmap    = gimp_dashboard_unmap;
}

static void
gimp_dashboard_init (GimpDashboard *dashboard)
{
  GtkWidget *box;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *meter;
  GtkWidget *label;
  gint       content_spacing;

  dashboard->update_interval        = DEFAULT_UPDATE_INTERVAL;
  dashboard->history_duration       = DEFAULT_HISTORY_DURATION;
  dashboard->low_swap_space_warning = DEFAULT_LOW_SWAP_SPACE_WARNING;

  gtk_widget_style_get (GTK_WIDGET (dashboard),
                        "content-spacing", &content_spacing,
                        NULL);

  /* we put the dashboard inside an event box, so that it gets its own window,
   * which reduces the overhead of updating the ui, since it gets updated
   * frequently.  unfortunately, this means that the dashboard's background
   * color may be a bit off for some themes.
   */
  box = dashboard->box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (dashboard), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  /* main vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2 * content_spacing);
  gtk_container_add (GTK_CONTAINER (box), vbox);
  gtk_widget_show (vbox);


  /* cache frame */
  frame = gimp_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* cache frame label */
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_frame_set_label_widget (GTK_FRAME (frame), box);
  gtk_widget_show (box);

  label = gtk_label_new (_("Cache"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* cache table */
  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), content_spacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /* cache meter */
  meter = dashboard->cache_meter = gimp_meter_new (1);
  gimp_meter_set_color (GIMP_METER (meter),
                        0, &(GimpRGB) CACHE_OCCUPIED_COLOR);
  gimp_meter_set_history_resolution (GIMP_METER (meter),
                                     dashboard->update_interval / 1000.0);
  gimp_meter_set_history_duration (GIMP_METER (meter),
                                   dashboard->history_duration / 1000.0);
  gtk_table_attach (GTK_TABLE (table), meter, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 1, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2 * content_spacing);
  gtk_widget_show (meter);

  /* cache occupied field */
  label = gtk_label_new (_("Occupied:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  label = dashboard->cache_occupied_label = gtk_label_new (_("N/A"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  /* cache limit field */
  label = gtk_label_new (_("Limit:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  label = dashboard->cache_limit_label = gtk_label_new (_("N/A"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);


  /* swap frame */
  frame = gimp_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* swap frame label */
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_frame_set_label_widget (GTK_FRAME (frame), box);
  gtk_widget_show (box);

  label = gtk_label_new (_("Swap"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* swap table */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), content_spacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /* swap meter */
  meter = dashboard->swap_meter = gimp_meter_new (2);
  gimp_meter_set_color (GIMP_METER (meter),
                        0, &(GimpRGB) SWAP_SIZE_COLOR);
  gimp_meter_set_color (GIMP_METER (meter),
                        1, &(GimpRGB) SWAP_OCCUPIED_COLOR);
  gimp_meter_set_history_resolution (GIMP_METER (meter),
                                     dashboard->update_interval / 1000.0);
  gimp_meter_set_history_duration (GIMP_METER (meter),
                                   dashboard->history_duration / 1000.0);
  gimp_meter_set_led_color (GIMP_METER (meter),
                            &(GimpRGB) SWAP_LED_COLOR);
  gtk_table_attach (GTK_TABLE (table), meter, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 1, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2 * content_spacing);
  gtk_widget_show (meter);

  /* swap occupied field */
  label = gtk_label_new (_("Occupied:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  label = dashboard->swap_occupied_label = gtk_label_new (_("N/A"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  /* swap size field */
  label = gtk_label_new (_("Size:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  label = dashboard->swap_size_label = gtk_label_new (_("N/A"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  /* swap limit field */
  label = gtk_label_new (_("Limit:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);

  label = dashboard->swap_limit_label = gtk_label_new (_("N/A"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (label);


  /* sampler thread */
  g_mutex_init (&dashboard->mutex);
  g_cond_init (&dashboard->cond);

  /* we use a separate thread for sampling, so that data is sampled even when
   * the main thread is busy
   */
  dashboard->thread = g_thread_new ("dashboard",
                                    (GThreadFunc) gimp_dashboard_sample,
                                    dashboard);
}

static void
gimp_dashboard_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_aux_info = gimp_dashboard_set_aux_info;
  iface->get_aux_info = gimp_dashboard_get_aux_info;
}

static void
gimp_dashboard_finalize (GObject *object)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (object);

  if (dashboard->timeout_id)
    {
      g_source_remove (dashboard->timeout_id);
      dashboard->timeout_id = 0;
    }

  if (dashboard->low_swap_space_idle_id)
    {
      g_source_remove (dashboard->low_swap_space_idle_id);
      dashboard->low_swap_space_idle_id = 0;
    }

  if (dashboard->thread)
    {
      g_mutex_lock (&dashboard->mutex);

      dashboard->quit = TRUE;
      g_cond_signal (&dashboard->cond);

      g_mutex_unlock (&dashboard->mutex);

      g_clear_pointer (&dashboard->thread, g_thread_join);
    }

  g_mutex_clear (&dashboard->mutex);
  g_cond_clear (&dashboard->cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dashboard_map (GtkWidget *widget)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (! dashboard->timeout_id)
    {
      dashboard->timeout_id = g_timeout_add (dashboard->update_interval,
                                             (GSourceFunc) gimp_dashboard_update,
                                             dashboard);
    }
}

static void
gimp_dashboard_unmap (GtkWidget *widget)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (widget);

  if (dashboard->timeout_id)
    {
      g_source_remove (dashboard->timeout_id);
      dashboard->timeout_id = 0;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

#define AUX_INFO_UPDATE_INTERVAL        "update-interval"
#define AUX_INFO_HISTORY_DURATION       "history-duration"
#define AUX_INFO_LOW_SWAP_SPACE_WARNING "low-swap-space-warning"

static void
gimp_dashboard_set_aux_info (GimpDocked *docked,
                             GList      *aux_info)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (docked);
  GList         *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_UPDATE_INTERVAL))
        {
          gint                       value = atoi (aux->value);
          GimpDashboardUpdateInteval update_interval;

          for (update_interval = GIMP_DASHBOARD_UPDATE_INTERVAL_0_25_SEC;
               update_interval < value &&
               update_interval < GIMP_DASHBOARD_UPDATE_INTERVAL_4_SEC;
               update_interval *= 2);

          gimp_dashboard_set_update_interval (dashboard, update_interval);
        }
      else if (! strcmp (aux->name, AUX_INFO_HISTORY_DURATION))
        {
          gint                         value = atoi (aux->value);
          GimpDashboardHistoryDuration history_duration;

          for (history_duration = GIMP_DASHBOARD_HISTORY_DURATION_15_SEC;
               history_duration < value &&
               history_duration < GIMP_DASHBOARD_HISTORY_DURATION_240_SEC;
               history_duration *= 2);

          gimp_dashboard_set_history_duration (dashboard, history_duration);
        }
      else if (! strcmp (aux->name, AUX_INFO_LOW_SWAP_SPACE_WARNING))
        {
          gimp_dashboard_set_low_swap_space_warning (dashboard,
                                                     ! strcmp (aux->value, "yes"));
        }
    }
}

static GList *
gimp_dashboard_get_aux_info (GimpDocked *docked)
{
  GimpDashboard      *dashboard = GIMP_DASHBOARD (docked);
  GList              *aux_info;
  GimpSessionInfoAux *aux;
  gchar              *value;

  aux_info = parent_docked_iface->get_aux_info (docked);

  value    = g_strdup_printf ("%d", dashboard->update_interval);
  aux      = gimp_session_info_aux_new (AUX_INFO_UPDATE_INTERVAL, value);
  aux_info = g_list_append (aux_info, aux);
  g_free (value);

  value    = g_strdup_printf ("%d", dashboard->history_duration);
  aux      = gimp_session_info_aux_new (AUX_INFO_HISTORY_DURATION, value);
  aux_info = g_list_append (aux_info, aux);
  g_free (value);

  value    = dashboard->low_swap_space_warning ? "yes" : "no";
  aux      = gimp_session_info_aux_new (AUX_INFO_LOW_SWAP_SPACE_WARNING, value);
  aux_info = g_list_append (aux_info, aux);

  return aux_info;
}

static gboolean
gimp_dashboard_update (GimpDashboard *dashboard)
{
  /* cache */
  {
    guint64  cache_occupied;
    guint64  cache_limit;

    g_object_get (gegl_stats (),
                  "tile-cache-total", &cache_occupied,
                  NULL);
    g_object_get (gegl_config (),
                  "tile-cache-size",  &cache_limit,
                  NULL);

    gimp_meter_set_range (GIMP_METER (dashboard->cache_meter),
                          0.0, cache_limit);

    gimp_dashboard_label_set_size (GTK_LABEL (dashboard->cache_occupied_label),
                                   cache_occupied, cache_limit);
    gimp_dashboard_label_set_size (GTK_LABEL (dashboard->cache_limit_label),
                                   cache_limit, 0);
  }

  /* swap */
  {
    guint64  swap_occupied;
    guint64  swap_size;
    guint64  swap_limit;
    gboolean swap_busy;

    g_mutex_lock (&dashboard->mutex);

    g_object_get (gegl_stats (),
                  "swap-total",     &swap_occupied,
                  "swap-file-size", &swap_size,
                  "swap-busy",      &swap_busy,
                  NULL);
    swap_limit = gimp_dashboard_get_swap_limit (swap_size);

    g_mutex_unlock (&dashboard->mutex);

    gimp_meter_set_range (GIMP_METER (dashboard->swap_meter),
                          0.0, swap_limit ? swap_limit : swap_size);
    gimp_meter_set_led_visible (GIMP_METER (dashboard->swap_meter), swap_busy);

    gimp_dashboard_label_set_size (GTK_LABEL (dashboard->swap_occupied_label),
                                   swap_occupied, swap_limit);
    gimp_dashboard_label_set_size (GTK_LABEL (dashboard->swap_size_label),
                                   swap_size, swap_limit);

    if (swap_limit)
      {
        gimp_dashboard_label_set_size (GTK_LABEL (dashboard->swap_limit_label),
                                       swap_limit, 0);
      }
    else
      {
        gimp_dashboard_label_set_text (GTK_LABEL (dashboard->swap_limit_label),
                                       _("N/A"));
      }
  }

  return G_SOURCE_CONTINUE;
}

static gboolean
gimp_dashboard_low_swap_space (GimpDashboard *dashboard)
{
  if (dashboard->gimp)
    {
      GdkScreen *screen;
      gint       monitor;

      monitor = gimp_get_monitor_at_pointer (&screen);

      gimp_window_strategy_show_dockable_dialog (
        GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (dashboard->gimp)),
        dashboard->gimp,
        gimp_dialog_factory_get_singleton (),
        screen, monitor,
        "gimp-dashboard");

      gimp_dashboard_update (dashboard);

      g_mutex_lock (&dashboard->mutex);

      dashboard->low_swap_space_idle_id = 0;

      g_mutex_unlock (&dashboard->mutex);
    }

  return G_SOURCE_REMOVE;
}

static gpointer
gimp_dashboard_sample (GimpDashboard *dashboard)
{
  GimpDashboardUpdateInteval update_interval;
  gint64                     end_time;
  gboolean                   seen_low_swap_space = FALSE;

  g_mutex_lock (&dashboard->mutex);

  update_interval = dashboard->update_interval;
  end_time        = g_get_monotonic_time ();

  while (! dashboard->quit)
    {
      if (! g_cond_wait_until (&dashboard->cond, &dashboard->mutex, end_time))
        {
          /* cache */
          {
            guint64 cache_occupied;
            gdouble sample[1];

            g_object_get (gegl_stats (),
                          "tile-cache-total", &cache_occupied,
                          NULL);

            sample[0] = cache_occupied;
            gimp_meter_add_sample (GIMP_METER (dashboard->cache_meter), sample);
          }

          /* swap */
          {
            guint64 swap_occupied;
            guint64 swap_size;
            gdouble sample[2];

            g_object_get (gegl_stats (),
                          "swap-total",     &swap_occupied,
                          "swap-file-size", &swap_size,
                          NULL);

            sample[0] = swap_size;
            sample[1] = swap_occupied;
            gimp_meter_add_sample (GIMP_METER (dashboard->swap_meter), sample);

            if (dashboard->low_swap_space_warning)
              {
                guint64 swap_limit = gimp_dashboard_get_swap_limit (swap_size);

                if (swap_limit)
                  {
                    if (! seen_low_swap_space &&
                        swap_occupied >= LOW_SWAP_SPACE_WARNING_ON * swap_limit)
                      {
                        if (! dashboard->low_swap_space_idle_id)
                          {
                            dashboard->low_swap_space_idle_id =
                              g_idle_add_full (G_PRIORITY_HIGH,
                                               (GSourceFunc) gimp_dashboard_low_swap_space,
                                               dashboard, NULL);
                          }

                        seen_low_swap_space = TRUE;
                      }
                    else if (seen_low_swap_space &&
                             swap_occupied <= LOW_SWAP_SPACE_WARNING_OFF * swap_limit)
                      {
                        if (dashboard->low_swap_space_idle_id)
                          {
                            g_source_remove (dashboard->low_swap_space_idle_id);

                            dashboard->low_swap_space_idle_id = 0;
                          }

                        seen_low_swap_space = FALSE;
                      }
                  }
              }
          }

          end_time = g_get_monotonic_time () +
                     update_interval * G_TIME_SPAN_SECOND / 1000;
        }

      if (dashboard->update_interval != update_interval)
        {
          update_interval = dashboard->update_interval;
          end_time        = g_get_monotonic_time () +
                            update_interval * G_TIME_SPAN_SECOND / 1000;
        }
    }

  g_mutex_unlock (&dashboard->mutex);

  return NULL;
}

static void
gimp_dashboard_label_set_text (GtkLabel    *label,
                               const gchar *text)
{
  /* the strcmp() reduces the overhead of gtk_label_set_text() when the text
   * isn't changed
   */
  if (strcmp (gtk_label_get_text (label), text))
    gtk_label_set_text (label, text);
}

static void
gimp_dashboard_label_set_size (GtkLabel *label,
                               guint64   size,
                               guint64   limit)
{
  gchar *text;

  text = g_format_size_full (size, G_FORMAT_SIZE_IEC_UNITS);

  if (limit)
    {
      gchar *temp;

      temp = g_strdup_printf ("%s (%d%%)", text, ROUND (100.0 * size / limit));
      g_free (text);

      text = temp;
    }

  gimp_dashboard_label_set_text (label, text);

  g_free (text);
}

static guint64
gimp_dashboard_get_swap_limit (guint64 swap_size)
{
  static guint64  free_space      = 0;
  static gboolean has_free_space  = FALSE;
  static gint64   last_check_time = 0;
  gint64          time;

  /* we don't have a config option for limiting the swap size, so we simply
   * return the free space available on the filesystem containing the swap
   */

  time = g_get_monotonic_time ();

  if (time - last_check_time >= G_TIME_SPAN_SECOND)
    {
      gchar *swap_dir;

      g_object_get (gegl_config (),
                    "swap", &swap_dir,
                    NULL);

      free_space     = 0;
      has_free_space = FALSE;

      if (swap_dir)
        {
          GFile     *file;
          GFileInfo *info;

          file = g_file_new_for_path (swap_dir);

          info = g_file_query_filesystem_info (file,
                                               G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
                                               NULL, NULL);

          if (info)
            {
              free_space     =
                g_file_info_get_attribute_uint64 (info,
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
              has_free_space = TRUE;

              g_object_unref (info);
            }

          g_object_unref (file);

          g_free (swap_dir);
        }

      last_check_time = time;
    }

  /* the swap limit is the sum of free_space and swap_size, since the swap
   * itself occupies space in the filesystem
   */
  return has_free_space ? free_space + swap_size : 0;
}


/*  public functions  */


GtkWidget *
gimp_dashboard_new (Gimp            *gimp,
                    GimpMenuFactory *menu_factory)
{
  GimpDashboard *dashboard;

  dashboard = g_object_new (GIMP_TYPE_DASHBOARD,
                            "menu-factory",    menu_factory,
                            "menu-identifier", "<Dashboard>",
                            "ui-path",         "/dashboard-popup",
                            NULL);

  dashboard->gimp = gimp;

  return GTK_WIDGET (dashboard);
}

void
gimp_dashboard_set_update_interval (GimpDashboard              *dashboard,
                                    GimpDashboardUpdateInteval  update_interval)
{
  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  if (update_interval != dashboard->update_interval)
    {
      g_mutex_lock (&dashboard->mutex);

      dashboard->update_interval = update_interval;

      gimp_meter_set_history_resolution (GIMP_METER (dashboard->cache_meter),
                                         update_interval / 1000.0);
      gimp_meter_set_history_resolution (GIMP_METER (dashboard->swap_meter),
                                         update_interval / 1000.0);

      if (dashboard->timeout_id)
        {
          g_source_remove (dashboard->timeout_id);

          dashboard->timeout_id = g_timeout_add (update_interval,
                                                 (GSourceFunc) gimp_dashboard_update,
                                                 dashboard);
        }

      g_cond_signal (&dashboard->cond);

      g_mutex_unlock (&dashboard->mutex);
    }
}

void
gimp_dashboard_set_history_duration (GimpDashboard                *dashboard,
                                     GimpDashboardHistoryDuration  history_duration)
{
  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  if (history_duration != dashboard->history_duration)
    {
      dashboard->history_duration = history_duration;

      gimp_meter_set_history_duration (GIMP_METER (dashboard->cache_meter),
                                       history_duration / 1000.0);
      gimp_meter_set_history_duration (GIMP_METER (dashboard->swap_meter),
                                       history_duration / 1000.0);
    }
}

void
gimp_dashboard_set_low_swap_space_warning (GimpDashboard *dashboard,
                                           gboolean       low_swap_space_warning)
{
  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  if (low_swap_space_warning != dashboard->low_swap_space_warning)
    {
      g_mutex_lock (&dashboard->mutex);

      dashboard->low_swap_space_warning = low_swap_space_warning;

      g_mutex_unlock (&dashboard->mutex);
    }
}
