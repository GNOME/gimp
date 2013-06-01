/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 * (c) Mircea Purdea : 2009 : someone_else@exhalus.net
 * (c) Jehan : 2012-2013 : jehan at girinstud.io
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
  DISPOSE_REPLACE   = 0x01,
  DISPOSE_TAGS      = 0x02,
} DisposeType;

typedef struct
{
  gint32  drawable_id;
  GList  *indexes;
  GList  *updated_indexes;
}
Frame;

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
static void        startframe_changed        (GtkAdjustment   *adjustment,
                                              gpointer         user_data);
static void        endframe_changed          (GtkAdjustment   *adjustment,
                                              gpointer         user_data);
static void        quality_checkbox_toggled  (GtkToggleButton *button,
                                              gpointer         data);
static gboolean    repaint_sda               (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);
static gboolean    repaint_da                (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);

static void        init_quality_checkbox     (void);
static void        init_frames               (void);
static void        render_frame              (guint            whichframe);
static void        show_playing_progress     (void);
static void        show_loading_progress     (gint             layer_nb);
static void        show_goto_progress        (guint            frame_nb);
static void        total_alpha_preview       (void);
static void        update_alpha_preview      (void);
static void        update_combobox           (void);
static gdouble     get_duration_factor       (gint             index);
static gint        get_fps                   (gint             index);
static gdouble     get_scale                 (gint             index);
static void        update_scale              (gdouble          scale);

/* Utils */
static void        remove_timer              (void);
static void        clean_exit                (void);

/* tag util functions*/
static gint        parse_ms_tag              (const gchar     *str);
static void        set_total_frames          (void);
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
static gulong             destroy_handler;
static GdkWindow         *root_win                  = NULL;
static GtkUIManager      *ui_manager                = NULL;
static GtkWidget         *progress;
static GtkWidget         *speedcombo                = NULL;
static GtkWidget         *fpscombo                  = NULL;
static GtkWidget         *zoomcombo                 = NULL;
static GtkWidget         *quality_checkbox          = NULL;
static GtkWidget         *frame_disposal_combo      = NULL;
static GtkAdjustment     *startframe_adjust         = NULL;
static GtkWidget         *startframe_spin           = NULL;
static GtkAdjustment     *endframe_adjust           = NULL;
static GtkWidget         *endframe_spin             = NULL;

static gint32             image_id;
static guint              width                     = -1,
                          height                    = -1,
                          preview_width, preview_height;
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

static gint32             frames_image_id;
static gint32             total_frames              = 0;
static Frame            **frames                    = NULL;
static guchar            *rawframe                  = NULL;
static guint32           *frame_durations           = NULL;
static guint              frame_number;
static guint              frame_number_min, frame_number_max;
static guint              start_frame, end_frame;
/* Since the application is single-thread, a boolean is enough.
 * It may become a mutex in the future with multi-thread support. */
static gboolean           frames_lock               = FALSE;

static gboolean           playing                   = FALSE;
static gboolean           force_render              = TRUE;
static gboolean           initialized_once          = FALSE;
static guint              timer                     = 0;
static gboolean           detached                  = FALSE;
static gdouble            scale, shape_scale;
static gulong             progress_entered_handler, progress_motion_handler,
                          progress_left_handler, progress_button_handler;

/* Some regexp used to parse layer tags. */
static GRegex* nospace_reg = NULL;
static GRegex* layers_reg = NULL;
static GRegex* all_reg = NULL;

/* Default settings. */
static AnimationSettings settings =
{
  3,
  DISPOSE_COMBINE,
  100 /* ms */
};

static gint32 frames_image_id = 0;

static void
clean_exit (void)
{
  if (playing)
    remove_timer ();

  if (frames_image_id)
    gimp_image_delete (frames_image_id);
  frames_image_id = 0;

  if (shape_window)
    gtk_widget_destroy (GTK_WIDGET (shape_window));

  g_signal_handler_disconnect (window, destroy_handler);
  if (window)
    gtk_widget_destroy (GTK_WIDGET (window));

  if (frames)
    g_free (frames);
  if (frame_durations)
    g_free (frame_durations);

  gegl_exit ();
  gtk_main_quit ();
  gimp_quit ();
}

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
  GeglConfig       *config;

  INIT_I18N ();
  gegl_init (NULL, NULL);
  config = gegl_config ();
  /* For preview, we want fast (0.0) over high quality (1.0). */
  g_object_set (config, "quality", 0.0, NULL);

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

      /* Frame parsing. */
      nospace_reg = g_regex_new("[ \t]*", G_REGEX_OPTIMIZE, 0, NULL);
      layers_reg = g_regex_new("\\[(([0-9]+(-[0-9]+)?)(,[0-9]+(-[0-9]+)?)*)\\]", G_REGEX_OPTIMIZE, 0, NULL);
      all_reg = g_regex_new("\\[\\*\\]", G_REGEX_OPTIMIZE, 0, NULL);

      initialize ();

      /* At least one full initialization finished. */
      initialized_once = TRUE;

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
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL, NULL, NULL,
                  event ? event->button : 0,
                  event ? event->time   : gtk_get_current_event_time ());

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
  scale = MIN ((gdouble) drawing_area_width / (gdouble) preview_width,
               (gdouble) drawing_area_height / (gdouble) preview_height);

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
      force_render = TRUE;
      if (frame_number - frame_number_min < total_frames)
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
  shape_scale = MIN ((gdouble) shape_drawing_area_width / (gdouble) preview_width,
                     (gdouble) shape_drawing_area_height / (gdouble) preview_height);

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

      force_render = TRUE;
      if (frame_number - frame_number_min < total_frames)
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
adjustment_pressed (GtkWidget      *widget,
                    GdkEventButton *event,
                    gpointer        user_data)
{
  if (event->type == GDK_2BUTTON_PRESS)
    {
      GtkSpinButton *spin = GTK_SPIN_BUTTON (widget);
      GtkAdjustment *adj = gtk_spin_button_get_adjustment (spin);

      gtk_adjustment_set_value (adj, (gdouble) frame_number);
    }

  return FALSE;
}

static gboolean
progress_button (GtkWidget      *widget,
                 GdkEventButton *event,
                 gpointer        user_data)
{
  /* ignore double and triple click */
  if (event->type == GDK_BUTTON_PRESS)
    {
      GtkAllocation  allocation;
      guint          goto_frame;

      gtk_widget_get_allocation (widget, &allocation);

      goto_frame = frame_number_min + (gint) (event->x / (allocation.width / total_frames));

      if (goto_frame < start_frame)
        gtk_adjustment_set_value (startframe_adjust, (gdouble) goto_frame);

      if (goto_frame > end_frame)
        gtk_adjustment_set_value (endframe_adjust, (gdouble) goto_frame);

      if (goto_frame >= frame_number_min && goto_frame < frame_number_min + total_frames)
        {
          frame_number = goto_frame;
          render_frame (frame_number);
        }
    }

  return FALSE;
}

static gboolean
progress_entered (GtkWidget        *widget,
                  GdkEventCrossing *event,
                  gpointer          user_data)
{
  GtkAllocation  allocation;
  guint          goto_frame;

  gtk_widget_get_allocation (widget, &allocation);

  goto_frame = frame_number_min + (gint) (event->x / (allocation.width / total_frames));

  show_goto_progress (goto_frame);

  return FALSE;
}

static gboolean
progress_motion (GtkWidget      *widget,
                 GdkEventMotion *event,
                 gpointer        user_data)
{
  GtkAllocation  allocation;
  guint          goto_frame;

  gtk_widget_get_allocation (widget, &allocation);

  goto_frame = frame_number_min + (gint) (event->x / (allocation.width / total_frames));

  show_goto_progress (goto_frame);

  return FALSE;
}

static gboolean
progress_left (GtkWidget        *widget,
               GdkEventCrossing *event,
               gpointer          user_data)
{
  show_playing_progress ();

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
                      (gint) ((drawing_area_width - scale * preview_width) / 2),
                      (gint) ((drawing_area_height - scale * preview_height) / 2),
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
                      (gint) ((shape_drawing_area_width - shape_scale * preview_width) / 2),
                      (gint) ((shape_drawing_area_height - shape_scale * preview_height) / 2),
                      shape_drawing_area_width, shape_drawing_area_height,
                      (total_frames == 1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      shape_drawing_area_data, shape_drawing_area_width * 3);

  return TRUE;
}

static void
close_callback (GtkAction *action,
                gpointer   data)
{
  clean_exit ();
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
      repaint_da (drawing_area, NULL, NULL);
    }
  else
    gtk_widget_hide (shape_window);

  force_render = TRUE;
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

  /* Update the presence of quality checkbox. */
  init_quality_checkbox ();
  quality_checkbox_toggled (GTK_TOGGLE_BUTTON (quality_checkbox), NULL);

  /* if the *window* size is bigger than the screen size,
   * diminish the drawing area by as much, then compute the corresponding scale. */
  if (window_width + 50 > screen_width || window_height + 50 > screen_height)
  {
      gint expected_drawing_area_width = MAX (1, preview_width - window_width + screen_width);
      gint expected_drawing_area_height = MAX (1, preview_height - window_height + screen_height);

      gdouble expected_scale = MIN ((gdouble) expected_drawing_area_width / (gdouble) preview_width,
                                    (gdouble) expected_drawing_area_height / (gdouble) preview_height);
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
  GtkWidget   *progress_hbox;
  GtkWidget   *config_hbox;
  GtkToolItem *item;
  GtkAction   *action;
  GdkCursor   *cursor;
  gint         index;
  gchar       *text;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_role (GTK_WINDOW (window), "animation-playback");

  destroy_handler = g_signal_connect (window, "destroy",
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

  /* Vbox for the preview window and lower option bar */

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

  hbox = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Progress box. */

  progress_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_add2 (GTK_PANED (hbox), progress_hbox);
  gtk_widget_show (progress_hbox);

  /* End frame spin button. */

  endframe_adjust = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 5.0, 0.0));
  endframe_spin = gtk_spin_button_new (endframe_adjust, 1.0, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (endframe_spin), 2);

  gtk_box_pack_end (GTK_BOX (progress_hbox), endframe_spin, FALSE, FALSE, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (endframe_spin), GTK_UPDATE_IF_VALID);
  gtk_widget_show (endframe_spin);

  g_signal_connect (endframe_adjust,
                    "value-changed",
                    G_CALLBACK (endframe_changed),
                    NULL);

  g_signal_connect (endframe_spin, "button-press-event",
                    G_CALLBACK (adjustment_pressed),
                    NULL);

  gimp_help_set_help_data (endframe_spin, _("End frame"), NULL);

  /* Progress bar. */

  progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (progress_hbox), progress, TRUE, TRUE, 0);
  gtk_widget_show (progress);

  gtk_widget_add_events (progress,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                         GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
  progress_entered_handler = g_signal_connect (progress, "enter-notify-event",
                                               G_CALLBACK (progress_entered),
                                               NULL);
  progress_left_handler = g_signal_connect (progress, "leave-notify-event",
                                            G_CALLBACK (progress_left),
                                            NULL);
  progress_motion_handler = g_signal_connect (progress, "motion-notify-event",
                                              G_CALLBACK (progress_motion),
                                              NULL);
  progress_button_handler = g_signal_connect (progress, "button-press-event",
                                              G_CALLBACK (progress_button),
                                              NULL);

  /* Start frame spin button. */

  startframe_adjust = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 5.0, 0.0));
  startframe_spin = gtk_spin_button_new (startframe_adjust, 1.0, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (startframe_spin), 2);

  gtk_box_pack_end (GTK_BOX (progress_hbox), startframe_spin, FALSE, FALSE, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (startframe_spin), GTK_UPDATE_IF_VALID);
  gtk_widget_show (startframe_spin);

  g_signal_connect (startframe_adjust,
                    "value-changed",
                    G_CALLBACK (startframe_changed),
                    NULL);

  g_signal_connect (startframe_spin, "button-press-event",
                    G_CALLBACK (adjustment_pressed),
                    NULL);

  gimp_help_set_help_data (startframe_spin, _("Start frame"), NULL);

  /* Configuration box. */

  config_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_add1 (GTK_PANED (hbox), config_hbox);
  gtk_widget_show (config_hbox);

  /* Degraded quality animation preview. */
  quality_checkbox = gtk_check_button_new_with_label (_("Preview Quality"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (quality_checkbox), FALSE);
  gtk_box_pack_end (GTK_BOX (config_hbox), quality_checkbox, FALSE, FALSE, 0);
  gtk_widget_show (quality_checkbox);

  init_quality_checkbox ();
  quality_checkbox_toggled (GTK_TOGGLE_BUTTON (quality_checkbox), NULL);

  g_signal_connect (GTK_TOGGLE_BUTTON (quality_checkbox),
                    "toggled",
                    G_CALLBACK (quality_checkbox_toggled),
                    NULL);

  gimp_help_set_help_data (quality_checkbox, _("Degraded image quality for low memory footprint"), NULL);

  /* Zoom */
  zoomcombo = gtk_combo_box_text_new_with_entry ();
  gtk_box_pack_end (GTK_BOX (config_hbox), zoomcombo, FALSE, FALSE, 0);
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
  gtk_box_pack_end (GTK_BOX (config_hbox), fpscombo, FALSE, FALSE, 0);
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
  gtk_box_pack_end (GTK_BOX (config_hbox), speedcombo, FALSE, FALSE, 0);
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

  text = g_strdup (_("Use layer tags (custom)"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (frame_disposal_combo), DISPOSE_TAGS, text);
  g_free (text);

  gtk_combo_box_set_active (GTK_COMBO_BOX (frame_disposal_combo), settings.default_frame_disposal);

  g_signal_connect (frame_disposal_combo, "changed",
                    G_CALLBACK (framecombo_changed),
                    NULL);

  gtk_box_pack_end (GTK_BOX (config_hbox), frame_disposal_combo, FALSE, FALSE, 0);
  gtk_widget_show (frame_disposal_combo);

  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (window), preview_width + 20, preview_height + 90);
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
  gdk_cursor_unref (cursor);

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
  gtk_widget_set_size_request (drawing_area, preview_width, preview_height);
  gtk_widget_set_size_request (shape_drawing_area, preview_width, preview_height);

  root_win = gdk_get_default_root_window ();
}

static void
init_quality_checkbox (void)
{
  GdkScreen         *screen;
  guint              screen_width, screen_height;
  static const gint  lower_quality_frame_limit = 10;

  screen = gtk_widget_get_screen (window);
  screen_height = gdk_screen_get_height (screen);
  screen_width = gdk_screen_get_width (screen);

  gtk_widget_show (quality_checkbox);
  /* Small resolution, no need for this button. */
  if (screen_width / 2 >= width && screen_height / 2 >= height)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (quality_checkbox), FALSE);
      gtk_widget_hide (quality_checkbox);
    }
  /* We will set a lower quality as default if image is more than the screen size
   * and there are more than a given limit of frames. */
  else if (total_frames > lower_quality_frame_limit &&
          (width > screen_width || height > screen_height))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (quality_checkbox), TRUE);
}

static void
init_frames (void)
{
  /* Frames are associated to an unused image. */
  static GList    *previous_frames = NULL;
  gint32           new_frame, previous_frame, new_layer;
  gboolean         animated;
  GtkAction       *action;
  gint             duration = 0;
  DisposeType      disposal = settings.default_frame_disposal;
  gchar           *layer_name;
  gint             layer_offx, layer_offy;
  gboolean         preview_quality;
  gint             frame_spin_size;

  if (total_frames <= 0)
    {
      gimp_message (_("This animation has no frame."));
      clean_exit ();
      return;
    }

  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));

  if (frames_lock)
    return;
  frames_lock = TRUE;

  /* Block the frame-related UI during frame initialization. */
  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/play");
  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/step-back");
  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/step");
  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/rewind");
  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/refresh");
  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/detach");
  gtk_action_set_sensitive (action, FALSE);

  gtk_widget_set_sensitive (GTK_WIDGET (zoomcombo), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (frame_disposal_combo), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (quality_checkbox), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (startframe_spin), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (endframe_spin), FALSE);

  g_signal_handler_block (progress, progress_entered_handler);
  g_signal_handler_block (progress, progress_left_handler);
  g_signal_handler_block (progress, progress_motion_handler);
  g_signal_handler_block (progress, progress_button_handler);

  /* Cleanup before re-generation. */
  if (frames)
    {
      GList *idx;

      gimp_image_delete (frames_image_id);

      /* Freeing previous frames. */
      for (idx = g_list_first (previous_frames); idx != NULL; idx = g_list_next (idx))
        {
          Frame* frame = (Frame*) idx->data;

          g_list_free (frame->indexes);
          g_list_free (frame->updated_indexes);
          g_free (frame);
        }
      g_list_free (previous_frames);
      previous_frames = NULL;

      g_free (frames);
      g_free (frame_durations);
    }

  frames = g_try_malloc0_n (total_frames, sizeof (Frame*));
  frame_durations = g_try_malloc0_n (total_frames, sizeof (guint32));
  if (! frames || ! frame_durations)
    {
      frames_lock = FALSE;
      gimp_message (_("Memory could not be allocated to the frame container."));
      clean_exit ();
      return;
    }

  /* We only use RGB images for display because indexed images would somehow
     render terrible colors. Layers from other types will be automatically
     converted. */
  frames_image_id = gimp_image_new (preview_width, preview_height, GIMP_RGB);

  /* Save processing time and memory by not saving history and merged frames. */
  gimp_image_undo_disable (frames_image_id);

  preview_quality = preview_width != width || preview_height != height;

  if (disposal == DISPOSE_TAGS)
    {
      gint        i, j, k;
      gchar*      nospace_name;
      GMatchInfo *match_info;

      for (i = 0; i < total_layers; i++)
        {
          Frame *empty_frame = NULL;

          show_loading_progress (i);
          layer_name = gimp_item_get_name (layers[total_layers - (i + 1)]);
          nospace_name = g_regex_replace_literal (nospace_reg, layer_name, -1, 0, "", 0, NULL);
          if (g_regex_match (all_reg, nospace_name, 0, NULL))
            {
              for (j = 0; j < total_frames; j++)
                {
                  if (! frames[j])
                    {
                      if (! empty_frame)
                        {
                          empty_frame = g_new (Frame, 1);
                          empty_frame->indexes = NULL;
                          empty_frame->updated_indexes = NULL;
                          empty_frame->drawable_id = 0;

                          previous_frames = g_list_append (previous_frames, empty_frame);
                        }

                      empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (j));

                      frames[j] = empty_frame;
                    }
                  else if (! g_list_find (frames[j]->updated_indexes, GINT_TO_POINTER (j)))
                    frames[j]->updated_indexes = g_list_append (frames[j]->updated_indexes, GINT_TO_POINTER (j));
                }
            }
          else
            {
              g_regex_match (layers_reg, nospace_name, 0, &match_info);

              while (g_match_info_matches (match_info))
                {
                  gchar *tag = g_match_info_fetch (match_info, 1);
                  gchar** tokens = g_strsplit(tag, ",", 0);

                  for (j = 0; tokens[j] != NULL; j++)
                    {
                      gchar* hyphen = g_strrstr(tokens[j], "-");

                      if (hyphen != NULL)
                        {
                          gint32 first = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);
                          gint32 second = (gint32) g_ascii_strtoll (&hyphen[1], NULL, 10);

                          for (k = first; k <= second; k++)
                            {
                              if (! frames[k - frame_number_min])
                                {
                                  if (! empty_frame)
                                    {
                                      empty_frame = g_new (Frame, 1);
                                      empty_frame->indexes = NULL;
                                      empty_frame->updated_indexes = NULL;
                                      empty_frame->drawable_id = 0;

                                      previous_frames = g_list_append (previous_frames, empty_frame);
                                    }

                                  empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (k - frame_number_min));

                                  frames[k - frame_number_min] = empty_frame;
                                }
                              else if (! g_list_find (frames[k - frame_number_min]->updated_indexes, GINT_TO_POINTER (k - frame_number_min)))
                                frames[k - frame_number_min]->updated_indexes = g_list_append (frames[k - frame_number_min]->updated_indexes,
                                                                                               GINT_TO_POINTER (k - frame_number_min));
                            }
                        }
                      else
                        {
                          gint32 num = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);

                          if (! frames[num - frame_number_min])
                            {
                              if (! empty_frame)
                                {
                                  empty_frame = g_new (Frame, 1);
                                  empty_frame->indexes = NULL;
                                  empty_frame->updated_indexes = NULL;
                                  empty_frame->drawable_id = 0;

                                  previous_frames = g_list_append (previous_frames, empty_frame);
                                }

                              empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (num - frame_number_min));

                              frames[num - frame_number_min] = empty_frame;
                            }
                          else if (! g_list_find (frames[num - frame_number_min]->updated_indexes, GINT_TO_POINTER (num - frame_number_min)))
                            frames[num - frame_number_min]->updated_indexes = g_list_append (frames[num - frame_number_min]->updated_indexes,
                                                                                             GINT_TO_POINTER (num - frame_number_min));
                        }
                    }
                  g_strfreev (tokens);
                  g_free (tag);
                  g_match_info_next (match_info, NULL);
                }
              g_match_info_free (match_info);
            }

          for (j = 0; j < total_frames; j++)
            {
                /* Check which frame must be updated with the current layer. */
                if (frames[j] && g_list_length (frames[j]->updated_indexes))
                  {
                    new_layer = gimp_layer_new_from_drawable (layers[total_layers - (i + 1)], frames_image_id);
                    gimp_image_insert_layer (frames_image_id, new_layer, 0, 0);

                    if (preview_quality)
                      {
                        gimp_layer_scale (new_layer,
                                          (gimp_drawable_width (layers[total_layers - (i + 1)]) * (gint) preview_width) / (gint) width,
                                          (gimp_drawable_height (layers[total_layers - (i + 1)]) * (gint) preview_height) / (gint) height,
                                          FALSE);
                        gimp_drawable_offsets (layers[total_layers - (i + 1)], &layer_offx, &layer_offy);
                        gimp_layer_set_offsets (new_layer, (layer_offx * (gint) preview_width) / (gint) width,
                                                (layer_offy * (gint) preview_height) / (gint) height);
                      }
                    gimp_layer_resize_to_image_size (new_layer);

                    if (frames[j]->drawable_id == 0)
                      {
                        frames[j]->drawable_id = new_layer;
                        frames[j]->indexes = frames[j]->updated_indexes;
                        frames[j]->updated_indexes = NULL;
                      }
                    else if (g_list_length (frames[j]->indexes) == g_list_length (frames[j]->updated_indexes))
                      {
                        gimp_item_set_visible (new_layer, TRUE);
                        gimp_item_set_visible (frames[j]->drawable_id, TRUE);

                        frames[j]->drawable_id = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
                        g_list_free (frames[j]->updated_indexes);
                        frames[j]->updated_indexes = NULL;
                      }
                    else
                      {
                        GList  *idx;
                        Frame  *forked_frame = g_new (Frame, 1);
                        gint32  forked_drawable_id = gimp_layer_new_from_drawable (frames[j]->drawable_id, frames_image_id);

                        /* if part only of the frames are updated, we fork the existing frame. */
                        gimp_image_insert_layer (frames_image_id, forked_drawable_id, 0, 1);
                        gimp_item_set_visible (new_layer, TRUE);
                        gimp_item_set_visible (forked_drawable_id, TRUE);
                        forked_drawable_id = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);

                        forked_frame->drawable_id = forked_drawable_id;
                        forked_frame->indexes = frames[j]->updated_indexes;
                        forked_frame->updated_indexes = NULL;
                        frames[j]->updated_indexes = NULL;

                        for (idx = g_list_first (forked_frame->indexes); idx != NULL; idx = g_list_next (idx))
                          {
                            frames[j]->indexes = g_list_remove (frames[j]->indexes, idx->data);
                            /* Frame j must also be moved to the forked frame, but only after the loop. */
                            if (GPOINTER_TO_INT (idx->data) != j)
                              frames[GPOINTER_TO_INT (idx->data)] = forked_frame;
                          }
                        frames[j] = forked_frame;
                        gimp_item_set_visible (forked_drawable_id, FALSE);

                        previous_frames = g_list_append (previous_frames, forked_frame);
                      }

                    gimp_item_set_visible (frames[j]->drawable_id, FALSE);
                  }
            }

          g_free (layer_name);
          g_free (nospace_name);
        }

      for (i = 0; i < total_frames; i++)
        {
          /* If for some reason a frame is absent, use the previous one.
           * We are ensured that there is at least a "first" frame for this. */
          if (! frames[i])
            {
              frames[i] = frames[i - 1];
              frames[i]->indexes = g_list_append (frames[i]->indexes, GINT_TO_POINTER (i));
            }

          frame_durations[i] = settings.default_frame_duration;
        }
    }
  else
    {
      gint i;

      for (i = 0; i < total_frames; i++)
        {
          show_loading_progress (i);

          frames[i] = g_new (Frame, 1);
          frames[i]->indexes = NULL;
          frames[i]->indexes = g_list_append (frames[i]->indexes, GINT_TO_POINTER (i));
          frames[i]->updated_indexes = NULL;

          previous_frames = g_list_append (previous_frames, frames[i]);

          layer_name = gimp_item_get_name (layers[total_layers - (i + 1)]);
          if (layer_name)
            {
              duration = parse_ms_tag (layer_name);
              disposal = parse_disposal_tag (layer_name);
              g_free (layer_name);
            }

          if (i > 0 && disposal != DISPOSE_REPLACE)
            {
              previous_frame = gimp_layer_copy (frames[i - 1]->drawable_id);

              gimp_image_insert_layer (frames_image_id, previous_frame, 0, 0);
              gimp_item_set_visible (previous_frame, TRUE);
            }

          new_layer = gimp_layer_new_from_drawable (layers[total_layers - (i + 1)], frames_image_id);

          gimp_image_insert_layer (frames_image_id, new_layer, 0, 0);
          gimp_item_set_visible (new_layer, TRUE);
          gimp_layer_scale (new_layer, (gimp_drawable_width (layers[total_layers - (i + 1)]) * (gint) preview_width) / (gint) width,
                            (gimp_drawable_height (layers[total_layers - (i + 1)]) * (gint) preview_height) / (gint) height, FALSE);
          gimp_drawable_offsets (layers[total_layers - (i + 1)], &layer_offx, &layer_offy);
          gimp_layer_set_offsets (new_layer, (layer_offx * (gint) preview_width) / (gint) width,
                                  (layer_offy * (gint) preview_height) / (gint) height);
          gimp_layer_resize_to_image_size (new_layer);

          new_frame = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
          frames[i]->drawable_id = new_frame;
          gimp_item_set_visible (new_frame, FALSE);

          if (duration <= 0)
            duration = settings.default_frame_duration;
          frame_durations[i] = (guint32) duration;
        }
    }

  /* Update the UI. */
  frame_spin_size = (gint) (log10 (frame_number_max - (frame_number_max % 10))) + 1;
  gtk_entry_set_width_chars (GTK_ENTRY (startframe_spin), frame_spin_size);
  gtk_entry_set_width_chars (GTK_ENTRY (endframe_spin), frame_spin_size);

  gtk_adjustment_configure (startframe_adjust, start_frame, frame_number_min, frame_number_max, 1.0, 5.0, 0.0);
  gtk_adjustment_configure (endframe_adjust, end_frame, start_frame, frame_number_max, 1.0, 5.0, 0.0);

  animated = end_frame - start_frame >= 1;
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

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/refresh");
  gtk_action_set_sensitive (action, TRUE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/anim-play-toolbar/detach");
  gtk_action_set_sensitive (action, TRUE);

  gtk_widget_set_sensitive (GTK_WIDGET (zoomcombo), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (frame_disposal_combo), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (quality_checkbox), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (startframe_spin), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (endframe_spin), TRUE);

  g_signal_handler_unblock (progress, progress_entered_handler);
  g_signal_handler_unblock (progress, progress_left_handler);
  g_signal_handler_unblock (progress, progress_motion_handler);
  g_signal_handler_unblock (progress, progress_button_handler);

  /* Keep the same frame number, unless it is now invalid. */
  if (frame_number > end_frame || frame_number < start_frame)
    frame_number = start_frame;

  force_render = TRUE;

  frames_lock = FALSE;

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
      clean_exit ();
      return;
    }

  width     = gimp_image_width (image_id);
  height    = gimp_image_height (image_id);
  layers    = gimp_image_get_layers (image_id, &total_layers);

  set_total_frames ();

  if (!window)
    build_dialog (gimp_image_get_name (image_id));
  refresh_dialog (gimp_image_get_name (image_id));

  /* I want to make sure the progress bar is realized before init_frames()
   * which may take quite a bit of time. */
  if (!gtk_widget_get_realized (progress))
    gtk_widget_realize (progress);

  init_frames ();
  render_frame (frame_number);
}

/* Rendering Functions */

static void
render_frame (guint whichframe)
{
  static gint    last_frame_index = -1;
  GeglBuffer    *buffer;
  gint           i, j, k;
  guchar        *srcptr;
  guchar        *destptr;
  GtkWidget     *da;
  guint          drawing_width, drawing_height;
  gdouble        drawing_scale;
  guchar        *preview_data;

  /* Do not try to update the drawing areas while init_frames() is still running. */
  if (frames_lock)
    return;

  /* Unless we are in a case where we always want to redraw
   * (after a zoom, preview mode change, reinitialization, and such),
   * we don't redraw if the same frame was already drawn. */
  if ((! force_render) && last_frame_index > -1 &&
      g_list_find (frames[last_frame_index]->indexes, GINT_TO_POINTER (whichframe - frame_number_min)))
    {
      show_playing_progress ();
      return;
    }

  frames_lock = TRUE;

  g_assert (whichframe >= start_frame && whichframe <= end_frame);

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

  buffer = gimp_drawable_get_buffer (frames[whichframe - frame_number_min]->drawable_id);

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
                      (gint) ((drawing_width - drawing_scale * preview_width) / 2),
                      (gint) ((drawing_height - drawing_scale * preview_height) / 2),
                      drawing_width, drawing_height,
                      (total_frames == 1 ?
                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                      preview_data, drawing_width * 3);

  /* clean up */
  g_object_unref (buffer);

  /* Update UI. */
  show_playing_progress ();

  last_frame_index = whichframe - frame_number_min;
  force_render = FALSE;

  frames_lock = FALSE;
}


static void
show_goto_progress (guint goto_frame)
{
  gchar         *text;

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
                                 ((gfloat) (frame_number - frame_number_min) /
                                  (gfloat) (total_frames - 0.999)));

  if (settings.default_frame_disposal != DISPOSE_TAGS || frame_number_min == 1)
    text = g_strdup_printf (_("Go to frame %d of %d"), goto_frame, total_frames);
  else
    text = g_strdup_printf (_("Go to frame %d (%d) of %d"), goto_frame - frame_number_min + 1, goto_frame, total_frames);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), text);
  g_free (text);
}

static void
show_loading_progress (gint layer_nb)
{
  gchar *text;
  gfloat load_rate = (gfloat) layer_nb / ((gfloat) total_layers - 0.999);

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), load_rate);

  text = g_strdup_printf (_("Loading animation %d %%"), (gint) (load_rate * 100));
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), text);
  g_free (text);

  /* Forcing the UI to update even with intensive computation. */
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static void
show_playing_progress (void)
{
  gchar *text;

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
                                 ((gfloat) (frame_number - frame_number_min) /
                                  (gfloat) (total_frames - 0.999)));

  if (settings.default_frame_disposal != DISPOSE_TAGS || frame_number_min == 1)
    text = g_strdup_printf (_("Frame %d of %d"), frame_number, total_frames);
  else
    text = g_strdup_printf (_("Frame %d (%d) of %d"), frame_number - frame_number_min + 1, frame_number, total_frames);
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
  if (frame_number == start_frame)
    frame_number = end_frame;
  else
    frame_number = frame_number - 1;
  render_frame (frame_number);
}

static void
do_step (void)
{
  frame_number = start_frame + ((frame_number - start_frame + 1) % (end_frame - start_frame + 1));
  render_frame (frame_number);
}


/*  Callbacks  */

static void
window_destroy (GtkWidget *widget)
{
  clean_exit ();
}


static gint
advance_frame_callback (gpointer data)
{
  gdouble duration;

  remove_timer();

  duration = frame_durations[(frame_number - frame_number_min + 1) % total_frames];

  timer = g_timeout_add (duration * get_duration_factor (settings.duration_index),
                         advance_frame_callback, NULL);

  do_step ();

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
      timer = g_timeout_add ((gdouble) frame_durations[frame_number - frame_number_min] *
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
}

static void
step_callback (GtkAction *action)
{
  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));
  do_step();
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
  frame_number = start_frame;
  render_frame (frame_number);
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

  set_total_frames ();
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
    scale = 0.51;

  expected_drawing_area_width = preview_width * scale;
  expected_drawing_area_height = preview_height * scale;

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

/* Update the tool sensitivity for playing, depending on the number of frames. */
static void
update_ui (void)
{
  GtkAction * action;
  gboolean animated;

  animated = end_frame - start_frame >= 1;
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
}

static void
startframe_changed (GtkAdjustment *adjustment,
                    gpointer       user_data)
{
  gdouble value = gtk_adjustment_get_value (adjustment);

  if (value < frame_number_min || value > frame_number_max)
    {
      value = frame_number_min;
      gtk_adjustment_set_value (adjustment, frame_number_min);
    }

  start_frame = (guint) value;

  if (gtk_adjustment_get_value (endframe_adjust) < value)
    {
      gtk_adjustment_set_value (endframe_adjust, frame_number_max);
      end_frame = (guint) frame_number_max;
    }

  if (frame_number < start_frame)
    {
      frame_number = start_frame;
      render_frame (frame_number);
    }

  update_ui ();
}

static void
endframe_changed (GtkAdjustment *adjustment,
                  gpointer       user_data)
{
  gdouble value = gtk_adjustment_get_value (adjustment);

  if (value < frame_number_min || value > frame_number_max)
    {
      value = frame_number_max;
      gtk_adjustment_set_value (adjustment, frame_number_max);
    }

  end_frame = (guint) value;

  if (gtk_adjustment_get_value (startframe_adjust) > value)
    {
      gtk_adjustment_set_value (startframe_adjust, frame_number_min);
      start_frame = (guint) frame_number_min;
    }

  if (frame_number > end_frame)
    {
      frame_number = end_frame;
      render_frame (frame_number);
    }

  update_ui ();
}

/*
 * Callback emitted when the user toggle the quality checkbox.
 */
static void
quality_checkbox_toggled (GtkToggleButton *button,
                          gpointer         data)
{
  gint previous_preview_width = preview_width;

  if (gtk_toggle_button_get_active (button))
    {
      GdkScreen         *screen;
      guint              screen_width, screen_height;

      screen = gtk_widget_get_screen (window);
      screen_height = gdk_screen_get_height (screen);
      screen_width = gdk_screen_get_width (screen);

      /* Get some maximum value for both dimension. */
      preview_width = MIN (screen_width / 2, width);
      preview_height = MIN (screen_height / 2, height);
      /* Get the correct ratio. */
      preview_width = MIN (preview_width, preview_height * width / height);
      preview_height = preview_width * height / width;
    }
  else
    {
      preview_width = width;
      preview_height = height;
    }

  if (initialized_once && previous_preview_width != preview_width)
    {
      gint index = gtk_combo_box_get_active (GTK_COMBO_BOX (zoomcombo));

      init_frames ();
      update_scale (get_scale (index));
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

/**
 * Set the `total_frames` for the DISPOSE_TAGS disposal.
 * Will set 0 if no layer has frame tags in tags mode, or if there is no layer in combine/replace.
 */
static void
set_total_frames (void)
{
  gchar        *layer_name;
  gchar        *nospace_name;
  GMatchInfo   *match_info;
  gint          max                 = G_MININT;
  gint          min                 = G_MAXINT;
  gboolean      start_from_first    = FALSE;
  gboolean      end_at_last         = FALSE;
  gint          i, j;

  /* As a special exception, when we start or end at first or last frames,
   * we want to stay that way, even with different first and last frames. */
  if (start_frame == frame_number_min)
    start_from_first = TRUE;
  if (end_frame == frame_number_max)
    end_at_last = TRUE;

  if (settings.default_frame_disposal != DISPOSE_TAGS)
    {
      total_frames = total_layers;
      frame_number_min = 1;
      frame_number_max = frame_number_min + total_frames - 1;
    }
  else for (i = 0; i < total_layers; i++)
    {
      layer_name = gimp_item_get_name (layers[i]);
      nospace_name = g_regex_replace_literal (nospace_reg, layer_name, -1, 0, "", 0, NULL);

      g_regex_match (layers_reg, nospace_name, 0, &match_info);

      while (g_match_info_matches (match_info))
        {
          gchar *tag = g_match_info_fetch (match_info, 1);
          gchar** tokens = g_strsplit(tag, ",", 0);

          for (j = 0; tokens[j] != NULL; j++)
            {
              gchar* hyphen;
              hyphen = g_strrstr(tokens[j], "-");
              if (hyphen != NULL)
                {
                  gint32 first = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);
                  gint32 second = (gint32) g_ascii_strtoll (&hyphen[1], NULL, 10);
                  max = (second > first && second > max)? second : max;
                  min = (second > first && first < min)? first : min;
                }
              else
                {
                  gint32 num = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);
                  max = (num > max)? num : max;
                  min = (num < min)? num : min;
                }
            }
          g_strfreev (tokens);
          g_free (tag);
          g_free (layer_name);
          g_match_info_next (match_info, NULL);
        }

      g_free (nospace_name);
      g_match_info_free (match_info);
      total_frames = (max > min)? max + 1 - min : 0;
      frame_number_min = min;
      frame_number_max = max;
    }

  /* Update frame counting UI widgets. */
  if (startframe_adjust)
    {
      start_frame = gtk_adjustment_get_value (startframe_adjust);
      if (start_from_first || start_frame < frame_number_min || start_frame > frame_number_max)
        start_frame = frame_number_min;
    }
  else
    start_frame = frame_number_min;

  if (endframe_adjust)
    {
      end_frame = gtk_adjustment_get_value (endframe_adjust);
      if (end_at_last || end_frame < frame_number_min || end_frame > frame_number_max || end_frame < start_frame)
        end_frame = frame_number_max;
    }
  else
    end_frame = frame_number_max;
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

