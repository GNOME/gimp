/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#include <stdio.h>
#include <string.h>

#include "gimp.h"
#include "gimpui.h"


/* Copy data from temp_PDB call */
struct _GBrushData {
  gint busy;
  gchar *bname;
  gdouble opacity;
  gint spacing;
  gint paint_mode;
  gint width;
  gint height;
  gchar *brush_mask_data;
  GRunBrushCallback callback;
  gint closing;
  gpointer udata;
};

typedef struct _GBrushData GBrushData;

/* Copy data from temp_PDB call */
struct _GPatternData {
  gint busy;
  gchar *pname;
  gint width;
  gint height;
  gint bytes;
  gchar *pattern_mask_data;
  GRunPatternCallback callback;
  gint closing;
  gpointer udata;
};

typedef struct _GPatternData GPatternData;

/* Copy data from temp_PDB call */
struct _GGradientData {
  gint busy;
  gchar *gname;
  gint width;
  gdouble *gradient_data;
  GRunGradientCallback callback;
  gint closing;
  gpointer udata;
};

typedef struct _GGradientData GGradientData;

static void    gimp_menu_callback    (GtkWidget        *w,
				      gint32           *id);
static void    do_brush_callback     (GBrushData       *bdata);
static gint    idle_test_brush       (GBrushData       *bdata);
static gint    idle_test_pattern     (GPatternData     *pdata);
static gint    idle_test_gradient    (GGradientData    *gdata);
static void    temp_brush_invoker    (char             *name,
				      int               nparams,
				      GParam           *param,
				      int              *nreturn_vals,
				      GParam          **return_vals);
static gboolean input_callback	     (GIOChannel       *channel,
				      GIOCondition      condition,
				      gpointer		data);
static void    gimp_setup_callbacks  (void);
static gchar*  gen_temp_plugin_name  (void);

/* From gimp.c */
void gimp_run_temp (void);

static GHashTable *gbrush_ht = NULL;
static GHashTable *gpattern_ht = NULL;
static GHashTable *ggradient_ht = NULL;
static GBrushData *active_brush_pdb = NULL;
static GPatternData *active_pattern_pdb = NULL;
static GGradientData *active_gradient_pdb = NULL;


GtkWidget*
gimp_image_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_image)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *filename;
  char *label;
  gint32 *images;
  int nimages;
  int i, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	filename = gimp_image_get_filename (images[i]);
	label = g_new (char, strlen (filename) + 16);
	sprintf (label, "%s-%d", g_basename (filename), images[i]);
	g_free (filename);

	menuitem = gtk_menu_item_new_with_label (label);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    (GtkSignalFunc) gimp_menu_callback,
			    &images[i]);
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_widget_show (menuitem);

	g_free (label);

	if (images[i] == active_image)
	  gtk_menu_set_active (GTK_MENU (menu), k);

	k += 1;
      }

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (images)
    {
      if (active_image == -1)
	active_image = images[0];
      (* callback) (active_image, data);
    }

  return menu;
}

GtkWidget*
gimp_layer_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_layer)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *name;
  char *image_label;
  char *label;
  gint32 *images;
  gint32 *layers;
  gint32 layer;
  int nimages;
  int nlayers;
  int i, j, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  layer = -1;

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_filename (images[i]);
	image_label = g_new (char, strlen (name) + 16);
	sprintf (image_label, "%s-%d", g_basename (name), images[i]);
	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);
	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      name = gimp_layer_get_name (layers[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &layers[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (layers[j] == active_layer)
		{
		  layer = active_layer;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (layer == -1)
		layer = layers[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (layer != -1)
    (* callback) (layer, data);

  return menu;
}

GtkWidget*
gimp_channel_menu_new (GimpConstraintFunc constraint,
		       GimpMenuCallback   callback,
		       gpointer           data,
		       gint32             active_channel)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *name;
  char *image_label;
  char *label;
  gint32 *images;
  gint32 *channels;
  gint32 channel;
  int nimages;
  int nchannels;
  int i, j, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  channel = -1;

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_filename (images[i]);
	image_label = g_new (char, strlen (name) + 16);
	sprintf (image_label, "%s-%d", g_basename (name), images[i]);
	g_free (name);

	channels = gimp_image_get_channels (images[i], &nchannels);
	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      name = gimp_channel_get_name (channels[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &channels[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (channels[j] == active_channel)
		{
		  channel = active_channel;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (channel == -1)
		channel = channels[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (channel != -1)
    (* callback) (channel, data);

  return menu;
}

GtkWidget*
gimp_drawable_menu_new (GimpConstraintFunc constraint,
			GimpMenuCallback   callback,
			gpointer           data,
			gint32             active_drawable)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *name;
  char *image_label;
  char *label;
  gint32 *images;
  gint32 *layers;
  gint32 *channels;
  gint32 drawable;
  int nimages;
  int nlayers;
  int nchannels;
  int i, j, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  drawable = -1;

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_filename (images[i]);
	image_label = g_new (char, strlen (name) + 16);
	sprintf (image_label, "%s-%d", g_basename (name), images[i]);
	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);
	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      name = gimp_layer_get_name (layers[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &layers[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (layers[j] == active_drawable)
		{
		  drawable = active_drawable;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (drawable == -1)
		drawable = layers[j];

	      k += 1;
	    }

	channels = gimp_image_get_channels (images[i], &nchannels);
	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      name = gimp_channel_get_name (channels[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &channels[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (channels[j] == active_drawable)
		{
		  drawable = active_drawable;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (drawable == -1)
		drawable = channels[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (drawable != -1)
    (* callback) (drawable, data);

  return menu;
}


static void
gimp_menu_callback (GtkWidget *w,
		    gint32    *id)
{
  GimpMenuCallback callback;
  gpointer callback_data;

  callback = (GimpMenuCallback) gtk_object_get_user_data (GTK_OBJECT (w->parent));
  callback_data = gtk_object_get_data (GTK_OBJECT (w->parent), "gimp_callback_data");

  (* callback) (*id, callback_data);
}


/* These functions allow the callback PDB work with gtk
 * ALT.
 * Note that currently PDB calls in libgimp are completely deterministic.
 * There is always one call followed by a reply.
 * If we are in the main gdk wait routine we can cannot get a reply
 * to a wire message, only the request for a new PDB proc to be run.
 * we will restrict this to a temp PDB function we have registered.
 */

static void 
do_brush_callback(GBrushData * bdata)
{
  if(!bdata->busy)
    return;

  if(bdata->callback)
    bdata->callback(bdata->bname,
		    bdata->opacity,
		    bdata->spacing,
		    bdata->paint_mode,
		    bdata->width,
		    bdata->height,
		    bdata->brush_mask_data,
		    bdata->closing,
		    bdata->udata);

  if(bdata->bname)
    g_free(bdata->bname);  
  
  if(bdata->brush_mask_data)
    g_free(bdata->brush_mask_data); 

  bdata->busy = 0;
  bdata->bname = bdata->brush_mask_data = 0;
}

static void 
do_pattern_callback(GPatternData * pdata)
{
  if(!pdata->busy)
    return;

  if(pdata->callback)
    pdata->callback(pdata->pname,
		    pdata->width,
		    pdata->height,
		    pdata->bytes,
		    pdata->pattern_mask_data,
		    pdata->closing,
		    pdata->udata);

  if(pdata->pname)
    g_free(pdata->pname);  
  
  if(pdata->pattern_mask_data)
    g_free(pdata->pattern_mask_data); 

  pdata->busy = 0;
  pdata->pname = pdata->pattern_mask_data = 0;
}

static void 
do_gradient_callback(GGradientData * gdata)
{
  if(!gdata->busy)
    return;

  if(gdata->callback)
    gdata->callback(gdata->gname,
		    gdata->width,
		    gdata->gradient_data,
		    gdata->closing,
		    gdata->udata);

  if(gdata->gname)
    g_free(gdata->gname);  
  
  if(gdata->gradient_data)
    g_free(gdata->gradient_data); 

  gdata->busy = 0;
  gdata->gname = NULL;
  gdata->gradient_data = NULL;
}

static gint
idle_test_brush (GBrushData * bdata)
{
  do_brush_callback(bdata);
  return FALSE;
}


static gint
idle_test_pattern (GPatternData * pdata)
{
  do_pattern_callback(pdata);
  return FALSE;
}

static gint
idle_test_gradient (GGradientData * gdata)
{
  do_gradient_callback(gdata);
  return FALSE;
}

static void
temp_brush_invoker(char    *name,
	     int      nparams,
	     GParam  *param,
	     int     *nreturn_vals,
	     GParam **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GBrushData *bdata = (GBrushData *)g_hash_table_lookup(gbrush_ht,name);

  if(!bdata)
    {
      g_warning("Can't find internal brush data");
    }
  else
    {
      if(!bdata->busy)
	{
	  /*       printf("\nXX  Here I am in the temp proc\n"); */
	  
	  /*       printf("String name passed is '%s'\n",param[0].data.d_string); */
	  /*       printf("opacity is '%g'\n",(gdouble)param[1].data.d_float); */
	  /*       printf("spacing is '%d'\n",param[2].data.d_int32); */
	  /*       printf("paint mode is '%d'\n",param[3].data.d_int32); */
	  /*       printf("width is '%d'\n",param[4].data.d_int32); */
	  /*       printf("height is '%d'\n",param[5].data.d_int32); */
	  /*       printf("String mask data length passed is '%d'\n",param[6].data.d_int32); */
	  /*       printf("closing is '%d'\n",param[8].data.d_int32); */
      
	  bdata->bname = g_strdup(param[0].data.d_string);
	  bdata->opacity = (gdouble)param[1].data.d_float;
	  bdata->spacing = param[2].data.d_int32;
	  bdata->paint_mode = param[3].data.d_int32;
	  bdata->width = param[4].data.d_int32;
	  bdata->height = param[5].data.d_int32;
	  bdata->brush_mask_data = g_malloc(param[6].data.d_int32);
	  g_memmove(bdata->brush_mask_data,param[7].data.d_int8array,param[6].data.d_int32); 
	  bdata->closing = param[8].data.d_int32;
	  active_brush_pdb = bdata;
	  bdata->busy = 1;
	  
	  gtk_idle_add((GtkFunction) idle_test_brush,active_brush_pdb);
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
temp_pattern_invoker(char    *name,
	     int      nparams,
	     GParam  *param,
	     int     *nreturn_vals,
	     GParam **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GPatternData *pdata = (GPatternData *)g_hash_table_lookup(gpattern_ht,name);

  if(!pdata)
    {
      g_warning("Can't find internal pattern data");
    }
  else
    {
      if(!pdata->busy)
	{

	  pdata->pname = g_strdup(param[0].data.d_string);
	  pdata->width = param[1].data.d_int32;
	  pdata->height = param[2].data.d_int32;
	  pdata->bytes = param[3].data.d_int32;
	  pdata->pattern_mask_data = g_malloc(param[4].data.d_int32);
	  g_memmove(pdata->pattern_mask_data,param[5].data.d_int8array,param[4].data.d_int32); 
	  pdata->closing = param[6].data.d_int32;
	  active_pattern_pdb = pdata;
	  pdata->busy = 1;
	  gtk_idle_add((GtkFunction) idle_test_pattern,active_pattern_pdb);
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
temp_gradient_invoker(char    *name,
		      int      nparams,
		      GParam  *param,
		      int     *nreturn_vals,
		      GParam **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GGradientData *gdata = (GGradientData *)g_hash_table_lookup(ggradient_ht,name);

  if(!gdata)
    {
      g_warning("Can't find internal gradient data");
    }
  else
    {
      if(!gdata->busy)
	{
	  int i;
	  gdouble *pv,*values;;
	  gdata->gname = g_strdup(param[0].data.d_string);
	  gdata->width = param[1].data.d_int32;
	  gdata->gradient_data = (gdouble *)g_malloc(param[1].data.d_int32*sizeof(gdouble));
	  values = param[2].data.d_floatarray;
	  pv = gdata->gradient_data;

	  for (i = 0; i < gdata->width; i++)
	    gdata->gradient_data[i] = param[2].data.d_floatarray[i];

	  gdata->closing = param[3].data.d_int32;
	  active_gradient_pdb = gdata;
	  gdata->busy = 1;
	  gtk_idle_add((GtkFunction) idle_test_gradient,active_gradient_pdb);
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static gboolean
input_callback (GIOChannel  *channel,
		GIOCondition condition,
		gpointer     data)
{
  /* We have some data in the wire - read it */
  /* The below will only ever run a single proc */
  gimp_run_temp();

  return TRUE;
}

static void
gimp_setup_callbacks (void)
{
  static int first_time = TRUE;

  if(first_time)
    {
      /* Tie into the gdk input function */
      /* only once */
      g_io_add_watch (_readchannel, G_IO_IN | G_IO_PRI, input_callback, NULL);
      /* This needed on Win32 */
      gimp_request_wakeups();
      first_time = FALSE;
    }
}

static gchar *
gen_temp_plugin_name (void)
{
  GParam *return_vals;
  int nreturn_vals;
  char *result;

  return_vals = gimp_run_procedure ("gimp_temp_PDB_name",
				    &nreturn_vals,
				    PARAM_END);

  result = "temp_name_gen_failed";
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}


/* Can only be used in conjuction with gdk since we need to tie into the input 
 * selection mech.
 */
void *
gimp_interactive_selection_brush(gchar *dialogname, 
				 gchar *brush_name,
				 gdouble opacity,
				 gint spacing,
				 gint paint_mode,
				 GRunBrushCallback callback,
				 gpointer udata)
{
  static GParamDef args[] =
  {
    { PARAM_STRING, "str", "String"},
    { PARAM_FLOAT, "opacity", "Opacity"},
    { PARAM_INT32, "spacing", "spacing"},
    { PARAM_INT32, "paint mode","paint mode"},
    { PARAM_INT32, "mask width","brush width"},
    { PARAM_INT32, "mask height","brush heigth"},
    { PARAM_INT32, "mask len","Length of brush mask data"},
    { PARAM_INT8ARRAY,"mask data","The brush mask data"},
    { PARAM_INT32, "dialog status","Registers if the dialog was closing [0 = No, 1 = Yes]"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;
  gint bnreturn_vals;
  GParam *pdbreturn_vals;
  gchar *pdbname = gen_temp_plugin_name();
  GBrushData *bdata = g_malloc0(sizeof(struct _GBrushData));

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  PROC_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_brush_invoker);

  pdbreturn_vals =
    gimp_run_procedure("gimp_brushes_popup",
		       &bnreturn_vals,
		       PARAM_STRING,pdbname,
		       PARAM_STRING,dialogname,
		       PARAM_STRING,brush_name,/*name*/
		       PARAM_FLOAT, opacity, /*Opacity*/
		       PARAM_INT32, spacing, /*default spacing*/
		       PARAM_INT32, paint_mode, /*paint mode*/
		       PARAM_END);

/*   if (pdbreturn_vals[0].data.d_status != STATUS_SUCCESS) */
/*     { */
/*       printf("ret failed = 0x%x\n",bnreturn_vals); */
/*     } */
/*   else */
/*       printf("worked = 0x%x\n",bnreturn_vals); */

  gimp_setup_callbacks(); /* New function to allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals,bnreturn_vals);

  /*   g_free(pdbname); */

  /* Now add to hash table so we can find it again */
  if(gbrush_ht == NULL)
    gbrush_ht = g_hash_table_new (g_str_hash,
				      g_str_equal);

  bdata->callback = callback;
  bdata->udata = udata;
  g_hash_table_insert(gbrush_ht,pdbname,bdata);

  return pdbname;
}


gchar *
gimp_brushes_get_brush_data (gchar *bname,
			   gdouble *opacity,
			   gint  *spacing,
			   gint  *paint_mode,
			   gint  *width,
			   gint  *height,
			   gchar  **mask_data)
{
  GParam *return_vals;
  int nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_brushes_get_brush_data",
				    &nreturn_vals,
				    PARAM_STRING, bname,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      ret_name = g_strdup(return_vals[1].data.d_string);
      *opacity = return_vals[2].data.d_float;
      *spacing = return_vals[3].data.d_int32;
      *paint_mode = return_vals[4].data.d_int32;
      *width = return_vals[5].data.d_int32;
      *height = return_vals[6].data.d_int32;
      *mask_data = g_new (gchar,return_vals[7].data.d_int32);
      g_memmove (*mask_data, return_vals[8].data.d_int32array,return_vals[7].data.d_int32);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}

gint 
gimp_brush_close_popup(void * popup_pnt)
{
  GParam *return_vals;
  int nreturn_vals;
  gint retval;

  return_vals = gimp_run_procedure ("gimp_brushes_close_popup",
				    &nreturn_vals,
				    PARAM_STRING, popup_pnt,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

  retval = (return_vals[0].data.d_status == STATUS_SUCCESS);

  return retval;
}

gint 
gimp_brush_set_popup(void * popup_pnt, 
		     gchar * pname,
		     gdouble opacity,
		     gint spacing,
		     gint paint_mode)
{
  GParam *return_vals;
  int nreturn_vals;
  gint retval;

  return_vals = gimp_run_procedure ("gimp_brushes_set_popup",
				    &nreturn_vals,
				    PARAM_STRING, popup_pnt,
				    PARAM_STRING, pname,
				    PARAM_FLOAT, opacity,
				    PARAM_INT32, spacing,
				    PARAM_INT32, paint_mode,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

  retval = (return_vals[0].data.d_status == STATUS_SUCCESS);

  return retval;
}



void *
gimp_interactive_selection_pattern(gchar *dialogname, 
				   gchar *pattern_name,
				   GRunPatternCallback callback,
				   gpointer udata)
{
  static GParamDef args[] =
  {
    { PARAM_STRING, "str", "String"},
    { PARAM_INT32, "mask width","pattern width"},
    { PARAM_INT32, "mask height","pattern heigth"},
    { PARAM_INT32, "mask bpp","pattern bytes per pixel"},
    { PARAM_INT32, "mask len","Length of pattern mask data"},
    { PARAM_INT8ARRAY,"mask data","The pattern mask data"},
    { PARAM_INT32, "dialog status","Registers if the dialog was closing [0 = No, 1 = Yes]"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;
  gint bnreturn_vals;
  GParam *pdbreturn_vals;
  gchar *pdbname = gen_temp_plugin_name();
  GPatternData *pdata = g_malloc0(sizeof(struct _GPatternData));

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  PROC_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_pattern_invoker);

  pdbreturn_vals =
    gimp_run_procedure("gimp_patterns_popup",
		       &bnreturn_vals,
		       PARAM_STRING,pdbname,
		       PARAM_STRING,dialogname,
		       PARAM_STRING,pattern_name,/*name*/
		       PARAM_END);

  gimp_setup_callbacks(); /* New function to allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals,bnreturn_vals);

  /* Now add to hash table so we can find it again */
  if(gpattern_ht == NULL)
    gpattern_ht = g_hash_table_new (g_str_hash,
				      g_str_equal);

  pdata->callback = callback;
  pdata->udata = udata;
  g_hash_table_insert(gpattern_ht,pdbname,pdata);

  return pdbname;
}


gchar *
gimp_pattern_get_pattern_data (gchar *pname,
			       gint  *width,
			       gint  *height,
			       gint  *bytes,
			       gchar  **mask_data)
{
  GParam *return_vals;
  int nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_patterns_get_pattern_data",
				    &nreturn_vals,
				    PARAM_STRING, pname,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      ret_name = g_strdup(return_vals[1].data.d_string);
      *width = return_vals[2].data.d_int32;
      *height = return_vals[3].data.d_int32;
      *bytes = return_vals[4].data.d_int32;
      *mask_data = g_new (gchar,return_vals[5].data.d_int32);
      g_memmove (*mask_data, return_vals[6].data.d_int32array,return_vals[5].data.d_int32);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}

gint 
gimp_pattern_close_popup(void * popup_pnt)
{
  GParam *return_vals;
  int nreturn_vals;
  gint retval;

  return_vals = gimp_run_procedure ("gimp_patterns_close_popup",
				    &nreturn_vals,
				    PARAM_STRING, popup_pnt,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

  retval = (return_vals[0].data.d_status == STATUS_SUCCESS);

  return retval;
}

gint 
gimp_pattern_set_popup(void * popup_pnt, gchar * pname)
{
  GParam *return_vals;
  int nreturn_vals;
  gint retval;

  return_vals = gimp_run_procedure ("gimp_patterns_set_popup",
				    &nreturn_vals,
				    PARAM_STRING, popup_pnt,
				    PARAM_STRING, pname,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

  retval = (return_vals[0].data.d_status == STATUS_SUCCESS);

  return retval;
}

void *
gimp_interactive_selection_gradient(gchar *dialogname, 
				    gchar *gradient_name,
				    gint sample_sz,
				    GRunGradientCallback callback,
				    gpointer udata)
{
  static GParamDef args[] =
  {
    { PARAM_STRING, "str", "String"},
    { PARAM_INT32, "grad width","grad width"},
    { PARAM_FLOATARRAY,"grad data","The gradient mask data"},
    { PARAM_INT32, "dialog status","Registers if the dialog was closing [0 = No, 1 = Yes]"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;
  gint bnreturn_vals;
  GParam *pdbreturn_vals;
  gchar *pdbname = gen_temp_plugin_name();
  GGradientData *gdata = g_malloc0(sizeof(struct _GGradientData));

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  PROC_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_gradient_invoker);

  pdbreturn_vals =
    gimp_run_procedure("gimp_gradients_popup",
		       &bnreturn_vals,
		       PARAM_STRING,pdbname,
		       PARAM_STRING,dialogname,
		       PARAM_STRING,gradient_name,/*name*/
		       PARAM_INT32,sample_sz, /* size of sample to be returned */ 
		       PARAM_END);

  gimp_setup_callbacks(); /* New function to allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals,bnreturn_vals);

  /* Now add to hash table so we can find it again */
  if(ggradient_ht == NULL)
    ggradient_ht = g_hash_table_new (g_str_hash,
				     g_str_equal);

  gdata->callback = callback;
  gdata->udata = udata;
  g_hash_table_insert(ggradient_ht,pdbname,gdata);

  return pdbname;
}

gchar *
gimp_gradient_get_gradient_data (gchar *gname,
				 gint  *width,
				 gint sample_sz,
				 gdouble **grad_data)
{
  GParam *return_vals;
  int nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_gradients_get_gradient_data",
				    &nreturn_vals,
				    PARAM_STRING, gname,
				    PARAM_INT32, sample_sz,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      int i;
      ret_name = g_strdup(return_vals[1].data.d_string);
      *width = return_vals[2].data.d_int32;
      *grad_data = g_new (gdouble,*width);
      for (i = 0; i < *width; i++)
	(*grad_data)[i] = return_vals[3].data.d_floatarray[i];
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}


gint 
gimp_gradient_close_popup(void * popup_pnt)
{
  GParam *return_vals;
  int nreturn_vals;
  gint retval;

  return_vals = gimp_run_procedure ("gimp_gradients_close_popup",
				    &nreturn_vals,
				    PARAM_STRING, popup_pnt,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

  retval = (return_vals[0].data.d_status == STATUS_SUCCESS);

  return retval;
}

gint 
gimp_gradient_set_popup(void * popup_pnt, gchar * gname)
{
  GParam *return_vals;
  int nreturn_vals;
  gint retval;

  return_vals = gimp_run_procedure ("gimp_gradients_set_popup",
				    &nreturn_vals,
				    PARAM_STRING, popup_pnt,
				    PARAM_STRING, gname,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

  retval = (return_vals[0].data.d_status == STATUS_SUCCESS);

  return retval;
}
