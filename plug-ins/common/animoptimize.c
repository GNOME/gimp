/*
 * Animation Optimizer plug-in version 0.61.0
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
 * 98.03.16 : version 0.61.0
 *            Support more rare opaque/transparent combinations.
 *
 * 97.12.09 : version 0.60.0
 *            Added support for INDEXED* and GRAY* images.
 *
 * 97.12.09 : version 0.52.0
 *            Fixed some bugs.
 *
 * 97.12.08 : version 0.51.0
 *            Relaxed bounding box on optimized layers marked
 *            'replace'.
 *
 * 97.12.07 : version 0.50.0
 *            Initial release.
 */

/*
 * BUGS:
 *  Probably a few
 */

/*
 * TODO:
 *   User interface
 *   PDB interface
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

static        void do_optimizations   (void);
/*static         int parse_ms_tag       (char *str);*/
static DisposeType parse_disposal_tag (char *str);

static void window_close_callback  (GtkWidget *widget,
				    gpointer   data);

static DisposeType  get_frame_disposal  (guint whichframe);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};




/* Global widgets'n'stuff */
GtkWidget* progress;
guint      width,height;
gint32     image_id;
gint32     new_image_id;
gint32     total_frames;
guint      frame_number;
gint32*    layers;
GDrawable* drawable;
gboolean   playing = FALSE;
GImageType imagetype;
GDrawableType drawabletype_alpha;
guchar     pixelstep;
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

  gimp_install_procedure("plug_in_animationoptimize",
			 "This plugin applies various optimizations to"
			 " a GIMP layer-based animation.",
			 "",
			 "Adam D. Moss <adam@gimp.org>",
			 "Adam D. Moss <adam@gimp.org>",
			 "1997",
			 "<Image>/Filters/Animation/Animation Optimize",
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
  
  if (run_mode == RUN_NONINTERACTIVE)
    {
      if (n_params != 3)
	{
	  status = STATUS_CALLING_ERROR;
	}
    }
  
  if (status == STATUS_SUCCESS)
    {
      image_id = param[1].data.d_image;
      
      do_optimizations();
      
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush();
    }
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}


#if 0 /* function is presently not used */

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

#endif /* parse_ms_tag */

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


#if 0 /* function is presently unused */

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
  /*  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());*/


  dlg = gtk_dialog_new ();
  windowname = g_malloc(strlen("Animation Playback: ")+strlen(imagename)+1);
  strcpy(windowname,"Animation Playback: ");
  strcat(windowname,imagename);
  gtk_window_set_title (GTK_WINDOW (dlg), windowname);
  g_free(windowname);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) window_close_callback,
		      NULL);

  
  /* Action area - 'close' button only. */

  button = gtk_button_new_with_label ("Close");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) window_close_callback,
			     NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  

  {
    /* The 'playback' half of the dialog */
    
    /*    windowname = g_malloc(strlen("Playback: ")+strlen(imagename)+1);
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
	  progress = gtk_progress_bar_new ();
	  gtk_widget_set_usize (progress, 150, 15);
	  gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, TRUE, 0);
	  gtk_widget_show (progress);

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
	  if (total_frames<=1) gtk_widget_set_sensitive (hbox2, FALSE);
	  gtk_widget_show(hbox2);

	  frame2 = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_ETCHED_IN);
	  gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, FALSE, 0);
	    
	  {
	    preview = gtk_preview_new (GTK_PREVIEW_COLOR);
	    gtk_preview_size (GTK_PREVIEW (preview), width, height);
	    gtk_container_add (GTK_CONTAINER (frame2), preview);
	    gtk_widget_show(preview);
	  }
	  gtk_widget_show(frame2);
	}
	gtk_widget_show(vbox);
	
      }
      gtk_widget_show(hbox);
      
    }
    gtk_widget_show(frame);
    */ 
  }
  gtk_widget_show(dlg);
}

#endif /* build_dialog */


/* Rendering Functions */

static void
total_alpha(guchar* imdata, guint32 numpix, guchar bytespp)
{
  /* Set image to total-transparency w/black
   */

  memset(imdata, 0, numpix*bytespp);
}

static void
do_optimizations(void)
{
  GPixelRgn pixel_rgn;
  static guchar* rawframe = NULL;
  static gint rawwidth=0, rawheight=0, rawbpp=0;
  gint rawx=0, rawy=0;
  guchar* srcptr;
  guchar* destptr;
  gint i,j,this_frame_num;
  guint32 frame_sizebytes;
  gint32 new_layer_id;
  DisposeType dispose;
  guchar* this_frame = NULL;
  guchar* last_frame = NULL;
  guchar* opti_frame = NULL;
  
  gboolean can_combine;

  gint32 bbox_top, bbox_bottom, bbox_left, bbox_right;
  gint32 rbox_top, rbox_bottom, rbox_left, rbox_right;

  width     = gimp_image_width(image_id);
  height    = gimp_image_height(image_id);
  layers    = gimp_image_get_layers (image_id, &total_frames);
  imagetype = gimp_image_base_type(image_id);
  pixelstep = (imagetype == RGB) ? 4 : 2;

  drawabletype_alpha = (imagetype == RGB) ? RGBA_IMAGE :
    ((imagetype == INDEXED) ? INDEXEDA_IMAGE : GRAYA_IMAGE);

  frame_sizebytes = width * height * pixelstep;

  this_frame = g_malloc(frame_sizebytes);
  last_frame = g_malloc(frame_sizebytes);
  opti_frame = g_malloc(frame_sizebytes);

  total_alpha(this_frame, width*height, pixelstep);
  total_alpha(last_frame, width*height, pixelstep);

  new_image_id = gimp_image_new(width, height, imagetype);

  if (imagetype == INDEXED)
    {
      palette = gimp_image_get_cmap(image_id, &ncolours);
      gimp_image_set_cmap(new_image_id, palette, ncolours);
    }

  if ( (this_frame == NULL) || (last_frame == NULL) || (opti_frame == NULL) )
    g_error("Not enough memory to allocate buffers for optimization.\n");

  for (this_frame_num=0; this_frame_num<total_frames; this_frame_num++)
    {
      /*
       * We actually unoptimize the animation before re-optimizing it,
       *  otherwise the logic can get a bit too hard... for me.
       */

      /*
       *
       * BUILD THIS FRAME
       *
       */

      drawable = gimp_drawable_get (layers[total_frames-(this_frame_num+1)]);

      /* Image has been closed/etc since we got the layer list? */
      /* FIXME - How do we tell if a gimp_drawable_get() fails? */
      if (gimp_drawable_width(drawable->id)==0)
	{
	  window_close_callback(NULL, NULL);
	}

      dispose = get_frame_disposal(this_frame_num);

      if (dispose==DISPOSE_REPLACE)
	{
	  total_alpha(this_frame, width*height, pixelstep);
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
		{ /* alpha RGB, same size */
		  destptr = this_frame;
		  srcptr  = rawframe;
	      
		  i = rawwidth*rawheight;
		  while (--i)
		    {
		      if (!((*(srcptr+3))&128))
			{
			  srcptr  += 4;
			  destptr += 4;
			  continue;
			}
		      *(destptr++) = *(srcptr++);
			  *(destptr++) = *(srcptr++);
			  *(destptr++) = *(srcptr++);
			  *(destptr++) = 255;
			  srcptr++;
		    }
		}
	      else /* RGB no alpha, same size */
		{
		  destptr = this_frame;
		  srcptr  = rawframe;
	      
		  i = rawwidth*rawheight;
		  while (--i)
		    {
		      *(destptr++) = *(srcptr++);
		      *(destptr++) = *(srcptr++);
		      *(destptr++) = *(srcptr++);
		      *(destptr++) = 255;
		    }		  
		}
	    }
	  else
	    {
	      /* --- These are suboptimal catch-all cases for when  --- */
	      /* --- this frame is bigger/smaller than the preview  --- */
	      /* --- buffer, and/or offset within it.               --- */
	  
	      /* FIXME: FINISH ME! */
	  
	      if (gimp_drawable_has_alpha (drawable->id))
		{ /* RGB alpha, diff size */
	      
		  destptr = this_frame;
		  srcptr = rawframe;
	      
		  for (j=rawy; j<rawheight+rawy; j++)
		    {
		      for (i=rawx; i<rawwidth+rawx; i++)
			{
			  if ((i>=0 && i<width) &&
			      (j>=0 && j<height))
			    {
			      if ((*(srcptr+3))&128)
				{
				  this_frame[(j*width+i)*4   ] = *(srcptr);
				  this_frame[(j*width+i)*4 +1] = *(srcptr+1);
				  this_frame[(j*width+i)*4 +2] = *(srcptr+2);
				  this_frame[(j*width+i)*4 +3] = 255;
				}
			    }
		      
			  srcptr += 4;
			}
		    }
		}
	      else
		{
		  /* RGB no alpha, diff size */
	      
		  destptr = this_frame;
		  srcptr = rawframe;
	      
		  for (j=rawy; j<rawheight+rawy; j++)
		    {
		      for (i=rawx; i<rawwidth+rawx; i++)
			{
			  if ((i>=0 && i<width) &&
			      (j>=0 && j<height))
			    {
			      this_frame[(j*width+i)*4   ] = *(srcptr);
			      this_frame[(j*width+i)*4 +1] = *(srcptr+1);
			      this_frame[(j*width+i)*4 +2] = *(srcptr+2);
			      this_frame[(j*width+i)*4 +3] = 255;
			    }
		      
			  srcptr += 3;
			}
		    }
		}
	    }
	  break; /* case RGB */

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
		{ /* I alpha, same size */
		  destptr = this_frame;
		  srcptr  = rawframe;
	      
		  i = rawwidth*rawheight;
		  while (--i)
		    {
		      if (!(*(srcptr+1)))
			{
			  srcptr  += 2;
			  destptr += 2;
			  continue;
			}
		      *(destptr++) = *(srcptr);
			  *(destptr++) = 255;
			  srcptr+=2;
		    }
		}
	      else /* I, no alpha, same size */
		{
		  destptr = this_frame;
		  srcptr  = rawframe;
	      
		  i = rawwidth*rawheight;
		  while (--i)
		    {
		      *(destptr++) = *(srcptr);
		      *(destptr++) = 255;
		      srcptr++;
		    }
		}
	    }
	  else
	    {
	      /* --- These are suboptimal catch-all cases for when  --- */
	      /* --- this frame is bigger/smaller than the preview  --- */
	      /* --- buffer, and/or offset within it.               --- */
	  
	      /* FIXME: FINISH ME! */
	  
	      if (gimp_drawable_has_alpha (drawable->id))
		{ /* I alpha, diff size */
	      
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
				  this_frame[(j*width+i)*pixelstep] =
				    *srcptr;
				  this_frame[(j*width+i)*pixelstep
					    + pixelstep - 1] =
				    255;
				}
			    }
		      
			  srcptr += 2;
			}
		    }
		}
	      else
		{
		  /* I, no alpha, diff size */
	      
		  srcptr = rawframe;
	      
		  for (j=rawy; j<rawheight+rawy; j++)
		    {
		      for (i=rawx; i<rawwidth+rawx; i++)
			{
			  if ((i>=0 && i<width) &&
			      (j>=0 && j<height))
			    {
			      this_frame[(j*width+i)*pixelstep]
				= *srcptr;
			      this_frame[(j*width+i)*pixelstep
					+ pixelstep - 1] = 255;
			    }
		      
			  srcptr ++;
			}
		    }
		}
	    }
	  break; /* case INDEXED/GRAY */

	}
  
      /* clean up */
      gimp_drawable_detach(drawable);



      can_combine = FALSE;
      bbox_left = 0;
      bbox_top = 0;
      bbox_right = width;
      bbox_bottom = height;
      rbox_left = 0;
      rbox_top = 0;
      rbox_right = width;
      rbox_bottom = height;
      /* copy 'this' frame into a buffer which we can safely molest */
      memcpy(opti_frame, this_frame, frame_sizebytes);
      /*
       *
       * OPTIMIZE HERE!
       *
       */
      if (this_frame_num != 0) /* Can't delta bottom frame! */
	{
	  int xit, yit, byteit;

	  can_combine = TRUE;

	  /*
	   * SEARCH FOR BOUNDING BOX
	   */
	  bbox_left = width;
	  bbox_top = height;
	  bbox_right = 0;
	  bbox_bottom = 0;
	  rbox_left = width;
	  rbox_top = height;
	  rbox_right = 0;
	  rbox_bottom = 0;
	  
	  for (yit=0; yit<height; yit++)
	    {
	      for (xit=0; xit<width; xit++)
		{
		  gboolean keep_pix;
		  gboolean opaq_pix;
		  /* Check if 'this' and 'last' are transparent */
		  if (!(this_frame[yit*width*pixelstep + xit*pixelstep
				  + pixelstep-1]&128)
		      &&
		      !(last_frame[yit*width*pixelstep + xit*pixelstep
				  + pixelstep-1]&128))
		    {
		      keep_pix = FALSE;
		      opaq_pix = FALSE;
		      goto decided;
		    }
		  /* Check if just 'this' is transparent */
		  if ((last_frame[yit*width*pixelstep + xit*pixelstep
				 + pixelstep-1]&128)
		      &&
		      !(this_frame[yit*width*pixelstep + xit*pixelstep
				  + pixelstep-1]&128))
		    {
		      keep_pix = TRUE;
		      opaq_pix = FALSE;
		      can_combine = FALSE;
		      goto decided;
		    }
		  /* Check if just 'last' is transparent */
		  if (!(last_frame[yit*width*pixelstep + xit*pixelstep
				  + pixelstep-1]&128)
		      &&
		      (this_frame[yit*width*pixelstep + xit*pixelstep
				 + pixelstep-1]&128))
		    {
		      keep_pix = TRUE;
		      opaq_pix = TRUE;
		      goto decided;
		    }
		  /* If 'last' and 'this' are opaque, we have
		   *  to check if they're the same colour - we
		   *  only have to keep the pixel if 'last' or
		   *  'this' are opaque and different.
		   */
		  keep_pix = FALSE;
		  opaq_pix = TRUE;
		  for (byteit=0; byteit<pixelstep-1; byteit++)
		    {
		      if (last_frame[yit*width*pixelstep + xit*pixelstep
				    + byteit]
			  !=
			  this_frame[yit*width*pixelstep + xit*pixelstep
				    + byteit])
			{
			  keep_pix = TRUE;
			  goto decided;
			}			    
		    }
		decided:
		  if (opaq_pix)
		    {
		      if (xit<rbox_left) rbox_left=xit;
		      if (xit>rbox_right) rbox_right=xit;
		      if (yit<rbox_top) rbox_top=yit;
		      if (yit>rbox_bottom) rbox_bottom=yit;
		    }
		  if (keep_pix)
		    {
		      if (xit<bbox_left) bbox_left=xit;
		      if (xit>bbox_right) bbox_right=xit;
		      if (yit<bbox_top) bbox_top=yit;
		      if (yit>bbox_bottom) bbox_bottom=yit;
		    }
		  else
		    {
		      /* pixel didn't change this frame - make
		       *  it transparent in our optimized buffer!
		       */
		      opti_frame[yit*width*pixelstep + xit*pixelstep
				+ pixelstep-1] = 0;
		    }
		} /* xit */
	    } /* yit */

	  if (!can_combine)
	    {
	      bbox_left = rbox_left;
	      bbox_top = rbox_top;
	      bbox_right = rbox_right;
	      bbox_bottom = rbox_bottom;
	    }

	  bbox_right++;
	  bbox_bottom++;

	  /*
	   * Collapse opti_frame data down such that the data
	   *  which occupies the bounding box sits at the start
	   *  of the data (for convenience with ..set_rect()).
	   */
	  destptr = opti_frame;
	  /*
	   * If can_combine, then it's safe to use our optimized
	   *  alpha information.  Otherwise, an opaque pixel became
	   *  transparent this frame, and we'll have to use the
	   *  actual true frame's alpha.
	   */
	  if (can_combine)
	    srcptr = opti_frame;
	  else
	    srcptr = this_frame;
	  for (yit=bbox_top; yit<bbox_bottom; yit++)
	    {
	      for (xit=bbox_left; xit<bbox_right; xit++)
		{
		  for (byteit=0; byteit<pixelstep; byteit++)
		    {
		      *(destptr++) = srcptr[yit*pixelstep*width +
					   pixelstep*xit + byteit];
		    }
		}
	    }
	} /* !bot frame? */
      else
	{
	  memcpy(opti_frame, this_frame, frame_sizebytes);
	}

      /*
       *
       * REMEMBER THE ANIMATION STATUS TO DELTA AGAINST NEXT TIME
       *
       */
      memcpy(last_frame, this_frame, frame_sizebytes);

      
      /*
       *
       * PUT THIS FRAME INTO A NEW LAYER IN THE NEW IMAGE
       *
       */

      new_layer_id = gimp_layer_new(new_image_id,
				    can_combine?"(combine)":"(replace)",
				    bbox_right-bbox_left,
				    bbox_bottom-bbox_top,
				    drawabletype_alpha,
				    100.0,
				    NORMAL_MODE);

      gimp_image_add_layer (new_image_id, new_layer_id, 0);

      drawable = gimp_drawable_get (new_layer_id);

      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
			   bbox_right-bbox_left,
			   bbox_bottom-bbox_top,
			   TRUE, FALSE);
      gimp_pixel_rgn_set_rect (&pixel_rgn, opti_frame, 0, 0,
			       bbox_right-bbox_left,
			       bbox_bottom-bbox_top);
      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);     
      gimp_layer_translate (new_layer_id, (gint)bbox_left, (gint)bbox_top); 
    }

  gimp_display_new (new_image_id);

  g_free(rawframe);
  rawframe = NULL;
  g_free(last_frame);
  last_frame = NULL;
  g_free(this_frame);
  this_frame = NULL;
  g_free(opti_frame);
  opti_frame = NULL;
}




/* Util. */

#if 0    /* function is presently unused */

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

#endif /* get_frame_duration */

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
  gtk_main_quit();
}

