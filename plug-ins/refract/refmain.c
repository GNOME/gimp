/* refmain.c, 1/2/98 - this file contains startup routine and dialogs.
 * refract: A plug-in for the GIMP 0.99.
 * Uses a height field as a lens of specified refraction index.
 *
 * by Kevin Turner <kevint@poboxes.com>
 * http://www.poboxes.com/kevint/gimp/refract.html
 */

/* I require megawidgets to compile!  A copy was probably compiled in
   the plug-ins directory of your GIMP source distribution, it will
   work nicely.  Just move me or it somewhere I can see it...  */

/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#ifdef REFRACT_DEBUG
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#endif /* DEBUG */

#include <stdlib.h>
#include <math.h>      /* It's not clear to me if this needs be here or no... */
#include "refract.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h" 

/* megawidget.h could be in any of several places relative to us... */
/* should this be an autoconf thing? */
#ifdef HAVE_CONFIG_H /* We're part of the GIMP distribution. */
#include <plug-ins/megawidget/megawidget.h>
#else
#include "megawidget.h"
/* #include <megawidget.h> */
#endif

typedef struct {
    gint run;
} RefractInterface;

/* go_refract is in refguts.c */
extern void      go_refract(GDrawable *drawable, 
			    gint32 image_id);
static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static gint      refract_dialog();
static gint      map_constrain(gint32 image_id, 
			       gint32 drawable_id,
			       gpointer data);
static void      newl_toggle_callback (GtkWidget *widget, 
				       gpointer   data);
static void      tooltips_toggle_callback (GtkWidget *widget, 
					   gpointer   data);
static void      refract_close_callback(GtkWidget *widget, 
					gpointer data);
static void      refract_ok_callback(GtkWidget *widget, 
				     gpointer data);
static void      map_menu_callback (gint32 id, 
				    gpointer data);
static GtkWidget* ior_menu_new(GtkWidget *tieto);
static void       ior_menu_callback (GtkWidget *widget, 
				    gfloat *data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* refractvals defined in refract.h */
/* not static, used in refguts.c */
RefractValues refractvals = 
{
    -1,     /* Lens map ID */
    32,     /* lens thickness */
    0,      /* distance */
    1.0003, /* index a */
    1.333,  /* index b */
    WRAP,   /* wrap behaviour */
    FALSE,  /* new layer? */
    0,      /* offset x */
    0,      /* offset y */
};

static RefractInterface refractint =
{
    FALSE  /* run */
};

MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    /* If we did have parameters, these be them: */
    { PARAM_DRAWABLE, "lensmap", "Lens map drawable" },
    { PARAM_INT32, "thick", "Lens thickness" },
    { PARAM_INT32, "dist", "Lens distance from image" },
    { PARAM_FLOAT, "na", "Index of Refraction A" },
    { PARAM_FLOAT, "nb", "Index of Refraction B" },
    { PARAM_INT32, "edge", "Background (0), Outside (1), Wrap (2)" },
    { PARAM_INT32, "newl", "New layer?" },
    { PARAM_INT32, "xofs", "X offset" },
    { PARAM_INT32, "yofs", "Y offset" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_refract",
			  "Uses a height field as a lens.",
			  "Distorts the image by refracting it through a height field 'lens' with a specified index of refraction.",
			  "Kevin Turner <kevint@poboxes.com>",
			  "Kevin Turner",
			  "1997",
			  "<Image>/Filters/Glass Effects/Refract",
			  "RGB*, GRAY*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
} /* query */

static void
run    (gchar    *name,
	gint      nparams,
	GParam   *param,
	gint     *nreturn_vals,
	GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

#ifdef REFRACT_DEBUG
  printf("refract: pid %d\n", getpid());
#endif

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode) {
  case RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_refract", &refractvals);
      
      /* Acquire info with a dialog */
      if (! refract_dialog ()) {
	  gimp_drawable_detach (drawable);
	  return;
      }
      break;

  case RUN_NONINTERACTIVE:
      if (status == STATUS_SUCCESS) {
	  refractvals.lensmap = param[3].data.d_drawable;
	  refractvals.thick = param[4].data.d_int32;
	  refractvals.dist = param[5].data.d_int32;
	  refractvals.na = param[6].data.d_float;
	  refractvals.nb = param[7].data.d_float;
	  refractvals.edge = param[8].data.d_int32;
	  refractvals.newl = param[9].data.d_int32;
	  refractvals.xofs = param[10].data.d_int32;
	  refractvals.yofs = param[11].data.d_int32;
      } /* if */

      break;

  case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_refract", &refractvals);
      break;

  default:
      break;
  } /* switch run_mode */
  
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id)) {
      gimp_progress_init ("Doing optics homework...");

      /* What's this do? */
      gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) 
			     / gimp_tile_width()); 

      go_refract (drawable, param[1].data.d_image);
      
      if (run_mode != RUN_NONINTERACTIVE)
	   gimp_displays_flush ();
      
      if (run_mode == RUN_INTERACTIVE /*|| run_mode == RUN_WITH_LAST_VALS*/)         
	   gimp_set_data ("plug_in_refract", &refractvals, sizeof (RefractValues));
  } else {
       status = STATUS_EXECUTION_ERROR;
  }

  values[0].data.d_status = status;

} /* run */

static gint 
refract_dialog()
{
    gint        argc;
    gchar     **argv;

    GtkTooltips *tooltips;
    GtkWidget *menu, *option_menu, *ior_a_menu, *ior_b_menu;
    GtkWidget *ok_button, *cancel_button;
    GtkWidget *layercheck, *toolcheck;
    GtkWidget *dlg;
    GtkWidget *table;
    GtkWidget *label;

#ifdef REFRACT_DEBUG
#if 0
    printf("refract: waiting... (pid %d)\n", getpid());
    kill(getpid(), SIGSTOP);
#endif
#endif

    /* Standard GTK startup sequence */
    argc = 1;
    argv = g_new (gchar *, 1);
    argv[0] = g_strdup ("refract");

    gtk_init (&argc, &argv);

    gdk_set_use_xshm(gimp_use_xshm());

    /* FIXME: Can we use the GIMP colormap when in 8-bit to reduce flashing? */
    /* end standard GTK startup */

    /* I guess we need a window... */
    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), REFRACT_TITLE);
    gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		       (GtkSignalFunc) refract_close_callback,
		       NULL);

    tooltips = gtk_tooltips_new ();
    
    /* Action area: */

    /* OK */
    ok_button = gtk_button_new_with_label ("OK");
    GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
			(GtkSignalFunc) refract_ok_callback, dlg);
    gtk_widget_grab_default (ok_button);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), ok_button,
			TRUE, TRUE, 0);
    gtk_widget_show (ok_button);

    /* Cancel */
    cancel_button = gtk_button_new_with_label ("Cancel");
    GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (cancel_button), "clicked",
			       (GtkSignalFunc) refract_close_callback,
			       NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), cancel_button,
			TRUE, TRUE, 0);
    gtk_widget_show (cancel_button);

    /* Paramater settings: */
    table = gtk_table_new(7, 3, FALSE);
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG (dlg)->vbox),table);
    gtk_widget_show (table);

    /* FIXME: add preview box */

    /* drop box for lens map */
    label = gtk_label_new("Lens map");

    option_menu = gtk_option_menu_new();

    menu = gimp_drawable_menu_new(map_constrain, map_menu_callback,
				      NULL, refractvals.lensmap);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
    gtk_tooltips_set_tips (tooltips, option_menu, 
			   "The drawable to use as the lens.");

    gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
    gtk_table_attach_defaults(GTK_TABLE(table),option_menu,1,3,0,1);
    gtk_widget_show(label);
    gtk_widget_show(option_menu);

    /* TODO? Add "Invert lens map"  Not anytime soon...  */
    /* Would require adding all sorts of conditional subtracting stuff
       in the main loop...  Let them invert it first! :) */

    /* Eek.  Megawidgets don't return a value I can tie tooltips to.
       Maybe I should look in to libgck. */

    /* entry/scale for lens thickness */

    mw_iscale_entry_new(table, "Thickness", 
			0, 256, 
			1, 10, 0,
			0, 2, 1, 2,
			&refractvals.thick);

    /* entry/scale pair for distance */
    mw_iscale_entry_new(table, "Distance", 
			0, 1000, 
			1, 10, 0/*what's this do?*/,
			0, 2, 2, 3,
			&refractvals.dist);

    /* a entry/scale/drop-menu for each index */
    mw_fscale_entry_new(table, "Index A", 
			INDEX_SCALE_MIN, INDEX_SCALE_MAX, 
			1.0, 0.1, 0,
			0,1, 3, 4,
			&refractvals.na);
 
    ior_a_menu = ior_menu_new(NULL/*FIXME*/);
    gtk_table_attach_defaults(GTK_TABLE(table),ior_a_menu,2,3,3,4);
    gtk_widget_show (ior_a_menu);

    gtk_tooltips_set_tips (tooltips, ior_a_menu, 
			  "FIXME (No, it doesn't work.)");

    mw_fscale_entry_new(table, "Index B", 
			INDEX_SCALE_MIN, INDEX_SCALE_MAX, 
			1.0, 0.1, 0,
			0, 1, 4, 5,
			&refractvals.nb);

    ior_b_menu = ior_menu_new(NULL/*FIXME*/);
    gtk_table_attach_defaults(GTK_TABLE(table),ior_b_menu,2,3,4,5);
    gtk_widget_show (ior_b_menu);

    gtk_tooltips_set_tips (tooltips, ior_b_menu, 
			   "FIXME (No, it doesn't work.)");

    /* entry/scale pairs for x and y offsets */

    mw_iscale_entry_new(table, "X Offset", 
			-1000, 1000, 
			1, 20, 0,
			0,2, 5, 6,
			&refractvals.xofs);

    mw_iscale_entry_new(table, "Y Offset", 
			-1000, 1000, 
			1, 20, 0,
			0,2, 6, 7,
			&refractvals.yofs);

    /* radio buttons for wrap/transparent (or bg, if image isn't layered) */

    /* button = gtk_check_button_new_with_label ("Wrap?");
    toggle_button_callback (button, gpointer   data);
    gtk_toggle_button_set_state (GtkToggleButton button, refractvals.edge); */

    /* Make new layer(s) or dirty the old? */
    layercheck = gtk_check_button_new_with_label ("New layer?");
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG (dlg)->vbox),layercheck);
    gtk_signal_connect (GTK_OBJECT (layercheck), "clicked",
                        GTK_SIGNAL_FUNC (newl_toggle_callback), NULL);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (layercheck), refractvals.newl);
    gtk_tooltips_set_tips (tooltips, layercheck, 
			   "Put the refracted image on a new layer or dirty this one?");

    gtk_widget_show (layercheck);

    toolcheck = gtk_check_button_new_with_label ("Tooltips?");
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG (dlg)->vbox),toolcheck);
    gtk_signal_connect (GTK_OBJECT (toolcheck), "clicked",
			GTK_SIGNAL_FUNC (tooltips_toggle_callback), (gpointer) tooltips);
    gtk_tooltips_set_tips (tooltips, toolcheck,
			   "Turn off these dumb tooltips.");
    gtk_widget_show (toolcheck);

    /* Tooltips OFF by default. */
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toolcheck), FALSE);
    gtk_tooltips_disable (tooltips);

    gtk_widget_show (dlg);

    gtk_main ();
    gdk_flush ();

    return refractint.run;
} /* refract_dialog */

static GtkWidget* 
ior_menu_new(GtkWidget *tieto)
{
  GtkWidget *chooser;
  GtkWidget *menu, *menuitem;
  guint i;

  struct foo 
  {
    const gfloat index;
    const gchar *name;
  };

/* If you change stuff, don't forget to change this. */
#define NUMSTUFF 9
  static const struct foo material[NUMSTUFF] = 
  {
    /* Common indicies of refraction (for yellow sodium light, 589 nm) */
    /* From my Sears, Zemansky, Young physics book. */
    /* For more, check your copy of the CRC or your favorite pov-ray
       include file.  */

    { 1.0003,	"Air" },	
    { 1.309,	"Ice" },
    { 1.333,	"Water"},
    { 1.36,	"Alcohol"},   
    { 1.473,	"Glycerine"},
    { 1.52,	"Glass"},
    { 1.544,	"Quartz"},
    { 1.923,	"Zircon"},
    { 2.417,	"Diamond"},
  };

  chooser = gtk_option_menu_new();

  menu = gtk_menu_new();

  for (i=0; i < NUMSTUFF; i++) {
    menuitem = gtk_menu_item_new_with_label(material[i].name);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		       (GtkSignalFunc)ior_menu_callback,(gfloat *)&material[i].index);
    gtk_widget_show(menuitem);
  }; /* next i */
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(chooser), menu);

  return chooser;
}

static void 
ior_menu_callback (GtkWidget *widget, gfloat *data)
{
#ifdef REFRACT_DEBUG
  printf("%f\n",*data);
#endif
}

static gint
map_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
	if (drawable_id == -1)
		return TRUE;

	return (gimp_drawable_color(drawable_id) || gimp_drawable_gray(drawable_id));
} /* map_constrain */

/* Callbacks */
static void 
newl_toggle_callback (GtkWidget *widget, gpointer   data)
{
    refractvals.newl = GTK_TOGGLE_BUTTON (widget)->active;
}

static void 
tooltips_toggle_callback (GtkWidget *widget, gpointer   data)
{
    GtkTooltips *tooltips;
    tooltips= (GtkTooltips *) data;

    if (GTK_TOGGLE_BUTTON (widget)->active)
	gtk_tooltips_enable (tooltips);
    else
	gtk_tooltips_disable (tooltips);

}

static void
refract_close_callback (GtkWidget *widget,
                        gpointer   data)
{
  gtk_main_quit ();
}

static void
refract_ok_callback (GtkWidget *widget, gpointer data)
{
    refractint.run = TRUE;
    gtk_widget_destroy (GTK_WIDGET (data));
}

static void
map_menu_callback (gint32 id, gpointer data)
{
    refractvals.lensmap = id;
}
