/*
 * Animation Playback plug-in version 0.84.0
 *
 * by Adam D. Moss, 1997-98
 *     adam@gimp.org
 *     adam@foxbox.org
 *
 * This is part of the GIMP package and falls under the GPL.
 */

/*
 * REVISION HISTORY:
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
 *  Leaks memory.  Not sure why.
 *  Gets understandably upset if the source image is deleted
 *    while the animation is playing.
 *  Decent solutions to either of the above are most welcome.
 *
 *  Any more?  Let me know!
 */

/*
 * TODO:
 *  pdb interface - should we bother?
 *  speedups (caching?  most bottlenecks seem to be in pixelrgns)
 *  write other half of the user interface
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"



typedef enum
{
  DISPOSE_UNDEFINED = 0x00,
  DISPOSE_COMBINE   = 0x01,
  DISPOSE_REPLACE   = 0x02
} DisposeType;



/* Declare local functions. */
static void query(void);
static void run(char *name,
		int nparams,
		GParam * param,
		int *nreturn_vals,
		GParam ** return_vals);

static        void do_playback        (void);
static         int parse_ms_tag       (char *str);
static DisposeType parse_disposal_tag (char *str);

static void window_close_callback  (GtkWidget *widget,
				    gpointer   data);
static void playstop_callback  (GtkWidget *widget,
				gpointer   data);
static void rewind_callback  (GtkWidget *widget,
			      gpointer   data);
static void step_callback  (GtkWidget *widget,
			    gpointer   data);

static DisposeType  get_frame_disposal  (guint whichframe);
static void         render_frame        (gint32 whichframe);
static void         show_frame          (void);
static void         total_alpha_preview (void);
static void         init_preview_misc   (void);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};




/* Global widgets'n'stuff */
guchar*    preview_data;
static     GtkPreview* preview = NULL;
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




MAIN()

static void query()
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_animationplay",
			 "This plugin allows you to preview a GIMP layer-based animation.",
			 "",
			 "Adam D. Moss <adam@gimp.org>",
			 "Adam D. Moss <adam@gimp.org>",
			 "1997",
			 "<Image>/Filters/Animation/Animation Playback",
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
		GParam ** return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == RUN_NONINTERACTIVE) {
    if (n_params != 3) {
      status = STATUS_CALLING_ERROR;
    }
  }

  if (status == STATUS_SUCCESS) {

    image_id = param[1].data.d_image;

    do_playback();
    
    if (run_mode != RUN_NONINTERACTIVE)
      gimp_displays_flush();
  }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}



static int
parse_ms_tag (char *str)
{
  gint sum = 0;
  gint offset = 0;
  gint length;

  length = strlen(str);

  while ((offset<length) && (str[offset]!='('))
    offset++;
  
  if (offset>=length)
    return(-1);

  if (!isdigit(str[++offset]))
    return(-2);

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

  return (DISPOSE_UNDEFINED); /* FIXME */
}




static void
build_dialog(GImageType basetype,
	     char*      imagename)
{
  gchar** argv;
  gint argc;

  gchar* windowname;

  GtkWidget* dlg;
  GtkWidget* button;
  GtkWidget* toggle;
  GtkWidget* label;
  GtkWidget* entry;
  GtkWidget* frame;
  GtkWidget* frame2;
  GtkWidget* vbox;
  GtkWidget* vbox2;
  GtkWidget* hbox;
  GtkWidget* hbox2;
  guchar* color_cube;
  GSList* group = NULL;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("animationplay");
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());


  dlg = gtk_dialog_new ();
  windowname = g_malloc(strlen("Animation Playback: ")+strlen(imagename)+1);
  strcpy(windowname,"Animation Playback: ");
  strcat(windowname,imagename);
  gtk_window_set_title (GTK_WINDOW (dlg), windowname);
  g_free(windowname);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) window_close_callback,
		      dlg);

  
  /* Action area - 'close' button only. */

  button = gtk_button_new_with_label ("Close");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) window_close_callback,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  

  {
    /* The 'playback' half of the dialog */
    
    windowname = g_malloc(strlen("Playback: ")+strlen(imagename)+1);
    strcpy(windowname,"Playback: ");
    strcat(windowname,imagename);
    frame = gtk_frame_new (windowname);
    g_free(windowname);
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
	  progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
	  gtk_widget_set_usize (GTK_WIDGET (progress), 150, 15);
	  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (progress),
			      TRUE, TRUE, 0);
	  gtk_widget_show (GTK_WIDGET (progress));

	  hbox2 = gtk_hbox_new (FALSE, 0);
	  gtk_container_border_width (GTK_CONTAINER (hbox2), 0);
	  gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);
	  
	  {
	    button = gtk_button_new_with_label ("Play/Stop");
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				(GtkSignalFunc) playstop_callback, NULL);
	    gtk_box_pack_start (GTK_BOX (hbox2), button, TRUE, TRUE, 0);
	    gtk_widget_show (button);
	    
	    button = gtk_button_new_with_label ("Rewind");
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				(GtkSignalFunc) rewind_callback, NULL);
	    gtk_box_pack_start (GTK_BOX (hbox2), button, TRUE, TRUE, 0);
	    gtk_widget_show (button);
	    
	    button = gtk_button_new_with_label ("Step");
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				(GtkSignalFunc) step_callback, NULL);
	    gtk_box_pack_start (GTK_BOX (hbox2), button, TRUE, TRUE, 0);
	    gtk_widget_show (button);
	  }
	  /* If there aren't multiple frames, playback controls make no
	     sense */
	  if (total_frames<=1) gtk_widget_set_sensitive (hbox2, FALSE);
	  gtk_widget_show(hbox2);

	  frame2 = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_ETCHED_IN);
	  gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, FALSE, 0);
	    
	  {
	    preview =
	      GTK_PREVIEW (gtk_preview_new (GTK_PREVIEW_COLOR)); /* FIXME */
	    gtk_preview_size (preview, width, height);
	    gtk_container_add (GTK_CONTAINER (frame2), GTK_WIDGET (preview));
	    gtk_widget_show(GTK_WIDGET (preview));
	  }
	  gtk_widget_show(frame2);
	}
	gtk_widget_show(vbox);
	
      }
      gtk_widget_show(hbox);
      
    }
    gtk_widget_show(frame);
    
  }
  gtk_widget_show(dlg);
}



static void do_playback(void)
{
  GPixelRgn srcPR, destPR;
  guchar *buffer;
  int nreturn_vals, i;

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
  total_alpha_preview();
  for (i=0;i<height;i++)
  {
    gtk_preview_draw_row (preview,
                          &preview_data[3*i*width],
                          0, i, width);
  }
  
  render_frame(0);
  show_frame();

  gtk_main ();
  gdk_flush ();
}


/* Rendering Functions */

static void
render_frame(gint32 whichframe)
{
  GPixelRgn pixel_rgn;
  static guchar *rawframe = NULL;
  static gint rawwidth=0, rawheight=0, rawbpp=0;
  gint rawx=0, rawy=0;
  guchar* srcptr;
  guchar* destptr;
  gint i,j;
  DisposeType dispose;


  if (whichframe >= total_frames)
    {
      printf("playback: Asked for frame number %d in a %d-frame animation!\n",
	     (int) (whichframe+1), (int) total_frames);
      exit(-1);
    }

  drawable = gimp_drawable_get (layers[total_frames-(whichframe+1)]);

  dispose = get_frame_disposal(frame_number);

  /* Image has been closed/etc since we got the layer list? */
  /* FIXME - How do we tell if a gimp_drawable_get() fails? */
  if (gimp_drawable_width(drawable->id)==0)
    {
      window_close_callback(NULL, NULL);
    }

  if (((dispose==DISPOSE_REPLACE)||(whichframe==0)) &&
      gimp_drawable_has_alpha(drawable->id))
    {
      total_alpha_preview();
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
	    }
	  else /* no alpha */
	    {
	      if ((rawwidth==width)&&(rawheight==height))
		{
		  /*printf("quickie\n");fflush(stdout);*/
		  memcpy(preview_data, rawframe, width*height*3);
		}
	    }
	  /* Display the preview buffer... finally. */
	  for (i=0;i<height;i++)
	    {
	      gtk_preview_draw_row (preview,
				    &preview_data[3*i*width],
				    0, i, width);
	    }
	}
      else
	{
	  /* --- These are suboptimal catch-all cases for when  --- */
	  /* --- this frame is bigger/smaller than the preview  --- */
	  /* --- buffer, and/or offset within it.               --- */
	  
	  /* FIXME: FINISH ME! */
	  
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
	    }
	  else
	    {
	      /* noalpha */
	    }
	  
	  /* Display the preview buffer... finally. */
	  if (dispose!=DISPOSE_REPLACE)
	    {
	      for (i=rawy;i<rawy+rawheight;i++)
		{
		  if (i>=0 && i<height)
		    gtk_preview_draw_row (preview,
					  &preview_data[3*i*width],
					  0, i, width);
		}
	    }
	  else
	    {
	      for (i=0;i<height;i++)
		{
		  gtk_preview_draw_row (preview,
					&preview_data[3*i*width],
					0, i, width);
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
	    }
	  
	  /* Display the preview buffer... finally. */
	  for (i=0;i<height;i++)
	    {
	      gtk_preview_draw_row (preview,
				    &preview_data[3*i*width],
				    0, i, width);
	    }
	}
      else
	{
	  /* --- These are suboptimal catch-all cases for when  --- */
	  /* --- this frame is bigger/smaller than the preview  --- */
	  /* --- buffer, and/or offset within it.               --- */
	  
	  /* FIXME: FINISH ME! */
	  
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
	    }
	  else
	    {
	      /* noalpha */
	    }
	  
	  /* Display the preview buffer... finally. */
	  if (dispose!=DISPOSE_REPLACE)
	    {
	      for (i=rawy;i<rawy+rawheight;i++)
		{
		  if (i>=0 && i<height)
		    gtk_preview_draw_row (preview,
					  &preview_data[3*i*width],
					  0, i, width);
		}
	    }
	  else
	    {
	      for (i=0;i<height;i++)
		{
		  gtk_preview_draw_row (preview,
					&preview_data[3*i*width],
					0, i, width);
		}
	    }
	}
      break;
      
    }


  
  /* clean up */
  
  gimp_drawable_detach(drawable);
}


static void
show_frame(void)
{
  /* Tell GTK to physically draw the preview */
  gtk_widget_draw (GTK_WIDGET (preview), NULL);
  gdk_flush ();

  /* update the dialog's progress bar */
  gtk_progress_bar_update (progress,
			   ((float)frame_number/(float)(total_frames-0.999)));
}


static void
init_preview_misc(void)
{
  int i;

  preview_data = g_malloc(width*height*3);
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
}


static void
total_alpha_preview(void)
{
  int i;

  for (i=0;i<height;i++)
    {
      if (i&8)
	memcpy(&preview_data[i*3*width], preview_alpha1_data, 3*width);
      else
	memcpy(&preview_data[i*3*width], preview_alpha2_data, 3*width);
    }
}



/* Util. */

static void
remove_timer(void)
{
  if (timer)
    {
      gtk_timeout_remove (timer);
      timer = 0;
    }
}

static void
do_step(void)
{
  frame_number = (frame_number+1)%total_frames;
  render_frame(frame_number);
}

static guint32
get_frame_duration (guint whichframe)
{
  gchar* layer_name;
  gint   duration;

  layer_name = gimp_layer_get_name(layers[total_frames-(whichframe+1)]);
  duration = parse_ms_tag(layer_name);
  g_free(layer_name);

  if (duration < 0) duration = 100; /* FIXME for default-if-not-said  */
  if (duration == 0) duration = 60; /* FIXME - 0-wait is nasty */

  return ((guint32) duration);
}

static DisposeType
get_frame_disposal (guint whichframe)
{
  gchar* layer_name;
  DisposeType disposal;
  
  layer_name = gimp_layer_get_name(layers[total_frames-(whichframe+1)]);
  disposal = parse_disposal_tag(layer_name);
  g_free(layer_name);

  return(disposal);
}



/*  Callbacks  */

static void
window_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
  gtk_main_quit();
}

static gint
advance_frame_callback (GtkWidget *widget,
			gpointer   data)
{
  remove_timer();
  timer = gtk_timeout_add (get_frame_duration(frame_number),
			   (GtkFunction) advance_frame_callback, NULL);
  show_frame();
  do_step();

  return FALSE;
}

static void
playstop_callback (GtkWidget *widget,
		   gpointer   data)
{
  if (!playing)
    { /* START PLAYING */
      playing = TRUE;
      timer = gtk_timeout_add (0, (GtkFunction) advance_frame_callback, NULL);
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
  playing = FALSE;
  remove_timer();

  frame_number = 0;
  render_frame(frame_number);
  show_frame();
}

static void
step_callback (GtkWidget *widget,
	       gpointer   data)
{
  playing = FALSE;
  remove_timer();

  do_step();
  show_frame();
}

