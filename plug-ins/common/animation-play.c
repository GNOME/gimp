/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 * (c) Mircea Purdea : 2009 : someone_else@exhalus.net
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
 *
 *  write other half of the user interface (zoom, disposal &c)
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
  DISPOSE_UNDEFINED = 0x00,
  DISPOSE_COMBINE   = 0x01,
  DISPOSE_REPLACE   = 0x02
} DisposeType;


/* Declare local functions. */
static void        query (void);
static void        run   (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static void        do_playback               (void);

static void        window_destroy            (GtkWidget       *widget);
static void        play_callback             (GtkToggleAction *action);
static void        step_callback             (GtkAction       *action);
static void        rewind_callback           (GtkAction       *action);
static void        speed_up_callback         (GtkAction       *action);
static void        speed_down_callback       (GtkAction       *action);
static void        speed_reset_callback      (GtkAction       *action);
static void        speedcombo_changed        (GtkWidget       *combo,
                                              gpointer         data);
static void        fpscombo_changed          (GtkWidget       *combo,
                                              gpointer         data);
static gboolean    repaint_sda               (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);
static gboolean    repaint_da                (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);

static void        init_frames               (void);
static void        generate_frames           (void);
static void        render_frame              (gint32           whichframe);
static void        show_frame                (void);
static void        total_alpha_preview       (guchar          *ptr);
static void        init_preview              (void);
static void        update_combobox           (void);
static gdouble     get_duration_factor       (gint             index);
static gint        get_fps                   (gint             index);


/* tag util functions*/
static gint        parse_ms_tag              (const gchar     *str);
static DisposeType parse_disposal_tag        (const gchar     *str);
static DisposeType get_frame_disposal        (guint            whichframe);
static guint32     get_frame_duration        (guint            whichframe);
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
static GtkWidget         *window                  = NULL;
static GtkUIManager      *ui_manager              = NULL;
static guchar            *preview_data            = NULL;
static GtkWidget         *drawing_area            = NULL;
static GtkWidget         *shape_drawing_area      = NULL;
static guchar            *shape_drawing_area_data = NULL;
static guchar            *drawing_area_data       = NULL;
static GtkWidget         *progress;
static guint              width, height;
static guchar            *preview_alpha1_data;
static guchar            *preview_alpha2_data;
static gint32             image_id;
static guchar            *rawframe                = NULL;
static gint32            *frames                  = NULL;
static gint32             total_frames;
static guint              frame_number;
static gint32            *layers;
static gint32             total_layers;
static gboolean           playing = FALSE;
static guint              timer   = 0;
static GimpImageBaseType  imagetype;
static guchar            *palette;
static gint               ncolours;
static gint               duration_index = 3;
static gint               default_frame_duration = 100; /* ms */


/* for shaping */
typedef struct
{
  gint x, y;
} CursorOffset;

static gchar     *shape_preview_mask = NULL;
static GtkWidget *shape_window       = NULL;
static GdkWindow *root_win           = NULL;
static gboolean   detached           = FALSE;
static GtkWidget *speedcombo         = NULL;

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
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GTK_STOCK_MEDIA_PLAY);
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
      image_id = param[1].data.d_image;

      do_playback ();

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
reshape_from_bitmap (const gchar *bitmap)
{
  static gchar *prev_bitmap = NULL;

  if ((!prev_bitmap) ||
      (memcmp (prev_bitmap, bitmap, (width * height) / 8 + height)))
    {
      GdkBitmap *shape_mask;

      shape_mask = gdk_bitmap_create_from_data (gtk_widget_get_window (shape_window),
                                                bitmap,
                                                width, height);
      gtk_widget_shape_combine_mask (shape_window, shape_mask, 0, 0);
      g_object_unref (shape_mask);

      if (!prev_bitmap)
        prev_bitmap = g_malloc ((width * height) / 8 + height);

      memcpy (prev_bitmap, bitmap, (width * height) / 8 + height);
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
                      0, 0, width, height,
                      (total_frames == 1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      drawing_area_data, width * 3);

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
                      0, 0, width, height,
                      (total_frames == 1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      shape_drawing_area_data, width * 3);

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
      /* Create a total-alpha buffer merely for the not-shaped
         drawing area to now display. */

      drawing_area_data = g_malloc (width * height * 3);
      total_alpha_preview (drawing_area_data);

      gtk_window_set_screen (GTK_WINDOW (shape_window),
                             gtk_widget_get_screen (drawing_area));

      if (gtk_widget_get_realized (drawing_area))
        {
          gint x, y;

          gdk_window_get_origin (gtk_widget_get_window (drawing_area), &x, &y);

          gtk_window_move (GTK_WINDOW (shape_window), x + 6, y + 6);
        }

      gtk_widget_show (shape_window);

      gdk_window_set_back_pixmap (gtk_widget_get_window (shape_drawing_area), NULL, TRUE);

      memset (shape_preview_mask, 0, (width * height) / 8 + height);
      render_frame (frame_number);
    }
  else
    {
      g_free (drawing_area_data);
      drawing_area_data = shape_drawing_area_data;

      render_frame (frame_number);

      gtk_widget_hide (shape_window);
    }

  gtk_widget_queue_draw (drawing_area);
}

static GtkUIManager *
ui_manager_new (GtkWidget *window)
{
  static GtkActionEntry actions[] =
  {
    { "step", GTK_STOCK_MEDIA_NEXT,
      N_("_Step"), NULL, N_("Step to next frame"),
      G_CALLBACK (step_callback) },

    { "rewind", GTK_STOCK_MEDIA_REWIND,
      NULL, NULL, N_("Rewind the animation"),
      G_CALLBACK (rewind_callback) },

    { "help", GTK_STOCK_HELP,
      NULL, NULL, NULL,
      G_CALLBACK (help_callback) },

    { "close", GTK_STOCK_CLOSE,
      NULL, "<control>W", NULL,
      G_CALLBACK (close_callback)
    },
    {
      "quit", GTK_STOCK_QUIT,
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
    { "play", GTK_STOCK_MEDIA_PLAY,
      NULL, NULL, N_("Start playback"),
      G_CALLBACK (play_callback), FALSE },

    { "detach", GIMP_STOCK_DETACH,
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
                                     "    <toolitem action=\"step\" />"
                                     "    <toolitem action=\"rewind\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"detach\" />"
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
                                     "    <menuitem action=\"step\" />"
                                     "    <menuitem action=\"rewind\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"speed-down\" />"
                                     "    <menuitem action=\"speed-up\" />"
                                     "    <menuitem action=\"speed-reset\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"detach\" />"
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
build_dialog (GimpImageBaseType  basetype,
              gchar             *imagename)
{
  GtkWidget   *toolbar;
  GtkWidget   *frame;
  GtkWidget   *main_vbox;
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *abox;
  GtkToolItem *item;
  GtkAction   *action;
  GdkCursor   *cursor;
  gchar       *name;
  gint         index;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  name = g_strconcat (_("Animation Playback:"), " ", imagename, NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), name);
  gtk_window_set_role (GTK_WINDOW (window), "animation-playback");

  g_free (name);

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
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, width, height);
  gtk_widget_add_events (drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (frame), drawing_area);
  gtk_widget_show (drawing_area);

  g_signal_connect (drawing_area, "button-press-event",
                    G_CALLBACK (button_press),
                    NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (hbox), progress, TRUE, TRUE, 0);
  gtk_widget_show (progress);

  speedcombo = gtk_combo_box_text_new ();
  gtk_box_pack_end (GTK_BOX (hbox), speedcombo, FALSE, FALSE, 0);
  gtk_widget_show (speedcombo);

  for (index = 0; index < 9; index++)
    {
      gchar *text;

      /* list is given in "fps" - frames per second */
      text = g_strdup_printf  (_("%d fps"), get_fps (index));
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (speedcombo), text);
      g_free (text);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (speedcombo), 0);

  g_signal_connect (speedcombo, "changed",
                    G_CALLBACK (fpscombo_changed),
                    NULL);

  gimp_help_set_help_data (speedcombo, _("Default framerate"), NULL);

  speedcombo = gtk_combo_box_text_new ();
  gtk_box_pack_end (GTK_BOX (hbox), speedcombo, FALSE, FALSE, 0);
  gtk_widget_show (speedcombo);

  for (index = 0; index < 7; index++)
    {
      gchar *text;

      text = g_strdup_printf  ("%g\303\227", (100 / get_duration_factor (index)) / 100);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (speedcombo), text);
      g_free (text);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (speedcombo), 3);

  g_signal_connect (speedcombo, "changed",
                    G_CALLBACK (speedcombo_changed),
                    NULL);

  gimp_help_set_help_data (speedcombo, _("Playback speed"), NULL);

  if (total_frames < 2)
    {
      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/play");
      gtk_action_set_sensitive (action, FALSE);

      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/step");
      gtk_action_set_sensitive (action, FALSE);

      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/rewind");
      gtk_action_set_sensitive (action, FALSE);
    }

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, FALSE);

  gtk_widget_show (window);

  /* let's get into shape. */
  shape_window = gtk_window_new (GTK_WINDOW_POPUP);

  shape_drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (shape_drawing_area, width, height);
  gtk_container_add (GTK_CONTAINER (shape_window), shape_drawing_area);
  gtk_widget_show (shape_drawing_area);
  gtk_widget_add_events (shape_drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_widget_realize (shape_window);

  gdk_window_set_back_pixmap (gtk_widget_get_window (shape_window), NULL, FALSE);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (shape_window),
                                       GDK_HAND2);
  gdk_window_set_cursor (gtk_widget_get_window (shape_window), cursor);
  gdk_cursor_unref (cursor);

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

  root_win = gdk_get_default_root_window ();
}

static void
do_playback (void)
{
  width     = gimp_image_width (image_id);
  height    = gimp_image_height (image_id);
  layers    = gimp_image_get_layers (image_id, &total_layers);
  imagetype = gimp_image_base_type (image_id);

  if (imagetype == GIMP_INDEXED)
    {
      palette = gimp_image_get_colormap (image_id, &ncolours);
    }
  else if (imagetype == GIMP_GRAY)
    {
      gint i;

      palette = g_new (guchar, 768);

      for (i = 0; i < 256; i++)
        palette[i * 3] = palette[i * 3 + 1] = palette[i * 3 + 2] = i;

      ncolours = 256;
    }

  frame_number = 0;

  init_preview ();

  build_dialog (gimp_image_base_type (image_id),
                gimp_image_get_name (image_id));

  init_frames ();

  render_frame (0);
  show_frame ();

  gtk_main ();
}


/* Rendering Functions */

static void
render_frame (gint32 whichframe)
{
  GtkStyle      *shape_style   = gtk_widget_get_style (shape_drawing_area);
  GtkStyle      *drawing_style = gtk_widget_get_style (drawing_area);
  GeglBuffer    *buffer;
  gint           drawable_id;
  gint i, j, k;
  guchar        *srcptr;
  guchar        *destptr;

  if (whichframe >= total_frames)
    {
      printf( "playback: Asked for frame number %d in a %d-frame animation!\n",
             (int) (whichframe+1), (int) total_frames);
      gimp_quit ();
    }

  drawable_id = frames[whichframe];
  buffer = gimp_drawable_get_buffer (drawable_id);

  /* Lame attempt to catch the case that a user has closed the image. */
  if (!buffer)
    {
      gimp_message (_("Tried to display an invalid layer."));
      gtk_main_quit ();
      return;
    }

  /* Initialize the rawframe only once. */
  if (rawframe == NULL)
    rawframe = g_malloc (width * height * 4);

  /* Initialise and fetch the whole raw new frame */
  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   babl_format ("R'G'B'A u8"),
                   rawframe, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /* render... */
  total_alpha_preview (preview_data);

  i = width * height;
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
      memset (shape_preview_mask, 0, (width * height) / 8 + height);
      srcptr = rawframe + 3;

      for (j = 0; j < height; j++)
        {
          k = j * ((7 + width) / 8);

          for (i = 0; i < width; i++)
            {
              if ((*srcptr) & 128)
                shape_preview_mask[k + i/8] |= (1 << (i&7));

              srcptr += 4;
            }
        }
      reshape_from_bitmap (shape_preview_mask);
    }

  /* Display the preview buffer. */
  gdk_draw_rgb_image (gtk_widget_get_window (detached? shape_drawing_area : drawing_area),
                      detached? shape_style->white_gc : drawing_style->white_gc,
                      0, 0, width, height,
                      (total_frames == 1 ?
                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                      preview_data, width * 3);

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
init_preview (void)
{
  gint i;

  preview_data = g_malloc (width * height * 3);
  shape_preview_mask = g_malloc ((width * height) / 8 + 1 + height);
  preview_alpha1_data = g_malloc (width * 3);
  preview_alpha2_data = g_malloc (width * 3);

  for (i = 0; i < width; i++)
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

  drawing_area_data = preview_data;
  shape_drawing_area_data = preview_data;
}

static void
total_alpha_preview (guchar *ptr)
{
  gint i;

  for (i = 0; i < height; i++)
    {
      if (i & 8)
        memcpy (&ptr[i * 3 * width], preview_alpha1_data, 3 * width);
      else
        memcpy (&ptr[i * 3 * width], preview_alpha2_data, 3 * width);
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

  duration = get_frame_duration ((frame_number + 1) % total_frames);

  timer = g_timeout_add (duration * get_duration_factor (duration_index),
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
      timer = g_timeout_add (get_frame_duration (frame_number) *
                             get_duration_factor (duration_index),
                             advance_frame_callback, NULL);

      gtk_action_set_stock_id (GTK_ACTION (action), GTK_STOCK_MEDIA_PAUSE);
    }
  else
    {
      gtk_action_set_stock_id (GTK_ACTION (action), GTK_STOCK_MEDIA_PLAY);
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
  if (duration_index > 0)
    --duration_index;

  gtk_action_set_sensitive (action, duration_index > 0);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, duration_index != 3);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, TRUE);

  update_combobox ();
}

static void
speed_down_callback (GtkAction *action)
{
  if (duration_index < 6)
    ++duration_index;

  gtk_action_set_sensitive (action, duration_index < 6);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, duration_index != 3);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, TRUE);

  update_combobox ();
}

static void
speed_reset_callback (GtkAction *action)
{
  duration_index = 3;

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
speedcombo_changed (GtkWidget *combo, gpointer data)
{
  GtkAction * action;

  duration_index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, duration_index != 3);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, duration_index < 6);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, duration_index > 0);
}

static void
fpscombo_changed (GtkWidget *combo, gpointer data)
{
  default_frame_duration = 1000 / get_fps(gtk_combo_box_get_active (GTK_COMBO_BOX (combo)));
}

static void
update_combobox (void)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (speedcombo), duration_index);
}

/* tag util. */

static DisposeType
get_frame_disposal (guint whichframe)
{
  DisposeType  disposal;
  gchar       *layer_name;

  layer_name = gimp_item_get_name (layers[total_frames-(whichframe+1)]);
  disposal = parse_disposal_tag (layer_name);
  g_free (layer_name);

  return disposal;
}

static guint32
get_frame_duration (guint whichframe)
{
  gchar *layer_name;
  gint   duration = 0;

  layer_name = gimp_item_get_name (layers[total_frames-(whichframe+1)]);
  if (layer_name)
    {
      duration = parse_ms_tag (layer_name);
      g_free (layer_name);
    }

  if (duration <= 0)
    duration = default_frame_duration;

  return (guint32) duration;
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

  return DISPOSE_UNDEFINED; /* FIXME */
}

static void
init_frames (void)
{
  GtkAction   *action;
  total_frames = total_layers;
  generate_frames();
  if (total_frames < 2)
    {
      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/play");
      gtk_action_set_sensitive (action, FALSE);

      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/step");
      gtk_action_set_sensitive (action, FALSE);

      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/rewind");
      gtk_action_set_sensitive (action, FALSE);
    }
  else
    {
      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/play");
      gtk_action_set_sensitive (action, TRUE);

      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/step");
      gtk_action_set_sensitive (action, TRUE);

      action = gtk_ui_manager_get_action (ui_manager,
                                          "/ui/anim-play-toolbar/rewind");
      gtk_action_set_sensitive (action, TRUE);
    }
}

static void
generate_frames (void)
{
  /* Frames are associated to an unused image. */
  static gint32 frames_image_id;
  gint          i;
  gint32        new_frame, previous_frame, new_layer;

  /* Cleanup before re-generation. */
  if (frames != NULL)
    {
      for (i = 0; i < total_frames; i++)
        gimp_image_remove_layer (frames_image_id, frames[i]);
      gimp_image_delete (frames_image_id);
      g_free (frames);
    }
  frames = g_try_malloc0_n (total_frames, sizeof (gint32));

  if (frames == NULL)
    {
      gimp_message (_("Memory could not be allocated to the frame container."));
      gtk_main_quit ();
      return;
    }
  frames_image_id = gimp_image_new (width, height, imagetype);

  for (i = 0; i < total_frames; i++)
    {
      if (i > 0 && get_frame_disposal (i) != DISPOSE_REPLACE)
        {
          previous_frame = gimp_layer_copy (frames[i - 1]);
          gimp_image_insert_layer (frames_image_id, previous_frame, 0, -1);
          gimp_item_set_visible (previous_frame, TRUE);
        }
      new_layer = gimp_layer_new_from_drawable (layers[total_layers - (i + 1)], frames_image_id);
      gimp_item_set_visible (new_layer, TRUE);
      gimp_image_insert_layer (frames_image_id, new_layer, 0, -1);
      new_frame = gimp_image_merge_visible_layers (frames_image_id, GIMP_CLIP_TO_IMAGE);
      frames[i] = new_frame;
      gimp_item_set_visible (new_frame, FALSE);
    }
}
