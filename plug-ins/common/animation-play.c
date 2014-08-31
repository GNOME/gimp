/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 * (c) Mircea Purdea : 2009 : someone_else@exhalus.net
 * (c) Jehan : 2012-2014 : jehan at girinstud.io
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

#define MAX_FRAMERATE  300.0

/* Some regexp used to parse layer tags.
 * A little ugly to have these here, but since the regexp will be
 * used many times, it is more efficient to store them globally .*/
static const GRegex *nospace_reg;
static const GRegex *layers_reg;
static const GRegex *all_reg;

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

  guint  duration;
}
Frame;

typedef struct
{
  /* Settings saved as parasites. */
  DisposeType frame_disposal;
  gdouble     frame_rate;

  /* Animation: the frame_min and frame_max are the first and last set
   * frames, whereas the (start|end)_frame are the range for playback.
   * These settings are computed at startup, and not saved. */
  guint       current_frame;
  gint32      num_frames;
  guint       frame_min;
  guint       frame_max;
  guint       start_frame;
  guint       end_frame;

  gdouble     scale; /* Zoom. 1.0 is 100%. */
}
AnimationSettings;

typedef struct
{
  /* Image */
  gint32             image_id;

  /* GUI */
  GtkWidget         *window;
  GtkWidget         *shape_window;

  GtkWidget         *play_bar;
  GtkWidget         *progress_bar;
  GtkWidget         *settings_bar;
  GtkWidget         *view_bar;
  GtkWidget         *refresh;

  GtkWidget         *fpscombo;
  GtkWidget         *zoomcombo;
  GtkWidget         *proxycombo;

  GtkWidget         *progress;

  GtkWidget         *startframe_spin;
  GtkWidget         *endframe_spin;

  GtkWidget         *drawing_area;
  guchar            *drawing_area_data;
  guint              drawing_area_width;
  guint              drawing_area_height;

  GtkWidget         *shape_drawing_area;
  guchar            *shape_drawing_area_data;
  guint              shape_drawing_area_width;
  guint              shape_drawing_area_height;

  guint              preview_width;
  guint              preview_height;

  /* Animations */
  AnimationSettings  settings;
  /* We don't associate the frames to the AnimationSettings, but to the
   * dialog box, because it takes quite a bit of memory. So we want to
   * allocate only the currently running drawables. */
  Frame            **frames;
  /* Since the application is single-threaded, a boolean is enough.
   * It may become a mutex in the future with multi-thread support. */
  gboolean           frames_lock;

  /* Actions */
  GtkUIManager      *ui_manager;

  GtkActionGroup    *play_actions;
  GtkActionGroup    *settings_actions;
  GtkActionGroup    *view_actions;
  GtkActionGroup    *various_actions;
}
AnimationPlayDialog;

/* for shaping */
typedef struct
{
  gint x, y;
} CursorOffset;

static void        query                     (void);
static void        run                       (const gchar          *name,
                                              gint                  nparams,
                                              const GimpParam      *param,
                                              gint                 *nreturn_vals,
                                              GimpParam           **return_vals);

/* Initialization. */
static void        initialize                (AnimationPlayDialog  *dialog);
static void        init_frame_numbers        (AnimationPlayDialog  *dialog,
                                              gint                 *layers,
                                              gint                  num_layers);
static gboolean    init_frames               (AnimationPlayDialog  *dialog);
static void        rec_init_frames           (AnimationPlayDialog  *dialog,
                                              gint32                frames_image_id,
                                              gint32                layer,
                                              GList                *previous_frames,
                                              gint                  image_width,
                                              gint                  image_height);
static void        init_ui                   (AnimationPlayDialog  *dialog,
                                              gchar                *imagename);
static GtkUIManager * ui_manager_new         (AnimationPlayDialog  *dialog);

static void        refresh_dialog            (AnimationPlayDialog  *dialog);
static void        update_ui_sensitivity     (AnimationPlayDialog  *dialog);

/* All callbacks. */
static void        close_callback            (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        help_callback             (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        window_destroy            (GtkWidget            *widget,
                                              AnimationPlayDialog  *dialog);
static gboolean    popup_menu                (GtkWidget            *widget,
                                              AnimationPlayDialog  *dialog);

static gboolean    adjustment_pressed        (GtkWidget            *widget,
                                              GdkEventButton       *event,
                                              AnimationPlayDialog  *dialog);
static gboolean    da_button_press           (GtkWidget            *widget,
                                              GdkEventButton       *event,
                                              AnimationPlayDialog  *dialog);
static void        da_size_callback          (GtkWidget            *widget,
                                              GtkAllocation        *allocation,
                                              AnimationPlayDialog  *dialog);
static gboolean    shape_pressed             (GtkWidget            *widget,
                                              GdkEventButton       *event,
                                              AnimationPlayDialog  *dialog);
static gboolean    shape_released            (GtkWidget            *widget);
static gboolean    shape_motion              (GtkWidget            *widget,
                                              GdkEventMotion       *event);
static gboolean    repaint_da                (GtkWidget            *darea,
                                              GdkEventExpose       *event,
                                              AnimationPlayDialog  *dialog);

static gboolean    progress_button           (GtkWidget            *widget,
                                              GdkEventButton       *event,
                                              AnimationPlayDialog  *dialog);
static gboolean    progress_entered          (GtkWidget            *widget,
                                              GdkEventCrossing     *event,
                                              AnimationPlayDialog  *dialog);
static gboolean    progress_motion           (GtkWidget            *widget,
                                              GdkEventMotion       *event,
                                              AnimationPlayDialog  *dialog);
static gboolean    progress_left             (GtkWidget            *widget,
                                              GdkEventCrossing     *event,
                                              AnimationPlayDialog  *dialog);

static void        detach_callback           (GtkToggleAction      *action,
                                              AnimationPlayDialog  *dialog);
static void        play_callback             (GtkToggleAction      *action,
                                              AnimationPlayDialog  *dialog);
static void        step_back_callback        (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        step_callback             (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        rewind_callback           (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        refresh_callback          (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        zoom_in_callback          (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        zoom_out_callback         (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        zoom_reset_callback       (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        speed_up_callback         (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        speed_down_callback       (GtkAction            *action,
                                              AnimationPlayDialog  *dialog);
static void        framecombo_changed        (GtkWidget             *combo,
                                              AnimationPlayDialog  *dialog);
static void        fpscombo_activated        (GtkEntry            *combo,
                                              AnimationPlayDialog *dialog);
static void        fpscombo_changed          (GtkWidget             *combo,
                                              AnimationPlayDialog  *dialog);
static void        zoomcombo_activated       (GtkEntry             *combo,
                                              AnimationPlayDialog  *dialog);
static void        zoomcombo_changed         (GtkWidget            *combo,
                                              AnimationPlayDialog  *dialog);
static void        startframe_changed        (GtkAdjustment        *adjustment,
                                              AnimationPlayDialog  *dialog);
static void        endframe_changed          (GtkAdjustment        *adjustment,
                                              AnimationPlayDialog  *dialog);
static void        proxy_checkbox_expanded   (GtkExpander          *expander,
                                              GParamSpec           *param_spec,
                                              AnimationPlayDialog  *dialog);
static void        proxycombo_activated      (GtkEntry             *combo_entry,
                                              AnimationPlayDialog  *dialog);
static void        proxycombo_changed        (GtkWidget            *combo,
                                              AnimationPlayDialog  *dialog);

/* Rendering/Playing Functions */
static void        render_frame              (AnimationPlayDialog  *dialog,
                                              gboolean              force);
static void        total_alpha_preview       (guchar               *drawing_data,
                                              guint                 drawing_width,
                                              guint                 drawing_height);
static void        reshape_from_bitmap       (AnimationPlayDialog   *dialog,
                                              const gchar           *bitmap);
static void        show_playing_progress     (AnimationPlayDialog  *dialog);
static void        show_loading_progress     (AnimationPlayDialog  *dialog,
                                              gint                  layer_nb,
                                              gint                  num_layers);
static void        do_back_step              (AnimationPlayDialog  *dialog);
static void        do_step                   (AnimationPlayDialog  *dialog);
static void        set_timer                 (guint                 new_timer);
static gboolean    advance_frame_callback    (AnimationPlayDialog  *dialog);
static void        show_goto_progress        (guint                 frame_nb,
                                              AnimationPlayDialog  *dialog);

/* Utils */
static void        connect_accelerators      (GtkUIManager         *ui_manager,
                                              GtkActionGroup       *group);
static gdouble     get_fps                   (gint                  index);
static gdouble     get_proxy                 (AnimationPlayDialog  *dialog,
                                              gint                  index);
static void        update_scale              (AnimationPlayDialog  *dialog,
                                              gdouble               scale);
static gdouble     get_scale                 (AnimationPlayDialog  *dialog,
                                              gint                  index);
static void        save_settings             (AnimationPlayDialog  *dialog);
static void        clean_exit                (AnimationPlayDialog *dialog);

/* tag util functions*/
static gboolean    is_disposal_tag           (const gchar          *str,
                                              DisposeType          *disposal,
                                              gint                 *taglength);
static DisposeType parse_disposal_tag        (AnimationPlayDialog  *dialog,
                                              const gchar          *str);
static gboolean    is_ms_tag                 (const gchar          *str,
                                              gint                 *duration,
                                              gint                 *taglength);
static gint        parse_ms_tag              (const gchar          *str);
static void        rec_set_total_frames      (const gint32          layer,
                                              gint                 *min,
                                              gint                 *max);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

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
                          N_("Preview a GIMP animation"),
                          "",
                          "Adam D. Moss <adam@gimp.org>",
                          "Adam D. Moss <adam@gimp.org>",
                          "1997, 1998...",
                          N_("Animation _Playback..."),
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
  static GimpParam   values[1];
  GimpPDBStatusType  status;
  GimpRunMode        run_mode;
  GeglConfig        *config;

  INIT_I18N ();

  gegl_init (NULL, NULL);
  config = gegl_config ();
  /* For preview, we want fast (0.0) over high quality (1.0). */
  g_object_set (config, "quality", 0.0, NULL);

  status        = GIMP_PDB_SUCCESS;
  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

 if (run_mode == GIMP_RUN_NONINTERACTIVE && n_params != 3)
   {
     /* This plugin is meaningless right now other than interactive. */
     status = GIMP_PDB_CALLING_ERROR;
   }
 else
   {
     AnimationPlayDialog *dialog;
     GimpParasite        *parasite;
     gint                 image_width;
     gint                 image_height;

     /********************************************/
     /* Init the global variable: frame parsing. */
     nospace_reg = g_regex_new("[ \t]*", G_REGEX_OPTIMIZE, 0, NULL);
     layers_reg  = g_regex_new("\\[(([0-9]+(-[0-9]+)?)(,[0-9]+(-[0-9]+)?)*)\\]", G_REGEX_OPTIMIZE, 0, NULL);
     all_reg     = g_regex_new("\\[\\*\\]", G_REGEX_OPTIMIZE, 0, NULL);

     /**********************************************************************/
     /* Now init the struct for the animation, which is its current state. */
     dialog = g_new (AnimationPlayDialog, 1);

     dialog->image_id    = param[1].data.d_image;
     image_width  = gimp_image_width (dialog->image_id);
     image_height = gimp_image_height (dialog->image_id);

     dialog->frames      = NULL;
     dialog->frames_lock = FALSE;

      /* We default at full preview size. */
     dialog->preview_width  = image_width;
     dialog->preview_height = image_height;

     /* Acceptable settings for the default. */
     dialog->settings.frame_disposal = DISPOSE_COMBINE;
     dialog->settings.frame_rate     = 24.0; /* fps */

     /* Not yet initialized animation. */
     dialog->settings.current_frame = 0;
     dialog->settings.num_frames    = 0;
     dialog->settings.frame_min     = 0;
     dialog->settings.frame_max     = 0;
     dialog->settings.start_frame   = 0;
     dialog->settings.end_frame     = 0;

     /* If we saved any settings globally, use the one from the last run. */
     gimp_get_data (PLUG_IN_PROC, &dialog->settings);

     /* If this image has specific settings already, override the global ones. */
     parasite = gimp_image_get_parasite (dialog->image_id, PLUG_IN_PROC "/frame-disposal");
     if (parasite)
       {
         const DisposeType *mode = gimp_parasite_data (parasite);

         dialog->settings.frame_disposal = *mode;
         gimp_parasite_free (parasite);
       }
     parasite = gimp_image_get_parasite (dialog->image_id, PLUG_IN_PROC "/frame-rate");
     if (parasite)
       {
         const gdouble *rate = gimp_parasite_data (parasite);

         dialog->settings.frame_rate = *rate;
         gimp_parasite_free (parasite);
       }

     /* UI setup. */
     dialog->window                    = NULL;

     dialog->play_bar                  = NULL;
     dialog->progress_bar              = NULL;
     dialog->settings_bar              = NULL;
     dialog->view_bar                  = NULL;
     dialog->refresh                   = NULL;

     dialog->fpscombo                  = NULL;
     dialog->zoomcombo                 = NULL;
     dialog->proxycombo                = NULL;

     dialog->ui_manager                = NULL;
     dialog->progress                  = NULL;
     dialog->startframe_spin           = NULL;
     dialog->endframe_spin             = NULL;
     dialog->shape_window              = NULL;
     dialog->shape_drawing_area        = NULL;
     dialog->shape_drawing_area_data   = NULL;
     dialog->drawing_area              = NULL;
     dialog->drawing_area_data         = NULL;
     dialog->drawing_area_width        = -1;
     dialog->drawing_area_height       = -1;
     dialog->shape_drawing_area_width  = -1;
     dialog->shape_drawing_area_height = -1;

     initialize (dialog);

     gtk_main ();
     gimp_set_data (PLUG_IN_PROC, &dialog->settings, sizeof (dialog->settings));

     if (run_mode != GIMP_RUN_NONINTERACTIVE)
       gimp_displays_flush ();
   }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gegl_exit ();
}

static void
initialize (AnimationPlayDialog *dialog)
{
  /* Freeing existing data after a refresh. */
  /* Catch the case when the user has closed the image in the meantime. */
  if (! gimp_image_is_valid (dialog->image_id))
    {
      gimp_message (_("Invalid image. Did you close it?"));
      clean_exit (dialog);
      return;
    }

  if (! dialog->window)
    {
      /* First run. */
      gchar *name = gimp_image_get_name (dialog->image_id);
      init_ui (dialog, name);
      g_free (name);
    }
  refresh_dialog (dialog);

  /* I want to make sure the progress bar is realized before init_frames()
   * which may take quite a bit of time. */
  if (!gtk_widget_get_realized (dialog->progress))
    gtk_widget_realize (dialog->progress);

  render_frame (dialog, init_frames (dialog));
}

/**
 * Set num_frames, which is not necessarily the number of layers, in
 * particular with the DISPOSE_TAGS disposal.
 * Will set 0 if no layer has frame tags in tags mode, or if there is
 * no layer in combine/replace.
 */
static void
init_frame_numbers (AnimationPlayDialog *dialog,
                    gint                *layers,
                    gint                 num_layers)
{
  GtkAdjustment *startframe_adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->startframe_spin));
  GtkAdjustment *endframe_adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->endframe_spin));
  gboolean       start_from_first = FALSE;
  gboolean       end_at_last      = FALSE;

  /* As a special exception, when we start or end at first or last frames,
   * we want to stay that way, even with different first and last frames. */
  if (dialog->settings.start_frame == dialog->settings.frame_min)
    start_from_first = TRUE;
  if (dialog->settings.end_frame == dialog->settings.frame_max)
    end_at_last = TRUE;

  if (dialog->settings.frame_disposal != DISPOSE_TAGS)
    {
      dialog->settings.num_frames = num_layers;
      dialog->settings.frame_min = 1;
      dialog->settings.frame_max = dialog->settings.frame_min + dialog->settings.num_frames - 1;
    }
  else
    {
      gint i;
      gint max = G_MININT;
      gint min = G_MAXINT;

      for (i = 0; i < num_layers; i++)
        rec_set_total_frames (layers[i], &min, &max);

      dialog->settings.num_frames = (max > min)? max + 1 - min : 0;
      dialog->settings.frame_min = min;
      dialog->settings.frame_max = max;
    }

  /* Keep the same frame number, unless it is now invalid. */
  if (dialog->settings.current_frame > dialog->settings.end_frame ||
      dialog->settings.current_frame < dialog->settings.start_frame)
    {
      dialog->settings.current_frame = dialog->settings.start_frame;
    }

  /* Update frame counting UI widgets. */
  if (startframe_adjust)
    {
      dialog->settings.start_frame = gtk_adjustment_get_value (startframe_adjust);
      if (start_from_first                        ||
          dialog->settings.start_frame < dialog->settings.frame_min ||
          dialog->settings.start_frame > dialog->settings.frame_max)
        {
          dialog->settings.start_frame = dialog->settings.frame_min;
        }
    }
  else
    {
      dialog->settings.start_frame = dialog->settings.frame_min;
    }

  if (endframe_adjust)
    {
      dialog->settings.end_frame = gtk_adjustment_get_value (endframe_adjust);
      if (end_at_last                           ||
          dialog->settings.end_frame < dialog->settings.frame_min ||
          dialog->settings.end_frame > dialog->settings.frame_max ||
          dialog->settings.end_frame < dialog->settings.start_frame)
        {
          dialog->settings.end_frame = dialog->settings.frame_max;
        }
    }
  else
    {
      dialog->settings.end_frame = dialog->settings.frame_max;
    }
}

/* Initialize the frames, and return TRUE if render must be forced. */
static gboolean
init_frames (AnimationPlayDialog *dialog)
{
  /* Frames are associated to an unused image. */
  static gint32    frames_image_id = 0;
  /* We keep track of the frames in a separate structure to free drawable
   * memory. We can't use easily dialog->frames because some of the
   * drawables may be used in more than 1 frame. */
  static GList    *previous_frames = NULL;

  gint            *layers;
  gint             num_layers;
  gint             image_width;
  gint             image_height;
  gint32           new_frame, previous_frame, new_layer;
  gint             duration = 0;
  DisposeType      disposal = dialog->settings.frame_disposal;
  gint             frame_spin_size;

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play"))))
    {
      gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
    }

  if (dialog->frames_lock)
    {
      return FALSE;
    }
  dialog->frames_lock = TRUE;

  /* Block most UI during frame initialization. */
  gtk_action_group_set_sensitive (dialog->play_actions, FALSE);
  gtk_widget_set_sensitive (dialog->play_bar, FALSE);
  gtk_widget_set_sensitive (dialog->progress_bar, FALSE);
  gtk_action_group_set_sensitive (dialog->settings_actions, FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->settings_bar), FALSE);
  gtk_action_group_set_sensitive (dialog->view_actions, FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->view_bar), FALSE);

  /* Cleanup before re-generation. */
  if (dialog->frames)
    {
      GList *idx;

      gimp_image_delete (frames_image_id);
      frames_image_id = 0;

      /* Freeing previous frames only once. */
      for (idx = g_list_first (previous_frames); idx != NULL; idx = g_list_next (idx))
        {
          Frame* frame = (Frame*) idx->data;

          g_list_free (frame->indexes);
          g_list_free (frame->updated_indexes);
          g_free (frame);
        }
      g_list_free (previous_frames);
      previous_frames = NULL;

      g_free (dialog->frames);
      dialog->frames = NULL;
    }
  if (! gimp_image_is_valid (dialog->image_id))
    {
      /* This is not necessarily an error. We may have simply wanted
       * to clean up our GEGL buffers. */
      return FALSE;
    }

  layers = gimp_image_get_layers (dialog->image_id, &num_layers);
  init_frame_numbers (dialog, layers, num_layers);
  if (dialog->settings.num_frames <= 0)
    {
      update_ui_sensitivity (dialog);
      dialog->frames_lock = FALSE;
      g_free (layers);
      return FALSE;
    }

  dialog->frames = g_try_malloc0_n (dialog->settings.num_frames, sizeof (Frame*));
  if (! dialog->frames)
    {
      dialog->frames_lock = FALSE;
      gimp_message (_("Memory could not be allocated to the frame container."));
      g_free (layers);
      clean_exit (dialog);
      return FALSE;
    }

  image_width  = gimp_image_width (dialog->image_id);
  image_height = gimp_image_height (dialog->image_id);

  /* We only use RGB images for display because indexed images would somehow
     render terrible colors. Layers from other types will be automatically
     converted. */
  frames_image_id = gimp_image_new (dialog->preview_width, dialog->preview_height, GIMP_RGB);

  /* Save processing time and memory by not saving history and merged frames. */
  gimp_image_undo_disable (frames_image_id);

  if (disposal == DISPOSE_TAGS)
    {
      gint        i;

      for (i = 0; i < num_layers; i++)
        {
          show_loading_progress (dialog, i, num_layers);
          rec_init_frames (dialog,
                           frames_image_id,
                           layers[num_layers - (i + 1)],
                           previous_frames,
                           image_width, image_height);
        }

      for (i = 0; i < dialog->settings.num_frames; i++)
        {
          /* If for some reason a frame is absent, use the previous one.
           * We are ensured that there is at least a "first" frame for this. */
          if (! dialog->frames[i])
            {
              dialog->frames[i] = dialog->frames[i - 1];
              dialog->frames[i]->indexes = g_list_append (dialog->frames[i]->indexes, GINT_TO_POINTER (i));
            }

          /* A zero duration only means we use the global duration, whatever it is at the time. */
          dialog->frames[i]->duration = 0;
        }
    }
  else
    {
      gint   layer_offx;
      gint   layer_offy;
      gchar *layer_name;
      gint   i;

      for (i = 0; i < dialog->settings.num_frames; i++)
        {
          show_loading_progress (dialog, i, num_layers);

          dialog->frames[i] = g_new (Frame, 1);
          dialog->frames[i]->indexes = NULL;
          dialog->frames[i]->indexes = g_list_append (dialog->frames[i]->indexes, GINT_TO_POINTER (i));
          dialog->frames[i]->updated_indexes = NULL;

          previous_frames = g_list_append (previous_frames, dialog->frames[i]);

          layer_name = gimp_item_get_name (layers[num_layers - (i + 1)]);
          if (layer_name)
            {
              duration = parse_ms_tag (layer_name);
              disposal = parse_disposal_tag (dialog, layer_name);
              g_free (layer_name);
            }

          if (i > 0 && disposal != DISPOSE_REPLACE)
            {
              previous_frame = gimp_layer_copy (dialog->frames[i - 1]->drawable_id);

              gimp_image_insert_layer (frames_image_id, previous_frame, 0, 0);
              gimp_item_set_visible (previous_frame, TRUE);
            }

          new_layer = gimp_layer_new_from_drawable (layers[num_layers - (i + 1)], frames_image_id);

          gimp_image_insert_layer (frames_image_id, new_layer, 0, 0);
          gimp_item_set_visible (new_layer, TRUE);
          gimp_layer_scale (new_layer, (gimp_drawable_width (layers[num_layers - (i + 1)]) * (gint) dialog->preview_width) / image_width,
                            (gimp_drawable_height (layers[num_layers - (i + 1)]) * (gint) dialog->preview_height) / image_height, FALSE);
          gimp_drawable_offsets (layers[num_layers - (i + 1)], &layer_offx, &layer_offy);
          gimp_layer_set_offsets (new_layer, (layer_offx * (gint) dialog->preview_width) / image_width,
                                  (layer_offy * (gint) dialog->preview_height) / image_height);
          gimp_layer_resize_to_image_size (new_layer);

          if (gimp_item_is_group (new_layer))
            {
              gint    num_children;
              gint32 *children;
              gint    j;

              /* I want to make all layers in the group visible, so that when I'll make
               * the group visible too at render time, it will display everything in it. */
              children = gimp_item_get_children (new_layer, &num_children);
              for (j = 0; j < num_children; j++)
                gimp_item_set_visible (children[j], TRUE);
            }

          new_frame = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
          dialog->frames[i]->drawable_id = new_frame;
          gimp_item_set_visible (new_frame, FALSE);

          if (duration <= 0)
            duration = 0;
          dialog->frames[i]->duration = (guint) duration;
        }
    }

  /* Update the UI. */
  frame_spin_size = (gint) (log10 (dialog->settings.frame_max - (dialog->settings.frame_max % 10))) + 1;
  gtk_entry_set_width_chars (GTK_ENTRY (dialog->startframe_spin), frame_spin_size);
  gtk_entry_set_width_chars (GTK_ENTRY (dialog->endframe_spin), frame_spin_size);

  gtk_adjustment_configure (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->startframe_spin)),
                            dialog->settings.start_frame,
                            dialog->settings.frame_min,
                            dialog->settings.frame_max,
                            1.0, 5.0, 0.0);
  gtk_adjustment_configure (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->endframe_spin)),
                            dialog->settings.end_frame,
                            dialog->settings.start_frame,
                            dialog->settings.frame_max,
                            1.0, 5.0, 0.0);

  update_ui_sensitivity (dialog);

  dialog->frames_lock = FALSE;
  g_free (layers);

  return TRUE;
}

/**
 * Recursive call to generate frames in TAGS mode.
 **/
static void
rec_init_frames (AnimationPlayDialog *dialog,
                 gint32               frames_image_id,
                 gint32               layer,
                 GList               *previous_frames,
                 gint                 image_width,
                 gint                 image_height)
{
  Frame        *empty_frame = NULL;
  gchar        *layer_name;
  gchar        *nospace_name;
  GMatchInfo   *match_info;
  gboolean      preview_quality;
  gint32        new_layer;
  gint          j, k;

  if (gimp_item_is_group (layer))
    {
      gint    num_children;
      gint32 *children;

      children = gimp_item_get_children (layer, &num_children);
      for (j = 0; j < num_children; j++)
        rec_init_frames (dialog, frames_image_id,
                         children[num_children - j - 1],
                         previous_frames,
                         image_width, image_height);

      return;
    }

  layer_name = gimp_item_get_name (layer);
  nospace_name = g_regex_replace_literal (nospace_reg, layer_name, -1, 0, "", 0, NULL);
  preview_quality = dialog->preview_width != image_width || dialog->preview_height != image_height;

  if (g_regex_match (all_reg, nospace_name, 0, NULL))
    {
      for (j = 0; j < dialog->settings.num_frames; j++)
        {
          if (! dialog->frames[j])
            {
              if (! empty_frame)
                {
                  empty_frame = g_new (Frame, 1);
                  empty_frame->indexes = NULL;
                  empty_frame->updated_indexes = NULL;
                  empty_frame->drawable_id = 0;

                  previous_frames = g_list_append (previous_frames, empty_frame);
                }

              if (! g_list_find (empty_frame->updated_indexes, GINT_TO_POINTER (j)))
                empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (j));

              dialog->frames[j] = empty_frame;
            }
          else if (! g_list_find (dialog->frames[j]->updated_indexes, GINT_TO_POINTER (j)))
            dialog->frames[j]->updated_indexes = g_list_append (dialog->frames[j]->updated_indexes, GINT_TO_POINTER (j));
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
                      if (! dialog->frames[k - dialog->settings.frame_min])
                        {
                          if (! empty_frame)
                            {
                              empty_frame = g_new (Frame, 1);
                              empty_frame->indexes = NULL;
                              empty_frame->updated_indexes = NULL;
                              empty_frame->drawable_id = 0;

                              previous_frames = g_list_append (previous_frames, empty_frame);
                            }

                          if (! g_list_find (dialog->frames[k - dialog->settings.frame_min]->updated_indexes, GINT_TO_POINTER (k - dialog->settings.frame_min)))
                            empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (k - dialog->settings.frame_min));

                          dialog->frames[k - dialog->settings.frame_min] = empty_frame;
                        }
                      else if (! g_list_find (dialog->frames[k - dialog->settings.frame_min]->updated_indexes, GINT_TO_POINTER (k - dialog->settings.frame_min)))
                        dialog->frames[k - dialog->settings.frame_min]->updated_indexes = g_list_append (dialog->frames[k - dialog->settings.frame_min]->updated_indexes,
                                                                                       GINT_TO_POINTER (k - dialog->settings.frame_min));
                    }
                }
              else
                {
                  gint32 num = (gint32) g_ascii_strtoll (tokens[j], NULL, 10);

                  if (! dialog->frames[num - dialog->settings.frame_min])
                    {
                      if (! empty_frame)
                        {
                          empty_frame = g_new (Frame, 1);
                          empty_frame->indexes = NULL;
                          empty_frame->updated_indexes = NULL;
                          empty_frame->drawable_id = 0;

                          previous_frames = g_list_append (previous_frames, empty_frame);
                        }

                      if (! g_list_find (dialog->frames[num - dialog->settings.frame_min]->updated_indexes, GINT_TO_POINTER (num - dialog->settings.frame_min)))
                        empty_frame->updated_indexes = g_list_append (empty_frame->updated_indexes, GINT_TO_POINTER (num - dialog->settings.frame_min));

                      dialog->frames[num - dialog->settings.frame_min] = empty_frame;
                    }
                  else if (! g_list_find (dialog->frames[num - dialog->settings.frame_min]->updated_indexes, GINT_TO_POINTER (num - dialog->settings.frame_min)))
                    dialog->frames[num - dialog->settings.frame_min]->updated_indexes = g_list_append (dialog->frames[num - dialog->settings.frame_min]->updated_indexes,
                                                                                     GINT_TO_POINTER (num - dialog->settings.frame_min));
                }
            }
          g_strfreev (tokens);
          g_free (tag);
          g_match_info_next (match_info, NULL);
        }
      g_match_info_free (match_info);
    }

  for (j = 0; j < dialog->settings.num_frames; j++)
    {
      /* Check which frame must be updated with the current layer. */
      if (dialog->frames[j] && g_list_length (dialog->frames[j]->updated_indexes))
        {
          new_layer = gimp_layer_new_from_drawable (layer, frames_image_id);
          gimp_image_insert_layer (frames_image_id, new_layer, 0, 0);

          if (preview_quality)
            {
              gint layer_offx, layer_offy;

              gimp_layer_scale (new_layer,
                                (gimp_drawable_width (layer) * (gint) dialog->preview_width) / (gint) image_width,
                                (gimp_drawable_height (layer) * (gint) dialog->preview_height) / (gint) image_height,
                                FALSE);
              gimp_drawable_offsets (layer, &layer_offx, &layer_offy);
              gimp_layer_set_offsets (new_layer, (layer_offx * (gint) dialog->preview_width) / (gint) image_width,
                                      (layer_offy * (gint) dialog->preview_height) / (gint) image_height);
            }
          gimp_layer_resize_to_image_size (new_layer);

          if (dialog->frames[j]->drawable_id == 0)
            {
              dialog->frames[j]->drawable_id = new_layer;
              dialog->frames[j]->indexes = dialog->frames[j]->updated_indexes;
              dialog->frames[j]->updated_indexes = NULL;
            }
          else if (g_list_length (dialog->frames[j]->indexes) == g_list_length (dialog->frames[j]->updated_indexes))
            {
              gimp_item_set_visible (new_layer, TRUE);
              gimp_item_set_visible (dialog->frames[j]->drawable_id, TRUE);

              dialog->frames[j]->drawable_id = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
              g_list_free (dialog->frames[j]->updated_indexes);
              dialog->frames[j]->updated_indexes = NULL;
            }
          else
            {
              GList    *idx;
              gboolean  move_j = FALSE;
              Frame    *forked_frame = g_new (Frame, 1);
              gint32    forked_drawable_id = gimp_layer_new_from_drawable (dialog->frames[j]->drawable_id, frames_image_id);

              /* if part only of the dialog->frames are updated, we fork the existing frame. */
              gimp_image_insert_layer (frames_image_id, forked_drawable_id, 0, 1);
              gimp_item_set_visible (new_layer, TRUE);
              gimp_item_set_visible (forked_drawable_id, TRUE);
              forked_drawable_id = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);

              forked_frame->drawable_id = forked_drawable_id;
              forked_frame->indexes = dialog->frames[j]->updated_indexes;
              forked_frame->updated_indexes = NULL;
              dialog->frames[j]->updated_indexes = NULL;

              for (idx = g_list_first (forked_frame->indexes); idx != NULL; idx = g_list_next (idx))
                {
                  dialog->frames[j]->indexes = g_list_remove (dialog->frames[j]->indexes, idx->data);
                  if (GPOINTER_TO_INT (idx->data) != j)
                    dialog->frames[GPOINTER_TO_INT (idx->data)] = forked_frame;
                  else
                    /* Frame j must also be moved to the forked frame, but only after the loop. */
                    move_j = TRUE;
                }
              if (move_j)
                dialog->frames[j] = forked_frame;

              gimp_item_set_visible (forked_drawable_id, FALSE);

              previous_frames = g_list_append (previous_frames, forked_frame);
            }

          gimp_item_set_visible (dialog->frames[j]->drawable_id, FALSE);
        }
    }

  g_free (layer_name);
  g_free (nospace_name);
}

static void
init_ui (AnimationPlayDialog *dialog,
         gchar               *imagename)
{
  GtkWidget     *upper_bar;
  GtkWidget     *widget;
  GtkAdjustment *adjust;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *abox;
  GdkCursor     *cursor;
  gchar         *text;
  gint           index;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_role (GTK_WINDOW (dialog->window), "animation-playback");

  /* Some basic signals. */
  g_signal_connect (dialog->window, "destroy",
                    G_CALLBACK (window_destroy),
                    dialog);
  g_signal_connect (dialog->window, "popup-menu",
                    G_CALLBACK (popup_menu),
                    dialog);

  /* Window Title */
  text = g_strconcat (_("Animation Playback:"), " ", imagename, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog->window), text);
  g_free (text);

  gimp_help_connect (dialog->window, gimp_standard_help_func, PLUG_IN_PROC, NULL);

  dialog->ui_manager = ui_manager_new (dialog);

  /* Main vertical box. */
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (dialog->window), main_vbox);
  gtk_widget_show (main_vbox);

  /* Upper Bar */
  upper_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), upper_bar, FALSE, FALSE, 0);
  gtk_widget_show (upper_bar);

  /**********************************/
  /* The upper bar is itself paned. */
  hbox = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (upper_bar), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*****************/
  /* Settings box. */
  /*****************/
  dialog->settings_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_add1 (GTK_PANED (hbox), dialog->settings_bar);
  gtk_widget_show (dialog->settings_bar);

  /* Settings: expander to display the proxy settings. */
  widget = gtk_expander_new_with_mnemonic (_("_Proxy Quality"));
  gtk_expander_set_expanded (GTK_EXPANDER (widget), FALSE);

  g_signal_connect (GTK_EXPANDER (widget),
                    "notify::expanded",
                    G_CALLBACK (proxy_checkbox_expanded),
                    dialog);

  gimp_help_set_help_data (widget, _("Degrade image quality for lower memory footprint"), NULL);

  gtk_box_pack_end (GTK_BOX (dialog->settings_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Settings: proxy image. */
  dialog->proxycombo = gtk_combo_box_text_new_with_entry ();

  for (index = 0; index < 3; index++)
    {
      text = g_strdup_printf  (_("%.1f %%"), get_proxy (dialog, index) * 100.0);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->proxycombo), text);
      g_free (text);
    }

  /* By default, we are at normal resolution. */
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->proxycombo), -1);
  text = g_strdup_printf  (_("%.1f %%"), 100.0);
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->proxycombo))), text);
  g_free (text);

  g_signal_connect (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->proxycombo))),
                    "activate",
                    G_CALLBACK (proxycombo_activated),
                    dialog);
  g_signal_connect (dialog->proxycombo, "changed",
                    G_CALLBACK (proxycombo_changed),
                    dialog);

  gimp_help_set_help_data (dialog->proxycombo, _("Proxy resolution quality"), NULL);

  gtk_widget_show (dialog->proxycombo);
  gtk_container_add (GTK_CONTAINER (widget), dialog->proxycombo);

  /* Settings: fps */
  dialog->fpscombo = gtk_combo_box_text_new_with_entry ();

  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->fpscombo), -1);
  for (index = 0; index < 5; index++)
    {
      /* list is given in "fps" - frames per second.
       * We allow accurate fps (double) for special cases. */
      text = g_strdup_printf  (_("%g fps"), get_fps (index));
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->fpscombo), text);
      g_free (text);
      if (get_fps (index) == dialog->settings.frame_rate)
        gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->fpscombo), index);
    }

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->fpscombo)) == -1)
    {
      text = g_strdup_printf  (_("%g fps"), dialog->settings.frame_rate);
      gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))), text);
      g_free (text);
    }

  g_signal_connect (dialog->fpscombo, "changed",
                    G_CALLBACK (fpscombo_changed),
                    dialog);
  g_signal_connect (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))),
                    "activate",
                    G_CALLBACK (fpscombo_activated),
                    dialog);

  gimp_help_set_help_data (dialog->fpscombo, _("Frame Rate"), NULL);

  gtk_box_pack_end (GTK_BOX (dialog->settings_bar), dialog->fpscombo, FALSE, FALSE, 0);
  gtk_widget_show (dialog->fpscombo);

  /* Settings: frame mode. */
  widget = gtk_combo_box_text_new ();

  text = g_strdup (_("Cumulative layers (combine)"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), DISPOSE_COMBINE, text);
  g_free (text);

  text = g_strdup (_("One frame per layer (replace)"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), DISPOSE_REPLACE, text);
  g_free (text);

  text = g_strdup (_("Use layer tags (custom)"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), DISPOSE_TAGS, text);
  g_free (text);

  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), dialog->settings.frame_disposal);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (framecombo_changed),
                    dialog);

  gtk_box_pack_end (GTK_BOX (dialog->settings_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /*************/
  /* View box. */
  /*************/
  dialog->view_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_pack2 (GTK_PANED (hbox), dialog->view_bar, FALSE, TRUE);
  gtk_widget_show (dialog->view_bar);

  /* View: zoom. */
  dialog->zoomcombo = gtk_combo_box_text_new_with_entry ();
  for (index = 0; index < 5; index++)
    {
      text = g_strdup_printf  (_("%.1f %%"), get_scale (dialog, index) * 100.0);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->zoomcombo), text);
      g_free (text);

      if (get_scale (dialog, index) == 1.0)
        gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->zoomcombo), index);
    }

  g_signal_connect (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->zoomcombo))),
                    "activate",
                    G_CALLBACK (zoomcombo_activated),
                    dialog);
  g_signal_connect (dialog->zoomcombo, "changed",
                    G_CALLBACK (zoomcombo_changed),
                    dialog);

  gimp_help_set_help_data (dialog->zoomcombo, _("Zoom"), NULL);

  gtk_box_pack_end (GTK_BOX (dialog->view_bar), dialog->zoomcombo, FALSE, FALSE, 0);
  gtk_widget_show (dialog->zoomcombo);

  /* View: detach. */
  widget = GTK_WIDGET (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (widget), GIMP_STOCK_DETACH);

  gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget),
                                      gtk_action_group_get_action (dialog->view_actions, "detach"));

  gtk_box_pack_end (GTK_BOX (dialog->view_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /***********/
  /* Various */
  /***********/

  /* Various: separator for some spacing in the UI. */
  widget = GTK_WIDGET (gtk_separator_tool_item_new ());
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (widget), FALSE);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
  gtk_box_pack_start (GTK_BOX (upper_bar), widget, TRUE, FALSE, 0);
  gtk_widget_show (widget);

  /* Various: refresh. */
  dialog->refresh = GTK_WIDGET (gtk_tool_button_new (NULL, N_("Reload the image")));

  gtk_activatable_set_related_action (GTK_ACTIVATABLE (dialog->refresh),
                                      gtk_action_group_get_action (dialog->settings_actions, "refresh"));

  gtk_box_pack_start (GTK_BOX (upper_bar), dialog->refresh, FALSE, FALSE, 0);
  gtk_widget_show (dialog->refresh);

  /***********/
  /* Drawing */
  /***********/

  /* Vbox for the preview window and lower option bar */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Alignment for the scrolling window, which can be resized by the user. */
  abox = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (abox), widget);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (widget);

  /* I add the drawing area inside an alignment box to prevent it from being resized. */
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (widget), abox);
  gtk_widget_show (abox);

  /* Build a drawing area, with a default size same as the image */
  dialog->drawing_area = gtk_drawing_area_new ();
  gtk_widget_add_events (dialog->drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (abox), dialog->drawing_area);
  gtk_widget_show (dialog->drawing_area);

  g_signal_connect (dialog->drawing_area, "size-allocate",
                    G_CALLBACK(da_size_callback),
                    dialog);
  g_signal_connect (dialog->drawing_area, "button-press-event",
                    G_CALLBACK (da_button_press),
                    dialog);

  /*****************/
  /* Play toolbar. */
  /*****************/

  hbox = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Play buttons. */
  dialog->play_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_add1 (GTK_PANED (hbox), dialog->play_bar);
  gtk_widget_show (dialog->play_bar);

  /* Play: play. */
  widget = GTK_WIDGET (gtk_toggle_tool_button_new ());
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget),
                                      gtk_action_group_get_action (dialog->play_actions, "play"));

  gtk_box_pack_start (GTK_BOX (dialog->play_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Play: step backward. */
  widget = GTK_WIDGET (gtk_tool_button_new (NULL, N_("Step back to previous frame")));
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget),
                                      gtk_action_group_get_action (dialog->play_actions, "step-back"));

  gtk_box_pack_start (GTK_BOX (dialog->play_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Play: step forward. */
  widget = GTK_WIDGET (gtk_tool_button_new (NULL, N_("Step to next frame")));
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget),
                                      gtk_action_group_get_action (dialog->play_actions, "step"));

  gtk_box_pack_start (GTK_BOX (dialog->play_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Play: rewind. */
  widget = GTK_WIDGET (gtk_tool_button_new (NULL, N_("Rewind the animation")));
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget),
                                      gtk_action_group_get_action (dialog->play_actions, "rewind"));

  gtk_box_pack_start (GTK_BOX (dialog->play_bar), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Progress box. */

  dialog->progress_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_paned_add2 (GTK_PANED (hbox), dialog->progress_bar);
  gtk_widget_show (dialog->progress_bar);

  /* End frame spin button. */

  adjust = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 5.0, 0.0));
  dialog->endframe_spin = gtk_spin_button_new (adjust, 1.0, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (dialog->endframe_spin), 2);

  gtk_box_pack_end (GTK_BOX (dialog->progress_bar), dialog->endframe_spin, FALSE, FALSE, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (dialog->endframe_spin), GTK_UPDATE_IF_VALID);
  gtk_widget_show (dialog->endframe_spin);

  g_signal_connect (adjust,
                    "value-changed",
                    G_CALLBACK (endframe_changed),
                    dialog);

  g_signal_connect (dialog->endframe_spin, "button-press-event",
                    G_CALLBACK (adjustment_pressed),
                    dialog);

  gimp_help_set_help_data (dialog->endframe_spin, _("End frame"), NULL);

  /* Progress bar. */

  dialog->progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (dialog->progress_bar), dialog->progress, TRUE, TRUE, 0);
  gtk_widget_show (dialog->progress);

  gtk_widget_add_events (dialog->progress,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                         GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
  g_signal_connect (dialog->progress, "enter-notify-event",
                    G_CALLBACK (progress_entered),
                    dialog);
  g_signal_connect (dialog->progress, "leave-notify-event",
                    G_CALLBACK (progress_left),
                    dialog);
  g_signal_connect (dialog->progress, "motion-notify-event",
                    G_CALLBACK (progress_motion),
                    dialog);
  g_signal_connect (dialog->progress, "button-press-event",
                    G_CALLBACK (progress_button),
                    dialog);

  /* Start frame spin button. */

  adjust = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 5.0, 0.0));
  dialog->startframe_spin = gtk_spin_button_new (adjust, 1.0, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (dialog->startframe_spin), 2);

  gtk_box_pack_end (GTK_BOX (dialog->progress_bar), dialog->startframe_spin, FALSE, FALSE, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (dialog->startframe_spin), GTK_UPDATE_IF_VALID);
  gtk_widget_show (dialog->startframe_spin);

  g_signal_connect (adjust,
                    "value-changed",
                    G_CALLBACK (startframe_changed),
                    dialog);

  g_signal_connect (GTK_ENTRY (dialog->startframe_spin), "button-press-event",
                    G_CALLBACK (adjustment_pressed),
                    dialog);

  gimp_help_set_help_data (dialog->startframe_spin, _("Start frame"), NULL);

  /* Finalization. */
  gtk_window_set_resizable (GTK_WINDOW (dialog->window), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dialog->window), dialog->preview_width + 20, dialog->preview_height + 90);
  gtk_widget_show (dialog->window);

  /* shape_drawing_area for detached feature. */
  dialog->shape_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_resizable (GTK_WINDOW (dialog->shape_window), FALSE);

  dialog->shape_drawing_area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (dialog->shape_window), dialog->shape_drawing_area);
  gtk_widget_show (dialog->shape_drawing_area);
  gtk_widget_add_events (dialog->shape_drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_widget_realize (dialog->shape_drawing_area);

  gdk_window_set_back_pixmap (gtk_widget_get_window (dialog->shape_window), NULL, FALSE);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (dialog->shape_window),
                                       GDK_HAND2);
  gdk_window_set_cursor (gtk_widget_get_window (dialog->shape_window), cursor);
  gdk_cursor_unref (cursor);

  g_signal_connect (dialog->shape_drawing_area, "size-allocate",
                    G_CALLBACK(da_size_callback),
                    dialog);
  g_signal_connect (dialog->shape_window, "button-press-event",
                    G_CALLBACK (shape_pressed),
                    dialog);
  g_signal_connect (dialog->shape_window, "button-release-event",
                    G_CALLBACK (shape_released),
                    NULL);
  g_signal_connect (dialog->shape_window, "motion-notify-event",
                    G_CALLBACK (shape_motion),
                    NULL);

  g_object_set_data (G_OBJECT (dialog->shape_window),
                     "cursor-offset", g_new0 (CursorOffset, 1));

  g_signal_connect (dialog->drawing_area, "expose-event",
                    G_CALLBACK (repaint_da),
                    dialog);

  g_signal_connect (dialog->shape_drawing_area, "expose-event",
                    G_CALLBACK (repaint_da),
                    dialog);

  /* We request a minimum size *after* having connecting the
   * size-allocate signal for correct initialization. */
  gtk_widget_set_size_request (dialog->drawing_area, dialog->preview_width, dialog->preview_height);
  gtk_widget_set_size_request (dialog->shape_drawing_area, dialog->preview_width, dialog->preview_height);
}

static GtkUIManager *
ui_manager_new (AnimationPlayDialog *dialog)
{
  static GtkActionEntry play_entries[] =
  {
    { "step-back", "media-skip-backward",
      N_("Step _back"), "d", N_("Step back to previous frame"),
      G_CALLBACK (step_back_callback) },

    { "step", "media-skip-forward",
      N_("_Step"), "f", N_("Step to next frame"),
      G_CALLBACK (step_callback) },

    { "rewind", "media-seek-backward",
      NULL, "s", N_("Rewind the animation"),
      G_CALLBACK (rewind_callback) },
  };

  static GtkToggleActionEntry play_toggle_entries[] =
  {
    { "play", "media-playback-start",
      NULL, "space", N_("Start playback"),
      G_CALLBACK (play_callback), FALSE },
  };

  static GtkActionEntry settings_entries[] =
  {
    /* Refresh is not really a settings, but it makes sense to be grouped
     * as such, because we want to be able to refresh and set things in the
     * same moments. */
    { "refresh", GIMP_ICON_VIEW_REFRESH,
      N_("Refresh"), "<control>R", N_("Reload the image"),
      G_CALLBACK (refresh_callback) },

    {
      "speed-up", NULL,
      N_("Faster"), "bracketright", N_("Increase the speed of the animation"),
      G_CALLBACK (speed_up_callback)
    },
    {
      "speed-down", NULL,
      N_("Slower"), "bracketleft", N_("Decrease the speed of the animation"),
      G_CALLBACK (speed_down_callback)
    },
  };

  static GtkActionEntry view_entries[] =
  {
    { "zoom-in", GTK_STOCK_ZOOM_IN,
      NULL, "plus", N_("Zoom in"),
      G_CALLBACK (zoom_in_callback) },

    { "zoom-in-accel", GTK_STOCK_ZOOM_IN,
      NULL, "KP_Add", N_("Zoom in"),
      G_CALLBACK (zoom_in_callback) },

    { "zoom-out", GTK_STOCK_ZOOM_OUT,
      NULL, "minus", N_("Zoom out"),
      G_CALLBACK (zoom_out_callback) },

    { "zoom-out-accel", GTK_STOCK_ZOOM_OUT,
      NULL, "KP_Subtract", N_("Zoom out"),
      G_CALLBACK (zoom_out_callback) },

    { "zoom-reset", GTK_STOCK_ZOOM_OUT,
      NULL, "equal", N_("Zoom out"),
      G_CALLBACK (zoom_reset_callback) },

    { "zoom-reset-accel", GTK_STOCK_ZOOM_OUT,
      NULL, "KP_Equal", N_("Zoom out"),
      G_CALLBACK (zoom_reset_callback) },
  };

  static GtkToggleActionEntry view_toggle_entries[] =
  {
    { "detach", GIMP_STOCK_DETACH,
      N_("Detach"), NULL,
      N_("Detach the animation from the dialog window"),
      G_CALLBACK (detach_callback), FALSE }
  };

  static GtkActionEntry various_entries[] =
  {
    { "help", "help-browser",
      N_("About the animation plug-in"), "question", NULL,
      G_CALLBACK (help_callback) },

    { "close", "window-close",
      N_("Quit"), "<control>W", NULL,
      G_CALLBACK (close_callback)
    },
    {
      "quit", "application-quit",
      N_("Quit"), "<control>Q", NULL,
      G_CALLBACK (close_callback)
    },
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GError         *error      = NULL;

  /* All playback related actions. */
  dialog->play_actions = gtk_action_group_new ("playback");
  gtk_action_group_set_translation_domain (dialog->play_actions, NULL);
  gtk_action_group_add_actions (dialog->play_actions,
                                play_entries,
                                G_N_ELEMENTS (play_entries),
                                dialog);
  gtk_action_group_add_toggle_actions (dialog->play_actions,
                                       play_toggle_entries,
                                       G_N_ELEMENTS (play_toggle_entries),
                                       dialog);
  connect_accelerators (ui_manager, dialog->play_actions);
  gtk_ui_manager_insert_action_group (ui_manager, dialog->play_actions, -1);

  /* All settings related actions. */
  dialog->settings_actions = gtk_action_group_new ("settings");
  gtk_action_group_set_translation_domain (dialog->settings_actions, NULL);
  gtk_action_group_add_actions (dialog->settings_actions,
                                settings_entries,
                                G_N_ELEMENTS (settings_entries),
                                dialog);
  connect_accelerators (ui_manager, dialog->settings_actions);
  gtk_ui_manager_insert_action_group (ui_manager, dialog->settings_actions, -1);

  /* All view actions. */
  dialog->view_actions = gtk_action_group_new ("view");
  gtk_action_group_set_translation_domain (dialog->view_actions, NULL);
  gtk_action_group_add_actions (dialog->view_actions,
                                view_entries,
                                G_N_ELEMENTS (view_entries),
                                dialog);
  gtk_action_group_add_toggle_actions (dialog->view_actions,
                                       view_toggle_entries,
                                       G_N_ELEMENTS (view_toggle_entries),
                                       dialog);
  connect_accelerators (ui_manager, dialog->view_actions);
  gtk_ui_manager_insert_action_group (ui_manager, dialog->view_actions, -1);

  /* Remaining various actions. */
  dialog->various_actions = gtk_action_group_new ("various");

  gtk_action_group_set_translation_domain (dialog->various_actions, NULL);

  gtk_action_group_add_actions (dialog->various_actions,
                                various_entries,
                                G_N_ELEMENTS (various_entries),
                                dialog);
  connect_accelerators (ui_manager, dialog->various_actions);
  gtk_ui_manager_insert_action_group (ui_manager, dialog->various_actions, -1);

  /* Finalize. */
  gtk_window_add_accel_group (GTK_WINDOW (dialog->window),
                              gtk_ui_manager_get_accel_group (ui_manager));
  gtk_accel_group_lock (gtk_ui_manager_get_accel_group (ui_manager));

  /* Finally make some limited popup menu. */
  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <popup name=\"anim-play-popup\" accelerators=\"true\">"
                                     "    <menuitem action=\"refresh\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"zoom-in\" />"
                                     "    <menuitem action=\"zoom-out\" />"
                                     "    <menuitem action=\"zoom-reset\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"speed-up\" />"
                                     "    <menuitem action=\"speed-down\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"help\" />"
                                     "    <separator />"
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
refresh_dialog (AnimationPlayDialog *dialog)
{
  GdkScreen *screen;
  guint      screen_width, screen_height;
  gint       window_width, window_height;

  /* Update GUI size. */
  screen = gtk_widget_get_screen (dialog->window);
  screen_height = gdk_screen_get_height (screen);
  screen_width = gdk_screen_get_width (screen);
  gtk_window_get_size (GTK_WINDOW (dialog->window), &window_width, &window_height);

  /* if the *window* size is bigger than the screen size,
   * diminish the drawing area by as much, then compute the corresponding scale. */
  if (window_width + 50 > screen_width || window_height + 50 > screen_height)
  {
      gint expected_drawing_area_width = MAX (1, dialog->preview_width - window_width + screen_width);
      gint expected_drawing_area_height = MAX (1, dialog->preview_height - window_height + screen_height);

      gdouble expected_scale = MIN ((gdouble) expected_drawing_area_width / (gdouble) dialog->preview_width,
                                    (gdouble) expected_drawing_area_height / (gdouble) dialog->preview_height);
      update_scale (dialog, expected_scale);

      /* There is unfortunately no good way to know the size of the decorations, taskbars, etc.
       * So we take a wild guess by making the window slightly smaller to fit into most case. */
      gtk_window_set_default_size (GTK_WINDOW (dialog->window),
                                   MIN (expected_drawing_area_width + 20, screen_width - 60),
                                   MIN (expected_drawing_area_height + 90, screen_height - 60));

      gtk_window_reshow_with_initial_size (GTK_WINDOW (dialog->window));
  }
}

/* Update the tool sensitivity for playing, depending on the number of frames. */
static void
update_ui_sensitivity (AnimationPlayDialog *dialog)
{
  gboolean animated;

  animated = dialog->settings.end_frame - dialog->settings.start_frame > 1;
  /* Play actions only if we selected several frames between start/end. */
  gtk_action_group_set_sensitive (dialog->play_actions, animated);
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->play_bar), animated);

  /* We can access the progress bar if there are several frames. */
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->progress_bar),
                            dialog->settings.num_frames > 1);

  /* Settings are always changeable. */
  gtk_action_group_set_sensitive (dialog->settings_actions, TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->settings_bar), TRUE);

  /* View are always meaningfull with at least 1 frame. */
  gtk_action_group_set_sensitive (dialog->view_actions,
                                  dialog->settings.num_frames >= 1);
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->view_bar),
                            dialog->settings.num_frames >= 1);
}

/***************** CALLBACKS ********************/

static void
close_callback (GtkAction           *action,
                AnimationPlayDialog *dialog)
{
  clean_exit (dialog);
}

static void
help_callback (GtkAction           *action,
               AnimationPlayDialog *dialog)
{
  gimp_standard_help_func (PLUG_IN_PROC, dialog->window);
}

static void
window_destroy (GtkWidget           *widget,
                AnimationPlayDialog *dialog)
{
  clean_exit (dialog);
}

static gboolean
popup_menu (GtkWidget           *widget,
            AnimationPlayDialog *dialog)
{
  GtkWidget *menu = gtk_ui_manager_get_widget (dialog->ui_manager, "/anim-play-popup");

  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL, NULL, NULL,
                  0, gtk_get_current_event_time ());

  return TRUE;
}

static gboolean
adjustment_pressed (GtkWidget           *widget,
                    GdkEventButton      *event,
                    AnimationPlayDialog *dialog)
{
  gboolean event_processed = FALSE;

  if (event->type == GDK_BUTTON_PRESS &&
      event->button == 2)
    {
      GtkSpinButton *spin = GTK_SPIN_BUTTON (widget);
      GtkAdjustment *adj = gtk_spin_button_get_adjustment (spin);

      gtk_adjustment_set_value (adj,
                                (gdouble) dialog->settings.current_frame);

      /* We don't want the middle click to have another usage (in
       * particular, there is likely no need to copy-paste in these spin
       * buttons). */
      event_processed = TRUE;
    }

  return event_processed;
}

static gboolean
da_button_press (GtkWidget           *widget,
              GdkEventButton      *event,
              AnimationPlayDialog *dialog)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      GtkWidget *menu = gtk_ui_manager_get_widget (dialog->ui_manager, "/anim-play-popup");

      gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
      gtk_menu_popup (GTK_MENU (menu),
                      NULL, NULL, NULL, NULL,
                      event->button,
                      event->time);
      return TRUE;
    }

  return FALSE;
}

/*
 * Update the actual drawing area metrics, which may be different from
 * requested, since there is no full control of the WM.
 */
static void
da_size_callback (GtkWidget           *drawing_area,
                  GtkAllocation       *allocation,
                  AnimationPlayDialog *dialog)
{
  guchar   **drawing_data;

  if (drawing_area == dialog->shape_drawing_area)
    {
      if (allocation->width  == dialog->shape_drawing_area_width &&
          allocation->height == dialog->shape_drawing_area_height)
        return;

      dialog->shape_drawing_area_width  = allocation->width;
      dialog->shape_drawing_area_height = allocation->height;

      g_free (dialog->shape_drawing_area_data);
      drawing_data = &dialog->shape_drawing_area_data;
    }
  else
    {
      if (allocation->width  == dialog->drawing_area_width &&
          allocation->height == dialog->drawing_area_height)
        return;

      dialog->drawing_area_width  = allocation->width;
      dialog->drawing_area_height = allocation->height;

      g_free (dialog->drawing_area_data);
      drawing_data = &dialog->drawing_area_data;
    }

  dialog->settings.scale = MIN ((gdouble) allocation->width / (gdouble) dialog->preview_width,
                                (gdouble) allocation->height / (gdouble) dialog->preview_height);

  *drawing_data = g_malloc (allocation->width * allocation->height * 3);

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->view_actions, "detach"))) &&
      drawing_area == dialog->drawing_area)
    {
      /* Set "alpha grid" background. */
      total_alpha_preview (dialog->drawing_area_data,
                           allocation->width,
                           allocation->height);
      repaint_da (dialog->drawing_area, NULL, dialog);
    }
  else 
    {
      /* Update the zoom information. */
      GtkEntry *zoomcombo_text_child;

      zoomcombo_text_child = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->zoomcombo)));
      if (zoomcombo_text_child)
        {
          char* new_entry_text = g_strdup_printf  (_("%.1f %%"), dialog->settings.scale * 100.0);

          gtk_entry_set_text (zoomcombo_text_child, new_entry_text);
          g_free (new_entry_text);
        }

      /* As we re-allocated the drawn data, let's render it again. */
      if (dialog->settings.current_frame - dialog->settings.frame_min < dialog->settings.num_frames)
        render_frame (dialog, TRUE);
    }
}

static gboolean
shape_pressed (GtkWidget           *widget,
               GdkEventButton      *event,
               AnimationPlayDialog *dialog)
{
  if (da_button_press (widget, event, dialog))
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
  GdkWindow       *root_win;

  root_win = gdk_get_default_root_window ();
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
repaint_da (GtkWidget           *darea,
            GdkEventExpose      *event,
            AnimationPlayDialog *dialog)
{
  GtkStyle *style = gtk_widget_get_style (darea);
  gint      da_width;
  gint      da_height;
  guchar   *da_data;

  if (darea == dialog->drawing_area)
    {
      da_width  = dialog->drawing_area_width;
      da_height = dialog->drawing_area_height;
      da_data   = dialog->drawing_area_data;
    }
  else
    {
      da_width  = dialog->shape_drawing_area_width;
      da_height = dialog->shape_drawing_area_height;
      da_data   = dialog->shape_drawing_area_data;
    }

  gdk_draw_rgb_image (gtk_widget_get_window (darea),
                      style->white_gc,
                      (gint) ((da_width - dialog->settings.scale * dialog->preview_width) / 2),
                      (gint) ((da_height - dialog->settings.scale * dialog->preview_height) / 2),
                      da_width, da_height,
                      (dialog->settings.num_frames == 1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      da_data, da_width * 3);

  return TRUE;
}

static gboolean
progress_button (GtkWidget           *widget,
                 GdkEventButton      *event,
                 AnimationPlayDialog *dialog)
{
  GtkAdjustment *startframe_adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->startframe_spin));
  GtkAdjustment *endframe_adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->endframe_spin));

  /* ignore double and triple click */
  if (event->type == GDK_BUTTON_PRESS)
    {
      GtkAllocation  allocation;
      guint          goto_frame;

      gtk_widget_get_allocation (widget, &allocation);

      goto_frame = dialog->settings.frame_min + (gint) (event->x / (allocation.width / dialog->settings.num_frames));

      if (goto_frame < dialog->settings.start_frame)
        gtk_adjustment_set_value (startframe_adjust, (gdouble) goto_frame);

      if (goto_frame > dialog->settings.end_frame)
        gtk_adjustment_set_value (endframe_adjust, (gdouble) goto_frame);

      if (goto_frame >= dialog->settings.frame_min && goto_frame < dialog->settings.frame_min + dialog->settings.num_frames)
        {
          dialog->settings.current_frame = goto_frame;
          render_frame (dialog, FALSE);
        }
    }

  return FALSE;
}

static gboolean
progress_entered (GtkWidget           *widget,
                  GdkEventCrossing    *event,
                  AnimationPlayDialog *dialog)
{
  GtkAllocation  allocation;
  guint          goto_frame;

  gtk_widget_get_allocation (widget, &allocation);

  goto_frame = dialog->settings.frame_min + (gint) (event->x / (allocation.width / dialog->settings.num_frames));

  show_goto_progress (goto_frame, dialog);

  return FALSE;
}

static gboolean
progress_motion (GtkWidget           *widget,
                 GdkEventMotion      *event,
                 AnimationPlayDialog *dialog)
{
  GtkAllocation  allocation;
  guint          goto_frame;

  gtk_widget_get_allocation (widget, &allocation);

  goto_frame = dialog->settings.frame_min + (gint) (event->x / (allocation.width / dialog->settings.num_frames));

  show_goto_progress (goto_frame, dialog);

  return FALSE;
}

static gboolean
progress_left (GtkWidget           *widget,
               GdkEventCrossing    *event,
               AnimationPlayDialog *dialog)
{
  show_playing_progress (dialog);

  return FALSE;
}

static void
detach_callback (GtkToggleAction     *action,
                 AnimationPlayDialog *dialog)
{
  gboolean detached = gtk_toggle_action_get_active (action);

  if (detached)
    {
      gint x, y;

      gtk_window_set_screen (GTK_WINDOW (dialog->shape_window),
                             gtk_widget_get_screen (dialog->drawing_area));

      gtk_widget_show (dialog->shape_window);

      if (! gtk_widget_get_realized (dialog->drawing_area))
        {
          gtk_widget_realize (dialog->drawing_area);
        }
      if (! gtk_widget_get_realized (dialog->shape_drawing_area))
        {
          gtk_widget_realize (dialog->shape_drawing_area);
        }

      gdk_window_get_origin (gtk_widget_get_window (dialog->drawing_area), &x, &y);

      gtk_window_move (GTK_WINDOW (dialog->shape_window), x + 6, y + 6);

      gdk_window_set_back_pixmap (gtk_widget_get_window (dialog->shape_drawing_area), NULL, TRUE);

      /* Set "alpha grid" background. */
      total_alpha_preview (dialog->drawing_area_data,
                           dialog->drawing_area_width,
                           dialog->drawing_area_height);
      repaint_da (dialog->drawing_area, NULL, dialog);
    }
  else
    {
      gtk_widget_hide (dialog->shape_window);
    }

  render_frame (dialog, TRUE);
}

static void
play_callback (GtkToggleAction     *action,
               AnimationPlayDialog *dialog)
{
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play"))))
    {
      guint timer;
      guint duration = dialog->frames[dialog->settings.current_frame - dialog->settings.frame_min]->duration;

      if (duration <= 0)
        duration = (guint) (1000.0 / dialog->settings.frame_rate);

      timer = g_timeout_add (duration, (GSourceFunc) advance_frame_callback, dialog);
      set_timer (timer);

      gtk_action_set_icon_name (GTK_ACTION (action), "media-playback-pause");
    }
  else
    {
      /* Remove any previous timer to stock playback. */
      set_timer (0);

      gtk_action_set_icon_name (GTK_ACTION (action), "media-playback-start");
    }

  g_object_set (action,
                "tooltip",
                gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play"))) ?
                  _("Pause playback") : _("Start playback"),
                NULL);
}

static void
step_back_callback (GtkAction           *action,
                    AnimationPlayDialog *dialog)
{
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play"))))
    {
      gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
    }
  do_back_step (dialog);
}

static void
step_callback (GtkAction           *action,
               AnimationPlayDialog *dialog)
{
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play"))))
    {
      gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
    }
  do_step (dialog);
}

static void
rewind_callback (GtkAction           *action,
                 AnimationPlayDialog *dialog)
{
  gboolean was_playing = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play")));

  if (was_playing)
    {
      gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
    }

  dialog->settings.current_frame = dialog->settings.start_frame;
  render_frame (dialog, FALSE);

  /* If we were playing, start playing again. */
  if (was_playing)
    {
      gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
    }
}

static void
refresh_callback (GtkAction           *action,
                  AnimationPlayDialog *dialog)
{
  initialize (dialog);
}

static void
zoom_in_callback (GtkAction           *action,
                  AnimationPlayDialog *dialog)
{
  gdouble  scale = get_scale (dialog, -1);

  g_signal_handlers_block_by_func (dialog->zoomcombo,
                                   G_CALLBACK (zoomcombo_changed),
                                   dialog);
  update_scale (dialog, scale + 0.1);

  g_signal_handlers_unblock_by_func (dialog->zoomcombo,
                                     G_CALLBACK (zoomcombo_changed),
                                     dialog);
}

static void
zoom_out_callback (GtkAction           *action,
                   AnimationPlayDialog *dialog)
{
  gdouble  scale = get_scale (dialog, -1);

  if (scale > 0.1)
    {
      g_signal_handlers_block_by_func (dialog->zoomcombo,
                                       G_CALLBACK (zoomcombo_changed),
                                       dialog);
      update_scale (dialog, scale - 0.1);
      g_signal_handlers_unblock_by_func (dialog->zoomcombo,
                                         G_CALLBACK (zoomcombo_changed),
                                         dialog);
    }
}

static void
zoom_reset_callback (GtkAction           *action,
                     AnimationPlayDialog *dialog)
{
  gdouble  scale = get_scale (dialog, -1);

  if (scale != 1.0)
    {
      g_signal_handlers_block_by_func (dialog->zoomcombo,
                                       G_CALLBACK (zoomcombo_changed),
                                       dialog);
      update_scale (dialog, 1.0);
      g_signal_handlers_unblock_by_func (dialog->zoomcombo,
                                         G_CALLBACK (zoomcombo_changed),
                                         dialog);
    }
}

static void
speed_up_callback (GtkAction           *action,
                   AnimationPlayDialog *dialog)
{
  if (dialog->settings.frame_rate <= MAX_FRAMERATE - 1)
    {
      gchar *text;

      ++dialog->settings.frame_rate;

      g_signal_handlers_block_by_func (dialog->fpscombo,
                                       G_CALLBACK (fpscombo_changed),
                                       dialog);
      g_signal_handlers_block_by_func (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))),
                                       G_CALLBACK (fpscombo_activated),
                                       dialog);

      text = g_strdup_printf  (_("%g fps"), dialog->settings.frame_rate);
      gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))), text);
      g_free (text);

      g_signal_handlers_unblock_by_func (dialog->fpscombo,
                                         G_CALLBACK (fpscombo_changed),
                                         dialog);
      g_signal_handlers_unblock_by_func (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))),
                                         G_CALLBACK (fpscombo_activated),
                                         dialog);
    }
}

static void
speed_down_callback (GtkAction           *action,
                     AnimationPlayDialog *dialog)
{
  if (dialog->settings.frame_rate > 1)
    {
      gchar *text;

      --dialog->settings.frame_rate;

      g_signal_handlers_block_by_func (dialog->fpscombo,
                                       G_CALLBACK (fpscombo_changed),
                                       dialog);
      g_signal_handlers_block_by_func (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))),
                                       G_CALLBACK (fpscombo_activated),
                                       dialog);

      text = g_strdup_printf  (_("%g fps"), dialog->settings.frame_rate);
      gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))), text);
      g_free (text);

      g_signal_handlers_unblock_by_func (dialog->fpscombo,
                                         G_CALLBACK (fpscombo_changed),
                                         dialog);
      g_signal_handlers_unblock_by_func (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->fpscombo))),
                                         G_CALLBACK (fpscombo_activated),
                                         dialog);
    }
}

static void
framecombo_changed (GtkWidget           *combo,
                    AnimationPlayDialog *dialog)
{
  gboolean was_playing;

  dialog->settings.frame_disposal = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  was_playing = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play")));

  render_frame (dialog, init_frames (dialog));

  if (was_playing)
    {
      /* Initializing frames stopped the playing. I restart it. */
      gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
    }
}

/*
 * Callback emitted when the user hits the Enter key of the fps combo.
 */
static void
fpscombo_activated (GtkEntry            *combo_entry,
                    AnimationPlayDialog *dialog)
{
  const gchar *active_text;
  gdouble      fps;
  gchar       *text;

  active_text = gtk_entry_get_text (combo_entry);
  /* Try a text conversion, locale-aware. */
  fps         = g_strtod (active_text, NULL);

  if (fps >= MAX_FRAMERATE)
    {
      /* Let's avoid huge frame rates. */
      fps = MAX_FRAMERATE;
    }
  else if (fps <= 0)
    {
      /* Null or negative framerates are impossible. */
      fps = 0.1;
    }

  dialog->settings.frame_rate = fps;

  /* Now let's format the text cleanly: "%g fps". */
  text = g_strdup_printf  (_("%g fps"), dialog->settings.frame_rate);
  gtk_entry_set_text (combo_entry, text);
  g_free (text);
}

static void
fpscombo_changed (GtkWidget           *combo,
                  AnimationPlayDialog *dialog)
{
  gint index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* If no index, user is probably editing by hand. We wait for him to click "Enter". */
  if (index != -1)
    {
      dialog->settings.frame_rate = get_fps (index);
    }
}

/*
 * Callback emitted when the user hits the Enter key of the zoom combo.
 */
static void
zoomcombo_activated (GtkEntry            *combo,
                     AnimationPlayDialog *dialog)
{
  update_scale (dialog, get_scale (dialog, -1));
}

/*
 * Callback emitted when the user selects a zoom in the dropdown,
 * or edits the text entry.
 * We don't want to process manual edits because it greedily emits
 * signals after each character deleted or added.
 */
static void
zoomcombo_changed (GtkWidget           *combo,
                   AnimationPlayDialog *dialog)
{
  gint index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* If no index, user is probably editing by hand. We wait for him to click "Enter". */
  if (index != -1)
    update_scale (dialog, get_scale (dialog, index));
}

static void
startframe_changed (GtkAdjustment       *adjustment,
                    AnimationPlayDialog *dialog)
{
  GtkAdjustment *endframe_adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->endframe_spin));
  gdouble        value = gtk_adjustment_get_value (adjustment);

  if (value < dialog->settings.frame_min || value > dialog->settings.frame_max)
    {
      value = dialog->settings.frame_min;
      gtk_adjustment_set_value (adjustment, dialog->settings.frame_min);
    }

  dialog->settings.start_frame = (guint) value;

  if (gtk_adjustment_get_value (endframe_adjust) < value)
    {
      gtk_adjustment_set_value (endframe_adjust, dialog->settings.frame_max);
      dialog->settings.end_frame = (guint) dialog->settings.frame_max;
    }

  if (dialog->settings.current_frame < dialog->settings.start_frame)
    {
      dialog->settings.current_frame = dialog->settings.start_frame;
      render_frame (dialog, FALSE);
    }

  update_ui_sensitivity (dialog);
}

static void
endframe_changed (GtkAdjustment       *adjustment,
                  AnimationPlayDialog *dialog)
{
  GtkAdjustment *startframe_adjust = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dialog->startframe_spin));
  gdouble        value = gtk_adjustment_get_value (adjustment);

  if (value < dialog->settings.frame_min || value > dialog->settings.frame_max)
    {
      value = dialog->settings.frame_max;
      gtk_adjustment_set_value (adjustment, dialog->settings.frame_max);
    }

  dialog->settings.end_frame = (guint) value;

  if (gtk_adjustment_get_value (startframe_adjust) > value)
    {
      gtk_adjustment_set_value (startframe_adjust, dialog->settings.frame_min);
      dialog->settings.start_frame = (guint) dialog->settings.frame_min;
    }

  if (dialog->settings.current_frame > dialog->settings.end_frame)
    {
      dialog->settings.current_frame = dialog->settings.end_frame;
      render_frame (dialog, FALSE);
    }

  update_ui_sensitivity (dialog);
}

/*
 * Callback emitted when the user toggle the proxy checkbox.
 */
static void
proxy_checkbox_expanded (GtkExpander         *expander,
                         GParamSpec          *param_spec,
                         AnimationPlayDialog *dialog)
{
  gint previous_preview_width  = dialog->preview_width;
  gint previous_preview_height = dialog->preview_height;
  gint image_width  = gimp_image_width (dialog->image_id);
  gint image_height = gimp_image_height (dialog->image_id);

  if (gtk_expander_get_expanded (expander))
    {
      proxycombo_activated (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->proxycombo))),
                            dialog);
    }
  else if (dialog->preview_width  != image_width ||
           dialog->preview_height != image_height)
    {
      dialog->preview_width  = image_width;
      dialog->preview_height = image_height;

      if (previous_preview_width  != dialog->preview_width ||
          previous_preview_height != dialog->preview_height)
        {
          gint index = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->zoomcombo));

          update_scale (dialog, get_scale (dialog, index));

          init_frames (dialog);
          render_frame (dialog, TRUE);
        }
    }
}

/*
 * Callback emitted when the user hits the Enter key of the fps combo.
 */
static void
proxycombo_activated (GtkEntry            *combo_entry,
                      AnimationPlayDialog *dialog)
{
  const gchar *active_text;
  gchar       *text;
  gint         image_width;
  gint         image_height;
  gdouble      ratio;

  active_text = gtk_entry_get_text (combo_entry);
  /* Try a text conversion, locale-aware. */
  ratio       = g_strtod (active_text, NULL);

  ratio = ratio / 100.0;
  if (ratio >= 1.0)
    {
      ratio = 1.0;
    }
  else if (ratio <= 0)
    {
      /* Null or negative ratio are impossible. */
      ratio = 0.1;
    }

  image_width  = gimp_image_width (dialog->image_id);
  image_height = gimp_image_height (dialog->image_id);

  /* Now let's format the text cleanly: "%.1f %%". */
  text = g_strdup_printf  (_("%.1f %%"), ratio * 100.0);
  gtk_entry_set_text (combo_entry, text);
  g_free (text);

  /* Finally set the preview size, unless they were already good. */
  if (dialog->preview_width  != image_width * ratio ||
      dialog->preview_height != image_height * ratio)
    {
      gboolean     was_playing;

      was_playing = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->play_actions, "play")));

      dialog->preview_width  = image_width * ratio;
      dialog->preview_height = image_height * ratio;
      update_scale (dialog, get_scale (dialog, -1));

      init_frames (dialog);
      render_frame (dialog, TRUE);

      if (was_playing)
        {
          /* Initializing frames stopped the playing. I restart it. */
          gtk_action_activate (gtk_action_group_get_action (dialog->play_actions, "play"));
        }
    }
}

static void
proxycombo_changed (GtkWidget           *combo,
                    AnimationPlayDialog *dialog)
{
  gint index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* If no index, user is probably editing by hand. We wait for him to click "Enter". */
  if (index != -1)
    {
      proxycombo_activated (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dialog->proxycombo))),
                            dialog);
    }
}

/* Rendering Functions */

static void
render_frame (AnimationPlayDialog *dialog,
              gboolean             force_render)
{
  static gint    last_frame_index        = -1;
  static gchar  *shape_preview_mask      = NULL;
  static guint   shape_preview_mask_size = 0;
  static guchar *rawframe                = NULL;
  static guint   rawframe_size           = 0;
  GeglBuffer    *buffer;
  gint           i, j, k;
  guchar        *srcptr;
  guchar        *destptr;
  GtkWidget     *da;
  guint          drawing_width;
  guint          drawing_height;
  guchar        *preview_data;

  /* Do not try to update the drawing areas while init_frames() is still running. */
  if (dialog->frames_lock)
    return;

  /* Unless we are in a case where we always want to redraw
   * (after a zoom, preview mode change, reinitialization, and such),
   * we don't redraw if the same frame was already drawn. */
  if ((! force_render) && dialog->settings.num_frames > 0 && last_frame_index > -1 &&
      g_list_find (dialog->frames[last_frame_index]->indexes,
                   GINT_TO_POINTER (dialog->settings.current_frame - dialog->settings.frame_min)))
    {
      show_playing_progress (dialog);
      return;
    }

  dialog->frames_lock = TRUE;

  g_assert (dialog->settings.num_frames < 1 || (dialog->settings.current_frame >= dialog->settings.start_frame &&
                                                dialog->settings.current_frame <= dialog->settings.end_frame));

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->view_actions, "detach"))))
    {
      da = dialog->shape_drawing_area;
      preview_data = dialog->shape_drawing_area_data;
      drawing_width = dialog->shape_drawing_area_width;
      drawing_height = dialog->shape_drawing_area_height;

      if (dialog->settings.num_frames < 1)
        total_alpha_preview (preview_data, drawing_width, drawing_height);
    }
  else
    {
      da = dialog->drawing_area;
      preview_data = dialog->drawing_area_data;
      drawing_width = dialog->drawing_area_width;
      drawing_height = dialog->drawing_area_height;

      /* Set "alpha grid" background. */
      total_alpha_preview (preview_data, drawing_width, drawing_height);
    }

  /* When there is no frame to show, we simply display the alpha background and return. */
  if (dialog->settings.num_frames > 0)
    {
      /* Update the rawframe. */
      if (rawframe_size < drawing_width * drawing_height * 4)
        {
          rawframe_size = drawing_width * drawing_height * 4;
          g_free (rawframe);
          rawframe = g_malloc (rawframe_size);
        }

      buffer = gimp_drawable_get_buffer (dialog->frames[dialog->settings.current_frame - dialog->settings.frame_min]->drawable_id);

      /* Fetch and scale the whole raw new frame */
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, drawing_width, drawing_height),
                       dialog->settings.scale, babl_format ("R'G'B'A u8"),
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
      if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->view_actions, "detach"))))
        {
          gint ideal_shape_size = (drawing_width * drawing_height) /
                                   8 + 1 + drawing_height;

          if (shape_preview_mask_size < ideal_shape_size)
            {
              shape_preview_mask_size = ideal_shape_size;
              g_free (shape_preview_mask);
              shape_preview_mask = g_malloc (ideal_shape_size);
            }

          memset (shape_preview_mask, 0,
                  (drawing_width * drawing_height) / 8 + drawing_height);
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
          reshape_from_bitmap (dialog, shape_preview_mask);
        }

      /* clean up */
      g_object_unref (buffer);

      /* Update UI. */
      show_playing_progress (dialog);

      last_frame_index = dialog->settings.current_frame - dialog->settings.frame_min;
    }

  /* Display the preview buffer. */
  gdk_draw_rgb_image (gtk_widget_get_window (da),
                      (gtk_widget_get_style (da))->white_gc,
                      (gint) ((drawing_width - dialog->settings.scale * dialog->preview_width) / 2),
                      (gint) ((drawing_height - dialog->settings.scale * dialog->preview_height) / 2),
                      drawing_width, drawing_height,
                      (dialog->settings.num_frames == 1 ?
                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                      preview_data, drawing_width * 3);

  dialog->frames_lock = FALSE;
}

/* total_alpha_preview:
 * Fill the @drawing_data with an alpha (grey chess) pattern.
 * This uses a static array, copied over each line (with some shift to
 * reproduce the pattern), using `memcpy()`.
 * The reason why we keep the pattern in the statically allocated memory,
 * instead of simply looping through @drawing_data and recreating the
 * pattern is simply because `memcpy()` implementations are supposed to
 * be more efficient than loops over an array. */
static void
total_alpha_preview (guchar *drawing_data,
                     guint   drawing_width,
                     guint   drawing_height)
{
  static guint   alpha_line_width = 0;
  static guchar *alpha_line       = NULL;
  gint           i;

  g_assert (drawing_width > 0);

  /* If width change, we update the "alpha" line. */
  if (alpha_line_width < drawing_width + 8)
    {
      alpha_line_width = drawing_width + 8;

      g_free (alpha_line);

      /* A full line + 8 pixels (1 square). */
      alpha_line = g_malloc (alpha_line_width * 3);

      for (i = 0; i < alpha_line_width; i++)
        {
          /* 8 pixels dark grey, 8 pixels light grey, and so on. */
          if (i & 8)
            {
              alpha_line[i * 3 + 0] =
                alpha_line[i * 3 + 1] =
                alpha_line[i * 3 + 2] = 102;
            }
          else
            {
              alpha_line[i * 3 + 0] =
                alpha_line[i * 3 + 1] =
                alpha_line[i * 3 + 2] = 154;
            }
        }
    }

  for (i = 0; i < drawing_height; i++)
    {
      if (i & 8)
        {
          memcpy (&drawing_data[i * 3 * drawing_width],
                  alpha_line,
                  3 * drawing_width);
        }
      else
        {
          /* Every 8 vertical pixels, we shift the horizontal line by 8 pixels. */
          memcpy (&drawing_data[i * 3 * drawing_width],
                  alpha_line + 24,
                  3 * drawing_width);
        }
    }
}

static void
reshape_from_bitmap (AnimationPlayDialog *dialog,
                     const gchar         *bitmap)
{
  static gchar *prev_bitmap = NULL;
  static guint  prev_width = -1;
  static guint  prev_height = -1;

  if ((! prev_bitmap)                                  ||
      prev_width != dialog->shape_drawing_area_width   ||
      prev_height != dialog->shape_drawing_area_height ||
      (memcmp (prev_bitmap, bitmap,
               (dialog->shape_drawing_area_width *
                dialog->shape_drawing_area_height) / 8 +
               dialog->shape_drawing_area_height)))
    {
      GdkBitmap *shape_mask;

      shape_mask = gdk_bitmap_create_from_data (gtk_widget_get_window (dialog->shape_window),
                                                bitmap,
                                                dialog->shape_drawing_area_width, dialog->shape_drawing_area_height);
      gtk_widget_shape_combine_mask (dialog->shape_window, shape_mask, 0, 0);
      g_object_unref (shape_mask);

      if (!prev_bitmap || prev_width != dialog->shape_drawing_area_width || prev_height != dialog->shape_drawing_area_height)
        {
          g_free(prev_bitmap);
          prev_bitmap = g_malloc ((dialog->shape_drawing_area_width * dialog->shape_drawing_area_height) / 8 + dialog->shape_drawing_area_height);
          prev_width = dialog->shape_drawing_area_width;
          prev_height = dialog->shape_drawing_area_height;
        }

      memcpy (prev_bitmap, bitmap, (dialog->shape_drawing_area_width * dialog->shape_drawing_area_height) / 8 + dialog->shape_drawing_area_height);
    }
}

static void
show_playing_progress (AnimationPlayDialog *dialog)
{
  gchar *text;

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->progress),
                                 ((gfloat) (dialog->settings.current_frame - dialog->settings.frame_min) /
                                  (gfloat) (dialog->settings.num_frames - 0.999)));

  if (dialog->settings.frame_disposal != DISPOSE_TAGS || dialog->settings.frame_min == 1)
    {
      text = g_strdup_printf (_("Frame %d of %d"),
                              dialog->settings.current_frame,
                              dialog->settings.num_frames);
    }
  else
    {
      text = g_strdup_printf (_("Frame %d (%d) of %d"),
                              dialog->settings.current_frame - dialog->settings.frame_min + 1,
                              dialog->settings.current_frame, dialog->settings.num_frames);
    }
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->progress), text);
  g_free (text);
}

static void
show_loading_progress (AnimationPlayDialog *dialog,
                       gint                 layer_nb,
                       gint                 num_layers)
{
  gchar *text;
  gfloat load_rate = (gfloat) layer_nb / ((gfloat) num_layers - 0.999);

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->progress), load_rate);

  text = g_strdup_printf (_("Loading animation %d %%"), (gint) (load_rate * 100));
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->progress), text);
  g_free (text);

  /* Forcing the UI to update even with intensive computation. */
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static void
do_back_step (AnimationPlayDialog *dialog)
{
  if (dialog->settings.current_frame == dialog->settings.start_frame)
    {
      dialog->settings.current_frame = dialog->settings.end_frame;
    }
  else
    {
      --dialog->settings.current_frame;
    }
  render_frame (dialog, FALSE);
}

static void
do_step (AnimationPlayDialog *dialog)
{
  dialog->settings.current_frame = dialog->settings.start_frame +
                                   ((dialog->settings.current_frame - dialog->settings.start_frame + 1) %
                                    (dialog->settings.end_frame - dialog->settings.start_frame + 1));

  render_frame (dialog, FALSE);
}

static void
set_timer (guint new_timer)
{
  static guint timer = 0;

  if (timer)
    {
      g_source_remove (timer);
    }
  timer = new_timer;
}

static gboolean
advance_frame_callback (AnimationPlayDialog *dialog)
{
  guint duration;
  guint timer;

  duration = dialog->frames[(dialog->settings.current_frame - dialog->settings.frame_min + 1) %
             dialog->settings.num_frames]->duration;
  if (duration <= 0)
    {
      duration = (guint) (1000.0 / ((AnimationPlayDialog *) dialog)->settings.frame_rate);
    }

  timer = g_timeout_add (duration,
                         (GSourceFunc) advance_frame_callback,
                         (AnimationPlayDialog *) dialog);
  set_timer (timer);

  do_step (dialog);

  return G_SOURCE_REMOVE;
}

static void
show_goto_progress (guint                goto_frame,
                    AnimationPlayDialog *dialog)
{
  gchar         *text;

  /* update the dialog's progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->progress),
                                 ((gfloat) (dialog->settings.current_frame - dialog->settings.frame_min) /
                                  (gfloat) (dialog->settings.num_frames - 0.999)));

  if (dialog->settings.frame_disposal != DISPOSE_TAGS || dialog->settings.frame_min == 1)
    text = g_strdup_printf (_("Go to frame %d of %d"), goto_frame, dialog->settings.num_frames);
  else
    text = g_strdup_printf (_("Go to frame %d (%d) of %d"), goto_frame - dialog->settings.frame_min + 1, goto_frame, dialog->settings.num_frames);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->progress), text);
  g_free (text);
}

/************ UTILS ****************/

static void
connect_accelerators (GtkUIManager   *ui_manager,
                      GtkActionGroup *group)
{
  GList          *action_list;
  GList          *iter;

  action_list = gtk_action_group_list_actions (group);
  iter = action_list;
  while (iter)
    {
      /* Make sure all the action's accelerator are correctly connected,
       * even when there are no associated UI item. */
      GtkAction *action = GTK_ACTION (iter->data);

      gtk_action_set_accel_group (action,
                                  gtk_ui_manager_get_accel_group (ui_manager));
      gtk_action_connect_accelerator (action);

      iter = iter->next;
    }
  g_list_free (action_list);
}

/* get_fps:
 * Frame rate proposed as default.
 * These are the most common framerates.
 */
static gdouble
get_fps (gint       index)
{
  gdouble fps;

  switch (index)
    {
    case 0:
      fps = 12.0;
      break;
    case 1:
      fps = 24.0;
      break;
    case 2:
      fps = 25.0;
      break;
    case 3:
      fps = 30.0;
      break;
    case 4:
      fps = 48.0;
      break;
    default:
      fps = 24.0;
      break;
    }

  return fps;
}

static gdouble
get_proxy (AnimationPlayDialog *dialog,
           gint                 index)
{
  switch (index)
    {
    case 0:
      return 0.25;
    case 1:
      return 0.5;
    case 2:
      return 0.75;
    default:
      {
        /* likely -1 returned if there is no active item from the list.
         * Try a text conversion, locale-aware in such a case, assuming people write in percent. */
        gchar   *active_text = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (dialog->proxycombo));
        gdouble  proxy       = g_strtod (active_text, NULL);

        /* Proxy over 100% are meaningless. */
        if (proxy > 100.0)
          {
            proxy = 100.0;
          }
        else if (proxy <= 0.0)
          {
            proxy = 1.0;
          }
        g_free (active_text);
        return proxy / 100.0;
      }
    }
}

static void
update_scale (AnimationPlayDialog *dialog,
              gdouble              scale)
{
  guint expected_drawing_area_width;
  guint expected_drawing_area_height;

  /* FIXME: scales under 0.5 are broken. See bug 690265. */
  if (scale <= 0.5)
    scale = 0.51;

  expected_drawing_area_width  = dialog->preview_width * scale;
  expected_drawing_area_height = dialog->preview_height * scale;

  /* We don't update dialog->settings.scale directly because this might
   * end up not being the real scale. Instead we request this size for
   * the drawing areas, and the actual scale update will be done on the
   * callback when size is actually allocated. */
  gtk_widget_set_size_request (dialog->drawing_area, expected_drawing_area_width, expected_drawing_area_height);
  gtk_widget_set_size_request (dialog->shape_drawing_area, expected_drawing_area_width, expected_drawing_area_height);
  /* I force the shape window to a smaller size if we scale down. */
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (gtk_action_group_get_action (dialog->view_actions, "detach"))))
    {
      gint x, y;

      gdk_window_get_origin (gtk_widget_get_window (dialog->shape_window), &x, &y);
      gtk_window_reshow_with_initial_size (GTK_WINDOW (dialog->shape_window));
      gtk_window_move (GTK_WINDOW (dialog->shape_window), x, y);
    }
}

static gdouble
get_scale (AnimationPlayDialog *dialog,
           gint                 index)
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
        gchar   *active_text = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (dialog->zoomcombo));
        gdouble  zoom        = g_strtod (active_text, NULL);

        /* Negative scales are inconsistent. And we want to avoid huge scaling. */
        if (zoom > 300.0)
          zoom = 300.0;
        else if (zoom <= 50.0)
          /* FIXME: scales under 0.5 are broken. See bug 690265. */
          zoom = 50.1;
        g_free (active_text);
        return zoom / 100.0;
      }
    }
}

static void
save_settings (AnimationPlayDialog *dialog)
{
  GimpParasite *old_parasite;
  gboolean      undo_step_started = FALSE;

  /* First saving the settings globally. */
  gimp_set_data (PLUG_IN_PROC, &dialog->settings, sizeof (&dialog->settings));

  /* Then as a parasite for the specific image.
   * If there was already parasites and they were all the same as the
   * current state, do not resave them.
   * This prevents setting the image in a dirty state while it stayed
   * the same. */
  old_parasite = gimp_image_get_parasite (dialog->image_id, PLUG_IN_PROC "/frame-disposal");
  if (! old_parasite ||
      *(DisposeType *) gimp_parasite_data (old_parasite) != dialog->settings.frame_disposal)
    {
      GimpParasite *parasite;

      gimp_image_undo_group_start (dialog->image_id);
      undo_step_started = TRUE;

      parasite = gimp_parasite_new (PLUG_IN_PROC "/frame-disposal",
                                    GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE,
                                    sizeof (dialog->settings.frame_disposal),
                                    &dialog->settings.frame_disposal);
      gimp_image_attach_parasite (dialog->image_id, parasite);
      gimp_parasite_free (parasite);
    }
  gimp_parasite_free (old_parasite);

  old_parasite = gimp_image_get_parasite (dialog->image_id, PLUG_IN_PROC "/frame-rate");
  if (! old_parasite ||
      *(gdouble *) gimp_parasite_data (old_parasite) != dialog->settings.frame_rate)
    {
      GimpParasite *parasite;
      if (! undo_step_started)
        {
          gimp_image_undo_group_start (dialog->image_id);
          undo_step_started = TRUE;
        }
      parasite = gimp_parasite_new (PLUG_IN_PROC "/frame-rate",
                                    GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE,
                                    sizeof (dialog->settings.frame_rate),
                                    &dialog->settings.frame_rate);
      gimp_image_attach_parasite (dialog->image_id, parasite);
      gimp_parasite_free (parasite);
    }
  gimp_parasite_free (old_parasite);

  if (undo_step_started)
    {
      gimp_image_undo_group_end (dialog->image_id);
    }
}

static void
clean_exit (AnimationPlayDialog *dialog)
{
  save_settings (dialog);

  /* Frames are associated to an unused GimpImage.
   * Initializing frames with a NULL image allows to free this image,
   * otherwise we would leak GEGL buffers. */
  dialog->image_id = 0;
  init_frames (dialog);

  if (dialog->shape_window)
    gtk_widget_destroy (GTK_WIDGET (dialog->shape_window));

  g_signal_handlers_disconnect_by_func (dialog->window,
                                        G_CALLBACK (window_destroy),
                                        dialog);
  if (dialog->window)
    gtk_widget_destroy (GTK_WIDGET (dialog->window));

  if (dialog->frames)
    g_free (dialog->frames);

  g_free (dialog);

  gegl_exit ();
  gtk_main_quit ();
  gimp_quit ();
}

/******* TAG UTILS **************/

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
parse_disposal_tag (AnimationPlayDialog *dialog,
                    const gchar         *str)
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

  return dialog->settings.frame_disposal;
}

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
 * A recursive call which will call itself for layer groups
 * and update the min/max progressively.
 **/
static void
rec_set_total_frames (const gint32  layer,
                      gint         *min,
                      gint         *max)
{
  gchar        *layer_name;
  gchar        *nospace_name;
  GMatchInfo   *match_info;
  gint          i;

  if (gimp_item_is_group (layer))
    {
      gint    num_children;
      gint32 *children;

      children = gimp_item_get_children (layer, &num_children);
      for (i = 0; i < num_children; i++)
        rec_set_total_frames (children[i], min, max);

      return;
    }

  /* TODO: use parasites instead! */
  layer_name = gimp_item_get_name (layer);
  nospace_name = g_regex_replace_literal (nospace_reg, layer_name, -1, 0, "", 0, NULL);

  g_regex_match (layers_reg, nospace_name, 0, &match_info);

  while (g_match_info_matches (match_info))
    {
      gchar *tag = g_match_info_fetch (match_info, 1);
      gchar** tokens = g_strsplit(tag, ",", 0);

      for (i = 0; tokens[i] != NULL; i++)
        {
          gchar* hyphen;
          hyphen = g_strrstr(tokens[i], "-");
          if (hyphen != NULL)
            {
              gint32 first = (gint32) g_ascii_strtoll (tokens[i], NULL, 10);
              gint32 second = (gint32) g_ascii_strtoll (&hyphen[1], NULL, 10);
              *max = (second > first && second > *max)? second : *max;
              *min = (second > first && first < *min)? first : *min;
            }
          else
            {
              gint32 num = (gint32) g_ascii_strtoll (tokens[i], NULL, 10);
              *max = (num > *max)? num : *max;
              *min = (num < *min)? num : *min;
            }
        }
      g_strfreev (tokens);
      g_free (tag);
      g_free (layer_name);
      g_match_info_next (match_info, NULL);
    }

  g_free (nospace_name);
  g_match_info_free (match_info);
}

