/*
 * Animation Playback plug-in version 0.98.8
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 *
 * The GIMP -- an image manipulation program
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
 * BUGS:
 *  Gets understandably upset if the source image is deleted
 *    while the animation is playing.  Decent solution welcome.
 *
 *  In shaped mode, the shaped-window's mask and its pixmap contents
 *    can get way out of sync (specifically, the mask changes but
 *    the contents are frozen).  Starvation of GTK's redrawing thread?
 *    How do I fix this?
 *
 *  Any more?  Let me know!
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
static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void do_playback                (void);

static void window_response            (GtkWidget      *widget,
                                        gint            response_id,
                                        gpointer        data);
static void play_callback              (GtkAction      *action,
                                        gpointer        data);
static void step_callback              (GtkAction      *action,
                                        gpointer        data);
static void rewind_callback            (GtkAction      *action,
                                        gpointer        data);
static gboolean repaint_sda            (GtkWidget      *darea,
                                        GdkEventExpose *event,
                                        gpointer        data);
static gboolean repaint_da             (GtkWidget      *darea,
                                        GdkEventExpose *event,
                                        gpointer        data);

static void        render_frame        (gint32 whichframe);
static void        show_frame          (void);
static void        total_alpha_preview (guchar* ptr);
static void        init_preview_misc   (void);


/* tag util functions*/
static int         parse_ms_tag        (const char  *str);
static DisposeType parse_disposal_tag  (const char  *str);
static DisposeType get_frame_disposal  (guint        whichframe);
static guint32     get_frame_duration  (guint        whichframe);
static gboolean    is_disposal_tag     (const char  *str,
                                        DisposeType *disposal,
                                        int         *taglength);
static gboolean    is_ms_tag           (const char  *str,
                                        int         *duration,
                                        int         *taglength);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/* Global widgets'n'stuff */
static GtkWidget         *dlg          = NULL;
static GtkUIManager      *ui_manager   = NULL;
static guchar            *preview_data = NULL;
static GtkWidget         *drawing_area = NULL;
static GtkWidget         *shape_drawing_area = NULL;
static guchar            *shape_drawing_area_data = NULL;
static guchar            *drawing_area_data = NULL;
static GtkProgressBar    *progress;
static guint              width, height;
static guchar            *preview_alpha1_data;
static guchar            *preview_alpha2_data;
static gint32             image_id;
static gint32             total_frames;
static guint              frame_number;
static gint32            *layers;
static GimpDrawable      *drawable;
static gboolean           playing = FALSE;
static guint              timer = 0;
static GimpImageBaseType  imagetype;
static guchar            *palette;
static gint               ncolours;

/* for shaping */
typedef struct
{
  gint x, y;
} CursorOffset;

static gchar     *shape_preview_mask;
static GtkWidget *shape_window;
static gint       shaping = 0;
static GdkWindow *root_win = NULL;


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"      }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          "This plugin allows you to preview a GIMP "
                          "layer-based animation.",
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
reshape_from_bitmap (gchar* bitmap)
{
  GdkBitmap *shape_mask;
  static gchar *prev_bitmap = NULL;

  if ((!prev_bitmap) ||
      (memcmp(prev_bitmap, bitmap, (width*height)/8 +height)))
    {
      shape_mask = gdk_bitmap_create_from_data (shape_window->window,
                                                bitmap,
                                                width, height);
      gtk_widget_shape_combine_mask (shape_window, shape_mask, 0, 0);
      g_object_unref (shape_mask);

      if (!prev_bitmap)
        {
          prev_bitmap = g_malloc ((width * height) / 8 + height);
        }
      memcpy (prev_bitmap, bitmap, (width * height) / 8 + height);
    }
}

static gboolean
shape_pressed (GtkWidget      *widget,
               GdkEventButton *event)
{
  CursorOffset *p;

  /* ignore double and triple click */
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  p = g_object_get_data (G_OBJECT(widget), "cursor-offset");
  if (!p)
    return FALSE;

  p->x = (int) event->x;
  p->y = (int) event->y;

  gtk_grab_add (widget);
  gdk_pointer_grab (widget->window, TRUE,
                    GDK_BUTTON_RELEASE_MASK |
                    GDK_BUTTON_MOTION_MASK |
                    GDK_POINTER_MOTION_HINT_MASK,
                    NULL, NULL, 0);
  gdk_window_raise (widget->window);

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
  gint             xp, yp;
  CursorOffset    *p;
  GdkModifierType  mask;

  gdk_window_get_pointer (root_win, &xp, &yp, &mask);

  /* if a button is still held by the time we process this event... */
  if (mask & (GDK_BUTTON1_MASK|
              GDK_BUTTON2_MASK|
              GDK_BUTTON3_MASK|
              GDK_BUTTON4_MASK|
              GDK_BUTTON5_MASK))
    {
      p = g_object_get_data (G_OBJECT (widget), "cursor-offset");
      if (!p)
        return FALSE;

      gtk_window_move (GTK_WINDOW (widget), xp  - p->x, yp  - p->y);
    }
  else /* the user has released all buttons */
    {
      shape_released(widget);
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
                      (total_frames==1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
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
                      (total_frames==1) ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                      shape_drawing_area_data, width * 3);

  return TRUE;
}

static gboolean
preview_pressed (GtkWidget      *widget,
                 GdkEventButton *event)
{
  gint            xp, yp;
  GdkModifierType mask;

  if (shaping)
    return FALSE;

  /* Create a total-alpha buffer merely for the not-shaped
     drawing area to now display. */

  drawing_area_data = g_malloc (width * height * 3);
  total_alpha_preview (drawing_area_data);

  gdk_window_get_pointer (root_win, &xp, &yp, &mask);
  gtk_window_move (GTK_WINDOW (shape_window), xp  - event->x, yp  - event->y);

  gtk_widget_show (shape_window);

  gdk_window_set_back_pixmap (shape_window->window, NULL, 0);
  gdk_window_set_back_pixmap (shape_drawing_area->window, NULL, 1);

  show_frame ();

  shaping = 1;
  memset (shape_preview_mask, 0, (width * height) / 8 + height);
  render_frame (frame_number);
  show_frame ();

  repaint_da (NULL, NULL, NULL);

  /* mildly amusing hack */
  return shape_pressed (shape_window, event);
}

static GtkUIManager *
ui_manager_new (GtkWidget *window)
{
  static GtkActionEntry actions[] =
  {
    { "play", GTK_STOCK_MEDIA_PLAY,
      NULL, NULL, N_("Start/Stop playback"),
      G_CALLBACK (play_callback) },

    { "step", GTK_STOCK_MEDIA_NEXT,
      N_("_Step"), NULL, N_("Step to next frame"),
      G_CALLBACK (step_callback) },

    { "rewind", GTK_STOCK_MEDIA_REWIND,
      NULL, NULL, N_("Rewind animation"),
      G_CALLBACK (rewind_callback) }
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GtkActionGroup *group      = gtk_action_group_new ("Actions");
  GError         *error      = NULL;

  gtk_action_group_set_translation_domain (group, NULL);
  gtk_action_group_add_actions (group, actions, G_N_ELEMENTS (actions), NULL);

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
                                     "  </toolbar>"
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
  CursorOffset *icon_pos;
  GtkWidget    *toolbar;
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *abox;
  GtkWidget    *eventbox;
  GdkCursor    *cursor;
  gchar        *name;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  name = g_strconcat (_("Animation Playback:"), " ", imagename, NULL);

  dlg = gimp_dialog_new (name, PLUG_IN_BINARY,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                         NULL);

  g_free (name);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  g_signal_connect (dlg, "response",
                    G_CALLBACK (window_response),
                    NULL);

  ui_manager = ui_manager_new (dlg);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/anim-play-toolbar");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), toolbar,
                      FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, TRUE, TRUE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  eventbox = gtk_event_box_new();
  gtk_widget_add_events (eventbox, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (eventbox));
  gtk_widget_show (eventbox);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, width, height);
  gtk_container_add (GTK_CONTAINER (eventbox), GTK_WIDGET (drawing_area));
  gtk_widget_show (drawing_area);

  progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (progress), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (progress));

  if (total_frames < 2)
    {
      GtkAction *action;

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

  gtk_widget_show (dlg);

  /* let's get into shape. */
  shape_window = gtk_window_new (GTK_WINDOW_POPUP);

  shape_drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (shape_drawing_area, width, height);
  gtk_container_add (GTK_CONTAINER (shape_window), shape_drawing_area);
  gtk_widget_show (shape_drawing_area);
  gtk_widget_add_events (shape_drawing_area, GDK_BUTTON_PRESS_MASK);
  gtk_widget_realize (shape_window);

  gdk_window_set_back_pixmap (shape_window->window, NULL, 0);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (shape_window),
                                       GDK_CENTER_PTR);
  gdk_window_set_cursor(shape_window->window, cursor);
  gdk_cursor_unref (cursor);

  g_signal_connect (shape_window, "button-press-event",
                    G_CALLBACK (shape_pressed),NULL);
  g_signal_connect (shape_window, "button-release-event",
                    G_CALLBACK (shape_released),NULL);
  g_signal_connect (shape_window, "motion-notify-event",
                    G_CALLBACK (shape_motion),NULL);

  icon_pos = g_new (CursorOffset, 1);
  g_object_set_data (G_OBJECT (shape_window), "cursor-offset", icon_pos);

  /*  gtk_widget_show (shape_window);*/

  g_signal_connect (eventbox, "button-press-event",
                    G_CALLBACK (preview_pressed), NULL);

  g_signal_connect (drawing_area, "expose-event",
                    G_CALLBACK (repaint_da), drawing_area);

  g_signal_connect (shape_drawing_area, "expose-event",
                    G_CALLBACK (maybeblocked_expose), shape_drawing_area);

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

      palette = g_malloc(768);

      for (i = 0; i < 256; i++)
        palette[i*3] = palette[i*3+1] = palette[i*3+2] = i;

      ncolours = 256;
    }

  frame_number = 0;

  /* cache hint "cache nothing", since we iterate over every
     tile in every layer. */
  gimp_tile_cache_size (0);

  init_preview_misc ();

  build_dialog (gimp_image_base_type (image_id),
                gimp_image_get_name (image_id));

  /* Make sure that whole preview is dirtied with pure-alpha */
  total_alpha_preview (preview_data);

  render_frame (0);
  show_frame ();

  gtk_main ();
  gdk_flush ();
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
  if (! (drawable->width > 0 && drawable->height > 0))
    {
      gtk_main_quit ();
      return;
    }

  dispose = get_frame_disposal (frame_number);

  /* Image has been closed/etc since we got the layer list? */
  /* FIXME - How do we tell if a gimp_drawable_get() fails? */
  if (gimp_drawable_width (drawable->drawable_id) == 0)
    gtk_dialog_response (GTK_DIALOG (dlg), GTK_RESPONSE_CLOSE);

  if (((dispose==DISPOSE_REPLACE) || (whichframe==0)) &&
      gimp_drawable_has_alpha (drawable->drawable_id))
    {
      total_alpha_preview (preview_data);
    }


  /* only get a new 'raw' drawable-data buffer if this and
     the previous raw buffer were different sizes*/

  if ((rawwidth * rawheight * rawbpp)
      !=
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
            { /* alpha */
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
              if (shaping)
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
                {
                  memcpy (preview_data, rawframe, width * height * 3);
                }

              if (shaping)
                {
                  /* opacify the shape mask */
                  memset (shape_preview_mask, 255,
                          (rawwidth * rawheight) / 8 + rawheight);
                }
            }
          /* Display the preview buffer... finally. */
          if (shaping)
            {
              reshape_from_bitmap (shape_preview_mask);
              gdk_draw_rgb_image (shape_drawing_area->window,
                                  shape_drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1) ?
                                  GDK_RGB_DITHER_MAX : DITHERTYPE,
                                  preview_data, width * 3);
            }
          else
            {
              reshape_from_bitmap (shape_preview_mask);
              gdk_draw_rgb_image (drawing_area->window,
                                  drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1) ?
                                  GDK_RGB_DITHER_MAX : DITHERTYPE,
                                  preview_data, width * 3);
            }
        }
      else
        {
          /* --- These are suboptimal catch-all cases for when  --- */
          /* --- this frame is bigger/smaller than the preview  --- */
          /* --- buffer, and/or offset within it.               --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            { /* alpha */

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
                              preview_data[(j * width + i)*3    ] = *(srcptr);
                              preview_data[(j * width + i)*3 + 1] = *(srcptr+1);
                              preview_data[(j * width + i)*3 + 2] = *(srcptr+2);
                            }
                        }

                      srcptr += 4;
                    }
                }

              if (shaping)
                {
                  srcptr = rawframe + 3;
                  for (j=rawy; j<rawheight+rawy; j++)
                    {
                      k = j * ((width+7)/8);
                      for (i=rawx; i<rawwidth+rawx; i++)
                        {
                          if ((i>=0 && i<width) &&
                              (j>=0 && j<height))
                            {
                              if ((*srcptr)&128)
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
          if (shaping)
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  int top, bottom;

                  top = (rawy < 0) ? 0 : rawy;
                  bottom = ((rawy + rawheight) < height ?
                            (rawy + rawheight) : height - 1);

                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, top, width, bottom-top,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
                                      preview_data, width * 3);
                }
            }
          else
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  int top, bottom;

                  top = (rawy < 0) ? 0 : rawy;
                  bottom = ((rawy + rawheight) < height ?
                            (rawy + rawheight) : height - 1);

                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, top, width, bottom-top,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
                                      preview_data, width * 3);
                }
            }
        }
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      if ((rawwidth==width) &&
          (rawheight==height) &&
          (rawx==0) &&
          (rawy==0))
        {
          /* --- These cases are for the best cases,  in        --- */
          /* --- which this frame is the same size and position --- */
          /* --- as the preview buffer itself                   --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            { /* alpha */
              destptr = preview_data;
              srcptr  = rawframe;

              i = rawwidth*rawheight;
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
              if (shaping)
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

              if (shaping)
                {
                  /* opacify the shape mask */
                  memset (shape_preview_mask, 255,
                          (rawwidth * rawheight) / 8 + rawheight);
                }
            }

          /* Display the preview buffer... finally. */
          if (shaping)
            {
              reshape_from_bitmap (shape_preview_mask);
              gdk_draw_rgb_image (shape_drawing_area->window,
                                  shape_drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1)
                                  ? GDK_RGB_DITHER_MAX : DITHERTYPE,
                                  preview_data, width * 3);
            }
          else
            {
              gdk_draw_rgb_image (drawing_area->window,
                                  drawing_area->style->white_gc,
                                  0, 0, width, height,
                                  (total_frames == 1) ?
                                  GDK_RGB_DITHER_MAX : DITHERTYPE,
                                  preview_data, width * 3);
            }
        }
      else
        {
          /* --- These are suboptimal catch-all cases for when  --- */
          /* --- this frame is bigger/smaller than the preview  --- */
          /* --- buffer, and/or offset within it.               --- */

          if (gimp_drawable_has_alpha (drawable->drawable_id))
            { /* alpha */

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

              if (shaping)
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
          if (shaping)
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  int top, bottom;

                  top = (rawy < 0) ? 0 : rawy;
                  bottom = ((rawy + rawheight) < height ?
                            (rawy + rawheight) : height - 1);

                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, top, width, bottom-top,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  reshape_from_bitmap (shape_preview_mask);
                  gdk_draw_rgb_image (shape_drawing_area->window,
                                      shape_drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames==1)?GDK_RGB_DITHER_MAX
                                                       :DITHERTYPE,
                                      preview_data, width * 3);
                }
            }
          else
            {
              if ((dispose != DISPOSE_REPLACE) && (whichframe != 0))
                {
                  int top, bottom;

                  top = (rawy < 0) ? 0 : rawy;
                  bottom = ((rawy + rawheight) < height ?
                            (rawy + rawheight) : height - 1);

                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, top, width, bottom-top,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
                                      preview_data + 3 * top * width,
                                      width * 3);
                }
              else
                {
                  gdk_draw_rgb_image (drawing_area->window,
                                      drawing_area->style->white_gc,
                                      0, 0, width, height,
                                      (total_frames == 1) ?
                                      GDK_RGB_DITHER_MAX : DITHERTYPE,
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
  gtk_progress_bar_set_fraction (progress,
                                 ((float) frame_number /
                                  (float) (total_frames - 0.999)));

  text = g_strdup_printf (_("Frame %d of %d"), frame_number + 1, total_frames);
  gtk_progress_bar_set_text (progress, text);
  g_free (text);
}

static void
init_preview_misc (void)
{
  int i;

  preview_data = g_malloc (width * height * 3);
  shape_preview_mask = g_malloc ((width * height) / 8 + 1 + height);
  preview_alpha1_data = g_malloc (width * 3);
  preview_alpha2_data = g_malloc (width * 3);

  for (i=0;i<width;i++)
    {
      if (i&8)
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
total_alpha_preview (guchar* ptr)
{
  if (shaping)
    {
      memset(shape_preview_mask, 0, (width * height) / 8 + height);
    }
  else
    {
      gint i;

      for (i = 0;i < height; i++)
        {
          if (i & 8)
            memcpy (&ptr[i * 3 * width], preview_alpha1_data, 3 * width);
          else
            memcpy (&ptr[i * 3 * width], preview_alpha2_data, 3 * width);
        }
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
  frame_number = (frame_number+1)%total_frames;
  render_frame (frame_number);
}


/*  Callbacks  */

static void
window_response (GtkWidget *widget,
                 gint       response_id,
                 gpointer   data)
{
  gtk_widget_destroy (widget);

  if (playing)
    remove_timer ();

  if (shape_window)
    gtk_widget_destroy (GTK_WIDGET (shape_window));

  gdk_flush ();
  gtk_main_quit ();
}

static gint
advance_frame_callback (gpointer data)
{
  remove_timer();

  timer = g_timeout_add (get_frame_duration ((frame_number + 1) % total_frames),
                         advance_frame_callback, NULL);

  do_step ();
  show_frame ();

  return FALSE;
}

static void
play_callback (GtkAction *action,
               gpointer   data)
{
  GtkWidget *widget;

  if (playing)
    {
      playing = FALSE;
      remove_timer ();
    }
  else
    {
      playing = TRUE;
      timer = g_timeout_add (get_frame_duration (frame_number),
                             advance_frame_callback, NULL);
    }

  widget = gtk_ui_manager_get_widget (ui_manager, "/anim-play-toolbar/play");
  g_object_set (widget,
                "label",        NULL,
                "label-widget", NULL,
                "stock-id",     (playing ?
                                 GTK_STOCK_MEDIA_PAUSE : GTK_STOCK_MEDIA_PLAY),
                NULL);
}

static void
step_callback (GtkAction *action,
               gpointer   data)
{
  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));
  do_step();
  show_frame();
}

static void
rewind_callback (GtkAction *action,
                 gpointer   data)
{
  if (playing)
    gtk_action_activate (gtk_ui_manager_get_action (ui_manager,
                                                    "/anim-play-toolbar/play"));
  frame_number = 0;
  render_frame (frame_number);
  show_frame ();
}

/* tag util. */

static DisposeType
get_frame_disposal (guint whichframe)
{
  gchar       *layer_name;
  DisposeType  disposal;

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

  layer_name = gimp_drawable_get_name(layers[total_frames-(whichframe+1)]);
  if (layer_name)
    {
      duration = parse_ms_tag(layer_name);
      g_free(layer_name);
    }

  if (duration < 0) duration = 100;  /* FIXME for default-if-not-said  */
  else
    if (duration == 0) duration = 100; /* FIXME - 0-wait is nasty */

  return (guint32) duration;
}

static gboolean
is_ms_tag (const char *str,
	   int *duration,
	   int *taglength)
{
  gint sum = 0;
  gint offset;
  gint length;

  length = strlen(str);

  if (str[0] != '(')
    return FALSE;

  offset = 1;

  /* eat any spaces between open-parenthesis and number */
  while ((offset<length) && (str[offset] == ' '))
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

static int
parse_ms_tag (const char *str)
{
  int i;
  int rtn;
  int dummy;
  int length;

  length = strlen (str);

  for (i = 0; i < length; i++)
    {
      if (is_ms_tag (&str[i], &rtn, &dummy))
        return rtn;
    }

  return -1;
}

static gboolean
is_disposal_tag (const char *str,
		 DisposeType *disposal,
		 int *taglength)
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
parse_disposal_tag (const char *str)
{
  DisposeType rtn;
  int         i, dummy;
  gint        length;

  length = strlen(str);

  for (i = 0; i < length; i++)
    {
      if (is_disposal_tag (&str[i], &rtn, &dummy))
        {
          return rtn;
        }
    }

  return DISPOSE_UNDEFINED; /* FIXME */
}
