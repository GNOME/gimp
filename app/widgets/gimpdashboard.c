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

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#define HAVE_CPU_GROUP
#elif defined (G_OS_WIN32)
#include <windows.h>
#define HAVE_CPU_GROUP
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpactiongroup.h"
#include "gimpdocked.h"
#include "gimpdashboard.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimpmeter.h"
#include "gimpsessioninfo-aux.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define DEFAULT_UPDATE_INTERVAL        GIMP_DASHBOARD_UPDATE_INTERVAL_0_25_SEC
#define DEFAULT_HISTORY_DURATION       GIMP_DASHBOARD_HISTORY_DURATION_60_SEC
#define DEFAULT_LOW_SWAP_SPACE_WARNING TRUE

#define LOW_SWAP_SPACE_WARNING_ON      /* swap occupied is above */ 0.90 /* of swap limit */
#define LOW_SWAP_SPACE_WARNING_OFF     /* swap occupied is below */ 0.85 /* of swap limit */

#define CPU_ACTIVE_ON                  /* individual cpu usage is above */ 0.75
#define CPU_ACTIVE_OFF                 /* individual cpu usage is below */ 0.25


typedef enum
{
  VARIABLE_NONE,
  FIRST_VARIABLE,


  /* cache */
  VARIABLE_CACHE_OCCUPIED = FIRST_VARIABLE,
  VARIABLE_CACHE_MAXIMUM,
  VARIABLE_CACHE_LIMIT,

  VARIABLE_CACHE_COMPRESSION,
  VARIABLE_CACHE_HIT_MISS,

  /* swap */
  VARIABLE_SWAP_OCCUPIED,
  VARIABLE_SWAP_SIZE,
  VARIABLE_SWAP_LIMIT,

  VARIABLE_SWAP_BUSY,

#ifdef HAVE_CPU_GROUP
  /* cpu */
  VARIABLE_CPU_USAGE,
  VARIABLE_CPU_ACTIVE,
  VARIABLE_CPU_ACTIVE_TIME,
#endif

  /* misc */
  VARIABLE_MIPMAPED,


  N_VARIABLES,

  VARIABLE_SEPARATOR
} Variable;

typedef enum
{
  VARIABLE_TYPE_BOOLEAN,
  VARIABLE_TYPE_SIZE,
  VARIABLE_TYPE_SIZE_RATIO,
  VARIABLE_TYPE_INT_RATIO,
  VARIABLE_TYPE_PERCENTAGE,
  VARIABLE_TYPE_DURATION
} VariableType;

typedef enum
{
  FIRST_GROUP,

  GROUP_CACHE = FIRST_GROUP,
  GROUP_SWAP,
#ifdef HAVE_CPU_GROUP
  GROUP_CPU,
#endif
  GROUP_MISC,

  N_GROUPS
} Group;


typedef struct _VariableInfo  VariableInfo;
typedef struct _FieldInfo     FieldInfo;
typedef struct _GroupInfo     GroupInfo;
typedef struct _VariableData  VariableData;
typedef struct _FieldData     FieldData;
typedef struct _GroupData     GroupData;

typedef void (* VariableFunc) (GimpDashboard *dashboard,
                               Variable       variable);


struct _VariableInfo
{
  const gchar  *name;
  const gchar  *title;
  const gchar  *description;
  VariableType  type;
  GimpRGB       color;
  VariableFunc  sample_func;
  VariableFunc  reset_func;
  gpointer      data;
};

struct _FieldInfo
{
  Variable variable;
  gboolean default_active;
  gboolean show_in_header;
  Variable meter_variable;
  gint     meter_value;
};

struct _GroupInfo
{
  const gchar     *name;
  const gchar     *title;
  const gchar     *description;
  gboolean         default_active;
  gboolean         default_expanded;
  gboolean         has_meter;
  Variable         meter_limit;
  Variable         meter_led;
  const FieldInfo *fields;
};

struct _VariableData
{
  gboolean available;

  union
  {
    gboolean  boolean;
    guint64   size;       /* in bytes    */
    struct
    {
      guint64 antecedent;
      guint64 consequent;
    } size_ratio;
    struct
    {
      gint    antecedent;
      gint    consequent;
    } int_ratio;
    gdouble   percentage; /* from 0 to 1 */
    gdouble   duration;   /* in seconds  */
  } value;
};

struct _FieldData
{
  gboolean          active;

  GtkCheckMenuItem *menu_item;
  GtkLabel         *value_label;
};

struct _GroupData
{
  gint             n_fields;
  gint             n_meter_values;

  gboolean         active;
  gdouble          limit;

  GtkToggleAction *action;
  GtkExpander     *expander;
  GtkLabel        *header_values_label;
  GtkButton       *menu_button;
  GtkMenu         *menu;
  GimpMeter       *meter;
  GtkTable        *table;

  FieldData       *fields;
};

struct _GimpDashboardPrivate
{
  Gimp                         *gimp;

  VariableData                  variables[N_VARIABLES];
  GroupData                     groups[N_GROUPS];

  GThread                      *thread;
  GMutex                        mutex;
  GCond                         cond;
  gboolean                      quit;
  gboolean                      update_now;

  gint                          update_idle_id;
  gint                          low_swap_space_idle_id;

  GimpDashboardUpdateInteval    update_interval;
  GimpDashboardHistoryDuration  history_duration;
  gboolean                      low_swap_space_warning;
};


/*  local function prototypes  */

static void       gimp_dashboard_docked_iface_init           (GimpDockedInterface *iface);

static void       gimp_dashboard_constructed                 (GObject             *object);
static void       gimp_dashboard_dispose                     (GObject             *object);
static void       gimp_dashboard_finalize                    (GObject             *object);

static void       gimp_dashboard_map                         (GtkWidget           *widget);
static void       gimp_dashboard_unmap                       (GtkWidget           *widget);

static void       gimp_dashboard_set_aux_info                (GimpDocked          *docked,
                                                              GList               *aux_info);
static GList    * gimp_dashboard_get_aux_info                (GimpDocked          *docked);

static gboolean   gimp_dashboard_group_expander_button_press (GimpDashboard       *dashboard,
                                                              GdkEventButton      *bevent,
                                                              GtkWidget           *widget);

static void       gimp_dashboard_group_action_toggled        (GimpDashboard       *dashboard,
                                                              GtkToggleAction     *action);
static void       gimp_dashboard_field_menu_item_toggled     (GimpDashboard       *dashboard,
                                                              GtkCheckMenuItem    *item);

static gpointer   gimp_dashboard_sample                      (GimpDashboard       *dashboard);

static gboolean   gimp_dashboard_update                      (GimpDashboard       *dashboard);
static gboolean   gimp_dashboard_low_swap_space              (GimpDashboard       *dashboard);

static void       gimp_dashboard_sample_gegl_config          (GimpDashboard       *dashboard,
                                                              Variable             variable);
static void       gimp_dashboard_sample_gegl_stats           (GimpDashboard       *dashboard,
                                                              Variable             variable);
static void       gimp_dashboard_sample_swap_limit           (GimpDashboard       *dashboard,
                                                              Variable             variable);
#ifdef HAVE_CPU_GROUP
static void       gimp_dashboard_sample_cpu_usage            (GimpDashboard       *dashboard,
                                                              Variable             variable);
static void       gimp_dashboard_sample_cpu_active           (GimpDashboard       *dashboard,
                                                              Variable             variable);
static void       gimp_dashboard_reset_cpu_active            (GimpDashboard       *dashboard,
                                                              Variable             variable);
static void       gimp_dashboard_sample_cpu_active_time      (GimpDashboard       *dashboard,
                                                              Variable             variable);
static void       gimp_dashboard_reset_cpu_active_time       (GimpDashboard       *dashboard,
                                                              Variable             variable);
#endif /* HAVE_CPU_GROUP */

static void       gimp_dashboard_sample_object               (GimpDashboard       *dashboard,
                                                              GObject             *object,
                                                              Variable             variable);

static void       gimp_dashboard_container_remove            (GtkWidget           *widget,
                                                              GtkContainer        *container);

static void       gimp_dashboard_group_menu_position         (GtkMenu             *menu,
                                                              gint                *x,
                                                              gint                *y,
                                                              gboolean            *push_in,
                                                              gpointer             user_data);

static void       gimp_dashboard_update_groups               (GimpDashboard       *dashboard);
static void       gimp_dashboard_update_group                (GimpDashboard       *dashboard,
                                                              Group                group);
static void       gimp_dashboard_update_group_values         (GimpDashboard       *dashboard,
                                                              Group                group);

static void       gimp_dashboard_group_set_active            (GimpDashboard       *dashboard,
                                                              Group                group,
                                                              gboolean             active);
static void       gimp_dashboard_field_set_active            (GimpDashboard       *dashboard,
                                                              Group                group,
                                                              gint                 field,
                                                              gboolean             active);

static gboolean   gimp_dashboard_variable_to_boolean         (GimpDashboard       *dashboard,
                                                              Variable             variable);
static gdouble    gimp_dashboard_variable_to_double          (GimpDashboard       *dashboard,
                                                              Variable             variable);

static gchar    * gimp_dashboard_field_to_string             (GimpDashboard       *dashboard,
                                                              Group                group,
                                                              gint                 field,
                                                              gboolean             full);

static void       gimp_dashboard_label_set_text              (GtkLabel            *label,
                                                              const gchar         *text);


/*  static variables  */

static const VariableInfo variables[] =
{
  /* cache variables */

  [VARIABLE_CACHE_OCCUPIED] =
  { .name             = "cache-occupied",
    .title            = NC_("dashboard-variable", "Occupied"),
    .description      = N_("Tile cache occupied size"),
    .type             = VARIABLE_TYPE_SIZE,
    .color            = {0.3, 0.6, 0.3, 1.0},
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "tile-cache-total"
  },

  [VARIABLE_CACHE_MAXIMUM] =
  { .name             = "cache-maximum",
    .title            = NC_("dashboard-variable", "Maximum"),
    .description      = N_("Maximal tile cache occupied size"),
    .type             = VARIABLE_TYPE_SIZE,
    .color            = {0.3, 0.7, 0.8, 1.0},
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "tile-cache-total-max"
  },

  [VARIABLE_CACHE_LIMIT] =
  { .name             = "cache-limit",
    .title            = NC_("dashboard-variable", "Limit"),
    .description      = N_("Tile cache size limit"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = gimp_dashboard_sample_gegl_config,
    .data             = "tile-cache-size"
  },

  [VARIABLE_CACHE_COMPRESSION] =
  { .name             = "cache-compression",
    .title            = NC_("dashboard-variable", "Compression"),
    .description      = N_("Tile cache compression ratio"),
    .type             = VARIABLE_TYPE_SIZE_RATIO,
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "tile-cache-total\0"
                        "tile-cache-total-uncloned"
  },

  [VARIABLE_CACHE_HIT_MISS] =
  { .name             = "cache-hit-miss",
    .title            = NC_("dashboard-variable", "Hit/Miss"),
    .description      = N_("Tile cache hit/miss ratio"),
    .type             = VARIABLE_TYPE_INT_RATIO,
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "tile-cache-hits\0"
                        "tile-cache-misses"
  },


  /* swap variables */

  [VARIABLE_SWAP_OCCUPIED] =
  { .name             = "swap-occupied",
    .title            = NC_("dashboard-variable", "Occupied"),
    .description      = N_("Swap file occupied size"),
    .type             = VARIABLE_TYPE_SIZE,
    .color            = {0.8, 0.2, 0.2, 1.0},
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "swap-total"
  },

  [VARIABLE_SWAP_SIZE] =
  { .name             = "swap-size",
    .title            = NC_("dashboard-variable", "Size"),
    .description      = N_("Swap file size"),
    .type             = VARIABLE_TYPE_SIZE,
    .color            = {0.8, 0.6, 0.4, 1.0},
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "swap-file-size"
  },

  [VARIABLE_SWAP_LIMIT] =
  { .name             = "swap-limit",
    .title            = NC_("dashboard-variable", "Limit"),
    .description      = N_("Swap file size limit"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = gimp_dashboard_sample_swap_limit,
  },

  [VARIABLE_SWAP_BUSY] =
  { .name             = "swap-busy",
    .title            = NC_("dashboard-variable", "Busy"),
    .description      = N_("Whether there is work queued for the swap file"),
    .type             = VARIABLE_TYPE_BOOLEAN,
    .color            = {0.8, 0.4, 0.4, 1.0},
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "swap-busy"
  },


#ifdef HAVE_CPU_GROUP
  /* cpu variables */

  [VARIABLE_CPU_USAGE] =
  { .name             = "cpu-usage",
    .title            = NC_("dashboard-variable", "Usage"),
    .description      = N_("Total CPU usage"),
    .type             = VARIABLE_TYPE_PERCENTAGE,
    .color            = {0.8, 0.7, 0.2, 1.0},
    .sample_func      = gimp_dashboard_sample_cpu_usage
  },

  [VARIABLE_CPU_ACTIVE] =
  { .name             = "cpu-active",
    .title            = NC_("dashboard-variable", "Active"),
    .description      = N_("Whether the CPU is active"),
    .type             = VARIABLE_TYPE_BOOLEAN,
    .color            = {0.9, 0.8, 0.3, 1.0},
    .sample_func      = gimp_dashboard_sample_cpu_active,
    .reset_func       = gimp_dashboard_reset_cpu_active
  },

  [VARIABLE_CPU_ACTIVE_TIME] =
  { .name             = "cpu-active-time",
    .title            = NC_("dashboard-variable", "Active"),
    .description      = N_("Total amount of time the CPU has been active"),
    .type             = VARIABLE_TYPE_DURATION,
    .color            = {0.8, 0.7, 0.2, 0.4},
    .sample_func      = gimp_dashboard_sample_cpu_active_time,
    .reset_func       = gimp_dashboard_reset_cpu_active_time
  },
#endif /* HAVE_CPU_GROUP */


  /* misc variables */

  [VARIABLE_MIPMAPED] =
  { .name             = "mipmapped",
    .title            = NC_("dashboard-variable", "Mipmapped"),
    .description      = N_("Total size of processed mipmapped data"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = gimp_dashboard_sample_gegl_stats,
    .data             = "zoom-total"
  },
};

static const GroupInfo groups[] =
{
  /* cache group */
  [GROUP_CACHE] =
  { .name             = "cache",
    .title            = NC_("dashboard-group", "Cache"),
    .description      = N_("In-memory tile cache"),
    .default_active   = TRUE,
    .default_expanded = TRUE,
    .has_meter        = TRUE,
    .meter_limit      = VARIABLE_CACHE_LIMIT,
    .fields           = (const FieldInfo[])
                        {
                          { .variable       = VARIABLE_CACHE_OCCUPIED,
                            .default_active = TRUE,
                            .show_in_header = TRUE,
                            .meter_value    = 2
                          },
                          { .variable       = VARIABLE_CACHE_MAXIMUM,
                            .default_active = FALSE,
                            .meter_value    = 1
                          },
                          { .variable       = VARIABLE_CACHE_LIMIT,
                            .default_active = TRUE
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable       = VARIABLE_CACHE_COMPRESSION,
                            .default_active = FALSE
                          },
                          { .variable       = VARIABLE_CACHE_HIT_MISS,
                            .default_active = FALSE
                          },

                          {}
                        }
  },

  /* swap group */
  [GROUP_SWAP] =
  { .name             = "swap",
    .title            = NC_("dashboard-group", "Swap"),
    .description      = N_("On-disk tile swap"),
    .default_active   = TRUE,
    .default_expanded = TRUE,
    .has_meter        = TRUE,
    .meter_limit      = VARIABLE_SWAP_LIMIT,
    .meter_led        = VARIABLE_SWAP_BUSY,
    .fields           = (const FieldInfo[])
                        {
                          { .variable       = VARIABLE_SWAP_OCCUPIED,
                            .default_active = TRUE,
                            .show_in_header = TRUE,
                            .meter_value    = 2
                          },
                          { .variable       = VARIABLE_SWAP_SIZE,
                            .default_active = TRUE,
                            .meter_value    = 1
                          },
                          { .variable       = VARIABLE_SWAP_LIMIT,
                            .default_active = TRUE
                          },

                          {}
                        }
  },

#ifdef HAVE_CPU_GROUP
  /* cpu group */
  [GROUP_CPU] =
  { .name             = "cpu",
    .title            = NC_("dashboard-group", "CPU"),
    .description      = N_("CPU usage"),
    .default_active   = TRUE,
    .default_expanded = FALSE,
    .has_meter        = TRUE,
    .meter_led        = VARIABLE_CPU_ACTIVE,
    .fields           = (const FieldInfo[])
                        {
                          { .variable       = VARIABLE_CPU_USAGE,
                            .default_active = TRUE,
                            .show_in_header = TRUE,
                            .meter_value    = 2
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable       = VARIABLE_CPU_ACTIVE_TIME,
                            .default_active = FALSE,
                            .meter_variable = VARIABLE_CPU_ACTIVE,
                            .meter_value    = 1
                          },

                          {}
                        }
  },
#endif /* HAVE_CPU_GROUP */

  /* misc group */
  [GROUP_MISC] =
  { .name             = "misc",
    .title            = NC_("dashboard-group", "Misc"),
    .description      = N_("Miscellaneous information"),
    .default_active   = FALSE,
    .default_expanded = FALSE,
    .has_meter        = FALSE,
    .fields           = (const FieldInfo[])
                        {
                          { .variable       = VARIABLE_MIPMAPED,
                            .default_active = TRUE
                          },

                          {}
                        }
  },
};


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

  object_class->constructed = gimp_dashboard_constructed;
  object_class->dispose     = gimp_dashboard_dispose;
  object_class->finalize    = gimp_dashboard_finalize;

  widget_class->map         = gimp_dashboard_map;
  widget_class->unmap       = gimp_dashboard_unmap;

  g_type_class_add_private (klass, sizeof (GimpDashboardPrivate));
}

static void
gimp_dashboard_init (GimpDashboard *dashboard)
{
  GimpDashboardPrivate *priv;
  GtkWidget            *box;
  GtkWidget            *vbox;
  GtkWidget            *expander;
  GtkWidget            *hbox;
  GtkWidget            *button;
  GtkWidget            *image;
  GtkWidget            *menu;
  GtkWidget            *item;
  GtkWidget            *frame;
  GtkWidget            *vbox2;
  GtkWidget            *meter;
  GtkWidget            *table;
  GtkWidget            *label;
  gint                  content_spacing;
  Group                 group;
  gint                  field;

  priv = dashboard->priv = G_TYPE_INSTANCE_GET_PRIVATE (dashboard,
                                                        GIMP_TYPE_DASHBOARD,
                                                        GimpDashboardPrivate);

  g_mutex_init (&priv->mutex);
  g_cond_init (&priv->cond);

  priv->update_interval        = DEFAULT_UPDATE_INTERVAL;
  priv->history_duration       = DEFAULT_HISTORY_DURATION;
  priv->low_swap_space_warning = DEFAULT_LOW_SWAP_SPACE_WARNING;

  gtk_widget_style_get (GTK_WIDGET (dashboard),
                        "content-spacing", &content_spacing,
                        NULL);

  /* we put the dashboard inside an event box, so that it gets its own window,
   * which reduces the overhead of updating the ui, since it gets updated
   * frequently.  unfortunately, this means that the dashboard's background
   * color may be a bit off for some themes.
   */
  box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (dashboard), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  /* main vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2 * content_spacing);
  gtk_container_add (GTK_CONTAINER (box), vbox);
  gtk_widget_show (vbox);

  /* construct the groups */
  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      GroupData       *group_data = &priv->groups[group];

      group_data->n_fields       = 0;
      group_data->n_meter_values = 0;

      for (field = 0; group_info->fields[field].variable; field++)
        {
          const FieldInfo *field_info = &group_info->fields[field];

          group_data->n_fields++;
          group_data->n_meter_values = MAX (group_data->n_meter_values,
                                            field_info->meter_value);
        }

      group_data->fields = g_new0 (FieldData, group_data->n_fields);

      /* group expander */
      expander = gtk_expander_new (NULL);
      group_data->expander = GTK_EXPANDER (expander);
      gtk_expander_set_expanded (GTK_EXPANDER (expander),
                                 group_info->default_expanded);
      gtk_expander_set_label_fill (GTK_EXPANDER (expander), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);

      g_object_set_data (G_OBJECT (expander),
                         "gimp-dashboard-group", GINT_TO_POINTER (group));
      g_signal_connect_swapped (expander, "button-press-event",
                                G_CALLBACK (gimp_dashboard_group_expander_button_press),
                                dashboard);

      /* group expander label box */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gimp_help_set_help_data (hbox,
                               g_dgettext (NULL, group_info->description),
                               NULL);
      gtk_expander_set_label_widget (GTK_EXPANDER (expander), hbox);
      gtk_widget_show (hbox);

      /* group expander label */
      label = gtk_label_new (g_dpgettext2 (NULL, "dashboard-group",
                                           group_info->title));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 -1);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      /* group expander values label */
      label = gtk_label_new (NULL);
      group_data->header_values_label = GTK_LABEL (label);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

      g_object_bind_property (expander, "expanded",
                              label,    "visible",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_INVERT_BOOLEAN);

      /* group expander menu button */
      button = gtk_button_new ();
      group_data->menu_button = GTK_BUTTON (button);
      gimp_help_set_help_data (button, _("Select fields"),
                               NULL);
      gtk_widget_set_can_focus (button, FALSE);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      image = gtk_image_new_from_icon_name (GIMP_ICON_MENU_LEFT,
                                            GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
      gtk_image_set_from_icon_name (GTK_IMAGE (image), GIMP_ICON_MENU_LEFT,
                                    GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      /* group menu */
      menu = gtk_menu_new ();
      group_data->menu = GTK_MENU (menu);
      gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);

      for (field = 0; field < group_data->n_fields; field++)
        {
          const FieldInfo *field_info = &group_info->fields[field];
          FieldData       *field_data = &group_data->fields[field];

          if (field_info->variable != VARIABLE_SEPARATOR)
            {
              const VariableInfo *variable_info = &variables[field_info->variable];

              item = gtk_check_menu_item_new_with_label (
                g_dpgettext2 (NULL, "dashboard-variable", variable_info->title));
              field_data->menu_item = GTK_CHECK_MENU_ITEM (item);
              gimp_help_set_help_data (item,
                                       g_dgettext (NULL, variable_info->description),
                                       NULL);

              g_object_set_data (G_OBJECT (item),
                                 "gimp-dashboard-group", GINT_TO_POINTER (group));
              g_object_set_data (G_OBJECT (item),
                                 "gimp-dashboard-field", GINT_TO_POINTER (field));
              g_signal_connect_swapped (item, "toggled",
                                        G_CALLBACK (gimp_dashboard_field_menu_item_toggled),
                                        dashboard);

              gimp_dashboard_field_set_active (dashboard, group, field,
                                               field_info->default_active);
            }
          else
            {
              item = gtk_separator_menu_item_new ();
            }

          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_show (item);
        }

      /* group frame */
      frame = gimp_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (expander), frame);
      gtk_widget_show (frame);

      /* group vbox */
      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2 * content_spacing);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      /* group meter */
      if (group_info->has_meter)
        {
          meter = gimp_meter_new (group_data->n_meter_values);
          group_data->meter = GIMP_METER (meter);
          gimp_help_set_help_data (meter,
                                   g_dgettext (NULL, group_info->description),
                                   NULL);
          gimp_meter_set_history_resolution (GIMP_METER (meter),
                                             priv->update_interval / 1000.0);
          gimp_meter_set_history_duration (GIMP_METER (meter),
                                           priv->history_duration / 1000.0);
          gtk_box_pack_start (GTK_BOX (vbox2), meter, FALSE, FALSE, 0);
          gtk_widget_show (meter);

          if (group_info->meter_led)
            {
              gimp_meter_set_led_color (GIMP_METER (meter),
                                        &variables[group_info->meter_led].color);
            }

          for (field = 0; field < group_data->n_fields; field++)
            {
              const FieldInfo *field_info = &group_info->fields[field];

              if (field_info->meter_value)
                {
                  const VariableInfo *variable_info       = &variables[field_info->variable];
                  const VariableInfo *meter_variable_info = variable_info;

                  if (field_info->meter_variable)
                    meter_variable_info = &variables[field_info->meter_variable];

                  gimp_meter_set_value_color (GIMP_METER (meter),
                                              field_info->meter_value - 1,
                                              &variable_info->color);

                  if (meter_variable_info->type == VARIABLE_TYPE_BOOLEAN)
                    {
                      gimp_meter_set_value_show_in_gauge (GIMP_METER (meter),
                                                          field_info->meter_value - 1,
                                                          FALSE);
                      gimp_meter_set_value_interpolation (GIMP_METER (meter),
                                                          field_info->meter_value - 1,
                                                          GIMP_INTERPOLATION_NONE);
                    }
                }
            }
        }

      /* group table */
      table = gtk_table_new (1, 1, FALSE);
      group_data->table = GTK_TABLE (table);
      gtk_table_set_row_spacings (GTK_TABLE (table), content_spacing);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      gimp_dashboard_group_set_active (dashboard, group,
                                       group_info->default_active);
      gimp_dashboard_update_group (dashboard, group);
    }

  /* sampler thread
   *
   * we use a separate thread for sampling, so that data is sampled even when
   * the main thread is busy
   */
  priv->thread = g_thread_new ("dashboard",
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
gimp_dashboard_constructed (GObject *object)
{
  GimpDashboard        *dashboard = GIMP_DASHBOARD (object);
  GimpDashboardPrivate *priv = dashboard->priv;
  GimpUIManager        *ui_manager;
  GimpActionGroup      *action_group;
  Group                 group;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ui_manager   = gimp_editor_get_ui_manager (GIMP_EDITOR (dashboard));
  action_group = gimp_ui_manager_get_action_group (ui_manager, "dashboard");

  /* group actions */
  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo       *group_info = &groups[group];
      GroupData             *group_data = &priv->groups[group];
      GimpToggleActionEntry  entry      = {};
      GtkAction             *action;

      entry.name      = g_strdup_printf ("dashboard-group-%s", group_info->name);
      entry.label     = g_dpgettext2 (NULL, "dashboard-group", group_info->title);
      entry.tooltip   = g_dgettext (NULL, group_info->description);
      entry.help_id   = GIMP_HELP_DASHBOARD_GROUPS;
      entry.is_active = group_data->active;

      gimp_action_group_add_toggle_actions (action_group, "dashboard-groups",
                                            &entry, 1);

      action = gimp_ui_manager_find_action (ui_manager, "dashboard", entry.name);
      group_data->action = GTK_TOGGLE_ACTION (action);

      g_object_set_data (G_OBJECT (action),
                         "gimp-dashboard-group", GINT_TO_POINTER (group));
      g_signal_connect_swapped (action, "toggled",
                                G_CALLBACK (gimp_dashboard_group_action_toggled),
                                dashboard);

      g_free ((gpointer) entry.name);
    }

  gimp_editor_add_action_button (GIMP_EDITOR (dashboard), "dashboard",
                                 "dashboard-reset", NULL);
}

static void
gimp_dashboard_dispose (GObject *object)
{
  GimpDashboard        *dashboard = GIMP_DASHBOARD (object);
  GimpDashboardPrivate *priv      = dashboard->priv;

  if (priv->thread)
    {
      g_mutex_lock (&priv->mutex);

      priv->quit = TRUE;
      g_cond_signal (&priv->cond);

      g_mutex_unlock (&priv->mutex);

      g_clear_pointer (&priv->thread, g_thread_join);
    }

  if (priv->update_idle_id)
    {
      g_source_remove (priv->update_idle_id);
      priv->update_idle_id = 0;
    }

  if (priv->low_swap_space_idle_id)
    {
      g_source_remove (priv->low_swap_space_idle_id);
      priv->low_swap_space_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dashboard_finalize (GObject *object)
{
  GimpDashboard        *dashboard = GIMP_DASHBOARD (object);
  GimpDashboardPrivate *priv      = dashboard->priv;
  gint                  i;

  for (i = FIRST_GROUP; i < N_GROUPS; i++)
    g_free (priv->groups[i].fields);

  g_mutex_clear (&priv->mutex);
  g_cond_clear (&priv->cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dashboard_map (GtkWidget *widget)
{
  GimpDashboard *dashboard = GIMP_DASHBOARD (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  gimp_dashboard_update (dashboard);
}

static void
gimp_dashboard_unmap (GtkWidget *widget)
{
  GimpDashboard        *dashboard = GIMP_DASHBOARD (widget);
  GimpDashboardPrivate *priv      = dashboard->priv;

  g_mutex_lock (&priv->mutex);

  if (priv->update_idle_id)
    {
      g_source_remove (priv->update_idle_id);
      priv->update_idle_id = 0;
    }

  g_mutex_unlock (&priv->mutex);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

#define AUX_INFO_UPDATE_INTERVAL        "update-interval"
#define AUX_INFO_HISTORY_DURATION       "history-duration"
#define AUX_INFO_LOW_SWAP_SPACE_WARNING "low-swap-space-warning"

static void
gimp_dashboard_set_aux_info (GimpDocked *docked,
                             GList      *aux_info)
{
  GimpDashboard        *dashboard = GIMP_DASHBOARD (docked);
  GimpDashboardPrivate *priv      = dashboard->priv;
  gchar                *name;
  GList                *list;

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
      else
        {
          Group group;
          gint  field;

          for (group = FIRST_GROUP; group < N_GROUPS; group++)
            {
              const GroupInfo *group_info = &groups[group];
              GroupData       *group_data = &priv->groups[group];

              name = g_strdup_printf ("%s-active", group_info->name);

              if (! strcmp (aux->name, name))
                {
                  gboolean active = ! strcmp (aux->value, "yes");

                  gimp_dashboard_group_set_active (dashboard, group, active);

                  g_free (name);
                  goto next_aux_info;
                }

              g_free (name);

              name = g_strdup_printf ("%s-expanded", group_info->name);

              if (! strcmp (aux->name, name))
                {
                  gboolean expanded = ! strcmp (aux->value, "yes");

                  gtk_expander_set_expanded (group_data->expander, expanded);

                  g_free (name);
                  goto next_aux_info;
                }

              g_free (name);

              for (field = 0; field < group_data->n_fields; field++)
                {
                  const FieldInfo *field_info = &group_info->fields[field];

                  if (field_info->variable != VARIABLE_SEPARATOR)
                    {
                      const VariableInfo *variable_info = &variables[field_info->variable];

                      name = g_strdup_printf ("%s-%s-active",
                                              group_info->name,
                                              variable_info->name);

                      if (! strcmp (aux->name, name))
                        {
                          gboolean active = ! strcmp (aux->value, "yes");

                          gimp_dashboard_field_set_active (dashboard,
                                                           group, field,
                                                           active);

                          g_free (name);
                          goto next_aux_info;
                        }

                      g_free (name);
                    }
                }
            }
        }
next_aux_info: ;
    }

  gimp_dashboard_update_groups (dashboard);
}

static GList *
gimp_dashboard_get_aux_info (GimpDocked *docked)
{
  GimpDashboard        *dashboard = GIMP_DASHBOARD (docked);
  GimpDashboardPrivate *priv      = dashboard->priv;
  GList                *aux_info;
  GimpSessionInfoAux   *aux;
  gchar                *name;
  gchar                *value;
  Group                 group;
  gint                  field;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (priv->update_interval != DEFAULT_UPDATE_INTERVAL)
    {
      value    = g_strdup_printf ("%d", priv->update_interval);
      aux      = gimp_session_info_aux_new (AUX_INFO_UPDATE_INTERVAL, value);
      aux_info = g_list_append (aux_info, aux);
      g_free (value);
    }

  if (priv->history_duration != DEFAULT_HISTORY_DURATION)
    {
      value    = g_strdup_printf ("%d", priv->history_duration);
      aux      = gimp_session_info_aux_new (AUX_INFO_HISTORY_DURATION, value);
      aux_info = g_list_append (aux_info, aux);
      g_free (value);
    }

  if (priv->low_swap_space_warning != DEFAULT_LOW_SWAP_SPACE_WARNING)
    {
      value    = priv->low_swap_space_warning ? "yes" : "no";
      aux      = gimp_session_info_aux_new (AUX_INFO_LOW_SWAP_SPACE_WARNING, value);
      aux_info = g_list_append (aux_info, aux);
    }

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      GroupData       *group_data = &priv->groups[group];
      gboolean         active     = group_data->active;
      gboolean         expanded   = gtk_expander_get_expanded (group_data->expander);

      if (active != group_info->default_active)
        {
          name     = g_strdup_printf ("%s-active", group_info->name);
          value    = active ? "yes" : "no";
          aux      = gimp_session_info_aux_new (name, value);
          aux_info = g_list_append (aux_info, aux);
          g_free (name);
        }

      if (expanded != group_info->default_expanded)
        {
          name     = g_strdup_printf ("%s-expanded", group_info->name);
          value    = expanded ? "yes" : "no";
          aux      = gimp_session_info_aux_new (name, value);
          aux_info = g_list_append (aux_info, aux);
          g_free (name);
        }

      for (field = 0; field < group_data->n_fields; field++)
        {
          const FieldInfo *field_info = &group_info->fields[field];
          FieldData       *field_data = &group_data->fields[field];
          gboolean         active     = field_data->active;

          if (field_info->variable != VARIABLE_SEPARATOR)
            {
              const VariableInfo *variable_info = &variables[field_info->variable];

              if (active != field_info->default_active)
                {
                  name = g_strdup_printf ("%s-%s-active",
                                          group_info->name,
                                          variable_info->name);
                  value    = active ? "yes" : "no";
                  aux      = gimp_session_info_aux_new (name, value);
                  aux_info = g_list_append (aux_info, aux);
                  g_free (name);
                }
            }
        }
    }

  return aux_info;
}

static gboolean
gimp_dashboard_group_expander_button_press (GimpDashboard  *dashboard,
                                            GdkEventButton *bevent,
                                            GtkWidget      *widget)
{
  GimpDashboardPrivate *priv = dashboard->priv;
  Group                 group;
  GroupData            *group_data;
  GtkAllocation         expander_allocation;
  GtkAllocation         allocation;

  group      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                   "gimp-dashboard-group"));
  group_data = &priv->groups[group];

  gtk_widget_get_allocation (GTK_WIDGET (group_data->expander),
                             &expander_allocation);
  gtk_widget_get_allocation (GTK_WIDGET (group_data->menu_button),
                             &allocation);

  allocation.x -= expander_allocation.x;
  allocation.y -= expander_allocation.y;

  if (bevent->button == 1                          &&
      bevent->x >= allocation.x                    &&
      bevent->x <  allocation.x + allocation.width &&
      bevent->y >= allocation.y                    &&
      bevent->y <  allocation.y + allocation.height)
    {
      gtk_menu_popup (group_data->menu,
                      NULL, NULL,
                      gimp_dashboard_group_menu_position,
                      group_data->menu_button,
                      bevent->button, bevent->time);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_dashboard_group_action_toggled (GimpDashboard   *dashboard,
                                     GtkToggleAction *action)
{
  GimpDashboardPrivate *priv = dashboard->priv;
  Group                 group;
  GroupData            *group_data;

  group      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (action),
                                                   "gimp-dashboard-group"));
  group_data = &priv->groups[group];

  group_data->active = gtk_toggle_action_get_active (action);

  gimp_dashboard_update_group (dashboard, group);
}

static void
gimp_dashboard_field_menu_item_toggled (GimpDashboard    *dashboard,
                                        GtkCheckMenuItem *item)
{
  GimpDashboardPrivate *priv = dashboard->priv;
  Group                 group;
  GroupData            *group_data;
  gint                  field;
  FieldData            *field_data;

  group      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                   "gimp-dashboard-group"));
  group_data = &priv->groups[group];

  field      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                   "gimp-dashboard-field"));
  field_data = &group_data->fields[field];

  field_data->active = gtk_check_menu_item_get_active (item);

  gimp_dashboard_update_group (dashboard, group);
}

static gpointer
gimp_dashboard_sample (GimpDashboard *dashboard)
{
  GimpDashboardPrivate       *priv                = dashboard->priv;
  GimpDashboardUpdateInteval  update_interval;
  gint64                      end_time;
  gboolean                    seen_low_swap_space = FALSE;

  g_mutex_lock (&priv->mutex);

  update_interval = priv->update_interval;
  end_time        = g_get_monotonic_time ();

  while (! priv->quit)
    {
      if (! g_cond_wait_until (&priv->cond, &priv->mutex, end_time) ||
          priv->update_now)
        {
          gboolean variables_changed = FALSE;
          Variable variable;
          Group    group;
          gint     field;

          /* sample all variables */
          for (variable = FIRST_VARIABLE; variable < N_VARIABLES; variable++)
            {
              const VariableInfo *variable_info      = &variables[variable];
              const VariableData *variable_data      = &priv->variables[variable];
              VariableData        prev_variable_data = *variable_data;

              variable_info->sample_func (dashboard, variable);

              variables_changed = variables_changed ||
                                  memcmp (variable_data, &prev_variable_data,
                                          sizeof (VariableData));
            }

          /* add samples to meters */
          for (group = FIRST_GROUP; group < N_GROUPS; group++)
            {
              const GroupInfo *group_info = &groups[group];
              GroupData       *group_data = &priv->groups[group];
              gdouble         *sample;

              if (! group_info->has_meter)
                continue;

              sample = g_new (gdouble, group_data->n_meter_values);

              for (field = 0; field < group_data->n_fields; field++)
                {
                  const FieldInfo *field_info = &group_info->fields[field];

                  if (field_info->meter_value)
                    {
                      if (field_info->meter_variable)
                        variable = field_info->meter_variable;
                      else
                        variable = field_info->variable;

                      sample[field_info->meter_value - 1] =
                        gimp_dashboard_variable_to_double (dashboard, variable);
                    }
                }

              gimp_meter_add_sample (group_data->meter, sample);

              g_free (sample);
            }

          if (variables_changed)
            {
              /* enqueue update source */
              if (! priv->update_idle_id &&
                  gtk_widget_get_mapped (GTK_WIDGET (dashboard)))
                {
                  priv->update_idle_id = g_idle_add_full (
                    G_PRIORITY_DEFAULT,
                    (GSourceFunc) gimp_dashboard_update,
                    dashboard, NULL);
                }

              /* check for low swap space */
              if (priv->low_swap_space_warning                      &&
                  priv->variables[VARIABLE_SWAP_OCCUPIED].available &&
                  priv->variables[VARIABLE_SWAP_LIMIT].available)
                {
                  guint64 swap_occupied;
                  guint64 swap_limit;

                  swap_occupied = priv->variables[VARIABLE_SWAP_OCCUPIED].value.size;
                  swap_limit    = priv->variables[VARIABLE_SWAP_LIMIT].value.size;

                  if (! seen_low_swap_space &&
                      swap_occupied >= LOW_SWAP_SPACE_WARNING_ON * swap_limit)
                    {
                      if (! priv->low_swap_space_idle_id)
                        {
                          priv->low_swap_space_idle_id =
                            g_idle_add_full (G_PRIORITY_HIGH,
                                             (GSourceFunc) gimp_dashboard_low_swap_space,
                                             dashboard, NULL);
                        }

                      seen_low_swap_space = TRUE;
                    }
                  else if (seen_low_swap_space &&
                           swap_occupied <= LOW_SWAP_SPACE_WARNING_OFF * swap_limit)
                    {
                      if (priv->low_swap_space_idle_id)
                        {
                          g_source_remove (priv->low_swap_space_idle_id);

                          priv->low_swap_space_idle_id = 0;
                        }

                      seen_low_swap_space = FALSE;
                    }
                }
            }

          priv->update_now = FALSE;

          end_time = g_get_monotonic_time () +
                     update_interval * G_TIME_SPAN_SECOND / 1000;
        }

      if (priv->update_interval != update_interval)
        {
          update_interval = priv->update_interval;
          end_time        = g_get_monotonic_time () +
                            update_interval * G_TIME_SPAN_SECOND / 1000;
        }
    }

  g_mutex_unlock (&priv->mutex);

  return NULL;
}

static gboolean
gimp_dashboard_update (GimpDashboard *dashboard)
{
  GimpDashboardPrivate *priv = dashboard->priv;
  Group                 group;

  g_mutex_lock (&priv->mutex);

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      GroupData       *group_data = &priv->groups[group];

      if (group_info->has_meter && group_info->meter_led)
        {
          gboolean active;

          active = gimp_dashboard_variable_to_boolean (dashboard,
                                                       group_info->meter_led);

          gimp_meter_set_led_active (group_data->meter, active);
        }

      gimp_dashboard_update_group_values (dashboard, group);
    }

  priv->update_idle_id = 0;

  g_mutex_unlock (&priv->mutex);

  return G_SOURCE_REMOVE;
}

static gboolean
gimp_dashboard_low_swap_space (GimpDashboard *dashboard)
{
  GimpDashboardPrivate *priv = dashboard->priv;

  if (priv->gimp)
    {
      GdkMonitor *monitor;
      gint        field;

      gtk_expander_set_expanded (priv->groups[GROUP_SWAP].expander, TRUE);

      for (field = 0; field < priv->groups[GROUP_SWAP].n_fields; field++)
        {
          const FieldInfo *field_info = &groups[GROUP_SWAP].fields[field];

          if (field_info->variable == VARIABLE_SWAP_OCCUPIED ||
              field_info->variable == VARIABLE_SWAP_LIMIT)
            {
              gimp_dashboard_field_set_active (dashboard,
                                               GROUP_SWAP, field, TRUE);
            }
        }

      gimp_dashboard_update_groups (dashboard);

      monitor = gimp_get_monitor_at_pointer ();

      gimp_window_strategy_show_dockable_dialog (
        GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (priv->gimp)),
        priv->gimp,
        gimp_dialog_factory_get_singleton (),
        monitor,
        "gimp-dashboard");

      g_mutex_lock (&priv->mutex);

      priv->low_swap_space_idle_id = 0;

      g_mutex_unlock (&priv->mutex);
    }

  return G_SOURCE_REMOVE;
}

static void
gimp_dashboard_sample_gegl_config (GimpDashboard *dashboard,
                                   Variable       variable)
{
  gimp_dashboard_sample_object (dashboard, G_OBJECT (gegl_config ()), variable);
}

static void
gimp_dashboard_sample_gegl_stats (GimpDashboard *dashboard,
                                  Variable       variable)
{
  gimp_dashboard_sample_object (dashboard, G_OBJECT (gegl_stats ()), variable);
}

static void
gimp_dashboard_sample_swap_limit (GimpDashboard *dashboard,
                                  Variable       variable)
{
  GimpDashboardPrivate *priv            = dashboard->priv;
  VariableData         *variable_data   = &priv->variables[variable];
  static guint64        free_space      = 0;
  static gboolean       has_free_space  = FALSE;
  static gint64         last_check_time = 0;
  gint64                time;

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

  variable_data->available = has_free_space;

  if (has_free_space)
    {
      variable_data->value.size = free_space;

      if (priv->variables[VARIABLE_SWAP_SIZE].available)
        {
          /* the swap limit is the sum of free_space and swap_size, since the
           * swap itself occupies space in the filesystem
           */
          variable_data->value.size +=
            priv->variables[VARIABLE_SWAP_SIZE].value.size;
        }
    }
}

#ifdef HAVE_CPU_GROUP

#ifdef HAVE_SYS_TIMES_H

static void
gimp_dashboard_sample_cpu_usage (GimpDashboard *dashboard,
                                 Variable       variable)
{
  GimpDashboardPrivate *priv            = dashboard->priv;
  VariableData         *variable_data   = &priv->variables[variable];
  static clock_t        prev_clock      = 0;
  static clock_t        prev_usage;
  clock_t               curr_clock;
  clock_t               curr_usage;
  struct tms            tms;

  curr_clock = times (&tms);

  if (curr_clock == (clock_t) -1)
    {
      prev_clock = 0;

      variable_data->available = FALSE;

      return;
    }

  curr_usage = tms.tms_utime + tms.tms_stime;

  if (prev_clock && curr_clock != prev_clock)
    {
      variable_data->available         = TRUE;
      variable_data->value.percentage  = (gdouble) (curr_usage - prev_usage) /
                                                   (curr_clock - prev_clock);
      variable_data->value.percentage /= g_get_num_processors ();
    }
  else
    {
      variable_data->available         = FALSE;
    }

  prev_clock = curr_clock;
  prev_usage = curr_usage;
}

#elif defined (G_OS_WIN32)

static void
gimp_dashboard_sample_cpu_usage (GimpDashboard *dashboard,
                                 Variable       variable)
{
  GimpDashboardPrivate *priv            = dashboard->priv;
  VariableData         *variable_data   = &priv->variables[variable];
  static guint64        prev_time       = 0;
  static guint64        prev_usage;
  guint64               curr_time;
  guint64               curr_usage;
  FILETIME              system_time;
  FILETIME              process_creation_time;
  FILETIME              process_exit_time;
  FILETIME              process_kernel_time;
  FILETIME              process_user_time;

  if (! GetProcessTimes (GetCurrentProcess (),
                         &process_creation_time,
                         &process_exit_time,
                         &process_kernel_time,
                         &process_user_time))
    {
      prev_time = 0;

      variable_data->available = FALSE;

      return;
    }

  GetSystemTimeAsFileTime (&system_time);

  curr_time   = ((guint64) system_time.dwHighDateTime << 32) |
                 (guint64) system_time.dwLowDateTime;

  curr_usage  = ((guint64) process_kernel_time.dwHighDateTime << 32) |
                 (guint64) process_kernel_time.dwLowDateTime;
  curr_usage += ((guint64) process_user_time.dwHighDateTime << 32) |
                 (guint64) process_user_time.dwLowDateTime;

  if (prev_time && curr_time != prev_time)
    {
      variable_data->available         = TRUE;
      variable_data->value.percentage  = (gdouble) (curr_usage - prev_usage) /
                                                   (curr_time  - prev_time);
      variable_data->value.percentage /= g_get_num_processors ();
    }
  else
    {
      variable_data->available         = FALSE;
    }

  prev_time  = curr_time;
  prev_usage = curr_usage;
}

#endif /* G_OS_WIN32 */

static gboolean cpu_active = FALSE;

static void
gimp_dashboard_sample_cpu_active (GimpDashboard *dashboard,
                                  Variable       variable)
{
  GimpDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  gboolean              active        = FALSE;

  if (priv->variables[VARIABLE_CPU_USAGE].available)
    {
      if (! cpu_active)
        {
          active =
            priv->variables[VARIABLE_CPU_USAGE].value.percentage *
            g_get_num_processors () > CPU_ACTIVE_ON;
        }
      else
        {
          active =
            priv->variables[VARIABLE_CPU_USAGE].value.percentage *
            g_get_num_processors () > CPU_ACTIVE_OFF;
        }

      variable_data->available = TRUE;
    }
  else
    {
      variable_data->available = FALSE;
    }

  cpu_active                   = active;
  variable_data->value.boolean = active;
}

static void
gimp_dashboard_reset_cpu_active (GimpDashboard *dashboard,
                                 Variable       variable)
{
  cpu_active = FALSE;
}

static gint64 cpu_active_time_prev_time = 0;
static gint64 cpu_active_time           = 0;

static void
gimp_dashboard_sample_cpu_active_time (GimpDashboard *dashboard,
                                       Variable       variable)
{
  GimpDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  gint64                curr_time;

  curr_time = g_get_monotonic_time ();

  if (priv->variables[VARIABLE_CPU_ACTIVE].available)
    {
      gboolean active = priv->variables[VARIABLE_CPU_ACTIVE].value.boolean;

      if (active && cpu_active_time_prev_time)
        cpu_active_time += curr_time - cpu_active_time_prev_time;
    }

  cpu_active_time_prev_time = curr_time;

  variable_data->available      = TRUE;
  variable_data->value.duration = cpu_active_time / 1000000.0;
}

static void
gimp_dashboard_reset_cpu_active_time (GimpDashboard *dashboard,
                                      Variable       variable)
{
  cpu_active_time = 0;
}

#endif /* HAVE_CPU_GROUP */

static void
gimp_dashboard_sample_object (GimpDashboard *dashboard,
                              GObject       *object,
                              Variable       variable)
{
  GimpDashboardPrivate *priv          = dashboard->priv;
  GObjectClass         *klass         = G_OBJECT_GET_CLASS (object);
  const VariableInfo   *variable_info = &variables[variable];
  VariableData         *variable_data = &priv->variables[variable];

  variable_data->available = FALSE;

  switch (variable_info->type)
    {
    case VARIABLE_TYPE_BOOLEAN:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.boolean,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_SIZE:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.size,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_SIZE_RATIO:
      {
        const gchar *antecedent = variable_info->data;
        const gchar *consequent = antecedent + strlen (antecedent) + 1;

        if (g_object_class_find_property (klass, antecedent) &&
            g_object_class_find_property (klass, consequent))
          {
            variable_data->available = TRUE;

            g_object_get (object,
                          antecedent, &variable_data->value.size_ratio.antecedent,
                          consequent, &variable_data->value.size_ratio.consequent,
                          NULL);
          }
      }
      break;

    case VARIABLE_TYPE_INT_RATIO:
      {
        const gchar *antecedent = variable_info->data;
        const gchar *consequent = antecedent + strlen (antecedent) + 1;

        if (g_object_class_find_property (klass, antecedent) &&
            g_object_class_find_property (klass, consequent))
          {
            variable_data->available = TRUE;

            g_object_get (object,
                          antecedent, &variable_data->value.int_ratio.antecedent,
                          consequent, &variable_data->value.int_ratio.consequent,
                          NULL);
          }
      }
      break;

    case VARIABLE_TYPE_PERCENTAGE:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.percentage,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_DURATION:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.duration,
                        NULL);
        }
      break;
    }
}

static void
gimp_dashboard_container_remove (GtkWidget    *widget,
                                 GtkContainer *container)
{
  gtk_container_remove (container, widget);
}

static void
gimp_dashboard_group_menu_position (GtkMenu  *menu,
                                    gint     *x,
                                    gint     *y,
                                    gboolean *push_in,
                                    gpointer  user_data)
{
  gimp_button_menu_position (user_data, menu, GTK_POS_LEFT, x, y);
}

static void
gimp_dashboard_update_groups (GimpDashboard *dashboard)
{
  Group group;

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    gimp_dashboard_update_group (dashboard, group);
}

static void
gimp_dashboard_update_group (GimpDashboard *dashboard,
                             Group          group)
{
  GimpDashboardPrivate *priv       = dashboard->priv;
  const GroupInfo      *group_info = &groups[group];
  GroupData            *group_data = &priv->groups[group];
  gint                  n_rows;
  gboolean              add_separator;
  gint                  field;

  gtk_widget_set_visible (GTK_WIDGET (group_data->expander),
                          group_data->active);

  if (! group_data->active)
    return;

  n_rows        = 0;
  add_separator = FALSE;

  for (field = 0; field < group_data->n_fields; field++)
    {
      const FieldInfo *field_info = &group_info->fields[field];
      const FieldData *field_data = &group_data->fields[field];

      if (field_info->variable != VARIABLE_SEPARATOR)
        {
          if (group_info->has_meter && field_info->meter_value)
            {
              gimp_meter_set_value_active (group_data->meter,
                                           field_info->meter_value - 1,
                                           field_data->active);
            }

          if (field_data->active)
            {
              if (add_separator)
                {
                  add_separator = FALSE;
                  n_rows++;
                }

              n_rows++;
            }
        }
      else
        {
          if (n_rows > 0)
            add_separator = TRUE;
        }
    }

  gtk_container_foreach (GTK_CONTAINER (group_data->table),
                         (GtkCallback) gimp_dashboard_container_remove,
                         group_data->table);
  gtk_table_resize (group_data->table, MAX (n_rows, 1), 3);

  n_rows        = 0;
  add_separator = FALSE;

  for (field = 0; field < group_data->n_fields; field++)
    {
      const FieldInfo *field_info = &group_info->fields[field];
      FieldData       *field_data = &group_data->fields[field];

      if (field_info->variable != VARIABLE_SEPARATOR)
        {
          const VariableInfo *variable_info = &variables[field_info->variable];
          GtkWidget          *separator;
          GtkWidget          *color_area;
          GtkWidget          *label;
          const gchar        *description;
          gchar              *str;

          if (! field_data->active)
            continue;

          description = g_dgettext (NULL, variable_info->description);

          if (add_separator)
            {
              separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
              gtk_table_attach (group_data->table, separator,
                                0, 3, n_rows, n_rows + 1,
                                GTK_EXPAND | GTK_FILL, 0,
                                0, 0);
              gtk_widget_show (separator);

              add_separator = FALSE;
              n_rows++;
            }

          if (variable_info->color.a)
            {
              color_area = gimp_color_area_new (&variable_info->color,
                                                GIMP_COLOR_AREA_FLAT, 0);
              gimp_help_set_help_data (color_area, description,
                                       NULL);
              gtk_widget_set_size_request (color_area, 5, 5);
              gtk_table_attach (group_data->table, color_area,
                                0, 1, n_rows, n_rows + 1,
                                0, 0,
                                0, 0);
              gtk_widget_show (color_area);
            }

          str = g_strdup_printf ("%s:",
                                 g_dpgettext2 (NULL, "dashboard-variable",
                                               variable_info->title));

          label = gtk_label_new (str);
          gimp_help_set_help_data (label, description,
                                   NULL);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_table_attach (group_data->table, label,
                            1, 2, n_rows, n_rows + 1,
                            GTK_FILL, 0,
                            0, 0);
          gtk_widget_show (label);

          g_free (str);

          label = gtk_label_new (NULL);
          field_data->value_label = GTK_LABEL (label);
          gimp_help_set_help_data (label, description,
                                   NULL);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_table_attach (group_data->table, label,
                            2, 3, n_rows, n_rows + 1,
                            GTK_EXPAND | GTK_FILL, 0,
                            0, 0);
          gtk_widget_show (label);

          n_rows++;
        }
      else
        {
          if (n_rows > 0)
            add_separator = TRUE;
        }
    }

  g_mutex_lock (&priv->mutex);

  gimp_dashboard_update_group_values (dashboard, group);

  g_mutex_unlock (&priv->mutex);
}

static void
gimp_dashboard_update_group_values (GimpDashboard *dashboard,
                                    Group          group)
{
  GimpDashboardPrivate *priv       = dashboard->priv;
  const GroupInfo      *group_info = &groups[group];
  GroupData            *group_data = &priv->groups[group];
  gdouble               limit      = 0.0;
  GString              *header_values;
  gint                  field;

  if (! group_data->active)
    return;

  if (group_info->has_meter)
    {
      if (group_info->meter_limit)
        {
          const VariableData *variable_data = &priv->variables[group_info->meter_limit];

          if (variable_data->available)
            {
              limit = gimp_dashboard_variable_to_double (dashboard,
                                                        group_info->meter_limit);
            }
          else
            {
              for (field = 0; field < group_data->n_fields; field++)
                {
                  const FieldInfo *field_info = &group_info->fields[field];
                  const FieldData *field_data = &group_data->fields[field];

                  if (field_info->meter_value && field_data->active)
                    {
                      gdouble value;

                      value = gimp_dashboard_variable_to_double (dashboard,
                                                                 field_info->variable);

                      limit = MAX (limit, value);
                    }
                }
            }

          gimp_meter_set_range (group_data->meter, 0.0, limit);
        }

      if (group_info->meter_led)
        {
          gimp_meter_set_led_active (
            group_data->meter,
            gimp_dashboard_variable_to_boolean (dashboard,
                                                group_info->meter_led));
        }
    }

  group_data->limit = limit;

  header_values = g_string_new (NULL);

  for (field = 0; field < group_data->n_fields; field++)
    {
      const FieldInfo *field_info = &group_info->fields[field];
      const FieldData *field_data = &group_data->fields[field];

      if (field_data->active)
        {
          gchar *text;

          text = gimp_dashboard_field_to_string (dashboard,
                                                 group, field, TRUE);

          gimp_dashboard_label_set_text (field_data->value_label, text);

          g_free (text);

          if (field_info->show_in_header)
            {
              text = gimp_dashboard_field_to_string (dashboard,
                                                     group, field, FALSE);

              if (header_values->len > 0)
                g_string_append (header_values, ", ");

              g_string_append (header_values, text);

              g_free (text);
            }
        }
    }

  if (header_values->len > 0)
    {
      g_string_prepend (header_values, "(");
      g_string_append  (header_values, ")");
    }

  gimp_dashboard_label_set_text (group_data->header_values_label,
                                 header_values->str);

  g_string_free (header_values, TRUE);
}

static void
gimp_dashboard_group_set_active (GimpDashboard *dashboard,
                                 Group          group,
                                 gboolean       active)
{
  GimpDashboardPrivate *priv       = dashboard->priv;
  GroupData            *group_data = &priv->groups[group];

  if (active != group_data->active)
    {
      group_data->active = active;

      if (group_data->action)
        {
          g_signal_handlers_block_by_func (group_data->action,
                                           gimp_dashboard_group_action_toggled,
                                           dashboard);

          gtk_toggle_action_set_active (group_data->action, active);

          g_signal_handlers_unblock_by_func (group_data->action,
                                             gimp_dashboard_group_action_toggled,
                                             dashboard);
        }
    }
}

static void
gimp_dashboard_field_set_active (GimpDashboard *dashboard,
                                 Group          group,
                                 gint           field,
                                 gboolean       active)
{
  GimpDashboardPrivate *priv       = dashboard->priv;
  GroupData            *group_data = &priv->groups[group];
  FieldData            *field_data = &group_data->fields[field];

  if (active != field_data->active)
    {
      field_data->active = active;

      g_signal_handlers_block_by_func (field_data->menu_item,
                                       gimp_dashboard_field_menu_item_toggled,
                                       dashboard);

      gtk_check_menu_item_set_active (field_data->menu_item, active);

      g_signal_handlers_unblock_by_func (field_data->menu_item,
                                         gimp_dashboard_field_menu_item_toggled,
                                         dashboard);
    }
}

static gboolean
gimp_dashboard_variable_to_boolean (GimpDashboard *dashboard,
                                    Variable       variable)
{
  GimpDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  const VariableData   *variable_data = &priv->variables[variable];

  if (variable_data->available)
    {
      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:
          return variable_data->value.boolean;

        case VARIABLE_TYPE_SIZE:
          return variable_data->value.size > 0;

        case VARIABLE_TYPE_SIZE_RATIO:
          return variable_data->value.size_ratio.antecedent != 0 &&
                 variable_data->value.size_ratio.consequent != 0;

        case VARIABLE_TYPE_INT_RATIO:
          return variable_data->value.int_ratio.antecedent != 0 &&
                 variable_data->value.int_ratio.consequent != 0;

        case VARIABLE_TYPE_PERCENTAGE:
          return variable_data->value.percentage != 0.0;

        case VARIABLE_TYPE_DURATION:
          return variable_data->value.duration != 0.0;
        }
    }

  return FALSE;
}

static gdouble
gimp_dashboard_variable_to_double (GimpDashboard *dashboard,
                                   Variable       variable)
{
  GimpDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  const VariableData   *variable_data = &priv->variables[variable];

  if (variable_data->available)
    {
      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:
          return variable_data->value.boolean ? 1.0 : 0.0;

        case VARIABLE_TYPE_SIZE:
          return variable_data->value.size;

        case VARIABLE_TYPE_SIZE_RATIO:
          if (variable_data->value.size_ratio.consequent)
            {
              return (gdouble) variable_data->value.size_ratio.antecedent /
                     (gdouble) variable_data->value.size_ratio.consequent;
            }
          break;

        case VARIABLE_TYPE_INT_RATIO:
          if (variable_data->value.int_ratio.consequent)
            {
              return (gdouble) variable_data->value.int_ratio.antecedent /
                     (gdouble) variable_data->value.int_ratio.consequent;
            }
          break;

        case VARIABLE_TYPE_PERCENTAGE:
          return variable_data->value.percentage;

        case VARIABLE_TYPE_DURATION:
          return variable_data->value.duration;
        }
    }

  return 0.0;
}

static gchar *
gimp_dashboard_field_to_string (GimpDashboard *dashboard,
                                Group          group,
                                gint           field,
                                gboolean       full)
{
  GimpDashboardPrivate *priv          = dashboard->priv;
  const GroupInfo      *group_info    = &groups[group];
  const GroupData      *group_data    = &priv->groups[group];
  const FieldInfo      *field_info    = &group_info->fields[field];
  const VariableInfo   *variable_info = &variables[field_info->variable];
  const VariableData   *variable_data = &priv->variables[field_info->variable];
  /* Tranlators: "N/A" is an abbreviation for "not available" */
  const gchar          *str           = C_("dashboard-value", "N/A");
  gboolean              static_str    = TRUE;
  gboolean              show_limit    = TRUE;

  if (variable_data->available)
    {
      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:
          str = variable_data->value.boolean ? C_("dashboard-value", "Yes") :
                                               C_("dashboard-value", "No");
          break;

        case VARIABLE_TYPE_SIZE:
          str        = g_format_size_full (variable_data->value.size,
                                           G_FORMAT_SIZE_IEC_UNITS);
          static_str = FALSE;
          break;

        case VARIABLE_TYPE_SIZE_RATIO:
          {
            if (variable_data->value.size_ratio.consequent)
              {
                gdouble value;

                value = 100.0 * variable_data->value.size_ratio.antecedent /
                                variable_data->value.size_ratio.consequent;

                str        = g_strdup_printf ("%d%%", SIGNED_ROUND (value));
                static_str = FALSE;
              }
          }
          break;

        case VARIABLE_TYPE_INT_RATIO:
          {
            gdouble  min;
            gdouble  max;
            gdouble  antecedent;
            gdouble  consequent;

            antecedent = variable_data->value.int_ratio.antecedent;
            consequent = variable_data->value.int_ratio.consequent;

            min = MIN (ABS (antecedent), ABS (consequent));
            max = MAX (ABS (antecedent), ABS (consequent));

            if (min)
              {
                antecedent /= min;
                consequent /= min;
              }
            else if (max)
              {
                antecedent /= max;
                consequent /= max;
              }

            if (max)
              {
                str        = g_strdup_printf ("%g:%g",
                                              RINT (100.0 * antecedent) / 100.0,
                                              RINT (100.0 * consequent) / 100.0);
                static_str = FALSE;
              }
          }
          break;

        case VARIABLE_TYPE_PERCENTAGE:
          str        = g_strdup_printf ("%d%%",
                                        SIGNED_ROUND (100.0 * variable_data->value.percentage));
          static_str = FALSE;
          show_limit = FALSE;
          break;

        case VARIABLE_TYPE_DURATION:
          str        = g_strdup_printf ("%02d:%02d:%04.1f",
                                        (gint) floor (variable_data->value.duration / 3600.0),
                                        (gint) floor (fmod (variable_data->value.duration / 60.0, 60.0)),
                                        floor (fmod (variable_data->value.duration, 60.0) * 10.0) / 10.0);
          static_str = FALSE;
          show_limit = FALSE;
          break;
        }

      if (show_limit               &&
          variable_data->available &&
          field_info->meter_value  &&
          group_data->limit)
        {
          gdouble  value;
          gchar   *tmp;

          value = gimp_dashboard_variable_to_double (dashboard,
                                                     field_info->variable);

          if (full)
            {
              tmp = g_strdup_printf ("%s (%d%%)",
                                     str,
                                     SIGNED_ROUND (100.0 * value /
                                                   group_data->limit));
            }
          else
            {
              tmp = g_strdup_printf ("%d%%",
                                     SIGNED_ROUND (100.0 * value /
                                                   group_data->limit));
            }

          if (! static_str)
            g_free ((gpointer) str);

          str        = tmp;
          static_str = FALSE;
        }
    }

  if (static_str)
    return g_strdup (str);
  else
    return (gpointer) str;
}

static void
gimp_dashboard_label_set_text (GtkLabel    *label,
                               const gchar *text)
{
  /* the strcmp() reduces the overhead of gtk_label_set_text() when the
   * text hasn't changed
   */
  if (g_strcmp0 (gtk_label_get_text (label), text))
    gtk_label_set_text (label, text);
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

  dashboard->priv->gimp = gimp;

  return GTK_WIDGET (dashboard);
}

void
gimp_dashboard_reset (GimpDashboard *dashboard)
{
  GimpDashboardPrivate *priv;
  Variable              variable;
  Group                 group;

  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  g_mutex_lock (&priv->mutex);

  gegl_reset_stats ();

  for (variable = FIRST_VARIABLE; variable < N_VARIABLES; variable++)
    {
      const VariableInfo *variable_info = &variables[variable];

      if (variable_info->reset_func)
        variable_info->reset_func (dashboard, variable);
    }

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      GroupData *group_data = &priv->groups[group];

      if (group_data->meter)
        gimp_meter_clear_history (group_data->meter);
    }

  priv->update_now = TRUE;
  g_cond_signal (&priv->cond);

  g_mutex_unlock (&priv->mutex);
}

void
gimp_dashboard_set_update_interval (GimpDashboard              *dashboard,
                                    GimpDashboardUpdateInteval  update_interval)
{
  GimpDashboardPrivate *priv;

  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  if (update_interval != priv->update_interval)
    {
      Group group;

      g_mutex_lock (&priv->mutex);

      priv->update_interval = update_interval;

      for (group = FIRST_GROUP; group < N_GROUPS; group++)
        {
          GroupData *group_data = &priv->groups[group];

          if (group_data->meter)
            {
              gimp_meter_set_history_resolution (group_data->meter,
                                                 update_interval / 1000.0);
            }
        }

      priv->update_now = TRUE;
      g_cond_signal (&priv->cond);

      g_mutex_unlock (&priv->mutex);
    }
}

GimpDashboardUpdateInteval
gimp_dashboard_get_update_interval (GimpDashboard *dashboard)
{
  g_return_val_if_fail (GIMP_IS_DASHBOARD (dashboard), DEFAULT_UPDATE_INTERVAL);

  return dashboard->priv->update_interval;
}

void
gimp_dashboard_set_history_duration (GimpDashboard                *dashboard,
                                     GimpDashboardHistoryDuration  history_duration)
{
  GimpDashboardPrivate *priv;

  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  if (history_duration != priv->history_duration)
    {
      Group group;

      g_mutex_lock (&priv->mutex);

      priv->history_duration = history_duration;

      for (group = FIRST_GROUP; group < N_GROUPS; group++)
        {
          GroupData *group_data = &priv->groups[group];

          if (group_data->meter)
            {
              gimp_meter_set_history_duration (group_data->meter,
                                               history_duration / 1000.0);
            }
        }

      priv->update_now = TRUE;
      g_cond_signal (&priv->cond);

      g_mutex_unlock (&priv->mutex);
    }
}

GimpDashboardHistoryDuration
gimp_dashboard_get_history_duration (GimpDashboard *dashboard)
{
  g_return_val_if_fail (GIMP_IS_DASHBOARD (dashboard), DEFAULT_HISTORY_DURATION);

  return dashboard->priv->history_duration;
}

void
gimp_dashboard_set_low_swap_space_warning (GimpDashboard *dashboard,
                                           gboolean       low_swap_space_warning)
{
  GimpDashboardPrivate *priv;

  g_return_if_fail (GIMP_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  if (low_swap_space_warning != priv->low_swap_space_warning)
    {
      g_mutex_lock (&priv->mutex);

      priv->low_swap_space_warning = low_swap_space_warning;

      g_mutex_unlock (&priv->mutex);
    }
}

gboolean
gimp_dashboard_get_low_swap_space_warning (GimpDashboard *dashboard)
{
  g_return_val_if_fail (GIMP_IS_DASHBOARD (dashboard), DEFAULT_LOW_SWAP_SPACE_WARNING);

  return dashboard->priv->low_swap_space_warning;
}

void
gimp_dashboard_menu_setup (GimpUIManager *manager,
                           const gchar   *ui_path)
{
  guint merge_id;
  Group group;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      gchar           *action_name;
      gchar           *action_path;

      action_name = g_strdup_printf ("dashboard-group-%s", group_info->name);
      action_path = g_strdup_printf ("%s/Groups/Groups", ui_path);

      gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                             action_path, action_name, action_name,
                             GTK_UI_MANAGER_MENUITEM,
                             FALSE);

      g_free (action_name);
      g_free (action_path);
    }
}
