/*
 * Animation Playback plug-in version 0.98.6
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 *
 *
 * This is part of the GIMP package and is released under the GNU
 * Public License.
 */

/*
 * REVISION HISTORY:
 *
 * 2000-01-13 : version 0.98.6
 *              Looser parsing of (XXXXX) layer-name tags
 *
 * 98.07.27 : version 0.98.5
 *            UI tweaks, fix for pseudocolor displays w/gdkrgb.
 *
 * 98.07.20 : version 0.98.4
 *            User interface improvements.
 *
 * 98.07.19 : version 0.98.2
 *            Another speedup for some kinds of shaped animations.
 *
 * 98.07.19 : version 0.98.0
 *            Adapted to use GDKRGB (from recent GTK >= 1.1) if
 *            available - good speed and reliability improvement.
 *            Plus some minor tweaks.
 *
 * 98.04.28 : version 0.94.2
 *            Fixed a time-parsing bug.
 *
 * 98.04.05 : version 0.94.0
 *            Improved performance and removed flicker when shaped.
 *            Shaped mode also works with RGB* images now.
 *            Fixed some longstanding potential visual debris.
 *
 * 98.04.04 : version 0.92.0
 *            Improved responsiveness and performance for the new
 *            shaped-animation mode.  Still some flicker.
 *
 * 98.04.02 : version 0.90.0
 *            EXPERIMENTAL wackyness - try dragging the animation
 *            out of the plugin dialog's preview box...
 *            (only works on non-RGB* images for now)
 *
 * 98.03.16 : version 0.85.0
 *            Implemented some more rare opaque/alpha combinations.
 *
 * 98.03.15 : version 0.84.0
 *            Tried to clear up the GTK object/cast warnings.  Only
 *            partially successful.  Could use some help.
 *
 * 97.12.11 : version 0.83.0
 *            GTK's timer logic changed a little... adjusted
 *            plugin to fit.
 *
 * 97.09.16 : version 0.81.7
 *            Fixed progress bar's off-by-one problem with
 *            the new timing.  Fixed erroneous black bars which
 *            were sometimes visible when the first frame was
 *            smaller than the image itself.  Made playback
 *            controls inactive when image doesn't have multiple
 *            frames.  Moved progress bar above control buttons,
 *            it's less distracting there.  More cosmetic stuff.
 *
 * 97.09.15 : version 0.81.0
 *            Now plays INDEXED and GRAY animations.
 *
 * 97.09.15 : version 0.75.0
 *            Next frame is generated ahead of time - results
 *            in more precise timing.
 *
 * 97.09.14 : version 0.70.0
 *            Initial release.  RGB only.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#if defined (G_OS_WIN32)
/* This just to be able to include windows.h and undef RGB */
#define WinMain WinMain_foo
#include <windows.h>
#undef WinMain
#undef RGB
#endif

#include <gtk/gtk.h>
#ifndef GDK_WINDOWING_WIN32
#include <gdk/gdkx.h>
#else
#include <gdk/win32/gdkwin32.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Test for GTK1.2-style gdkrgb code, else use old 'preview' code. */
#ifdef __GDK_RGB_H__
#define RAPH_IS_HOME yep
/* Dithertype for animated GIFs */
#define DITHERTYPE GDK_RGB_DITHER_NORMAL
#endif

/* #define I_AM_STUPID yesiam */

typedef enum
{
  DISPOSE_UNDEFINED = 0x00,
  DISPOSE_COMBINE   = 0x01,
  DISPOSE_REPLACE   = 0x02
} DisposeType;



/* Declare local functions. */
static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static        void do_playback        (void);

static gint window_delete_callback    (GtkWidget *widget,
				       GdkEvent  *event,
				       gpointer   data);
static void window_close_callback     (GtkWidget *widget,
				       gpointer   data);
static void playstop_callback         (GtkWidget *widget,
				       gpointer   data);
static void rewind_callback           (GtkWidget *widget,
				       gpointer   data);
static void step_callback             (GtkWidget *widget,
				       gpointer   data);
#ifdef RAPH_IS_HOME
static void repaint_sda               (GtkWidget *darea,
				       gpointer data);
static void repaint_da                (GtkWidget *darea,
				       gpointer data);
#endif

static void         render_frame        (gint32 whichframe);
static void         show_frame          (void);
static void         total_alpha_preview (guchar* ptr);
static void         init_preview_misc   (void);


/* tag util functions*/
static         int parse_ms_tag        (const char *str);
static DisposeType parse_disposal_tag  (const char *str);
static DisposeType get_frame_disposal  (const guint whichframe);
static     guint32 get_frame_duration  (const guint whichframe);
static int is_disposal_tag (const char *str,
			    DisposeType *disposal,
			    int *taglength);
static int is_ms_tag (const char *str,
		      int *duration,
		      int *taglength);




GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};


/* Global widgets'n'stuff */
guchar*    preview_data;
#ifdef RAPH_IS_HOME
static GtkWidget* drawing_area = NULL;
static GtkWidget* shape_drawing_area = NULL;
guchar*    shape_drawing_area_data = NULL;
guchar*    drawing_area_data = NULL;
#else
static GtkPreview* preview = NULL;
#endif
GtkProgressBar* progress;
guint      width,height;
guchar*    preview_alpha1_data;
guchar*    preview_alpha2_data;
gint32     image_id;
gint32     total_frames;
guint      frame_number;
gint32*    layers;
GDrawable* drawable;
gboolean   playing = FALSE;
int        timer = 0;
GImageType imagetype;
guchar*    palette;
gint       ncolours;
GtkWidget *psbutton;



/* for shaping */
gchar    *shape_preview_mask;
GtkWidget *shape_window;
#ifdef RAPH_IS_HOME
#else
GtkWidget *shape_fixed;
GtkPreview *shape_preview;
GdkPixmap *shape_pixmap;
#endif
typedef struct _cursoroffset {gint x,y;} CursorOffset;
gint shaping = 0;
static GdkWindow *root_win = NULL;






MAIN()

static void 
query (void)
{
  static GParamDef args[] =
  {
    {PARAM_INT32,    "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE,    "image",    "Input image"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  INIT_I18N ();

  gimp_install_procedure ("plug_in_animationplay",
			  _("This plugin allows you to preview a GIMP layer-based animation."),
			  "",
			  "Adam D. Moss <adam@gimp.org>",
			  "Adam D. Moss <adam@gimp.org>",
			  "1997, 1998...",
			  N_("<Image>/Filters/Animation/Animation Playback..."),
			  "RGB*, INDEXED*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void 
run (gchar   *name, 
     gint     n_params, 
     GParam  *param, 
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == RUN_NONINTERACTIVE) 
    {
      if (n_params != 3) 
	{
	  status = STATUS_CALLING_ERROR;
	}
      INIT_I18N();
    } 
  else 
    {
      INIT_I18N_UI();
    }

  if (status == STATUS_SUCCESS) 
    {
      image_id = param[1].data.d_image;
      
      do_playback();
      
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush();
    }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}



/*
static int
parse_ms_tag (char *str)
{
  gint sum = 0;
  gint offset = 0;
  gint length;

  length = strlen(str);

find_another_bra:

  while ((offset<length) && (str[offset]!='('))
    offset++;
  
  if (offset>=length)
    return(-1);

  if (!isdigit(str[++offset]))
    goto find_another_bra;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset<length) && (isdigit(str[offset])));  

  if (length-offset <= 2)
    return(-3);

  if ((toupper(str[offset]) != 'M') || (toupper(str[offset+1]) != 'S'))
    return(-4);

  return (sum);
}


static DisposeType
parse_disposal_tag (char *str)
{
  gint offset = 0;
  gint length;

  length = strlen(str);

  while ((offset+9)<=length)
    {
      if (strncmp(&str[offset],"(combine)",9)==0) 
	return(DISPOSE_COMBINE);
      if (strncmp(&str[offset],"(replace)",9)==0) 
	return(DISPOSE_REPLACE);
      offset++;
    }

  return (DISPOSE_UNDEFINED);
}*/


static void
reshape_from_bitmap (gchar* bitmap)
{
  GdkBitmap *shape_mask;
  static gchar *prev_bitmap = NULL;

  if ((!prev_bitmap) ||
      (memcmp(prev_bitmap, bitmap, (width*height)/8 +height)))
    {
      shape_mask = gdk_bitmap_create_from_data(shape_window->window,
					       bitmap,
					       width, height);
      gtk_widget_shape_combine_mask (shape_window, shape_mask, 0, 0);
      gdk_bitmap_unref (shape_mask);

      if (!prev_bitmap)
	{
	  prev_bitmap = g_malloc ((width*height)/8 +height);
	}
      memcpy (prev_bitmap, bitmap, (width*height)/8 +height);
    }
}


static void
shape_pressed (GtkWidget      *widget, 
	       GdkEventButton *event)
{
  CursorOffset *p;

  /* ignore double and triple click */
  if (event->type != GDK_BUTTON_PRESS)
    return;

  p = gtk_object_get_user_data (GTK_OBJECT(widget));
  p->x = (int) event->x;
  p->y = (int) event->y;

  gtk_grab_add (widget);
  gdk_pointer_grab (widget->window, TRUE,
                    GDK_BUTTON_RELEASE_MASK |
                    GDK_BUTTON_MOTION_MASK |
                    GDK_POINTER_MOTION_HINT_MASK,
                    NULL, NULL, 0);
  gdk_window_raise (widget->window);
}


#ifdef RAPH_IS_HOME
static void
maybeblocked_expose (GtkWidget      *widget, 
		     GdkEventExpose *event)
{
  if (playing)
    gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "expose_event");
  else
    repaint_sda (widget, event);
}
#else


static void
blocked_expose (GtkWidget      *widget, 
		GdkEventExpose *event)
{
  gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "expose_event");
}
#endif


#ifdef I_AM_STUPID
static void
xblocked_expose (GtkWidget      *widget, 
		 GdkEventExpose *event)
{
  printf("eep!\n");fflush(stdout);
  abort();
}

static void
unblocked_expose (GtkWidget      *widget, 
		  GdkEventExpose *event)
{
  gboolean should_block;
  
  printf("%p: t%d w:%p s:%d c:%d \n",widget, event->type, event->window,
	 event->send_event, event->count);
  fflush(stdout);

  return;
  
  /*
   * If the animation is playing, we only respond to exposures
   * which are artificially generated as a result of i.e.
   * draw_widget.  This is to avoid needlessly redrawing twice
   * per frame, because also 'real' exposure events may be generated
   * by reshaping the windoow.
   *
   * If the animation is not playing, then we respond to any type
   * of expose event.
   */
	 
  if (playing)
    should_block = (!event->send_event) || (event->count != 0);
  else
    should_block = FALSE;

  if (should_block)
    {
      /*
       * Since a whole load of exposures can come back-to-back,
       * starvation can occur for the dialog etc.  This alleviates
       * the pain.
       */
      while (gtk_events_pending())
	gtk_main_iteration_do(TRUE);

      /*
       * Block the expose from being acted upon.
       */
      blocked_expose(widget, event);

      return;
    }
}
#endif


static void
shape_released (GtkWidget *widget)
{
  gtk_grab_remove (widget);
  gdk_pointer_ungrab (0);
  gdk_flush();
}


static void
shape_motion (GtkWidget      *widget,
              GdkEventMotion *event)
{
  gint xp, yp;
  CursorOffset * p;
  GdkModifierType mask;

  gdk_window_get_pointer (root_win, &xp, &yp, &mask);

  /*  printf("%u %d\n", mask, event->state);fflush(stdout); */

  /* if a button is still held by the time we process this event... */
  if (mask & (GDK_BUTTON1_MASK|
	      GDK_BUTTON2_MASK|
	      GDK_BUTTON3_MASK|
	      GDK_BUTTON4_MASK|
	      GDK_BUTTON5_MASK))
    {
      p = gtk_object_get_user_data (GTK_OBJECT (widget));

      gtk_widget_set_uposition (widget, xp  - p->x, yp  - p->y);
    }
  else /* the user has released all buttons */
    {
      shape_released(widget);
    }
}


#ifdef RAPH_IS_HOME
static void
repaint_da (GtkWidget *darea, 
	    gpointer   data)
{
  /*  printf("Repaint!  Woohoo!\n");*/
  gdk_draw_rgb_image (drawing_area->window,
		      drawing_area->style->white_gc,
		      0, 0, width, height,
		      (total_frames==1)?GDK_RGB_DITHER_MAX:DITHERTYPE,
		      drawing_area_data, width * 3);
}


static void
repaint_sda (GtkWidget *darea, 
	     gpointer   data)
{
  /*printf("Repaint!  Woohoo!\n");*/
  gdk_draw_rgb_image (shape_drawing_area->window,
		      shape_drawing_area->style->white_gc,
		      0, 0, width, height,
		      (total_frames==1)?GDK_RGB_DITHER_MAX:DITHERTYPE,
		      shape_drawing_area_data, width * 3);
}
#endif


static void
preview_pressed (GtkWidget      *widget, 
		 GdkEventButton *event)
{
#ifdef RAPH_IS_HOME
#else
  gint i;
#endif
  gint xp, yp;
  GdkModifierType mask;

  if (shaping) return;
  
#ifdef RAPH_IS_HOME
  /* Create a total-alpha buffer merely for the not-shaped
     drawing area to now display. */

  drawing_area_data = g_malloc (width * height * 3);
  total_alpha_preview (drawing_area_data);

#else
  /* put current preview buf into shaped buf */
  for (i=0;i<height;i++)
    {
      gtk_preview_draw_row (shape_preview,
			    &preview_data[3*i*width],
			    0, i, width);
  }
  total_alpha_preview (preview_data);
#endif
      
  gdk_window_get_pointer (root_win, &xp, &yp, &mask);
  gtk_widget_set_uposition (shape_window, xp  - event->x, yp  - event->y);
  
  gtk_widget_show (shape_window);

  gdk_window_set_back_pixmap(shape_window->window, NULL, 0);
#ifdef RAPH_IS_HOME
  gdk_window_set_back_pixmap(shape_drawing_area->window, NULL, 1);
#else
  gdk_window_set_back_pixmap(shape_fixed->window, NULL, 1);
#endif

#ifdef RAPH_IS_HOME
#else
  for (i=0;i<height;i++)
    {
      gtk_preview_draw_row (preview,
			    &preview_data[3*i*width],
			    0, i, width);
    }
#endif
  show_frame();

  shaping = 1;
  memset(shape_preview_mask, 0, (width*height)/8 + height);
  render_frame(frame_number);
  show_frame();
  
#ifdef RAPH_IS_HOME
  repaint_da (NULL,NULL);
#endif

  /* mildly amusing hack */
  shape_pressed(shape_window, event);
}



static void
build_dialog (GImageType  basetype,
	      char       *imagename)
{
  gchar** argv;
  gint argc;

  gchar* windowname;
  CursorOffset* icon_pos;
  GtkAdjustment *adj;

  GtkWidget* dlg;
  GtkWidget* button;
  GtkWidget* frame;
  GtkWidget* frame2;
  GtkWidget* vbox;
  GtkWidget* hbox;
  GtkWidget* hbox2;
  GtkWidget* eventbox;
  GdkCursor* cursor;

#ifndef RAPH_IS_HOME
  guchar* color_cube;
#endif

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("animationplay");
  gtk_init (&argc, &argv);

#ifdef RAPH_IS_HOME
  gdk_rgb_init ();
#endif

  gtk_rc_parse (gimp_gtkrc ());
  gdk_set_use_xshm (gimp_use_xshm ());

#ifdef RAPH_IS_HOME
  gtk_widget_set_default_visual (gdk_rgb_get_visual());
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap());
#else
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());
#endif

  windowname = g_strconcat (_("Animation Playback: "), imagename, NULL);

  dlg = gimp_dialog_new (windowname, "animationplay",
			 gimp_plugin_help_func, "filters/animationplay.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("Close"), window_close_callback,
			 NULL, 1, NULL, TRUE, TRUE,

			 NULL);

  g_free (windowname);

  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (window_delete_callback),
		      NULL);

  {
    /* The 'playback' half of the dialog */
    
    windowname = g_malloc (strlen (_("Playback: ")) + strlen (imagename) + 1);
    if (total_frames > 1)
      {
	strcpy (windowname, _("Playback: "));
	strcat (windowname, imagename);
      }
    else
      {
	strcpy (windowname, imagename);	
      }
    frame = gtk_frame_new (windowname);
    g_free (windowname);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 3);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			frame, TRUE, TRUE, 0);
    
    {
      hbox = gtk_hbox_new (FALSE, 5);
      gtk_container_border_width (GTK_CONTAINER (hbox), 3);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      
      {
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);
	gtk_container_add (GTK_CONTAINER (hbox), vbox);
	
	{
	  hbox2 = gtk_hbox_new (FALSE, 0);
	  gtk_container_border_width (GTK_CONTAINER (hbox2), 0);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);
	  
	  {
	    psbutton = gtk_toggle_button_new_with_label ( _("Play/Stop"));
	    gtk_signal_connect (GTK_OBJECT (psbutton), "toggled",
				(GtkSignalFunc) playstop_callback, NULL);
	    gtk_box_pack_start (GTK_BOX (hbox2), psbutton, TRUE, TRUE, 0);
	    gtk_widget_show (psbutton);
	    
	    button = gtk_button_new_with_label ( _("Rewind"));
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				(GtkSignalFunc) rewind_callback, NULL);
	    gtk_box_pack_start (GTK_BOX (hbox2), button, TRUE, TRUE, 0);
	    gtk_widget_show (button);
	    
	    button = gtk_button_new_with_label ( _("Step"));
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				(GtkSignalFunc) step_callback, NULL);
	    gtk_box_pack_start (GTK_BOX (hbox2), button, TRUE, TRUE, 0);
	    gtk_widget_show (button);
	  }
	  /* If there aren't multiple frames, playback controls make no
	     sense */
	  /*	  if (total_frames<=1) gtk_widget_set_sensitive (hbox2, FALSE);
	  gtk_widget_show(hbox2);*/

	  if (total_frames>1)
	    gtk_widget_show(hbox2);

	  hbox2 = gtk_hbox_new (TRUE, 0);
	  gtk_container_border_width (GTK_CONTAINER (hbox2), 0);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
	  {
	    frame2 = gtk_frame_new (NULL);
	    gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_ETCHED_IN);
	    gtk_box_pack_start (GTK_BOX (hbox2), frame2, FALSE, FALSE, 0);
	    
	    {
	      eventbox = gtk_event_box_new();
	      gtk_container_add (GTK_CONTAINER (frame2), GTK_WIDGET (eventbox));
	      
	      {
#ifdef RAPH_IS_HOME
		drawing_area = gtk_drawing_area_new ();
                gtk_widget_set_usize (drawing_area, width, height);
                gtk_container_add (GTK_CONTAINER (eventbox),
                                   GTK_WIDGET (drawing_area));
                gtk_widget_show (drawing_area);
#else
		preview =
		  GTK_PREVIEW (gtk_preview_new (GTK_PREVIEW_COLOR));/* FIXME */
		gtk_preview_size (preview, width, height);
		gtk_container_add (GTK_CONTAINER (eventbox),
				   GTK_WIDGET (preview));
		gtk_widget_show(GTK_WIDGET (preview));
#endif
	      }
	      gtk_widget_show(eventbox);
	      gtk_widget_set_events (eventbox,
				     gtk_widget_get_events (eventbox)
				     | GDK_BUTTON_PRESS_MASK);
	    }
	    gtk_widget_show(frame2);
	  }
	  gtk_widget_show(hbox2);


	  adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, total_frames,
						      1, 1, 1);
	  progress = GTK_PROGRESS_BAR (gtk_progress_bar_new_with_adjustment
				       (adj));
	  {
	    gtk_progress_set_format_string (GTK_PROGRESS (progress),
					    _("Frame %v of %u"));
	    gtk_progress_set_show_text (GTK_PROGRESS (progress), TRUE);
	    /*	  gtk_widget_set_usize (GTK_WIDGET (progress), 150, 15);*/
	    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (progress),
				TRUE, TRUE, 0);
	  }
	  if (total_frames>1)
	    gtk_widget_show (GTK_WIDGET (progress));
	}
	gtk_widget_show(vbox);
	
      }
      gtk_widget_show(hbox);
      
    }
    gtk_widget_show(frame);
    
  }
  gtk_widget_show(dlg);


  /* let's get into shape. */
  shape_window = gtk_window_new (GTK_WINDOW_POPUP);
  {
#ifdef RAPH_IS_HOME
    shape_drawing_area = gtk_drawing_area_new ();
    {
      gtk_widget_set_usize (shape_drawing_area, width, height);
      gtk_container_add (GTK_CONTAINER (shape_window), shape_drawing_area);
    }
    gtk_widget_show (shape_drawing_area);
    gtk_widget_set_events (shape_drawing_area,
			   gtk_widget_get_events (shape_drawing_area)
			   | GDK_BUTTON_PRESS_MASK);
#else
    shape_fixed = gtk_fixed_new ();
    {
      gtk_widget_set_usize (shape_fixed, width,height);
      gtk_container_add (GTK_CONTAINER (shape_window), shape_fixed);

      shape_preview =
	GTK_PREVIEW (gtk_preview_new (GTK_PREVIEW_COLOR)); /* FIXME */
      {
	gtk_preview_size (shape_preview, width, height);
      }
    }
    gtk_widget_show (shape_fixed);
#endif
    gtk_widget_realize (shape_window);

#ifdef RAPH_IS_HOME
#else
    shape_pixmap = gdk_pixmap_new (shape_window->window,
				   width, height,
				   gtk_preview_get_visual()->depth);
#endif
    
    gdk_window_set_back_pixmap(shape_window->window, NULL, 0);

    cursor = gdk_cursor_new (GDK_CENTER_PTR);
    gdk_window_set_cursor(shape_window->window, cursor);
    gdk_cursor_destroy (cursor);

    gtk_signal_connect (GTK_OBJECT (shape_window), "button_press_event",
			GTK_SIGNAL_FUNC (shape_pressed),NULL);
    gtk_signal_connect (GTK_OBJECT (shape_window), "button_release_event",
			GTK_SIGNAL_FUNC (shape_released),NULL);
    gtk_signal_connect (GTK_OBJECT (shape_window), "motion_notify_event",
			GTK_SIGNAL_FUNC (shape_motion),NULL);

    icon_pos = g_new (CursorOffset, 1);
    gtk_object_set_user_data(GTK_OBJECT(shape_window), icon_pos);
  }
  /*  gtk_widget_show (shape_window);*/

  gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
		      GTK_SIGNAL_FUNC (preview_pressed),NULL);

#ifdef I_AM_STUPID
  gtk_signal_connect (GTK_OBJECT (shape_window), "expose_event",
		      GTK_SIGNAL_FUNC (unblocked_expose),shape_window);
#endif

#ifdef RAPH_IS_HOME
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      GTK_SIGNAL_FUNC (repaint_da), drawing_area);

  gtk_signal_connect (GTK_OBJECT (shape_drawing_area), "expose_event",
		      GTK_SIGNAL_FUNC (maybeblocked_expose),
		      shape_drawing_area);
#else
  gtk_signal_connect (GTK_OBJECT (shape_fixed), "expose_event",
		      GTK_SIGNAL_FUNC (blocked_expose),shape_fixed);
#endif

  root_win = gdk_window_foreign_new (GDK_ROOT_WINDOW ());
}



static void 
do_playback (void)
{
  int i;

  width     = gimp_image_width(image_id);
  height    = gimp_image_height(image_id);
  layers    = gimp_image_get_layers (image_id, &total_frames);
  imagetype = gimp_image_base_type(image_id);

  if (imagetype == INDEXED)
    palette = gimp_image_get_cmap(image_id, &ncolours);
  else if (imagetype == GRAY)
    {
      /* This is a bit sick, until this plugin ever gets
	 real GRAY support (not worth it?) */
      palette = g_malloc(768);
      for (i=0;i<256;i++)
      {
          palette[i*3] = palette[i*3+1] = palette[i*3+2] = i;
      }
      ncolours = 256;
    }


  frame_number = 0;
  
  /* cache hint "cache nothing", since we iterate over every
     tile in every layer. */
  gimp_tile_cache_size (0);

  init_preview_misc();

  build_dialog(gimp_image_base_type(image_id),
               gimp_image_get_filename(image_id));

  /* Make sure that whole preview is dirtied with pure-alpha */
  total_alpha_preview(preview_data);

#ifdef RAPH_IS_HOME
  /*  gdk_draw_rgb_image (drawing_area->window,
		      drawing_area->style->white_gc,
		      0, 0, width, height,
		      DITHERTYPE,
		      preview_data, width * 3);*/
#else
  for (i=0;i<height;i++)
  {
    gtk_preview_draw_row (preview,
                          &preview_data[3*i*width],
                          0, i, width);
  }
#endif
  
  render_frame(0);
  show_frame();

  gtk_main ();
  gdk_flush ();
}


/* Rendering Functions */

static void
render_frame (gint32 whichframe)
{
  GPixelRgn pixel_rgn;
  static guchar *rawframe = NULL;
  static gint rawwidth=0, rawheight=0, rawbpp=0;
  gint rawx=0, rawy=0;
  guchar* srcptr;
  guchar* destptr;
  gint i,j,k; /* imaginative loop variables */
  DisposeType dispose;


  if (whichframe >= total_frames)
    {
      printf( "playback: Asked for frame number %d in a %d-frame animation!\n",
	     (int) (whichframe+1), (int) total_frames);
      exit(-1);
    }

  drawable = gimp_drawable_get (layers[total_frames-(whichframe+1)]);

  dispose = get_frame_disposal(frame_number);

  /* Image has been closed/etc since we got the layer list? */
  /* FIXME - How do we tell if a gimp_drawable_get() fails? */
  if (gimp_drawable_width(drawable->id)==0)
    window_close_callback (NULL, NULL);

  if (((dispose==DISPOSE_REPLACE)||(whichframe==0)) &&
      gimp_drawable_has_alpha(drawable->id))
    {
      total_alpha_preview(preview_data);
    }


  /* only get a new 'raw' drawable-data buffer if this and
     the previous raw buffer were different sizes*/

  if ((rawwidth*rawheight*rawbpp)
      !=
      ((gimp_drawable_width(drawable->id)*
	gimp_drawable_height(drawable->id)*
	gimp_drawable_bpp(drawable->id))))
    {
      if (rawframe != NULL) g_free(rawframe);
      rawframe = g_malloc((gimp_drawable_width(drawable->id)) *
			  (gimp_drawable_height(drawable->id)) *
			  (gimp_drawable_bpp(drawable->id)));
    }
	
  rawwidth = gimp_drawable_width(drawable->id);
  rawheight = gimp_drawable_height(drawable->id);
  rawbpp = gimp_drawable_bpp(drawable->id);


  /* Initialise and fetch the whole raw new frame */

  gimp_pixel_rgn_init (&pixel_rgn,
		       drawable,
		       0, 0,
		       drawable->width, drawable->height,
		       FALSE,
		       FALSE);
  gimp_pixel_rgn_get_rect (&pixel_rgn,
			   rawframe,
			   0, 0,
			   drawable->width, drawable->height);
  /*  gimp_pixel_rgns_register (1, &pixel_rgn);*/

  gimp_drawable_offsets (drawable->id,
			 &rawx,
			 &rawy);


  /* render... */

  switch (imagetype)
    {
    case RGB:
      if ((rawwidth==width) &&
	  (rawheight==height) &&
	  (rawx==0) &&
	  (rawy==0))
	{
	  /* --- These cases are for the best cases,  in        --- */
	  /* --- which this frame is the same size and position --- */
	  /* --- as the preview buffer itself                   --- */
	  
	  if (gimp_drawable_has_alpha (drawable->id))
	    { /* alpha */
	      destptr = preview_data;
	      srcptr  = rawframe;
	      
	      i = rawwidth*rawheight;
	      while (--i)
		{
		  if (!(*(srcptr+3)&128))
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
		  for (j=0;j<rawheight;j++)
		    {
		      k = j * ((7 + rawwidth) / 8);
		      for (i=0;i<rawwidth;i++)
			{
			  if ((*srcptr)&128)
			    shape_preview_mask[k+i/8] |= (1<<(i&7));
			  srcptr += 4;
			}
		    }
		}
	    }
	  else /* no alpha */
	    {
	      if ((rawwidth==width)&&(rawheight==height))
		{
		  /*printf("quickie\n");fflush(stdout);*/
		  memcpy(preview_data, rawframe, width*height*3);
		}

	      if (shaping)
		{
		  /* opacify the shape mask */
		  memset(shape_preview_mask, 255,
			 (rawwidth*rawheight)/8 + rawheight);
		}
	    }
	  /* Display the preview buffer... finally. */
	  if (shaping)
	    {
#ifdef RAPH_IS_HOME
	      reshape_from_bitmap (shape_preview_mask);
	      gdk_draw_rgb_image (shape_drawing_area->window,
				  shape_drawing_area->style->white_gc,
				  0, 0, width, height,
				  (total_frames==1)?GDK_RGB_DITHER_MAX
				  :DITHERTYPE,
				  preview_data, width * 3);
#else
	      for (i=0;i<height;i++)
		{
		  gtk_preview_draw_row (shape_preview,
					&preview_data[3*i*width],
					0, i, width);
		}
#endif
	    }
	  else
	    {
#ifdef RAPH_IS_HOME
	      reshape_from_bitmap (shape_preview_mask);
	      gdk_draw_rgb_image (drawing_area->window,
				  drawing_area->style->white_gc,
				  0, 0, width, height,
				  (total_frames==1)?GDK_RGB_DITHER_MAX
				  :DITHERTYPE,
				  preview_data, width * 3);
#else
	      for (i=0;i<height;i++)
		{
		  gtk_preview_draw_row (preview,
					&preview_data[3*i*width],
					0, i, width);
		}
#endif
	    }
	}
      else
	{
	  /* --- These are suboptimal catch-all cases for when  --- */
	  /* --- this frame is bigger/smaller than the preview  --- */
	  /* --- buffer, and/or offset within it.               --- */
	  
	  if (gimp_drawable_has_alpha (drawable->id))
	    { /* alpha */
	      
	      srcptr = rawframe;
	      
	      for (j=rawy; j<rawheight+rawy; j++)
		{
		  for (i=rawx; i<rawwidth+rawx; i++)
		    {
		      if ((i>=0 && i<width) &&
			  (j>=0 && j<height))
			{
			  if (*(srcptr+3)&128)
			    {
			      preview_data[(j*width+i)*3   ] = *(srcptr);
			      preview_data[(j*width+i)*3 +1] = *(srcptr+1);
			      preview_data[(j*width+i)*3 +2] = *(srcptr+2);
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
				shape_preview_mask[k+i/8] |= (1<<(i&7));
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
	      
	      for (j=rawy; j<rawheight+rawy; j++)
		{
		  for (i=rawx; i<rawwidth+rawx; i++)
		    {
		      if ((i>=0 && i<width) &&
			  (j>=0 && j<height))
			{
			  preview_data[(j*width+i)*3   ] = *(srcptr);
			  preview_data[(j*width+i)*3 +1] = *(srcptr+1);
			  preview_data[(j*width+i)*3 +2] = *(srcptr+2);
			}
		      
		      srcptr += 3;
		    }
		}
	    }
	  
	  /* Display the preview buffer... finally. */
	  if (shaping)
	    {
	      if ((dispose!=DISPOSE_REPLACE)&&(whichframe!=0))
		{
#ifdef RAPH_IS_HOME
		  int top, bottom;

		  top = (rawy < 0) ? 0 : rawy;
		  bottom = (rawy+rawheight) < height ?
		    (rawy+rawheight) : height-1;

		  reshape_from_bitmap (shape_preview_mask);
		  gdk_draw_rgb_image (shape_drawing_area->window,
				      shape_drawing_area->style->white_gc,
				      0, top, width, bottom-top,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      &preview_data[3*top*width], width * 3);
#else
		  for (i=rawy;i<rawy+rawheight;i++)
		    {
		      if (i>=0 && i<height)
			gtk_preview_draw_row (shape_preview,
					      &preview_data[3*i*width],
					      0, i, width);
		    }
#endif
		}
	      else
		{
#ifdef RAPH_IS_HOME
		  reshape_from_bitmap (shape_preview_mask);
		  gdk_draw_rgb_image (shape_drawing_area->window,
				      shape_drawing_area->style->white_gc,
				      0, 0, width, height,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      preview_data, width * 3);
#else
		  for (i=0;i<height;i++)
		    {
		      gtk_preview_draw_row (shape_preview,
					    &preview_data[3*i*width],
					    0, i, width);
		    }
#endif
		}
	    }
	  else
	    {
	      if ((dispose!=DISPOSE_REPLACE)&&(whichframe!=0))
		{
#ifdef RAPH_IS_HOME
		  int top, bottom;

		  top = (rawy < 0) ? 0 : rawy;
		  bottom = (rawy+rawheight) < height ?
		    (rawy+rawheight) : height-1;

		  gdk_draw_rgb_image (drawing_area->window,
				      drawing_area->style->white_gc,
				      0, top, width, bottom-top,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      &preview_data[3*top*width], width * 3);
#else
		  for (i=rawy;i<rawy+rawheight;i++)
		    {
		      if (i>=0 && i<height)
			gtk_preview_draw_row (preview,
					      &preview_data[3*i*width],
					      0, i, width);
		    }
#endif
		}
	      else
		{
#ifdef RAPH_IS_HOME
		  gdk_draw_rgb_image (drawing_area->window,
				      drawing_area->style->white_gc,
				      0, 0, width, height,
				      (total_frames==1)?GDK_RGB_DITHER_MAX:
				      DITHERTYPE,
				      preview_data, width * 3);
#else
		  for (i=0;i<height;i++)
		    {
		      gtk_preview_draw_row (preview,
					    &preview_data[3*i*width],
					    0, i, width);
		    }
#endif
		}
	    }
	}
      break;

    case GRAY:
    case INDEXED:
      if ((rawwidth==width) &&
	  (rawheight==height) &&
	  (rawx==0) &&
	  (rawy==0))
	{
	  /* --- These cases are for the best cases,  in        --- */
	  /* --- which this frame is the same size and position --- */
	  /* --- as the preview buffer itself                   --- */
	  
	  if (gimp_drawable_has_alpha (drawable->id))
	    { /* alpha */
	      destptr = preview_data;
	      srcptr  = rawframe;
	      
	      i = rawwidth*rawheight;
	      while (--i)
		{
		  if (!(*(srcptr+1)))
		    {
		      srcptr  += 2;
		      destptr += 3;
		      continue;
		    }
		  
		  *(destptr++) = palette[3*(*(srcptr))];
		  *(destptr++) = palette[1+3*(*(srcptr))];
		  *(destptr++) = palette[2+3*(*(srcptr))];
		  srcptr+=2;
		}

	      /* calculate the shape mask */
	      if (shaping)
		{
		  srcptr = rawframe + 1;
		  for (j=0;j<rawheight;j++)
		    {
		      k = j * ((7 + rawwidth) / 8);
		      for (i=0;i<rawwidth;i++)
			{
			  if (*srcptr)
			    shape_preview_mask[k+i/8] |= (1<<(i&7));
			  srcptr += 2;
			}
		    }
		}
	    }
	  else /* no alpha */
	    {
	      destptr = preview_data;
	      srcptr  = rawframe;
	      
	      i = rawwidth*rawheight;
	      while (--i)
		{
		  *(destptr++) = palette[3*(*(srcptr))];
		  *(destptr++) = palette[1+3*(*(srcptr))];
		  *(destptr++) = palette[2+3*(*(srcptr))];
		  srcptr++;
		}

	      if (shaping)
		{
		  /* opacify the shape mask */
		  memset(shape_preview_mask, 255,
			 (rawwidth*rawheight)/8 + rawheight);
		}
	    }
	  
	  /* Display the preview buffer... finally. */
	  if (shaping)
	    {
#ifdef RAPH_IS_HOME
	      reshape_from_bitmap (shape_preview_mask);
	      gdk_draw_rgb_image (shape_drawing_area->window,
				  shape_drawing_area->style->white_gc,
				  0, 0, width, height,
				  (total_frames==1)?GDK_RGB_DITHER_MAX:
				  DITHERTYPE,
				  preview_data, width * 3);
#else
	      for (i=0;i<height;i++)
		{
		  gtk_preview_draw_row (shape_preview,
					&preview_data[3*i*width],
					0, i, width);
		}
#endif
	    }
	  else
	    {
#ifdef RAPH_IS_HOME
	      gdk_draw_rgb_image (drawing_area->window,
				  drawing_area->style->white_gc,
				  0, 0, width, height,
				  (total_frames==1)?GDK_RGB_DITHER_MAX:
				  DITHERTYPE,
				  preview_data, width * 3);
#else
	      for (i=0;i<height;i++)
		{
		  gtk_preview_draw_row (preview,
					&preview_data[3*i*width],
					0, i, width);
		}
#endif
	    }
	}
      else
	{
	  /* --- These are suboptimal catch-all cases for when  --- */
	  /* --- this frame is bigger/smaller than the preview  --- */
	  /* --- buffer, and/or offset within it.               --- */
	  
	  if (gimp_drawable_has_alpha (drawable->id))
	    { /* alpha */
	      
	      srcptr = rawframe;
	      
	      for (j=rawy; j<rawheight+rawy; j++)
		{
		  for (i=rawx; i<rawwidth+rawx; i++)
		    {
		      if ((i>=0 && i<width) &&
			  (j>=0 && j<height))
			{
			  if (*(srcptr+1))
			    {
			      preview_data[(j*width+i)*3   ] =
				palette[3*(*(srcptr))];
			      preview_data[(j*width+i)*3 +1] =
				palette[1+3*(*(srcptr))];
			      preview_data[(j*width+i)*3 +2] =
				palette[2+3*(*(srcptr))];
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
		      k = j * ((width+7)/8);
		      for (i=rawx; i<rawwidth+rawx; i++)
			{
			  if ((i>=0 && i<width) &&
			      (j>=0 && j<height))
			    {
			      if (*srcptr)
				shape_preview_mask[k+i/8] |= (1<<(i&7));
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
	      
	      for (j=rawy; j<rawheight+rawy; j++)
		{
		  for (i=rawx; i<rawwidth+rawx; i++)
		    {
		      if ((i>=0 && i<width) &&
			  (j>=0 && j<height))
			{
			  preview_data[(j*width+i)*3   ] =
			    palette[3*(*(srcptr))];
			  preview_data[(j*width+i)*3 +1] =
			    palette[1+3*(*(srcptr))];
			  preview_data[(j*width+i)*3 +2] =
			    palette[2+3*(*(srcptr))];
			}
		      
		      srcptr ++;
		    }
		}
	    }
	  
	  /* Display the preview buffer... finally. */
	  if (shaping)
	    {
	      if ((dispose!=DISPOSE_REPLACE)&&(whichframe!=0))
		{
#ifdef RAPH_IS_HOME
		  int top, bottom;

		  top = (rawy < 0) ? 0 : rawy;
		  bottom = (rawy+rawheight) < height ?
		    (rawy+rawheight) : height-1;

		  reshape_from_bitmap (shape_preview_mask);
		  gdk_draw_rgb_image (shape_drawing_area->window,
				      shape_drawing_area->style->white_gc,
				      0, top, width, bottom-top,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      &preview_data[3*top*width], width * 3);
#else
		  for (i=rawy;i<rawy+rawheight;i++)
		    {
		      if (i>=0 && i<height)
			gtk_preview_draw_row (shape_preview,
					      &preview_data[3*i*width],
					      0, i, width);
		    }
#endif
		}
	      else
		{
#ifdef RAPH_IS_HOME
		  reshape_from_bitmap (shape_preview_mask);
		  gdk_draw_rgb_image (shape_drawing_area->window,
				      shape_drawing_area->style->white_gc,
				      0, 0, width, height,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      preview_data, width * 3);
#else
		  for (i=0;i<height;i++)
		    {
		      gtk_preview_draw_row (shape_preview,
					    &preview_data[3*i*width],
					    0, i, width);
		    }
#endif
		}
	    }
	  else
	    {
	      if ((dispose!=DISPOSE_REPLACE)&&(whichframe!=0))
		{
#ifdef RAPH_IS_HOME
		  int top, bottom;

		  top = (rawy < 0) ? 0 : rawy;
		  bottom = (rawy+rawheight) < height ?
		    (rawy+rawheight) : height-1;

		  gdk_draw_rgb_image (drawing_area->window,
				      drawing_area->style->white_gc,
				      0, top, width, bottom-top,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      &preview_data[3*top*width], width * 3);
#else
		  for (i=rawy;i<rawy+rawheight;i++)
		    {
		      if (i>=0 && i<height)
			gtk_preview_draw_row (preview,
					      &preview_data[3*i*width],
					      0, i, width);
		    }
#endif
		}
	      else
		{
#ifdef RAPH_IS_HOME
		  gdk_draw_rgb_image (drawing_area->window,
				      drawing_area->style->white_gc,
				      0, 0, width, height,
				      (total_frames==1)?GDK_RGB_DITHER_MAX
				      :DITHERTYPE,
				      preview_data, width * 3);
#else
		  for (i=0;i<height;i++)
		    {
		      gtk_preview_draw_row (preview,
					    &preview_data[3*i*width],
					    0, i, width);
		    }
#endif
		}
	    }
	}
      break;
      
    }

  /* clean up */  
  gimp_drawable_detach(drawable);
}


/* If we're using GDKRGB, we don't reshape in this function because
   it's too late (GDKRGB is synchronous).  So this just updates the
   progress bar. */
static void
show_frame (void)
{
#ifndef RAPH_IS_HOME
  GdkGC *gc;

  /* Tell GTK to physically draw the preview */
  if (!shaping)
    {
      gtk_widget_draw (GTK_WIDGET (preview), NULL);
    }

  if (shaping)
    {
      /* Try to avoid starvation of UI events */
      while (gtk_events_pending())
	gtk_main_iteration_do(TRUE);

      gc = gdk_gc_new (shape_pixmap);
      gtk_preview_put (GTK_PREVIEW (shape_preview),
		       shape_pixmap, gc,
		       0, 0, 0, 0, width, height);
      gdk_gc_destroy (gc);
      gdk_window_set_back_pixmap(shape_window->window, shape_pixmap,
				 FALSE);

      reshape_from_bitmap(shape_preview_mask);

      gdk_flush();

      gtk_widget_queue_draw(shape_window);
    }
#endif /* ! RAPH_IS_HOME */

  /* update the dialog's progress bar */
  gtk_progress_bar_update (progress,
			   ((float)frame_number/(float)(total_frames-0.999)));
}


static void
init_preview_misc (void)
{
  int i;

  preview_data = g_malloc(width*height*3);
  shape_preview_mask = g_malloc((width*height)/8 + 1 + height);
  preview_alpha1_data = g_malloc(width*3);
  preview_alpha2_data = g_malloc(width*3);

  for (i=0;i<width;i++)
    {
      if (i&8)
	{
	  preview_alpha1_data[i*3 +0] =
	  preview_alpha1_data[i*3 +1] =
	  preview_alpha1_data[i*3 +2] = 102;
	  preview_alpha2_data[i*3 +0] =
	  preview_alpha2_data[i*3 +1] =
	  preview_alpha2_data[i*3 +2] = 154;
	}
      else
	{
	  preview_alpha1_data[i*3 +0] =
	  preview_alpha1_data[i*3 +1] =
	  preview_alpha1_data[i*3 +2] = 154;
	  preview_alpha2_data[i*3 +0] =
	  preview_alpha2_data[i*3 +1] =
	  preview_alpha2_data[i*3 +2] = 102;
	}
    }

#ifdef RAPH_IS_HOME
  drawing_area_data = preview_data;
  shape_drawing_area_data = preview_data;
#endif
}


static void
total_alpha_preview (guchar* ptr)
{
  int i;

  if (shaping)
    {
      memset(shape_preview_mask, 0, (width*height)/8 + height);
    }
  else
    {
      for (i=0;i<height;i++)
	{
	  if (i&8)
	    memcpy(&ptr[i*3*width], preview_alpha1_data, 3*width);
	  else
	    memcpy(&ptr[i*3*width], preview_alpha2_data, 3*width);
	}
    }
}



/* Util. */

static void
remove_timer (void)
{
  if (timer)
    {
      gtk_timeout_remove (timer);
      timer = 0;
    }
}

static void
do_step (void)
{
  frame_number = (frame_number+1)%total_frames;
  render_frame(frame_number);
}


/*  Callbacks  */

static gint
window_delete_callback (GtkWidget *widget,
		        GdkEvent  *event,
		        gpointer   data)
{
  if (playing)
    playstop_callback(NULL, NULL);

  if (shape_window)
    gtk_widget_destroy(GTK_WIDGET(shape_window));

  gdk_flush();
  gtk_main_quit();

  return FALSE;
}

static void
window_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  if (data)
    gtk_widget_destroy(GTK_WIDGET(data));

  window_delete_callback (NULL, NULL, NULL);
}

static gint
advance_frame_callback (GtkWidget *widget,
			gpointer   data)
{
  remove_timer();

  timer = gtk_timeout_add (get_frame_duration((frame_number+1)%total_frames),
			   (GtkFunction) advance_frame_callback, NULL);

  do_step();
  show_frame();

  return FALSE;
}

static void
playstop_callback (GtkWidget *widget,
		   gpointer   data)
{
  if (!playing)
    { /* START PLAYING */
      playing = TRUE;
      timer = gtk_timeout_add (get_frame_duration(frame_number),
			       (GtkFunction) advance_frame_callback, NULL);
    }
  else
    { /* STOP PLAYING */
      playing = FALSE;
      remove_timer();
    }
}

static void
rewind_callback (GtkWidget *widget,
		 gpointer   data)
{
  if (playing)
    {
      playstop_callback(NULL, NULL); /* GTK weirdness workaround */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psbutton), FALSE);
      playing = FALSE;
      remove_timer();
    }

  frame_number = 0;
  render_frame(frame_number);
  show_frame();
}

static void
step_callback (GtkWidget *widget,
	       gpointer   data)
{
  if (playing)
    {
      playstop_callback(NULL, NULL); /* GTK weirdness workaround */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psbutton), FALSE);
      playing = FALSE;
      remove_timer();
    }

  do_step();
  show_frame();
}





/* tag util. */

static DisposeType
get_frame_disposal (const guint whichframe)
{
  gchar* layer_name;
  DisposeType disposal;
  
  layer_name = gimp_layer_get_name(layers[total_frames-(whichframe+1)]);
  disposal = parse_disposal_tag(layer_name);
  g_free(layer_name);

  return(disposal);
}



static guint32
get_frame_duration (const guint whichframe)
{
  gchar* layer_name;
  gint   duration = 0;

  layer_name = gimp_layer_get_name(layers[total_frames-(whichframe+1)]);
  if (layer_name != NULL)
    {
      duration = parse_ms_tag(layer_name);
      g_free(layer_name);
    }
  
  if (duration < 0) duration = 125;  /* FIXME for default-if-not-said  */
  if (duration == 0) duration = 125; /* FIXME - 0-wait is nasty */

  return ((guint32) duration);
}


static int
is_ms_tag (const char *str, int *duration, int *taglength)
{
  gint sum = 0;
  gint offset;
  gint length;

  length = strlen(str);

  if (str[0] != '(')
    return 0;

  offset = 1;

  /* eat any spaces between open-parenthesis and number */
  while ((offset<length) && (str[offset] == ' '))
    offset++;
  
  if ((offset>=length) || (!isdigit(str[offset])))
    return 0;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset<length) && (isdigit(str[offset])));  

  if (length-offset <= 2)
    return 0;

  /* eat any spaces between number and 'ms' */
  while ((offset<length) && (str[offset] == ' '))
    offset++;

  if ((length-offset <= 2) ||
      (toupper(str[offset]) != 'M') || (toupper(str[offset+1]) != 'S'))
    return 0;

  offset += 2;

  /* eat any spaces between 'ms' and close-parenthesis */
  while ((offset<length) && (str[offset] == ' '))
    offset++;

  if ((length-offset < 1) || (str[offset] != ')'))
    return 0;

  offset++;
  
  *duration = sum;
  *taglength = offset;

  return 1;
}


static int
parse_ms_tag (const char *str)
{
  int i;
  int rtn;
  int dummy;
  int length;

  length = strlen(str);

  for (i=0; i<length; i++)
    {
      if (is_ms_tag(&str[i], &rtn, &dummy))
	return rtn;
    }
  
  return -1;
}


static int
is_disposal_tag (const char *str, DisposeType *disposal, int *taglength)
{
  if (strlen(str) != 9)
    return 0;
  
  if (strncmp(str, "(combine)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_COMBINE;
      return 1;
    }
  else if (strncmp(str, "(replace)", 9) == 0)
    {
      *taglength = 9;
      *disposal = DISPOSE_REPLACE;
      return 1;
    }

  return 0;
}


static DisposeType
parse_disposal_tag (const char *str)
{
  DisposeType rtn;
  int i, dummy;
  gint length;

  length = strlen(str);

  for (i=0; i<length; i++)
    {
      if (is_disposal_tag(&str[i], &rtn, &dummy))
	{
	  return rtn;
	}
    }

  return (DISPOSE_UNDEFINED); /* FIXME */
}


