/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 * (c) Mircea Purdea : 2009 : someone_else@exhalus.net
 * (c) Jehan : 2012 : jehan at girinstud.io
 *
 * GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * TODO:
 *  pdb interface - should we bother?
 *
 *  speedups (caching?  most bottlenecks seem to be in pixelrgns)
 *    -> do pixelrgns properly!
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#undef GDK_DISABLE_DEPRECATED
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-animationplay"
#define PLUG_IN_BINARY "animation-play"
#define PLUG_IN_ROLE   "gimp-animation-play"
#define DITHERTYPE     GDK_RGB_DITHER_NORMAL


typedef enum
{
  DISPOSE_COMBINE   = 0x00,
  DISPOSE_REPLACE   = 0x01
} DisposeType;

typedef struct
{
  gint        duration_index;
  DisposeType default_frame_disposal;
  guint32     default_frame_duration;
}
AnimationSettings;

/* for shaping */
typedef struct
{
  gint x, y;
} CursorOffset;

/* Declare local functions. */
static void        query                     (void);
static void        run                       (const gchar      *name,
                                              gint              nparams,
                                              const GimpParam  *param,
                                              gint             *nreturn_vals,
                                              GimpParam       **return_vals);

static void        initialize                (void);
static void        build_dialog              (gchar           *imagename);
static void        refresh_dialog            (gchar           *imagename);

static void        da_size_callback          (GtkWidget *widget,
                                              GtkAllocation *allocation, void *data);
static void        sda_size_callback         (GtkWidget *widget,
                                              GtkAllocation *allocation, void *data);

static void        window_destroy            (GtkWidget       *widget);
static void        play_callback             (GtkToggleAction *action);
static void        step_back_callback        (GtkAction       *action);
static void        step_callback             (GtkAction       *action);
static void        refresh_callback          (GtkAction       *action);
static void        rewind_callback           (GtkAction       *action);
static void        speed_up_callback         (GtkAction       *action);
static void        speed_down_callback       (GtkAction       *action);
static void        speed_reset_callback      (GtkAction       *action);
static void        framecombo_changed        (GtkWidget       *combo,
                                              gpointer         data);
static void        speedcombo_changed        (GtkWidget       *combo,
                                              gpointer         data);
static void        fpscombo_changed          (GtkWidget       *combo,
                                              gpointer         data);
static void        zoomcombo_activated       (GtkEntry        *combo,
                                              gpointer         data);
static void        zoomcombo_changed         (GtkWidget       *combo,
                                              gpointer         data);
static gboolean    repaint_sda               (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);
static gboolean    repaint_da                (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);

static void        init_frames               (void);
static void        render_frame              (gint32           whichframe);
static void        show_frame                (void);
static void        total_alpha_preview       (void);
static void        update_alpha_preview      (void);
static void        update_combobox           (void);
static gdouble     get_duration_factor       (gint             index);
static gint        get_fps                   (gint             index);
static gdouble     get_scale                 (gint             index);
static void        update_scale              (gdouble          scale);


/* tag util functions*/
static gint        parse_ms_tag              (const gchar     *str);
static DisposeType parse_disposal_tag        (const gchar     *str);
static gboolean    is_disposal_tag           (const gchar     *str,
                                              DisposeType     *disposal,
                                              gint            *taglength);
static gboolean    is_ms_tag                 (const gchar     *str,
                                              gint            *duration,
                                              gint            *taglength);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/* Global widgets'n'stuff */
static GtkWidget         *window                    = NULL;
static GdkWindow         *root_win                  = NULL;
static GtkUIManager      *ui_manager                = NULL;
static GtkWidget         *progress;
static GtkWidget         *speedcombo                = NULL;
static GtkWidget         *fpscombo                  = NULL;
static GtkWidget         *zoomcombo                 = NULL;
static GtkWidget         *frame_disposal_combo      = NULL;

static gint32             image_id;
static guint              width                     = -1,
                          height                    = -1;
static gint32            *layers                    = NULL;
static gint32             total_layers              = 0;

static GtkWidget         *drawing_area              = NULL;
static guchar            *drawing_area_data         = NULL;
static guint              drawing_area_width        = -1,
                          drawing_area_height       = -1;
static guchar            *preview_alpha1_data       = NULL;
static guchar            *preview_alpha2_data       = NULL;

static GtkWidget         *shape_window              = NULL;
static GtkWidget         *shape_drawing_area        = NULL;
static guchar            *shape_drawing_area_data   = NULL;
static guint              shape_drawing_area_width  = -1,
                          shape_drawing_area_height = -1;
static gchar             *shape_preview_mask        = NULL;


static gint32             total_frames              = 0;
static gint32            *frames                    = NULL;
static guchar            *rawframe                  = NULL;
static guint32           *frame_durations           = NULL;
static guint              frame_number              = 0;

static gboolean           playing                   = FALSE;
static guint              timer                     = 0;
static gboolean           detached                  = FALSE;
static gdouble            scale, shape_scale;

/* Default settings. */
static AnimationSettings settings =
{
  3,
  DISPOSE_COMBINE,
  100 /* ms */
};

static gint32 frames_image_id = 0;

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"      }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Preview a GIMP layer-based animation"),
                          "",
                          "Adam D. Moss <adam@gimp.org>",
                          "Adam D. Moss <adam@gimp.org>",
                          "1997, 1998...",
                          N_("_Playback..."),
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Animation");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) "media-playback-start");
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

 if (run_mode == GIMP_RUN_NONINTERACTIVE && n_params != 3)
   {
     status = GIMP_PDB_CALLING_ERROR;
   }

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_get_data (PLUG_IN_PROC, &settings);
      image_id = param[1].data.d_image;

      initialize ();
      gtk_main ();
      gimp_set_data (PLUG_IN_PROC, &settings, sizeof (settings));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gimp_image_delete (frames_image_id);
  gegl_exit ();
}

static void
reshape_from_bitmap (const gchar *bitmap)
{
  static gchar *prev_bitmap = NULL;
  static guint  prev_width = -1;
  static guint  prev_height = -1;

  if ((!prev_bitmap) ||
      prev_width != shape_drawing_area_width || prev_height != shape_drawing_area_height ||
      (memcmp (prev_bitmap, bitmap, (shape_drawing_area_width * shape_drawing_area_height) / 8 + shape_drawing_area_height)))
    {
      GdkBitmap *shape_mask;

      shape_mask = gdk_bitmap_create_from_data (gtk_widget_get_window (shape_window),
                                                bitmap,
                                                shape_drawing_area_width, shape_drawing_area_height);
      gtk_widget_shape_combine_mask (shape_window, shape_mask, 0, 0);
      g_object_unref (shape_mask);

      if (!prev_bitmap || prev_width != shape_drawing_area_width || prev_height != shape_drawing_area_height)
        {
          g_free(prev_bitmap);
          prev_bitmap = g_malloc ((shape_drawing_area_width * shape_drawing_area_height) / 8 + shape_drawing_area_height);
          prev_width = shape_drawing_area_width;
          prev_height = shape_drawing_area_height;
        }

      memcpy (prev_bitmap, bitmap, (shape_drawing_area_width * shape_drawing_area_height) / 8 + shape_drawing_area_height);
    }
}

static gboolean
popup_menu (GtkWidget      *widget,
            GdkEventButton *event)
{
  GtkWidget *menu = gtk_ui_manager_get_widget (ui_manager, "/anim-play-popup");

  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);

  return TRUE;
}

static gboolean
button_press (GtkWidget      *widget,
              GdkEventButton *event)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    return popup_menu (widget, event);

  return FALSE;
}

/*
 * Update the actual drawing area metrics, which may be different as requested,
 * because there is no full control of the WM.
 * data is always NULL. */
static void
da_size_callback (GtkWidget *widget,
                  GtkAllocation *allocation, void *data)
{
  if (allocation->width == drawing_area_width && allocation->height == drawing_area_height)
    return;

  drawing_area_width = allocation->width;
  drawing_area_height = allocation->height;
  scale = MIN ((gdouble) drawing_area_width / (gdouble) width, (gdouble) drawing_area_height / (gdouble) height);

  g_free (drawing_area_data);
  drawing_area_data = g_malloc (drawing_area_width * drawing_area_height * 3);

  update_alpha_preview ();

  if (! detached)
    {
      /* Update the zoom information. */
      GtkEntry *zoomcombo_text_child;

      zoomcombo_text_child = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (zoomcombo)));
      if (zoomcombo_text_child)
        {
          char* new_entry_text = g_strdup_printf  (_("%.1f %%"), scale * 100.0);

          gtk_entry_set_text (zoomcombo_text_child, new_entry_text);
          g_free (new_entry_text);
        }

      /* Update the rawframe. */
      g_free (rawframe);
      rawframe = g_malloc ((unsigned long) drawing_area_width * drawing_area_height * 4);

      /* As we re-allocated the drawn data, let's render it again. */
      if (frame_number < total_frames)
        render_frame (frame_number);
    }
  else
    {
      /* Set "alpha grid" background. */
      total_alpha_preview ();
      repaint_da(drawing_area, NULL, NULL);
    }
}

/*
 * Update the actual shape drawing area metrics, which may be different as requested,
 * They *should* be the same as the drawing area, but the safe way is to make sure
 * and process it separately.
 * data is always NULL. */
static void
sda_size_callback (GtkWidget *widget,
                   GtkAllocation *allocation, void *data)
{
  if (allocation->width == shape_drawing_area_width && allocation->height == shape_drawing_area_height)
    return;

  shape_drawing_area_width = allocation->width;
  shape_drawing_area_height = allocation->height;
  shape_scale = MIN ((gdouble) shape_drawing_area_width / (gdouble) width, (gdouble) shape_drawing_area_height / (gdouble) height);

  g_free (shape_drawing_area_data);
  g_free (shape_preview_mask);

  shape_drawing_area_data = g_malloc (shape_drawing_area_width * shape_drawing_area_height * 3);
  shape_preview_mask = g_malloc ((shape_drawing_area_width * shape_drawing_area_height) / 8 + 1 + shape_drawing_area_height);

  if (detached)
    {
      /* Update the zoom information. */
      GtkEntry *zoomcombo_text_child;

      zoomcombo_text_child = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (zoomcombo)));
      if (zoomcombo_text_child)
        {
          char* new_entry_text = g_strdup_printf  (_("%.1f %%"), shape_scale * 100.0);

          gtk_entry_set_text (zoomcombo_text_child, new_entry_text);
          g_free (new_entry_text);
        }

      /* Update the rawframe. */
      g_free (rawframe);
      rawframe = g_malloc ((unsigned long) shape_drawing_area_width * shape_drawing_area_height * 4);

      if (frame_number < total_frames)
        render_frame (frame_number);
    }
}

static gboolean
shape_pressed (GtkWidget      *widget,
               GdkEventButton *event)
{
  if (button_press (widget, event))
    return TRUE;

  /* ignore double and triple click */
  if (event->type == GDK_BUTTON_PRESS)
    {
      CursorOffset *p = g_object_get_data (G_OBJECT(widget), "cursor-offset");

      if (!p)
        return FALSE;

      p->x = (gint) event->x;
      p->y = (gint) event->y;

      gtk_grab_add (widget);
      gdk_pointer_grab (gtk_widget_get_window (widget), TRUE,
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_BUTTON_MOTION_MASK  |
                        GDK_POINTER_MOTION_HINT_MASK,
                        NULL, NULL, 0);
      gdk_window_raise (gtk_widget_get_window (widget));
    }

  return FALSE;
}

static gboolean
shape_released (GtkWidget *widget)
{
  gtk_grab_remove (widget);
  gdk_display_pointer_ungrab (gtk_widget_get_display (widget), 0);
  gdk_flush ();

  return FALSE;
}

static gboolean
shape_motion (GtkWidget      *widget,
              GdkEventMotion *event)
{
  GdkModifierType  mask;
  gint             xp, yp;

  gdk_window_get_pointer (root_win, &xp, &yp, &mask);

  /* if a button is still held by the time we process this event... */
  if (mask & GDK_BUTTON1_MASK)
    {
      CursorOffset *p = g_object_get_data (G_OBJECT (widget), "cursor-offset");

      if (!p)
        return FALSE;

      gtk_window_move (GTK_WINDOW (widget), xp  - p->x, yp  - p->y);
    }
  else /* the user has released all buttons */
    {
      shape_released (widget);
    }

  return FALSE;
}

static gboolean
repaint_da (GtkWidget      *darea,
            GdkEventExpose *event,
            gpointer        data)
{
  GtkStyle *style = gtk_widget_get_style (darea);

  gdk_draw_rgb_image (gtk_widget_get_window (darea),
                      style->white_gc,
                      (gint) ((drawing_area_width - scale * width) / 2),
                      (gint) ((drawing_area_height - scale * height) / 2),
                      drawing_area_width, drawing_area_height,
                      (total_frames == 1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      drawing_area_data, drawing_area_width * 3);

  return TRUE;
}

static gboolean
repaint_sda (GtkWidget      *darea,
             GdkEventExpose *event,
             gpointer        data)
{
  GtkStyle *style = gtk_widget_get_style (darea);

  gdk_draw_rgb_image (gtk_widget_get_window (darea),
                      style->white_gc,
                      (gint) ((shape_drawing_area_width - shape_scale * width) / 2),
                      (gint) ((shape_drawing_area_height - shape_scale * height) / 2),
                      shape_drawing_area_width, shape_drawing_area_height,
                      (total_frames == 1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      shape_drawing_area_data, shape_drawing_area_width * 3);

  return TRUE;
}

static void
close_callback (GtkAction *action,
                gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
help_callback (GtkAction *action,
               gpointer   data)
{
  gimp_standard_help_func (PLUG_IN_PROC, data);
}


static void
detach_callback (GtkToggleAction *action)
{
  gboolean active = gtk_toggle_action_get_active (action);

  if (active == detached)
    {
      g_warning ("detached state and toggle action got out of sync");
      return;
    }

  detached = active;

  if (detached)
    {
      gint x, y;

      /* Create a total-alpha buffer merely for the not-shaped
         drawing area to now display. */

      gtk_window_set_screen (GTK_WINDOW (shape_window),
                             gtk_widget_get_screen (drawing_area));

      gtk_widget_show (shape_window);

      if (!gtk_widget_get_realized (drawing_area))
        gtk_widget_realize (drawing_area);
      if (!gtk_widget_get_realized (shape_drawing_area))
        gtk_widget_realize (shape_drawing_area);

      gdk_window_get_origin (gtk_widget_get_window (drawing_area), &x, &y);

      gtk_window_move (GTK_WINDOW (shape_window), x + 6, y + 6);

      gdk_window_set_back_pixmap (gtk_widget_get_window (shape_drawing_area), NULL, TRUE);


      /* Set "alpha grid" background. */
      total_alpha_preview ();
      repaint_da(drawing_area, NULL, NULL);
    }
  else
    gtk_widget_hide (shape_window);

  render_frame (frame_number);
}

static GtkUIManager *
ui_manager_new (GtkWidget *window)
{
  static GtkActionEntry actions[] =
  {
    { "step-back", "media-skip-backward",
      N_("Step _back"), "d", N_("Step back to previous frame"),
      G_CALLBACK (step_back_callback) },

    { "step", "media-skip-forward",
      N_("_Step"), "f", N_("Step to next frame"),
      G_CALLBACK (step_callback) },

    { "rewind", "media-seek-backward",
      NULL, NULL, N_("Rewind the animation"),
      G_CALLBACK (rewind_callback) },

    { "refresh", GIMP_ICON_VIEW_REFRESH,
      NULL, "<control>R", N_("Reload the image"),
      G_CALLBACK (refresh_callback) },

    { "help", "help-browser",
      NULL, NULL, NULL,
      G_CALLBACK (help_callback) },

    { "close", "window-close",
      NULL, "<control>W", NULL,
      G_CALLBACK (close_callback)
    },
    {
      "quit", "application-quit",
      NULL, "<control>Q", NULL,
      G_CALLBACK (close_callback)
    },
    {
      "speed-up", NULL,
      N_("Faster"), "<control>L", N_("Increase the speed of the animation"),
      G_CALLBACK (speed_up_callback)
    },
    {
      "speed-down", NULL,
      N_("Slower"), "<control>J", N_("Decrease the speed of the animation"),
      G_CALLBACK (speed_down_callback)
    },
    {
      "speed-reset", NULL,
      N_("Reset speed"), "<control>K", N_("Reset the speed of the animation"),
      G_CALLBACK (speed_reset_callback)
    }
  };

  static GtkToggleActionEntry toggle_actions[] =
  {
    { "play", "media-playback-start",
      NULL, "space", N_("Start playback"),
      G_CALLBACK (play_callback), FALSE },

    { "detach", GIMP_ICON_DETACH,
      N_("Detach"), NULL,
      N_("Detach the animation from the dialog window"),
      G_CALLBACK (detach_callback), FALSE }
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GtkActionGroup *group      = gtk_action_group_new ("Actions");
  GError         *error      = NULL;

  gtk_action_group_set_translation_domain (group, NULL);

  gtk_action_group_add_actions (group,
                                actions,
                                G_N_ELEMENTS (actions),
                                window);
  gtk_action_group_add_toggle_actions (group,
                                       toggle_actions,
                                       G_N_ELEMENTS (toggle_actions),
                                       NULL);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              gtk_ui_manager_get_accel_group (ui_manager));
  gtk_accel_group_lock (gtk_ui_manager_get_accel_group (ui_manager));

  gtk_ui_manager_insert_action_group (ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <toolbar name=\"anim-play-toolbar\">"
                                     "    <toolitem action=\"play\" />"
                                     "    <toolitem action=\"step-back\" />"
                                     "    <toolitem action=\"step\" />"
                                     "    <toolitem action=\"rewind\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"detach\" />"
                                     "    <toolitem action=\"refresh\" />"
                                     "    <separator name=\"space\" />"
                                     "    <toolitem action=\"help\" />"
                                     "  </toolbar>"
                                     "  <accelerator action=\"close\" />"
                                     "  <accelerator action=\"quit\" />"
                                     "</ui>",
                                     -1, &error);

  if (error)
    {
      g_warning ("error parsing ui: %s", error->message);
      g_clear_error (&error);
    }

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <popup name=\"anim-play-popup\">"
                                     "    <menuitem action=\"play\" />"
                                     "    <menuitem action=\"step-back\" />"
                                     "    <menuitem action=\"step\" />"
                                     "    <menuitem action=\"rewind\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"speed-down\" />"
                                     "    <menuitem action=\"speed-up\" />"
                                     "    <menuitem action=\"speed-reset\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"detach\" />"
                                     "    <menuitem action=\"refresh\" />"
                                     "    <menuitem action=\"close\" />"
                                     "  </popup>"
                                     "</ui>",
                                     -1, &error);

  if (error)
    {
      g_warning ("error parsing ui: %s", error->message);
      g_clear_error (&error);
    }

  return ui_manager;
}

static void
refresh_dialog (gchar *imagename)
{
  gchar     *name;
  GdkScreen *screen;
  guint      screen_width, screen_height;
  gint       window_width, window_height;

  /* Image Name */
  name = g_strconcat (_("Animation Playback:"), " ", imagename, NULL);
  gtk_window_set_title (GTK_WINDOW (window), name);
  g_free (name);

  /* Update GUI size. */
  screen = gtk_widget_get_screen (window);
  screen_height = gdk_screen_get_height (screen);
  screen_width = gdk_screen_get_width (screen);
  gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);

  /* if the *window* size is bigger than the screen size,
   * diminish the drawing area by as much, then compute the corresponding scale. */
  if (window_width + 50 > screen_width || window_height + 50 > screen_height)
  {
      guint expected_drawing_area_width = MAX (1, width - window_width + screen_width);
      guint expected_drawing_area_height = MAX (1, height - window_height + screen_height);
      gdouble expected_scale = MIN ((gdouble) expected_drawing_area_width / (gdouble) width,
                                    (gdouble) expected_drawing_area_height / (gdouble) height);
      update_scale (expected_scale);

      /* There is unfortunately no good way to know the size of the decorations, taskbars, etc.
       * So we take a wild guess by making the window slightly smaller to fit into any case. */
      gtk_window_set_default_size (GTK_WINDOW (window),
                                   MIN (expected_drawing_area_width + 20, screen_width - 60),
                                   MIN (expected_drawing_area_height + 90, screen_height - 60));

      gtk_window_reshow_with_initial_size (GTK_WINDOW (window));
  }
}

static void
build_dialog (gchar             *imagename)
{
  GtkWidget   *toolbar;
  GtkWidget   *frame;
  GtkWidget   *viewport;
  GtkWidget   *main_vbox;
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *abox;
  GtkToolItem *item;
  GtkAction   *action;
  GdkCursor   *cursor;
  gint         index;
  gchar       *text;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_role (GTK_WINDOW (window), "animation-playback");

  g_signal_connect (window, "destroy",
                    G_CALLBACK (window_destroy),
                    NULL);
  g_signal_connect (window, "popup-menu",
                    G_CALLBACK (popup_menu),
                    NULL);

  gimp_help_connect (window, gimp_standard_help_func, PLUG_IN_PROC, NULL);

  ui_manager = ui_manager_new (window);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/anim-play-toolbar");
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  item =
    GTK_TOOL_ITEM (gtk_ui_manager_get_widget (ui_manager,
                                              "/anim-play-toolbar/space"));
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Alignment for the scrolling window, which can be resized by the user. */
  abox = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  frame = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (frame),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (frame);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (frame), viewport);
  gtk_widget_show (viewport);

  /* I add the drawing area inside an alignment box to prevent it from being resized. */
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (viewport), abox);
  gtk_widget_show (abox);

  /* Build a drawing area, with a default size same as the image */
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_add_events (drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (abox), drawing_area);
  gtk_widget_show (drawing_area);

  g_signal_connect (drawing_area, "size-allocate",
                    G_CALLBACK(da_size_callback),
                    NULL);
  g_signal_connect (drawing_area, "button-press-event",
                    G_CALLBACK (button_press),
                    NULL);

  /* Lower option bar. */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Progress bar. */

  progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (hbox), progress, TRUE, TRUE, 0);
  gtk_widget_show (progress);

  /* Zoom */
  zoomcombo = gtk_combo_box_text_new_with_entry ();
  gtk_box_pack_end (GTK_BOX (hbox), zoomcombo, FALSE, FALSE, 0);
  gtk_widget_show (zoomcombo);
  for (index = 0; index < 5; index++)
    {
      /* list is given in "fps" - frames per second */
      text = g_strdup_printf  (_("%.1f %%"), get_scale (index) * 100.0);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (zoomcombo), text);
      g_free (text);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (zoomcombo), 2); /* 1.0 by default. */

  g_signal_connect (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (zoomcombo))),
                    "activate",
                    G_CALLBACK (zoomcombo_activated),
                    NULL);
  g_signal_connect (zoomcombo, "changed",
                    G_CALLBACK (zoomcombo_changed),
                    NULL);

  gimp_help_set_help_data (zoomcombo, _("Zoom"), NULL);

  /* fps combo */
  fpscombo = gtk_combo_box_text_new ();
  gtk_box_pack_end (GTK_BOX (hbox), fpscombo, FALSE, FALSE, 0);
  gtk_widget_show (fpscombo);

  for (index = 0; index < 9; index++)
    {
      /* list is given in "fps" - frames per second */
      text = g_strdup_printf  (_("%d fps"), get_fps (index));
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (fpscombo), text);
      g_free (text);
      if (settings.default_frame_duration == 1000 / get_fps(index))
        gtk_combo_box_set_active (GTK_COMBO_BOX (fpscombo), index);
    }

  g_signal_connect (fpscombo, "changed",
                    G_CALLBACK (fpscombo_changed),
                    NULL);

  gimp_help_set_help_data (fpscombo, _("Default framerate"), NULL);

  /* Speed Combo */
  speedcombo = gtk_combo_box_text_new ();
  gtk_box_pack_end (GTK_BOX (hbox), speedcombo, FALSE, FALSE, 0);
  gtk_widget_show (speedcombo);

  for (index = 0; index < 7; index++)
    {
      text = g_strdup_printf  ("%g\303\227", (100 / get_duration_factor (index)) / 100);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (speedcombo), text);
      g_free (text);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (speedcombo), settings.duration_index);

  g_signal_connect (speedcombo, "changed",
                    G_CALLBACK (speedcombo_changed),
                    NULL);

  gimp_help_set_help_data (speedcombo, _("Playback speed"), NULL);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, FALSE);

  /* Set up the frame disposal combo. */
  frame_disposal_combo = gtk_combo_box_text_new ();

  /* 2 styles of default frame disposals: cumulative layers and one frame per layer. */
  text = g_strdup (_("Cumulative layers (combine)"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (frame_disposal_combo), DISPOSE_COMBINE, text);
  g_free (text);

  text = g_strdup (_("One frame per layer (replace)"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (frame_disposal_combo), DISPOSE_REPLACE, text);
  g_free (text);

  gtk_combo_box_set_active (GTK_COMBO_BOX (frame_disposal_combo), settings.default_frame_disposal);

  g_signal_connect (frame_disposal_combo, "changed",
                    G_CALLBACK (framecombo_changed),
                    NULL);

  gtk_box_pack_end (GTK_BOX (hbox), frame_disposal_combo, FALSE, FALSE, 0);
  gtk_widget_show (frame_disposal_combo);

  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (window), width + 20, height + 90);
  gtk_widget_show (window);

  /* shape_drawing_area for detached feature. */
  shape_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_resizable (GTK_WINDOW (shape_window), FALSE);

  shape_drawing_area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (shape_window), shape_drawing_area);
  gtk_widget_show (shape_drawing_area);
  gtk_widget_add_events (shape_drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_widget_realize (shape_drawing_area);

  gdk_window_set_back_pixmap (gtk_widget_get_window (shape_window), NULL, FALSE);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (shape_window),
                                       GDK_HAND2);
  gdk_window_set_cursor (gtk_widget_get_window (shape_window), cursor);
  g_object_unref (cursor);

  g_signal_connect(shape_drawing_area, "size-allocate",
                   G_CALLBACK(sda_size_callback),
                   NULL);
  g_signal_connect (shape_window, "button-press-event",
                    G_CALLBACK (shape_pressed),
                    NULL);
  g_signal_connect (shape_window, "button-release-event",
                    G_CALLBACK (shape_released),
                    NULL);
  g_signal_connect (shape_window, "motion-notify-event",
                    G_CALLBACK (shape_motion),
                    NULL);

  g_object_set_data (G_OBJECT (shape_window),
                     "cursor-offset", g_new0 (CursorOffset, 1));

  g_signal_connect (drawing_area, "expose-event",
                    G_CALLBACK (repaint_da),
                    NULL);

  g_signal_connect (shape_drawing_area, "expose-event",
                    G_CALLBACK (repaint_sda),
                    NULL);

  /* We request a minimum size *after* having connecting the
   * size-allocate signal for correct initialization. */
  gtk_widget_set_size_request (drawing_area, width, height);
  gtk_widget_set_size_request (shape_drawing_area, width, height);

  root_win = gdk_get_default_root_window ();
}

static void
init_frames (void)
{
  /* Frames are associated to an unused image. */
  gint          i;
  gint32        new_frame, previous_frame, new_layer;
  gboolean      animated;
  GtkAction    *action;
  gint          duration = 0;
  DisposeType   disposal = settings.default_frame_disposal;
  gchar        *layer_name;

  total_frames = total_layers;

  /* Cleanup before re-generation. */
  if (frames)
    {
      gimp_image_delete (frames_image_id);
      g_free (frames);
      g_free (frame_durations);
    }
  frames = g_try_malloc0_n (total_frames, sizeof (gint32));
  frame_durations = g_try_malloc0_n (total_frames, sizeof (guint32));
  if (! frames || ! frame_durations)
    {
      gimp_message (_("Memory could not be allocated to the frame container."));
      gtk_main_quit ();
      gimp_quit ();
      return;
    }
  /* We only use RGB images for display because indexed images would somehow
     render terrible colors. Layers from other types will be automatically
     converted. */
  frames_image_id = gimp_image_new (width, height, GIMP_RGB);
  /* Save processing time and memory by not saving history and merged frames. */
  gimp_image_undo_disable (frames_image_id);

  for (i = 0; i < total_frames; i++)
    {
      layer_name = gimp_item_get_name (layers[total_layers - (i + 1)]);
      if (layer_name)
        {
          duration = parse_ms_tag (layer_name);
          disposal = parse_disposal_tag (layer_name);
          g_free (layer_name);
        }

      if (i > 0 && disposal != DISPOSE_REPLACE)
        {
          previous_frame = gimp_layer_copy (frames[i - 1]);
          gimp_image_insert_layer (frames_image_id, previous_frame, 0, -1);
          gimp_item_set_visible (previous_frame, TRUE);
        }
      new_layer = gimp_layer_new_from_drawable (layers[total_layers - (i + 1)], frames_image_id);
      gimp_image_insert_layer (frames_image_id, new_layer, 0, -1);
      gimp_item_set_visible (new_layer, TRUE);
      new_frame = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
      frames[i] = new_frame;
      gimp_item_set_visible (new_frame, FALSE);

      if (duration <= 0)
        duration = settings.default_frame_duration;
      frame_durations[i] = (guint32) duration;
    }

  /* Update the UI. */
  animated = total_frames >= 2;
  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/play");
  gtk_action_set_sensitive (action, animated);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/step-back");
  gtk_action_set_sensitive (action, animated);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/step");
  gtk_action_set_sensitive (action, animated);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/rewind");
  gtk_action_set_sensitive (action, animated);

  /* Keep the same frame number, unless it is now invalid. */
  if (frame_number >= total_frames)
    frame_number = 0;
}

static void
initialize (void)
{
  /* Freeing existing data after a refresh. */
  g_free (layers);

  /* Catch the case when the user has closed the image in the meantime. */
  if (! gimp_image_is_valid (image_id))
    {
      gimp_message (_("Invalid image. Did you close it?"));
      gtk_main_quit ();
      return;
    }

  width     = gimp_image_width (image_id);
  height    = gimp_image_height (image_id);
  layers    = gimp_image_get_layers (image_id, &total_layers);

  if (!window)
    build_dialog (gimp_image_get_name (image_id));
  refresh_dialog (gimp_image_get_name (image_id));

  init_frames ();
  render_frame (frame_number);
  show_frame ();
}

/* Rendering Functions */

static void
render_frame (gint32 whichframe)
{
  GeglBuffer    *buffer;
  gint           i, j, k;
  guchar        *srcptr;
  guchar        *destptr;
  GtkWidget     *da;
  guint          drawing_width, drawing_height;
  gdouble        drawing_scale;
  guchar        *preview_data;

  g_assert (whichframe < total_frames);

  if (detached)
    {
      da = shape_drawing_area;
      preview_data = shape_drawing_area_data;
      drawing_width = shape_drawing_area_width;
      drawing_height = shape_drawing_area_height;
      drawing_scale = shape_scale;
    }
  else
    {
      da = drawing_area;
      preview_data = drawing_area_data;
      drawing_width = drawing_area_width;
      drawing_height = drawing_area_height;
      drawing_scale = scale;

      /* Set "alpha grid" background. */
      total_alpha_preview ();
    }

  buffer = gimp_drawable_get_buffer (frames[whichframe]);

  /* Fetch and scale the whole raw new frame */
  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, drawing_width, drawing_height),
                   drawing_scale, babl_format ("R'G'B'A u8"),
                   rawframe, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  /* Number of pixels. */
  i = drawing_width * drawing_height;
  destptr = preview_data;
  srcptr  = rawframe;
  while (i--)
    {
      if (! (srcptr[3] & 128))
        {
          srcptr  += 4;
          destptr += 3;
          continue;
        }

      *(destptr++) = *(srcptr++);
      *(destptr++) = *(srcptr++);
      *(destptr++) = *(srcptr++);

      srcptr++;
    }

  /* calculate the shape mask */
  if (detached)
    {
      memset (shape_preview_mask, 0, (drawing_width * drawing_height) / 8 + drawing_height);
      srcptr = rawframe + 3;

      for (j = 0; j < drawing_height; j++)
        {
          k = j * ((7 + drawing_width) / 8);

          for (i = 0; i < drawing_width; i++)
            {
              if ((*srcptr) & 128)
                shape_preview_mask[k + i/8] |= (1 << (i&7));

              srcptr += 4;
            }
        }
      reshape_from_bitmap (shape_preview_mask);
    }

  /* Display the preview buffer. */
  gdk_draw_rgb_image (gtk_widget_get_window (da),
                      (gtk_widget_get_style (da))->white_gc,
                      (gint) ((drawing_width - drawing_scale * width) / 2),
                      (gint) ((drawing_height - drawing_scale * height) / 2),
                      drawing_width, drawing_height,
                      (total_frames == 1 ?
                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                      preview_data, drawing_width * 3);

  /* clean up */
  g_object_unref (buffer);
}

static void
show_frame (void)
{
  gchar *text;

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
                                 ((gfloat) frame_number /
                                  (gfloat) (total_frames - 0.999)));

  text = g_strdup_printf (_("Frame %d of %d"), frame_number + 1, total_frames);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), text);
  g_free (text);
}

static void
update_alpha_preview (void)
{
  gint i;

  g_free (preview_alpha1_data);
  g_free (preview_alpha2_data);

  preview_alpha1_data = g_malloc (drawing_area_width * 3);
  preview_alpha2_data = g_malloc (drawing_area_width * 3);

  for (i = 0; i < drawing_area_width; i++)
    {
      if (i & 8)
        {
          preview_alpha1_data[i*3 + 0] =
          preview_alpha1_data[i*3 + 1] =
          preview_alpha1_data[i*3 + 2] = 102;
          preview_alpha2_data[i*3 + 0] =
          preview_alpha2_data[i*3 + 1] =
          preview_alpha2_data[i*3 + 2] = 154;
        }
      else
        {
          preview_alpha1_data[i*3 + 0] =
          preview_alpha1_data[i*3 + 1] =
          preview_alpha1_data[i*3 + 2] = 154;
          preview_alpha2_data[i*3 + 0] =
          preview_alpha2_data[i*3 + 1] =
          preview_alpha2_data[i*3 + 2] = 102;
        }
    }
}

static void
total_alpha_preview (void)
{
  gint i;

  for (i = 0; i < drawing_area_height; i++)
    {
      if (i & 8)
        memcpy (&drawing_area_data[i * 3 * drawing_area_width], preview_alpha1_data, 3 * drawing_area_width);
      else
        memcpy (&drawing_area_data[i * 3 * drawing_area_width], preview_alpha2_data, 3 * drawing_area_width);
    }
}

/* Util. */

static void
remove_timer (void)
{
  if (timer)
    {
      g_source_remove (timer);
      timer = 0;
    }
}

static void
do_back_step (void)
{
  if (frame_number == 0)
    frame_number = total_frames - 1;
  else
    frame_number = (frame_number - 1) % total_frames;
  render_frame (frame_number);
}

static void
do_step (void)
{
  frame_number = (frame_number + 1) % total_frames;
  render_frame (frame_number);
}


/*  Callbacks  */

static void
window_destroy (GtkWidget *widget)
{
  if (playing)
    remove_timer ();

  if (shape_window)
    gtk_widget_destroy (GTK_WIDGET (shape_window));

  gtk_main_quit ();
}


static gint
advance_frame_callback (gpointer data)
{
  gdouble duration;

  remove_timer();

  duration = frame_durations[(frame_number + 1) % total_frames];

  timer = g_timeout_add (duration * get_duration_factor (settings.duration_index),
                         advance_frame_callback, NULL);

  do_step ();
  show_frame ();

  return FALSE;
}


static void
play_callback (GtkToggleAction *action)
{
  if (playing)
    remove_timer ();

  playing = gtk_toggle_action_get_active (action);

  if (playing)
    {
      timer = g_timeout_add ((gdouble) frame_durations[frame_number] *
                             get_duration_factor (settings.duration_index),
                             advance_frame_callback, NULL);

      gtk_action_set_icon_name (GTK_ACTION (action), "media-playback-pause");
    }
  else
    {
      gtk_action_set_icon_name (GTK_ACTION (action), "media-playback-start");
    }

  g_object_set (action,
                "tooltip", playing ? _("Stop playback") : _("Start playback"),
                NULL);
}

static gdouble
get_duration_factor (gint index)
{
  switch (index)
    {
    case 0:
      return 0.125;
    case 1:
      return 0.25;
    case 2:
      return 0.5;
    case 3:
      return 1.0;
    case 4:
      return 2.0;
    case 5:
      return 4.0;
    case 6:
      return 8.0;
    default:
      return 1.0;
    }
}

static gint
get_fps (gint index)
{
  switch (index)
    {
    case 0:
      return 10;
    case 1:
      return 12;
    case 2:
      return 15;
    case 3:
      return 24;
    case 4:
      return 25;
    case 5:
      return 30;
    case 6:
      return 50;
    case 7:
      return 60;
    case 8:
      return 72;
    default:
      return 10;
    }
}

static gdouble
get_scale (gint index)
{
  switch (index)
    {
    case 0:
      return 0.51;
    case 1:
      return 1.0;
    case 2:
      return 1.25;
    case 3:
      return 1.5;
    case 4:
      return 2.0;
    default:
      {
        /* likely -1 returned if there is no active item from the list.
         * Try a text conversion, locale-aware in such a case, assuming people write in percent. */
        gchar* active_text = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (zoomcombo));
        gdouble zoom = g_strtod (active_text, NULL);

        /* Negative scales are inconsistent. And we want to avoid huge scaling. */
        if (zoom > 400.0)
          zoom = 400.0;
        else if (zoom <= 50.0)
          /* FIXME: scales under 0.5 are broken. See bug 690265. */
          zoom = 50.1;
        g_free (active_text);
        return zoom / 100.0;
      }
    }
}

static void
step_back_callback (GtkAction *action)
{
  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));
  do_back_step();
  show_frame();
}

static void
step_callback (GtkAction *action)
{
  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));
  do_step();
  show_frame();
}

static void
refresh_callback (GtkAction *action)
{
  initialize ();
}

static void
rewind_callback (GtkAction *action)
{
  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));
  frame_number = 0;
  render_frame (frame_number);
  show_frame ();
}

static void
speed_up_callback (GtkAction *action)
{
  if (settings.duration_index > 0)
    --settings.duration_index;

  gtk_action_set_sensitive (action, settings.duration_index > 0);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, settings.duration_index != 3);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, TRUE);

  update_combobox ();
}

static void
speed_down_callback (GtkAction *action)
{
  if (settings.duration_index < 6)
    ++settings.duration_index;

  gtk_action_set_sensitive (action, settings.duration_index < 6);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, settings.duration_index != 3);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, TRUE);

  update_combobox ();
}

static void
speed_reset_callback (GtkAction *action)
{
  settings.duration_index = 3;

  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, TRUE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, TRUE);

  update_combobox ();
}

static void
framecombo_changed (GtkWidget *combo, gpointer data)
{
  settings.default_frame_disposal = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  init_frames ();
  render_frame (frame_number);
}

static void
speedcombo_changed (GtkWidget *combo, gpointer data)
{
  GtkAction * action;

  settings.duration_index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, settings.duration_index != 3);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, settings.duration_index < 6);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, settings.duration_index > 0);
}

static void
update_scale (gdouble scale)
{
  guint expected_drawing_area_width;
  guint expected_drawing_area_height;

  /* FIXME: scales under 0.5 are broken. See bug 690265. */
  if (scale <= 0.5)
    scale = 0.501;

  expected_drawing_area_width = width * scale;
  expected_drawing_area_height = height * scale;

  gtk_widget_set_size_request (drawing_area, expected_drawing_area_width, expected_drawing_area_height);
  gtk_widget_set_size_request (shape_drawing_area, expected_drawing_area_width, expected_drawing_area_height);
  /* I force the shape window to a smaller size if we scale down. */
  if (detached)
    {
      gint x, y;

      gdk_window_get_origin (gtk_widget_get_window (shape_window), &x, &y);
      gtk_window_reshow_with_initial_size (GTK_WINDOW (shape_window));
      gtk_window_move (GTK_WINDOW (shape_window), x, y);
    }
}

/*
 * Callback emitted when the user hits the Enter key of the zoom combo.
 */
static void
zoomcombo_activated (GtkEntry *combo, gpointer data)
{
  update_scale (get_scale (-1));
}

/*
 * Callback emitted when the user selects a zoom in the dropdown,
 * or edits the text entry.
 * We don't want to process manual edits because it greedily emits
 * signals after each character deleted or added.
 */
static void
zoomcombo_changed (GtkWidget *combo, gpointer data)
{
  gint index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* If no index, user is probably editing by hand. We wait for him to click "Enter". */
  if (index != -1)
    update_scale (get_scale (index));
}

static void
fpscombo_changed (GtkWidget *combo, gpointer data)
{
  settings.default_frame_duration = 1000 / get_fps (gtk_combo_box_get_active (GTK_COMBO_BOX (combo)));
}

static void
update_combobox (void)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (speedcombo), settings.duration_index);
}

/* tag util. */

static gboolean
is_ms_tag (const gchar *str,
           gint        *duration,
           gint        *taglength)
{
  gint sum = 0;
  gint offset;
  gint length;

  length = strlen(str);

  if (str[0] != '(')
    return FALSE;

  offset = 1;

  /* eat any spaces between open-parenthesis and number */
  while ((offset < length) && (str[offset] == ' '))
    offset++;

  if ((offset>=length) || (!g_ascii_isdigit (str[offset])))
    return FALSE;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset<length) && (g_ascii_isdigit (str[offset])));

  if (length - offset <= 2)
    return FALSE;

  /* eat any spaces between number and 'ms' */
  while ((offset < length) && (str[offset] == ' '))
    offset++;

  if (length - offset <= 2                     ||
      g_ascii_toupper (str[offset])     != 'M' ||
      g_ascii_toupper (str[offset + 1]) != 'S')
    return FALSE;

  offset += 2;

  /* eat any spaces between 'ms' and close-parenthesis */
  while ((offset < length) && (str[offset] == ' '))
    offset++;

  if ((length - offset < 1) || (str[offset] != ')'))
    return FALSE;

  offset++;

  *duration = sum;
  *taglength = offset;

  return TRUE;
}

static gint
parse_ms_tag (const gchar *str)
{
  gint i;
  gint length = strlen (str);

  for (i = 0; i < length; i++)
    {
      gint rtn;
      gint dummy;

      if (is_ms_tag (&str[i], &rtn, &dummy))
        return rtn;
    }

  return -1;
}

static gboolean
is_disposal_tag (const gchar *str,
                 DisposeType *disposal,
                 gint        *taglength)
{
  if (strlen (str) != 9)
    return FALSE;

  if (strncmp (str, "(combine)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_COMBINE;
      return TRUE;
    }
  else if (strncmp (str, "(replace)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_REPLACE;
      return TRUE;
    }

  return FALSE;
}

static DisposeType
parse_disposal_tag (const gchar *str)
{
  gint i;
  gint length = strlen (str);

  for (i = 0; i < length; i++)
    {
      DisposeType rtn;
      gint        dummy;

      if (is_disposal_tag (&str[i], &rtn, &dummy))
        return rtn;
    }

  return settings.default_frame_disposal;
}

