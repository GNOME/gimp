/*
 * Animation Playback plug-in version 0.98.8
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 *
 * GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * TODO:
 *  pdb interface - should we bother?
 *
 *  speedups (caching?  most bottlenecks seem to be in pixelrgns)
 *    -> do pixelrgns properly!
 *
 *  write other half of the user interface (default timing, disposal &c)
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-animationplay"
#define PLUG_IN_BINARY "animationplay"
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
static gboolean    repaint_sda               (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);
static gboolean    repaint_da                (GtkWidget       *darea,
                                              GdkEventExpose  *event,
                                              gpointer         data);

static void        render_frame              (gint32           whichframe);
static void        show_frame                (void);
static void        total_alpha_preview       (guchar          *ptr);
static void        init_preview              (void);
static void        update_statusbar          (void);


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
static gint32             total_frames;
static guint              frame_number;
static gint32            *layers;
static GimpDrawable      *drawable;
static gboolean           playing = FALSE;
static guint              timer   = 0;
static GimpImageBaseType  imagetype;
static guchar            *palette;
static gint               ncolours;
static gdouble             duration_factor = 1.0;


/* for shaping */
typedef struct
{
  gint x, y;
} CursorOffset;

static gchar     *shape_preview_mask = NULL;
static GtkWidget *shape_window       = NULL;
static GdkWindow *root_win           = NULL;
static gboolean   detached           = FALSE;
static GtkWidget *statusbar          = NULL;
static guint      message_context_id = 0;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
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

  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

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

      shape_mask = gdk_bitmap_create_from_data (shape_window->window,
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
  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
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
      gdk_pointer_grab (widget->window, TRUE,
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_BUTTON_MOTION_MASK  |
                        GDK_POINTER_MOTION_HINT_MASK,
                        NULL, NULL, 0);
      gdk_window_raise (widget->window);
    }

  return FALSE;
}

static gboolean
maybeblocked_expose (GtkWidget      *widget,
                     GdkEventExpose *event)
{
  if (playing)
    return TRUE;

  return repaint_sda (widget, event, NULL);
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
  gdk_draw_rgb_image (drawing_area->window,
                      drawing_area->style->white_gc,
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
  gdk_draw_rgb_image (shape_drawing_area->window,
                      shape_drawing_area->style->white_gc,
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

      if (GTK_WIDGET_REALIZED (drawing_area))
        {
          gint x, y;

          gdk_window_get_origin (drawing_area->window, &x, &y);

          gtk_window_move (GTK_WINDOW (shape_window), x + 6, y + 6);
        }

      gtk_widget_show (shape_window);

      gdk_window_set_back_pixmap (shape_drawing_area->window, NULL, TRUE);

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
  GtkWidget   *vbox;
  GtkWidget   *abox;
  GtkToolItem *item;
  GtkAction   *action;
  GdkCursor   *cursor;
  gchar       *name;

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

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/anim-play-toolbar");
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  item =
    GTK_TOOL_ITEM (gtk_ui_manager_get_widget (ui_manager,
                                              "/anim-play-toolbar/space"));
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, TRUE);

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

  progress = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), progress, FALSE, FALSE, 0);
  gtk_widget_show (progress);

  statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
  gtk_widget_show (statusbar);
  update_statusbar ();

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

  gdk_window_set_back_pixmap (shape_window->window, NULL, FALSE);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (shape_window),
                                       GDK_HAND2);
  gdk_window_set_cursor (shape_window->window, cursor);
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
                    G_CALLBACK (maybeblocked_expose),
                    NULL);

  root_win = gdk_get_default_root_window ();
}

static void
do_playback (void)
{
  width     = gimp_image_width (image_id);
  height    = gimp_image_height (image_id);
  layers    = gimp_image_get_layers (image_id, &total_frames);
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

  /* cache hint "cache nothing", since we iterate over every
     tile in every layer. */
  gimp_tile_cache_size (0);

  init_preview ();

  build_dialog (gimp_image_base_type (image_id),
                gimp_image_get_name (image_id));

  /* Make sure that whole preview is dirtied with pure-alpha */
  total_alpha_preview (preview_data);

  render_frame (0);
  show_frame ();

  gtk_main ();
}


/* Rendering Functions */

static void
render_frame (gint32 whichframe)
{
  GimpPixelRgn   pixel_rgn;
  static guchar *rawframe = NULL;
  static gint    rawwidth = 0, rawheight = 0, rawbpp = 0;
  gint           rawx = 0, rawy = 0;
  guchar        *srcptr;
  guchar        *destptr;
  gint           i, j, k; /* imaginative loop variables */
  DisposeType    dispose;

  if (whichframe >= total_frames)
    {
      printf( "playback: Asked for frame number %d in a %d-frame animation!\n",
             (int) (whichframe+1), (int) total_frames);
      gimp_quit ();
    }

  drawable = gimp_drawable_get (layers[total_frames - (whichframe + 1)]);
  /* Lame attempt to catch the case that a user has closed the image. */
  if (!drawable)
    {
      gimp_message (_("Tried to display an invalid layer."));
      gtk_main_quit ();
      return;
    }

  dispose = get_frame_disposal (frame_number);

  /* Image has been closed/etc since we got the layer list? */
  /* FIXME - How do we tell if a gimp_drawable_get() fails? */
  if (gimp_drawable_width (drawable->drawable_id) == 0)
    gtk_widget_destroy (window);

  if (((dispose == DISPOSE_REPLACE) || (whichframe == 0)) &&
      gimp_drawable_has_alpha (drawable->drawable_id))
    {
      total_alpha_preview (preview_data);
    }


  /* only get a new 'raw' drawable-data buffer if this and
     the previous raw buffer were different sizes */

  if ((rawwidth * rawheight * rawbpp) !=
      ((gimp_drawable_width (drawable->drawable_id) *
        gimp_drawable_height (drawable->drawable_id) *
        gimp_drawable_bpp (drawable->drawable_id))))
    {
      if (rawframe != NULL)
        g_free (rawframe);

      rawframe = g_malloc ((gimp_drawable_width (drawable->drawable_id)) *
                           (gimp_drawable_height (drawable->drawable_id)) *
                           (gimp_drawable_bpp (drawable->drawable_id)));
    }

  rawwidth  = gimp_drawable_width (drawable->drawable_id);
  rawheight = gimp_drawable_height (drawable->drawable_id);
  rawbpp    = gimp_drawable_bpp (drawable->drawable_id);


  /* Initialise and fetch the whole raw new frame */

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height,
                       FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&pixel_rgn, rawframe,
                           0, 0, drawable->width, drawable->height);

  gimp_drawable_offsets (drawable->drawable_id, &rawx, &rawy);


  /* render... */

  switch (imagetype)
    {
    case GIMP_RGB:
      if ((rawwidth == width) &&
          (rawheight == height) &&
          (rawx == 0) &&
          (rawy == 0))
        {
          /* --- These cases are for the best cases,  in        --- */
          /* --- which this frame is the same size and position --- */
          /* --- as the preview buffer itself                   --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            {
              destptr = preview_data;
              srcptr  = rawframe;

              i = rawwidth * rawheight;
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
                  srcptr = rawframe + 3;

                  for (j = 0; j < rawheight; j++)
                    {
                      k = j * ((7 + rawwidth) / 8);

                      for (i = 0; i < rawwidth; i++)
                        {
                          if ((*srcptr) & 128)
                            shape_preview_mask[k + i/8] |= (1 << (i&7));

                          srcptr += 4;
                        }
                    }
                }
            }
          else /* no alpha */
            {
              if ((rawwidth == width) && (rawheight == height))
                memcpy (preview_data, rawframe, width * height * 3);

              if (detached)
                {
                  /* opacify the shape mask */
                  memset (shape_preview_mask, 255,
                          (rawwidth * rawheight) / 8 + rawheight);
                }
            }

          /* Display the preview buffer... finally. */
          if (detached)
            {
              reshape_from_bitmap (shape_preview_mask);
              gdk_draw_rgb_image (shape_drawing_area->window,
                                  shape_drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1 ?
                                   GDK_RGB_DITHER_MAX : DITHERTYPE),
                                  preview_data, width * 3);
            }
          else
            {
              reshape_from_bitmap (shape_preview_mask);
              gdk_draw_rgb_image (drawing_area->window,
                                  drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1 ?
                                   GDK_RGB_DITHER_MAX : DITHERTYPE),
                                  preview_data, width * 3);
            }
        }
      else
        {
          /* --- These are suboptimal catch-all cases for when  --- */
          /* --- this frame is bigger/smaller than the preview  --- */
          /* --- buffer, and/or offset within it.               --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            {
              srcptr = rawframe;

              for (j = rawy; j < rawheight + rawy; j++)
                {
                  for (i = rawx; i < rawwidth + rawx; i++)
                    {
                      if ((i >= 0 && i < width) &&
                          (j >= 0 && j < height))
                        {
                          if (srcptr[3] & 128)
                            {
                              preview_data[(j * width + i) * 3    ] = *(srcptr);
                              preview_data[(j * width + i) * 3 + 1] = *(srcptr + 1);
                              preview_data[(j * width + i) * 3 + 2] = *(srcptr + 2);
                            }
                        }

                      srcptr += 4;
                    }
                }

              if (detached)
                {
                  srcptr = rawframe + 3;

                  for (j = rawy; j < rawheight + rawy; j++)
                    {
                      k = j * ((width + 7) / 8);

                      for (i = rawx; i < rawwidth + rawx; i++)
                        {
                          if ((i>=0 && i<width) &&
                              (j>=0 && j<height))
                            {
                              if ((*srcptr) & 128)
                                shape_preview_mask[k + i/8] |= (1 << (i&7));
                            }

                          srcptr += 4;
                        }
                    }
                }
            }
          else
            {
              /* noalpha */

              srcptr = rawframe;

              for (j = rawy; j < rawheight + rawy; j++)
                {
                  for (i = rawx; i < rawwidth + rawx; i++)
                    {
                      if ((i >= 0 && i < width) &&
                          (j >= 0 && j < height))
                        {
                          preview_data[(j * width + i) * 3    ] = *(srcptr);
                          preview_data[(j * width + i) * 3 + 1] = *(srcptr + 1);
                          preview_data[(j * width + i) * 3 + 2] = *(srcptr + 2);
                        }

                      srcptr += 3;
                    }
                }
            }

          /* Display the preview buffer... finally. */
          if (detached)
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  gint top    = MAX (rawy, 0);
                  gint bottom = MIN (rawy + rawheight, height);

                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, top, width, bottom - top,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data, width * 3);
                }
            }
          else
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  gint top    = MAX (rawy, 0);
                  gint bottom = MIN (rawy + rawheight, height);

                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, top, width, bottom - top,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data, width * 3);
                }
            }
        }
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      if ((rawwidth == width) &&
          (rawheight == height) &&
          (rawx == 0) &&
          (rawy == 0))
        {
          /* --- These cases are for the best cases,  in        --- */
          /* --- which this frame is the same size and position --- */
          /* --- as the preview buffer itself                   --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            {
              destptr = preview_data;
              srcptr  = rawframe;

              i = rawwidth * rawheight;

              while (i--)
                {
                  if (! srcptr[1])
                    {
                      srcptr  += 2;
                      destptr += 3;
                      continue;
                    }

                  *(destptr++) = palette[0 + 3 * (*(srcptr))];
                  *(destptr++) = palette[1 + 3 * (*(srcptr))];
                  *(destptr++) = palette[2 + 3 * (*(srcptr))];

                  srcptr+=2;
                }

              /* calculate the shape mask */
              if (detached)
                {
                  srcptr = rawframe + 1;

                  for (j = 0; j < rawheight; j++)
                    {
                      k = j * ((7 + rawwidth) / 8);

                      for (i = 0; i < rawwidth; i++)
                        {
                          if (*srcptr)
                            shape_preview_mask[k + i/8] |= (1 << (i&7));

                          srcptr += 2;
                        }
                    }
                }
            }
          else /* no alpha */
            {
              destptr = preview_data;
              srcptr  = rawframe;

              i = rawwidth * rawheight;

              while (--i)
                {
                  *(destptr++) = palette[0 + 3 * (*(srcptr))];
                  *(destptr++) = palette[1 + 3 * (*(srcptr))];
                  *(destptr++) = palette[2 + 3 * (*(srcptr))];

                  srcptr++;
                }

              if (detached)
                {
                  /* opacify the shape mask */
                  memset (shape_preview_mask, 255,
                          (rawwidth * rawheight) / 8 + rawheight);
                }
            }

          /* Display the preview buffer... finally. */
          if (detached)
            {
              reshape_from_bitmap (shape_preview_mask);
              gdk_draw_rgb_image (shape_drawing_area->window,
                                  shape_drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1 ?
                                   GDK_RGB_DITHER_MAX : DITHERTYPE),
                                  preview_data, width * 3);
            }
          else
            {
              gdk_draw_rgb_image (drawing_area->window,
                                  drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1 ?
                                   GDK_RGB_DITHER_MAX : DITHERTYPE),
                                  preview_data, width * 3);
            }
        }
      else
        {
          /* --- These are suboptimal catch-all cases for when  --- */
          /* --- this frame is bigger/smaller than the preview  --- */
          /* --- buffer, and/or offset within it.               --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            {
              srcptr = rawframe;

              for (j = rawy; j < rawheight + rawy; j++)
                {
                  for (i = rawx; i < rawwidth + rawx; i++)
                    {
                      if ((i >= 0 && i < width) &&
                          (j >= 0 && j < height))
                        {
                          if (*(srcptr+1))
                            {
                              preview_data[(j * width + i) * 3 + 0] =
                                palette[0 + 3 * (*(srcptr))];
                              preview_data[(j * width + i) * 3 + 1] =
                                palette[1 + 3 * (*(srcptr))];
                              preview_data[(j * width + i) * 3 + 2] =
                                palette[2 + 3 * (*(srcptr))];
                            }
                        }

                      srcptr += 2;
                    }
                }

              if (detached)
                {
                  srcptr = rawframe + 1;

                  for (j=rawy; j<rawheight+rawy; j++)
                    {
                      k = j * ((width + 7) / 8);

                      for (i = rawx; i < rawwidth + rawx; i++)
                        {
                          if ((i >= 0 && i < width) &&
                              (j >= 0 && j < height))
                            {
                              if (*srcptr)
                                shape_preview_mask[k + i/8] |= (1 << (i&7));
                            }

                          srcptr += 2;
                        }
                    }
                }
            }
          else
            {
              /* noalpha */
              srcptr = rawframe;

              for (j = rawy; j < rawheight + rawy; j++)
                {
                  for (i = rawx; i < rawwidth + rawx; i++)
                    {
                      if ((i >= 0 && i < width) &&
                          (j >= 0 && j < height))
                        {
                          preview_data[(j * width + i) * 3 + 0] =
                            palette[0 + 3 * (*(srcptr))];
                          preview_data[(j * width + i) * 3 + 1] =
                            palette[1 + 3 * (*(srcptr))];
                          preview_data[(j * width + i) * 3 + 2] =
                            palette[2 + 3 * (*(srcptr))];
                        }

                      srcptr ++;
                    }
                }
            }

          /* Display the preview buffer... finally. */
          if (detached)
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  gint top    = MAX (rawy, 0);
                  gint bottom = MIN (rawy + rawheight, height);

                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, top, width, bottom - top,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data, width * 3);
                }
            }
          else
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  gint top    = MAX (rawy, 0);
                  gint bottom = MIN (rawy + rawheight, height);

                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, top, width, bottom - top,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1 ?
                                       GDK_RGB_DITHER_MAX : DITHERTYPE),
                                      preview_data, width * 3);
                }
            }
        }
      break;

    }

  /* clean up */
  gimp_drawable_detach (drawable);
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
  remove_timer();

  timer = g_timeout_add (get_frame_duration ((frame_number + 1) % total_frames) * duration_factor,
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
    timer = g_timeout_add (get_frame_duration (frame_number) * duration_factor,
                           advance_frame_callback, NULL);

  g_object_set (action,
		"tooltip", playing ? _("Stop playback") : _("Start playback"),
		NULL);
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
  if (duration_factor > 0.125)
    duration_factor /= 2.0;

  gtk_action_set_sensitive (action, duration_factor > 0.125);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, duration_factor != 1.0);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, TRUE);

  update_statusbar ();
}

static void
speed_down_callback (GtkAction *action)
{
  if (duration_factor < 8.0)
    duration_factor *= 2.0;

  gtk_action_set_sensitive (action, duration_factor < 8.0);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-reset");
  gtk_action_set_sensitive (action, duration_factor != 1.0);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, TRUE);

  update_statusbar ();
}

static void
speed_reset_callback (GtkAction *action)
{
  duration_factor = 1.0;

  gtk_action_set_sensitive (action, FALSE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-down");
  gtk_action_set_sensitive (action, TRUE);

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/anim-play-popup/speed-up");
  gtk_action_set_sensitive (action, TRUE);

  update_statusbar ();
}

static void
update_statusbar (void)
{
  gchar *status_message;

  if (message_context_id == 0)
    {
      message_context_id =
        gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar),
                                      "speed-playback");
    }
  else
    {
      gtk_statusbar_pop (GTK_STATUSBAR (statusbar),
                         message_context_id);
    }

  status_message = g_strdup_printf  (_("Playback speed: %d %%"),
                                     (gint) ROUND (100.0 / duration_factor));

  gtk_statusbar_push (GTK_STATUSBAR (statusbar),
                      message_context_id, status_message);

  g_free (status_message);
}

/* tag util. */

static DisposeType
get_frame_disposal (guint whichframe)
{
  DisposeType  disposal;
  gchar       *layer_name;

  layer_name = gimp_drawable_get_name (layers[total_frames-(whichframe+1)]);
  disposal = parse_disposal_tag (layer_name);
  g_free (layer_name);

  return disposal;
}

static guint32
get_frame_duration (guint whichframe)
{
  gchar *layer_name;
  gint   duration = 0;

  layer_name = gimp_drawable_get_name (layers[total_frames-(whichframe+1)]);
  if (layer_name)
    {
      duration = parse_ms_tag (layer_name);
      g_free (layer_name);
    }

  if (duration < 0) duration = 100;  /* FIXME for default-if-not-said  */
  else
    if (duration == 0) duration = 100; /* FIXME - 0-wait is nasty */

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
