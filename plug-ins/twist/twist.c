/* Twist - A geometric image distortion plugin for 'The GIMP'
 * (see legal notice below).
 * Copyright (C) 1997 Peter Uray.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
 
 
 
/* The GIMP -- an image manipulation program
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
 
 
 
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"


#define ENTRY_WIDTH 70
#define epsilon 0.001
#define preview_width 128
#define preview_height 128
                      
/* The number of parameters needed for non-interative mode */
#define N_PARAMETERS 15

/* The number of available functions */
#define N_FUNCTIONS 8


/* Identifiers for the distortion function types */
#define FUNCTION_R_r             0
#define FUNCTION_R_phi           1
#define FUNCTION_Phi_r           2
#define FUNCTION_Phi_phi         3
#define FUNCTION_R_r_Phi_phi     4
#define FUNCTION_R_r_R_phi       5
#define FUNCTION_Cosines         6
#define FUNCTION_Gauss           7


/***************************************************************************
 *
 *
 *  Global data definition.
 *
 *
 ***************************************************************************/




/* do_action is used for preventing the plugin to act when the
   [Cancel] button was pressed.
*/
static gint do_action = 0;


static gint do_update_preview = 1;


/* Some frequently used strings. */
static char PluginName[] = "Twist";
static char PluginResource[] = "plug_in_Twist";
static char PluginAuthor[] = "Peter Uray";
static char PluginYear[] = "1997";
static char PluginDescription[] = "Twist - Geometric image distortion";
static char PluginGroup[] = "<Image>/Filters/Distorts/Twist";


/* Definition of global structure types */

/* A GtkObject and a label - used below */
typedef struct 
{
 GtkWidget *object;
 GtkLabel *label;
}ObjectAndLabel;


/* These parameters are modified in the 'Parameter settings' section. */
typedef struct
{
 gfloat a1;
 gfloat a2;
 gfloat a3;
 gfloat a4;
 gfloat a5;
 gfloat a6;
 gfloat a7;
 gfloat a8;
 
 gchar lettering01[30];
 gchar lettering02[30];
 gchar lettering03[30];
 gchar lettering04[30];
 gchar lettering05[30];
 gchar lettering06[30];
 gchar lettering07[30];
 gchar lettering08[30];
 
 gint use_cutoff;
 gfloat r;
 gfloat dr;
 
 gint current_function;

}Parameters;


/* The labels of the parameter selection box */
/* These depend on the selected function     */
typedef struct 
{
 char name_a1[30];
 char name_a2[30];
 char name_a3[30];
 char name_a4[30];
 char name_a5[30];
 char name_a6[30];
 char name_a7[30];
 char name_a8[30];
}ParameterNames;


/* Data fields used for creating a dialog and the action area. */
typedef struct
{
 GtkWidget *dlg;
 GtkWidget *ok_button;
 GtkWidget *cancel_button;
 GtkWidget *dialog_frame;
 gchar **argv;
 gint argc;
}DialogTemplateData;


/* Data used for maintaining the preview image */
typedef struct
{
 gint width;
 gint height;
 gint bpp;
 gfloat scale;
 guchar *bits;
 GtkWidget *display;
}PreviewData;



/* These pointers are needed for modifying the sliders in update_dialog */
typedef struct 
{
 GtkWidget *slider01;
 GtkWidget *slider02;
 GtkWidget *slider03;
 GtkWidget *slider04;
 GtkWidget *slider05;
 GtkWidget *slider06;
 GtkWidget *slider07;
 GtkWidget *slider08;
 GtkWidget *slider09;
 GtkWidget *slider10;
 
 GtkLabel *label01;
 GtkLabel *label02;
 GtkLabel *label03;
 GtkLabel *label04;
 GtkLabel *label05;
 GtkLabel *label06;
 GtkLabel *label07;
 GtkLabel *label08;
 
 GtkWidget *toggle01;
}WidgetData; 


/* Parameter initialization */
static Parameters parameter_values = { 0.0,            /* a1               */
                                       0.0,            /* a2               */
                                       0.0,            /* a3               */
                                       0.0,            /* a4               */
                                       0.0,            /* a5               */
                                       0.0,            /* a6               */
                                       0.0,            /* a7               */
                                       0.0,            /* a8               */
                                       "unused",       /* lettering01      */
                                       "unused",       /* lettering02      */
                                       "unused",       /* lettering03      */
                                       "unused",       /* lettering04      */
                                       "unused",       /* lettering05      */
                                       "unused",       /* lettering06      */
                                       "unused",       /* lettering07      */
                                       "unused",       /* lettering08      */
                                       FALSE,          /* use_cutoff       */
                                       0.5,            /* r                */                              
                                       0.2,            /* dr               */     
                                       FUNCTION_R_r    /* current_function */                             
                                      };
static DialogTemplateData dtd;
static PreviewData preview_data;
static WidgetData widget_data;
static ParameterNames parameter_names[N_FUNCTIONS];
static Parameters EffectsParameters[N_FUNCTIONS][9];



/***************************************************************************
 *
 *
 *  Forward declarations
 *
 *
 ***************************************************************************/



/* Declare local functions. */
static void      query(void);
static void      run(gchar *name,
                     gint nparams,
                     GParam *param,
                     gint *nreturn_vals,
                     GParam **return_vals);




/* Callback functions */
static void     close_callback(GtkWidget *widget, gpointer data);
static void     ok_callback(GtkWidget *widget, gpointer data);
static void     cancel_callback(GtkWidget *widget, gpointer data);

static void     a1_callback(GtkAdjustment *adjustment, gpointer data);
static void     a2_callback(GtkAdjustment *adjustment, gpointer data);
static void     a3_callback(GtkAdjustment *adjustment, gpointer data);
static void     a4_callback(GtkAdjustment *adjustment, gpointer data);
static void     a5_callback(GtkAdjustment *adjustment, gpointer data);
static void     a6_callback(GtkAdjustment *adjustment, gpointer data);
static void     a7_callback(GtkAdjustment *adjustment, gpointer data);
static void     a8_callback(GtkAdjustment *adjustment, gpointer data);


static void	use_cutoff_callback(GtkWidget *widget, gpointer data);
static void	r_callback(GtkAdjustment *adjustment, gpointer data);
static void	dr_callback(GtkAdjustment *adjustment, gpointer data);


static void	effect01_callback(GtkWidget *widget, gpointer data);
static void	effect02_callback(GtkWidget *widget, gpointer data);
static void	effect03_callback(GtkWidget *widget, gpointer data);

static void	effect04_callback(GtkWidget *widget, gpointer data);
static void	effect05_callback(GtkWidget *widget, gpointer data);
static void	effect06_callback(GtkWidget *widget, gpointer data);

static void	effect07_callback(GtkWidget *widget, gpointer data);
static void	effect08_callback(GtkWidget *widget, gpointer data);
static void	effect09_callback(GtkWidget *widget, gpointer data);



static void	function01_callback(GtkWidget *widget, gpointer data);
static void	function02_callback(GtkWidget *widget, gpointer data);
static void	function03_callback(GtkWidget *widget, gpointer data);
static void	function04_callback(GtkWidget *widget, gpointer data);
static void	function05_callback(GtkWidget *widget, gpointer data);
static void	function06_callback(GtkWidget *widget, gpointer data);
static void	function07_callback(GtkWidget *widget, gpointer data);
static void	function08_callback(GtkWidget *widget, gpointer data);





/* Dialog creation/updating functions */

/* This function executes the user dialog */
static gint	user_dialog(void);

/* This function syncs the dialog display with the contents of the
   parameter_values
*/
static void	update_dialog(void);

static void	create_base_dialog(void);
static ObjectAndLabel create_slider_with_label(gchar *text,                                                            
                                               GtkWidget *container,
                                               GtkSignalFunc callback_func,
                                               gfloat initial_value,
                                               gfloat min_value,
                                               gfloat max_value);
                                               
static void	create_functions_menu(GtkWidget *container);                                               
static void	create_effects_menu(GtkWidget *container);                                 
                                               
static void	create_effects_panel(GtkWidget *container,
                                   gchar *text1,
                                   gchar *text2,
                                   gchar *text3,
                                   GtkSignalFunc callback_func1,
                                   GtkSignalFunc callback_func2,
                                   GtkSignalFunc callback_func3);
                                   
static void	create_cutoff_panel(GtkWidget *container);




/* Functions for handling the preview */
static void	create_preview(GDrawable *drawable);
static void	update_preview(GtkWidget *preview);


/* This function actually modifies the image */
void		image_action(GDrawable *drawable, Parameters params);

/* Functions for calculating the image distortion */
static void	polar_coordinates(gfloat x, gfloat y, gfloat *rho, gfloat *phi);


/* The distortion functions. */
static void	distortion_function01(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function02(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function03(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function04(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function05(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function06(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function07(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);
static void	distortion_function08(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2);


static void	calculate_distortion_function(gint i, gint j, gint width, gint height, gfloat *u, gfloat *v);
static gfloat	cutoff_function(gfloat u, gfloat v);
static void	calculate_pixel_color(gfloat u, gfloat f, guchar *source, gint width, gint height, gint bpp, guchar *r, guchar *g, guchar *b);

/* Functions for image data exchange between GIMP and the plugin */
static void	get_source_image(GPixelRgn source_rgn, guchar *buffer, gint x, gint y, gint width, gint height, gint bytes); 





/***************************************************************************
 *
 *
 *  Implementation
 *
 *
 ***************************************************************************/


/* An auxiliary function to make life easier while initializing the effects */
static void SET_EFFECTS_PARAMS(gint i, gint j,
                               gfloat a1, gfloat a2, 
                               gfloat a3, gfloat a4, 
                               gfloat a5, gfloat a6,
                               gfloat a7, gfloat a8,
                               gint uc,
                               gfloat r, gfloat dr) 
{
 EffectsParameters[i][j].a1 = a1;                 
 EffectsParameters[i][j].a2 = a2;                 
 EffectsParameters[i][j].a3 = a3;                 
 EffectsParameters[i][j].a4 = a4;                 
 EffectsParameters[i][j].a5 = a5;                 
 EffectsParameters[i][j].a6 = a6;                 
 EffectsParameters[i][j].a7 = a7;                 
 EffectsParameters[i][j].a8 = a8;                 
 EffectsParameters[i][j].use_cutoff = uc;         
 EffectsParameters[i][j].r = r;                   
 EffectsParameters[i][j].dr = dr;              
}                    



GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};



/******************************************************************************/
/*                           Standard functions                               */
/******************************************************************************/


/* The main function */
MAIN ();





static void query ()
{
 static GParamDef args[] =
 {
  { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
  { PARAM_IMAGE, "image", "Input image" },
  { PARAM_DRAWABLE, "drawable", "Input drawable" },
  { PARAM_FLOAT, "a1", "float parameter #1" },
  { PARAM_FLOAT, "a2", "float parameter #2" },
  { PARAM_FLOAT, "a3", "float parameter #3" },
  { PARAM_FLOAT, "a4", "float parameter #4" },
  { PARAM_FLOAT, "a5", "float parameter #5" },
  { PARAM_FLOAT, "a6", "float parameter #6" },
  { PARAM_FLOAT, "a7", "float parameter #7" },
  { PARAM_FLOAT, "a8", "float parameter #8" },
  { PARAM_STRING , "lettering01", "lettering #1" },
  { PARAM_STRING , "lettering02", "lettering #2" },
  { PARAM_STRING , "lettering03", "lettering #3" },
  { PARAM_STRING , "lettering04", "lettering #4" },
  { PARAM_STRING , "lettering05", "lettering #5" },
  { PARAM_STRING , "lettering06", "lettering #6" },
  { PARAM_STRING , "lettering07", "lettering #7" },
  { PARAM_STRING , "lettering08", "lettering #8" },
  { PARAM_INT32, "use_cutoff", "Use cutoff or not" },
  { PARAM_FLOAT, "r", "The cutoff radius" },
  { PARAM_FLOAT, "dr", "The falloff radius" },
  { PARAM_INT32, "current_function", "The current function" },
 };
 static GParamDef *return_vals = NULL;
 static gint nargs = sizeof (args) / sizeof (args[0]);
 static gint nreturn_vals = 0;

 gimp_install_procedure (PluginResource,
			 PluginDescription,
			 PluginDescription,
			 PluginAuthor,
			 PluginAuthor,
			 PluginYear,
			 PluginGroup,
			 "RGB*, GRAY*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

static void run(gchar *name, gint nparams, GParam *param, gint *nreturn_vals, GParam **return_vals)
{
 static GParam values[1];
 GDrawable *drawable;
 GRunModeType run_mode;
 GStatusType status = STATUS_SUCCESS;
 gint i;
 
 run_mode = param[0].data.d_int32;
 
 *nreturn_vals = 1;
 *return_vals = values;

 values[0].type = PARAM_STATUS;
 values[0].data.d_status = status;

 /*  Get the drawable  */
 drawable = gimp_drawable_get(param[2].data.d_drawable);
 
 /* Parameter names for R(r) distortion */
 strcpy(parameter_names[0].name_a1, "Linear coefficient:");
 strcpy(parameter_names[0].name_a2, "Quadratic coefficient:");
 strcpy(parameter_names[0].name_a3, "Cubic coefficient:");
 strcpy(parameter_names[0].name_a4, "Quartic coefficient:");
 strcpy(parameter_names[0].name_a5, "unused");
 strcpy(parameter_names[0].name_a6, "unused");
 strcpy(parameter_names[0].name_a7, "unused");
 strcpy(parameter_names[0].name_a8, "unused");
 
 
 /* Parameter names for R(phi) distortion */
 strcpy(parameter_names[1].name_a1, "Amplitude 1:");
 strcpy(parameter_names[1].name_a2, "Frequency 1:");
 strcpy(parameter_names[1].name_a3, "Amplitude 2:");
 strcpy(parameter_names[1].name_a4, "Frequency 2");
 strcpy(parameter_names[1].name_a5, "unused");
 strcpy(parameter_names[1].name_a6, "unused");
 strcpy(parameter_names[1].name_a7, "unused");
 strcpy(parameter_names[1].name_a8, "unused");
 
 /* Parameter names for Phi(r) distortion */
 strcpy(parameter_names[2].name_a1, "Linear coefficient:");
 strcpy(parameter_names[2].name_a2, "Quadratic coefficient:");
 strcpy(parameter_names[2].name_a3, "Cubic coefficient:");
 strcpy(parameter_names[2].name_a4, "Quartic coefficient:");
 strcpy(parameter_names[2].name_a5, "unused");
 strcpy(parameter_names[2].name_a6, "unused");
 strcpy(parameter_names[2].name_a7, "unused");
 strcpy(parameter_names[2].name_a8, "unused");
 
 
 /* Parameter names for Phi(phi) distortion */
 strcpy(parameter_names[3].name_a1, "Amplitude 1:");
 strcpy(parameter_names[3].name_a2, "Frequency 1:");
 strcpy(parameter_names[3].name_a3, "Amplitude 2:");
 strcpy(parameter_names[3].name_a4, "Frequency 2");
 strcpy(parameter_names[3].name_a5, "unused");
 strcpy(parameter_names[3].name_a6, "unused");
 strcpy(parameter_names[3].name_a7, "unused");
 strcpy(parameter_names[3].name_a8, "unused");
 
 
 /* Parameter names for R(r)Phi(phi) distortion */
 strcpy(parameter_names[4].name_a1, "Linear coefficient:");
 strcpy(parameter_names[4].name_a2, "Quadratic coefficient:");
 strcpy(parameter_names[4].name_a3, "Cubic coefficient:");
 strcpy(parameter_names[4].name_a4, "Quartic coefficient:");
 strcpy(parameter_names[4].name_a5, "Amplitude 1:");
 strcpy(parameter_names[4].name_a6, "Frequency 1:");
 strcpy(parameter_names[4].name_a7, "Amplitude 2:");
 strcpy(parameter_names[4].name_a8, "Frequency 2");
 
 /* Parameter names for R(r)R(phi) distortion */
 strcpy(parameter_names[5].name_a1, "Linear coefficient:");
 strcpy(parameter_names[5].name_a2, "Quadratic coefficient:");
 strcpy(parameter_names[5].name_a3, "Cubic coefficient:");
 strcpy(parameter_names[5].name_a4, "Quartic coefficient:");
 strcpy(parameter_names[5].name_a5, "Amplitude 1:");
 strcpy(parameter_names[5].name_a6, "Frequency 1:");
 strcpy(parameter_names[5].name_a7, "Amplitude 2:");
 strcpy(parameter_names[5].name_a8, "Frequency 2");
 
 
 /* Parameter names for Cosines distortion */
 strcpy(parameter_names[6].name_a1, "x-Amplitude:");
 strcpy(parameter_names[6].name_a2, "y-Amplitude:");
 strcpy(parameter_names[6].name_a3, "x-Frequency #1:");
 strcpy(parameter_names[6].name_a4, "x-Frequency #2:");
 strcpy(parameter_names[6].name_a5, "y-Frequency #1:");
 strcpy(parameter_names[6].name_a6, "y-Frequency #2:");
 strcpy(parameter_names[6].name_a7, "unused:");
 strcpy(parameter_names[6].name_a8, "unused");
 
 
  /* Parameter names for Gauss distortion */
 strcpy(parameter_names[7].name_a1, "x-Strength:");
 strcpy(parameter_names[7].name_a2, "y-Strength:");
 strcpy(parameter_names[7].name_a3, "x-Offset:");
 strcpy(parameter_names[7].name_a4, "y-Offset:");
 strcpy(parameter_names[7].name_a5, "x-Deviation:");
 strcpy(parameter_names[7].name_a6, "y-Deviation:");
 strcpy(parameter_names[7].name_a7, "unused:");
 strcpy(parameter_names[7].name_a8, "unused");
 
 
 
 /* this paragraph initializes the letterings */
 strcpy(parameter_values.lettering01, parameter_names[0].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[0].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[0].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[0].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[0].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[0].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[0].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[0].name_a8);
 
 
 /* This section defines the 'effects' parameters */
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 0, 0.0, 0.0, 0.0, -3.5, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 1, -0.2, 0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 2, 0.45, -0.4, -0.5, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 3, -0.25, -0.5, 0.0, -4.5, 0.0, 0.0, 0.0, 0.0, TRUE, 0.3, 0.01);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 4, 1.6, -2.5, -3.7, -1.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 5, 1.8, -0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 6, -6.0, 4.0, -10.0, 0.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.0, 0.7);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 7, 0.15, 0.2, 5.0, -1.3, 0.0, 0.0, 0.0, 0.0, TRUE, 0.2, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_R_r, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 0, -0.65, 0.35, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 1, 0.1, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 2, 0.2, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 3, 0.2, 3.0, 0.5, 2.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 4, 0.2, 3.0, 0.5, 2.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.0, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 5, -1.0, -7.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 6, 0.7, -0.85, -0.25, -0.4, 0.0, 0.0, 0.0, 0.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 7, 0.0, 0.0, 10.0, -10.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.0, 0.7);
 SET_EFFECTS_PARAMS(FUNCTION_R_phi, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 0, 1.3, 0.9, -10.0, -10.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 1, 1.5, 0.5, -4.2, -6.7, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 2, -2.1, 1.0, 2.45, 5.8, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 3, -1.9, 1.0, 1.0, 6.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.2, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 4, -4.0, 10.0, 10.0, 10.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 5, 0.6, 3.0, 10.0, 10.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.25, 0.01);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 6, 0.0, 0.0, -1.85, -2.45, 0.0, 0.0, 0.0, 0.0, TRUE, 0.4, 0.01);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 7, -0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.4, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_r, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 0, 0.8, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 1, -0.3, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 2, -0.3, 10.0, 0.9, 8.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 3, -7.0, 10.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 4, 1.0, 8.0, -1.0, 4.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 5, -6.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.25, 0.4);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 6, -5.0, 5.0, 10.0, 1.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.25, 0.4);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 7, -0.5, 3.0, -1.0, 1.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Phi_phi, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 0, 1.0, 0.0, 0.0, 0.0, 0.8, 1.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 1, 0.6, 0.0, 0.0, 0.0, -0.3, 10.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 2, 0.3, -0.25, -2.5, 2.4, -0.3, 10.0, 0.9, 8.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 3, -0.05, 0.0, 0.0, 0.0, -7.0, 10.0, 2.0, 1.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 4, 1.85, -3.9, 0.0, 0.0, 1.0, 8.0, -1.0, 4.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 5, 0.1, 0.0, 0.0, 0.0, -6.0, 4.0, 0.0, 0.0, TRUE, 0.25, 0.4);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 6, 0.3, -0.35, 0.4, -2.8, -5.0, 5.0, 10.0, 1.0, TRUE, 0.25, 0.4);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 7, -2.6, -0.6, -3.1, 10.0, -0.5, 3.0, -1.0, 1.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_Phi_phi, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 0, 0.0, 2.1, -0.5, 10.0 ,-0.65, 0.35, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 1, 2.5, 0.0, 10.0, 6.0, 0.1, 4.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 2, 0.75, -6.5, -2.6, 5.6, 0.2, 6.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 3, 0.4, 1.0, 0.7, 0.7, 0.2, 3.0, 0.5, 2.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 4, -1.0, -1.4, 1.3, 1.3, 0.2, 3.0, 0.5, 2.0, TRUE, 0.0, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 5, 0.1, 0.4, -1.1, 0.1, -1.0, -7.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 6, -10.0, -10.0, -10.0, -10.0, 0.7, -1.0, -0.25, 3.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 7, 0.1, 0.35, -1.7, -2.3, 0.0, 0.0, 10.0, -10.0, TRUE, 0.0, 0.7);
 SET_EFFECTS_PARAMS(FUNCTION_R_r_R_phi, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 0, 0.8, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 1, 2.75, -2.75, 3.0, 0.0, -3.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 2, 0.8, -0.8, 1.0, -1.6, -1.0, -1.4, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 3, -0.1, -2.0, 1.0, -0.7, -10.0, -1.4, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 4, -0.2, -2.0, 1.0, -10.0, 0.25, 6.7, 0.0, 0.0, TRUE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 5, -6.2, 6.6, -3.8, -1.4, 2.0, 1.2, 0.0, 0.0, TRUE, 0.15, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 6, -6.2, 6.6, -3.8, -1.4, 2.0, 1.2, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 7, 10.0, 10.0, -1.7, -1.7, 2.1, 2.1, 0.0, 0.0, TRUE, 0.3, 0.6);
 SET_EFFECTS_PARAMS(FUNCTION_Cosines, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 
 
 
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 0, 4.0, 0.0, 0.0, 0.0, -9.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 1, 4.0, 4.0, 0.0, 0.0, -9.0, -9.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 2, 4.0, 4.0, 0.0, 0.0, -9.0, -9.0, 0.0, 0.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 3, 0.0, -2.4, 0.0, -3.0, 0.0, 0.0, 0.0, 0.0, FALSE, 0.5, 0.2);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 4, 2.3, -3.4, 0.0, -3.0, -9.0, 0.2, 0.0, 0.0, TRUE, 0.0, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 5, 10.0, -10.0, 4.0, -3.0, -6.0, 5.6, 0.0, 0.0, TRUE, 0.05, 0.5);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 6, 5.25, 0.0, -10.0, 0.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.25, 1.0);
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 7, -10.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, TRUE, 0.05, 1.0);
 
 SET_EFFECTS_PARAMS(FUNCTION_Gauss, 8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0.5, 0.2);
 
 
 
 
 switch (run_mode)
 {
  case RUN_INTERACTIVE: /*  Possibly retrieve data  */
                        gimp_get_data(PluginResource, &parameter_values);
                                 
                        /*  Create the preview  */
                        create_preview(drawable);

                        /*  First acquire information with a dialog  */
                        if(! user_dialog ()) return;                       
                        break;

  case RUN_NONINTERACTIVE: /*  Make sure all the arguments are there!  */
                           
                           if(nparams != N_PARAMETERS) status = STATUS_CALLING_ERROR;
                           if(status == STATUS_SUCCESS)
	                   {
	                    parameter_values.a1 =                param[3].data.d_float;
	                    parameter_values.a2 =                param[4].data.d_float;
	                    parameter_values.a3 =                param[5].data.d_float;
	                    parameter_values.a4 =                param[6].data.d_float;
	                    parameter_values.a5 =                param[7].data.d_float;
	                    parameter_values.a6 =                param[8].data.d_float;
	                    parameter_values.a7 =                param[9].data.d_float;
	                    parameter_values.a8 =                param[10].data.d_float;
	                    parameter_values.use_cutoff =        param[11].data.d_int32;
	                    parameter_values.r =                 param[12].data.d_float;
	                    parameter_values.dr =                param[13].data.d_float;
	                    parameter_values.current_function =  param[14].data.d_int32;
	                   }
      
                           break;

  case RUN_WITH_LAST_VALS: /*  Possibly retrieve data  */
                           gimp_get_data(PluginResource, &parameter_values);
                           break;

  default: break;
 }

 /*  Make sure that the drawable is gray or RGB color  */
 if(gimp_drawable_color(drawable->id) || gimp_drawable_gray(drawable->id))
 {
  if(do_action == 0) gimp_progress_init(PluginName);

  /* set the tile cache size  */
  gimp_tile_cache_ntiles(2 * (MAX(drawable->width, drawable->height) / gimp_tile_width () + 1));
  
  
  if(do_action == 0) image_action(drawable, parameter_values);

  if(run_mode != RUN_NONINTERACTIVE) gimp_displays_flush();

  /*  Store parameter data  */
  if(run_mode == RUN_INTERACTIVE) gimp_set_data(PluginResource, &parameter_values, sizeof(Parameters));
 }
 else
 {
  status = STATUS_EXECUTION_ERROR;
 }

 values[0].data.d_status = status;
 gimp_drawable_detach(drawable);
}





/******************************************************************************/
/*                       Dialog creation functions                            */
/******************************************************************************/

/* create_base_dialog() creates the dialog window and the action area. */
static void create_base_dialog(void)
{
 dtd.argc = 1;
 dtd.argv = g_new(gchar *, 1);
 dtd.argv[0] = g_strdup(PluginName);

 gtk_init(&dtd.argc, &dtd.argv);
 gtk_rc_parse(gimp_gtkrc ());

 dtd.dlg = gtk_dialog_new();
 gtk_window_set_title(GTK_WINDOW(dtd.dlg), PluginName);
 gtk_window_position(GTK_WINDOW(dtd.dlg), GTK_WIN_POS_MOUSE);
 gtk_signal_connect(GTK_OBJECT(dtd.dlg), "destroy",
		     (GtkSignalFunc)close_callback,
		     NULL);

 /*  Action area  */
 dtd.ok_button = gtk_button_new_with_label("OK");
 GTK_WIDGET_SET_FLAGS(dtd.ok_button, GTK_CAN_DEFAULT);
 gtk_signal_connect(GTK_OBJECT (dtd.ok_button), "clicked",
                   (GtkSignalFunc) ok_callback, GTK_OBJECT(dtd.dlg));
 gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dtd.dlg)->action_area), dtd.ok_button, TRUE, TRUE, 0);
 gtk_widget_grab_default(dtd.ok_button);
 gtk_widget_show(dtd.ok_button);

 dtd.cancel_button = gtk_button_new_with_label("Cancel");
 GTK_WIDGET_SET_FLAGS(dtd.cancel_button, GTK_CAN_DEFAULT);
 gtk_signal_connect_object(GTK_OBJECT(dtd.cancel_button), "clicked",
			  (GtkSignalFunc)cancel_callback,
			   GTK_OBJECT(dtd.dlg));
 gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dtd.dlg)->action_area), dtd.cancel_button, TRUE, TRUE, 0);
 gtk_widget_show(dtd.cancel_button);
}



/* Not really surprisingly this functon creates a slider and a label. */
/* The widget returned is the GtkObject *data, NOT the slider widget!!! */
static ObjectAndLabel create_slider_with_label(gchar *text, GtkWidget *container, GtkSignalFunc callback_func, gfloat initial_value, gfloat min_value, gfloat max_value)
{
 GtkObject *data;
 GtkWidget *label;
 GtkWidget *slider_field;
 GtkWidget *slider_widget;
 
 ObjectAndLabel ret;
 
 
 slider_field = gtk_hbox_new(FALSE, 1);
 gtk_container_border_width(GTK_CONTAINER(slider_field), 2);
 gtk_box_pack_start(GTK_BOX(container), slider_field, TRUE, TRUE, 0);
 
 label = gtk_label_new(text);
 gtk_widget_set_usize(label, 140, 0);
 gtk_box_pack_start(GTK_BOX(slider_field), label, TRUE, TRUE, 0);
 gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
 gtk_widget_show(label);
 
 data = gtk_adjustment_new(initial_value, min_value, max_value, 0.01, 0.01, 0);
 slider_widget = gtk_hscale_new(GTK_ADJUSTMENT(data));
 gtk_box_pack_start(GTK_BOX(slider_field), slider_widget, TRUE, TRUE, 0);
 
 gtk_widget_set_usize(slider_widget, 300, 0);
 gtk_scale_set_value_pos(GTK_SCALE(slider_widget), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(slider_widget), 2);
 gtk_range_set_update_policy(GTK_RANGE(slider_widget), GTK_UPDATE_DELAYED);
 
 gtk_signal_connect(GTK_OBJECT(data), "value_changed",
		   (GtkSignalFunc)callback_func,
		    &initial_value);

 gtk_widget_show(slider_widget);
 gtk_widget_show(slider_field);
 
 ret.object = (GtkWidget *)data;
 ret.label = (GtkLabel *)label;
 return(ret);
}



/*                        WARNING                    */
/* I need this function, taken directly from the gtk code, here.
   My linker refuses to link this function from the gtk library.
   
   Further information:
   
   GIMP version:   0.99.10
   gcc  version:   2.7.2.
   ld   version:   cygnus-2.6 (with BFD 2.6.0.14)
   
   All other gtk functions worked so far.
   
                                Peter
*/
static void
gtk_option_menu_remove_contents(GtkOptionMenu *option_menu)
{
  g_return_if_fail (option_menu != NULL);
  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

  if (GTK_BUTTON (option_menu)->child)
    {
      gtk_container_block_resize (GTK_CONTAINER (option_menu));
      if (GTK_WIDGET (option_menu->menu_item)->state != GTK_BUTTON (option_menu)->child->state)
	gtk_widget_set_state (GTK_BUTTON (option_menu)->child,
			      GTK_WIDGET (option_menu->menu_item)->state);
      GTK_WIDGET_UNSET_FLAGS (GTK_BUTTON (option_menu)->child, GTK_MAPPED | GTK_REALIZED);
      gtk_widget_reparent (GTK_BUTTON (option_menu)->child, option_menu->menu_item);
      gtk_container_unblock_resize (GTK_CONTAINER (option_menu));
      option_menu->menu_item = NULL;
    }
}




static void create_functions_menu(GtkWidget *container)
{
 GtkWidget *menu_field;
 GtkWidget *label;
 GtkWidget *menu;
 GtkWidget *menuitem;
 GtkWidget *chooser;
 GtkWidget *child;
 GtkWidget *current_item;
 gint dummy;
 
 menu_field = gtk_hbox_new(FALSE, 1);
 gtk_container_border_width(GTK_CONTAINER(menu_field), 2);
 gtk_box_pack_start(GTK_BOX(container), menu_field, TRUE, TRUE, 0);
 
 label = gtk_label_new("Functions:");
 gtk_widget_set_usize(label, 70, 0);
 gtk_box_pack_start(GTK_BOX(menu_field), label, TRUE, TRUE, 0);
 gtk_widget_show(label);
 
 
 chooser = gtk_option_menu_new();
 gtk_box_pack_start(GTK_BOX(menu_field), chooser, TRUE, TRUE, 0);
 gtk_widget_set_usize(chooser, 180, 0);
 
 
 menu = gtk_menu_new();
 
 
 menuitem = gtk_menu_item_new_with_label("R(r) - 4th order poly");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function01_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_R_r) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("R(phi) - cosine");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function02_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_R_phi) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("Phi(r) - 4th order poly");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function03_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_Phi_r) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("Phi(phi) - cosine");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function04_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_Phi_phi) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("R(r)Phi(phi)");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function05_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_R_r_Phi_phi) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("R(r)R(phi)");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function06_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_R_r_R_phi) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("Cosines");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function07_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_Cosines) current_item = menuitem;
 
 menuitem = gtk_menu_item_new_with_label("Gauss");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)function08_callback, &dummy);
 gtk_widget_show(menuitem);
 if(parameter_values.current_function == FUNCTION_Gauss) current_item = menuitem;
 
 
 
 gtk_option_menu_set_menu(GTK_OPTION_MENU(chooser), menu);

 /* BEGIN - This code was taken from gtk 0.99.10*/
 
 /* Update the chooser in order to display the CORRECT function */
 gtk_option_menu_remove_contents(GTK_OPTION_MENU(chooser));

 GTK_OPTION_MENU(chooser)->menu_item = current_item;
 child = GTK_BIN(GTK_OPTION_MENU(chooser)->menu_item)->child;
 if(child)
 {
  gtk_container_block_resize(GTK_CONTAINER(chooser));
  if(GTK_WIDGET(chooser)->state != child->state)
        gtk_widget_set_state (child, GTK_WIDGET(chooser)->state);
  gtk_widget_reparent(child, GTK_WIDGET(chooser));
  gtk_container_unblock_resize(GTK_CONTAINER(chooser));
 }
 
 gtk_widget_size_allocate(GTK_WIDGET(chooser), &(GTK_WIDGET(chooser)->allocation));

 if(GTK_WIDGET_DRAWABLE(chooser)) gtk_widget_draw(GTK_WIDGET(chooser), NULL);

 /* END - This code was taken from gtk 0.99.10. Thanks to the developers.*/

 
 gtk_widget_show(chooser);
 
 
 gtk_widget_show(menu_field);
}
                                  
                                  
                                  
                                  
                                               
static void create_effects_menu(GtkWidget *container)
{
 GtkWidget *menu_field;
 GtkWidget *label;
 GtkWidget *menu;
 GtkWidget *menuitem;
 GtkWidget *chooser;
 
 gint dummy;
 
 menu_field = gtk_hbox_new(FALSE, 1);
 gtk_container_border_width(GTK_CONTAINER(menu_field), 2);
 gtk_box_pack_start(GTK_BOX(container), menu_field, TRUE, TRUE, 0);
 
 label = gtk_label_new("Effects:");
 gtk_widget_set_usize(label, 70, 0);
 gtk_box_pack_start(GTK_BOX(menu_field), label, TRUE, TRUE, 0);
 gtk_widget_show(label);
 
 
 menu = gtk_menu_new();
 
 
 menuitem = gtk_menu_item_new_with_label("Effect #1");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect01_callback, &dummy); 
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #2");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect02_callback, &dummy); 
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #3");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect03_callback, &dummy);  
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #4");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect04_callback, &dummy); 
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #5");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect05_callback, &dummy);  
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #6");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect06_callback, &dummy); 
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #7");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect07_callback, &dummy); 
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Effect #8");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect08_callback, &dummy);
 gtk_widget_show(menuitem);
 
 menuitem = gtk_menu_item_new_with_label("Reset");
 gtk_menu_append(GTK_MENU(menu), menuitem);
 gtk_signal_connect(GTK_OBJECT(menuitem), "activate", (GtkSignalFunc)effect09_callback, &dummy);
 gtk_widget_show(menuitem);
 
 
 
 
 
 chooser = gtk_option_menu_new();
 gtk_box_pack_start(GTK_BOX(menu_field), chooser, TRUE, TRUE, 0);
 
 gtk_widget_set_usize(chooser, 180, 0);
 
 
 gtk_option_menu_set_menu(GTK_OPTION_MENU(chooser), menu);
 gtk_widget_show(chooser);
 
 
 gtk_widget_show(menu_field);
}
                 




/* This function creates the panel for the cutoff function parameters. */
static void create_cutoff_panel(GtkWidget *container)
{
 GtkObject *data;
 GtkWidget *label;
 GtkWidget *slider;
 GtkWidget *toggle;
 
 gfloat dummy;
 gint int_dummy;
 
 
 toggle = gtk_check_button_new_with_label("Use cutoff");
 widget_data.toggle01 = toggle;
 gtk_box_pack_start(GTK_BOX(container), toggle, FALSE, FALSE, 0);
 gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		   (GtkSignalFunc)use_cutoff_callback,
		    &int_dummy);
 gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), parameter_values.use_cutoff);
 
 gtk_misc_set_alignment(GTK_MISC(toggle), 0.0, 0.5);
 gtk_widget_show (toggle);

 
 label = gtk_label_new("r");
 gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 0);
 gtk_misc_set_alignment(GTK_MISC(label), 0.8, 0.5);
 gtk_widget_show(label);
 
 
 data = gtk_adjustment_new(parameter_values.r, 0.0, 1.0, 0.01, 0.01, 0);
 widget_data.slider09 = (GtkWidget *)data;
 slider = gtk_hscale_new(GTK_ADJUSTMENT(data));
 gtk_box_pack_start(GTK_BOX(container), slider, TRUE, TRUE, 0);
 
 gtk_widget_set_usize(slider, 100, 0);
 gtk_scale_set_value_pos(GTK_SCALE(slider), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(slider), 2);
 gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DELAYED);
 
 gtk_signal_connect(GTK_OBJECT(data), "value_changed",
		   (GtkSignalFunc)r_callback,
		    &dummy);
		    
 gtk_widget_show(slider);
 
 		    
		    
		    
 label = gtk_label_new("dr");
 gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 0);
 gtk_misc_set_alignment(GTK_MISC(label), 0.8, 0.5);
 gtk_widget_show(label);
 
 data = gtk_adjustment_new(parameter_values.dr, 0.01, 1.0, 0.01, 0.01, 0);
 widget_data.slider10 = (GtkWidget *)data;
 slider = gtk_hscale_new(GTK_ADJUSTMENT(data));
 gtk_box_pack_start(GTK_BOX(container), slider, TRUE, TRUE, 0);
 
 gtk_widget_set_usize(slider, 100, 0);
 gtk_scale_set_value_pos(GTK_SCALE(slider), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(slider), 2);
 gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DELAYED);
 
 gtk_signal_connect(GTK_OBJECT(data), "value_changed",
		   (GtkSignalFunc)dr_callback,
		    &dummy);		    
		    
 gtk_widget_show(slider);		    
		   		  		    	 
}



/* This function creates a preview. It should be called only once. */
/* This function writes all data to a global PreviewData preview_data */
static void create_preview(GDrawable *drawable)
{
 gint i, j;
 gint x0, y0;
 gint x1, y1, x2, y2;
 gint image_width, image_height;
 guchar *image_bytes;
 gint image_bpp;
 gint offset;
 gint count;
 gfloat delta_pixel;
 
 GPixelRgn image_rgn;
 
 
 gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

 image_width = (x2 - x1);
 image_height = (y2 - y1);
 image_bpp = drawable->bpp;
 image_bytes = (guchar *)malloc(image_bpp*image_width);
 

 if(image_width > image_height)
 {
  preview_data.scale = (gfloat)image_width/(gfloat)preview_width;
  preview_data.width = preview_width;
  preview_data.height = (gfloat)(image_height)/(preview_data.scale);
  x0 = 0;
  y0 = (preview_height - preview_data.height)/2;
 }
 else
 {
  preview_data.scale = (gfloat)image_height/(gfloat)preview_height;
  preview_data.height = preview_height;
  preview_data.width = (gfloat)(image_width)/(preview_data.scale);
  x0 = (preview_width - preview_data.width)/2;
  y0 = 0;
 }

 preview_data.bpp = 3;
 
 preview_data.bits = (guchar *)malloc(preview_data.bpp*preview_width*preview_height);

 /* Clear the preview area */
 count = 0;
 for(i = 0; i < preview_data.bpp*preview_width*preview_height; i++) preview_data.bits[count++] = 0;


 /* Copy a thumbnail of the original image to the preview image. */
 gimp_pixel_rgn_init(&image_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
 
 for(j = 0; j < preview_data.height; j++)
 {
  offset = preview_data.bpp*((y0 + j)*preview_width + x0);
  count = 0;
  delta_pixel = 0.0;
  gimp_pixel_rgn_get_row(&image_rgn, image_bytes, x1, y1 + (int)(preview_data.scale*(gfloat)j), image_width);
  
  for(i = 0; i < preview_data.width; i++)
  {
   switch(image_bpp)
   {
    case 4:  preview_data.bits[offset++] = image_bytes[count++];
             preview_data.bits[offset++] = image_bytes[count++];
             preview_data.bits[offset++] = image_bytes[count++];
   
             delta_pixel += preview_data.scale;
             count = 4*(int)delta_pixel;
             break;
             
    case 3:  preview_data.bits[offset++] = image_bytes[count++];
             preview_data.bits[offset++] = image_bytes[count++];
             preview_data.bits[offset++] = image_bytes[count++];
   
             delta_pixel += preview_data.scale;
             count = 3*(int)delta_pixel;
             break;
             
    case 2:  preview_data.bits[offset++] = image_bytes[count];
             preview_data.bits[offset++] = image_bytes[count];
             preview_data.bits[offset++] = image_bytes[count];
   
             delta_pixel += preview_data.scale;
             count = 2*(int)delta_pixel;
             break;
             
     case 1: preview_data.bits[offset++] = image_bytes[count];
             preview_data.bits[offset++] = image_bytes[count];
             preview_data.bits[offset++] = image_bytes[count];
   
             delta_pixel += preview_data.scale;
             count = (int)delta_pixel;
             break;
             
   }
  }
 } 
}



/* This function re-calculates the preview image and displays it */
static void update_preview(GtkWidget *preview)
{
 guchar *dest;
 gint i, j;
 gfloat u, v;
 gint count;
 
 if(do_update_preview != 1) return;
 
 dest = (guchar *)malloc(3*preview_width);
 

 /* Distort and display the preview */
 for(j = 0; j < preview_height; j++)
 {
  count = 0;
  for(i = 0; i < preview_width; i++)
  {
   calculate_distortion_function(i, j, preview_width, preview_height, &u, &v);
   
   if((u >= 0) && (u < preview_width) && (v >= 0) && (v < preview_height))
   {         
    dest[count] = preview_data.bits[3*((int)v*preview_width + (int)u)]; count++;
    dest[count] = preview_data.bits[3*((int)v*preview_width + (int)u)+1]; count++;
    dest[count] = preview_data.bits[3*((int)v*preview_width + (int)u)+2]; count++;           
   }
   else
   {
    dest[count] = 0; count++;
    dest[count] = 0; count++;
    dest[count] = 0; count++;             
   }
  }
  gtk_preview_draw_row(GTK_PREVIEW(preview), dest, 0, j, preview_width);      
 }
            
           
 gtk_widget_draw(preview, NULL);
 
 free(dest);			     
}




/* This function creates the complete user dialog */
static gint user_dialog()
{
 GtkWidget *frame;
 
 GtkWidget *main_box;
 GtkWidget *north_box;
 GtkWidget *south_box;
 GtkWidget *west_box;
 
 GtkWidget *preview_box;
 GtkWidget *parameter_box;
 GtkWidget *effects_box;
 GtkWidget *cutoff_box;
 
 GtkWidget *preview_frame;
 GtkWidget *preview_display;
 
 GtkWidget *functions_frame;
 GtkWidget *functions_box;
 
 ObjectAndLabel oal;
 
 
 GtkWidget *toggle;
 
 
 gchar buffer[12];
 
 /* Create a basic dialog window */
 create_base_dialog();
 
 
 /*  The main box -- contains the north and south box  */
 main_box = gtk_vbox_new(FALSE, 1);
 gtk_container_border_width(GTK_CONTAINER(main_box), 15);
 gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dtd.dlg)->vbox), main_box, TRUE, TRUE, 0);


 /* Add the north box */
 north_box = gtk_hbox_new(FALSE, 1);
 gtk_box_pack_start(GTK_BOX(main_box), north_box, TRUE, TRUE, 0);


 west_box = gtk_vbox_new(FALSE, 0);
 gtk_container_border_width(GTK_CONTAINER(west_box), 0);
 gtk_container_add(GTK_CONTAINER(north_box), west_box);

 /* Add the preview display */ 
 preview_frame = gtk_frame_new("Preview image");
 gtk_frame_set_shadow_type(GTK_FRAME(preview_frame), GTK_SHADOW_ETCHED_IN);
 gtk_box_pack_start(GTK_BOX(west_box), preview_frame, TRUE, TRUE, 0);
 
 preview_box = gtk_vbox_new(FALSE, 0);
 gtk_container_border_width(GTK_CONTAINER(preview_box), 10);
 gtk_container_add(GTK_CONTAINER(preview_frame), preview_box);

 frame = gtk_frame_new(NULL);
 gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
 gtk_box_pack_start(GTK_BOX(preview_box), frame, TRUE, TRUE, 0);

 preview_display = gtk_preview_new(GTK_PREVIEW_COLOR);
 gtk_preview_size(GTK_PREVIEW(preview_display), preview_width, preview_height);
 gtk_container_add(GTK_CONTAINER(frame), preview_display);
 
 preview_data.display = preview_display;
 update_preview(preview_data.display);
 
 
 
 
 /* Add the functions/effects frame */ 
 functions_frame = gtk_frame_new("Functions / Effects");
 gtk_frame_set_shadow_type(GTK_FRAME(functions_frame), GTK_SHADOW_ETCHED_IN);
 gtk_box_pack_start(GTK_BOX(west_box), functions_frame, FALSE, FALSE, 0);
 
 functions_box = gtk_vbox_new(FALSE, 0);
 gtk_container_border_width(GTK_CONTAINER(functions_box), 10);
 gtk_container_add(GTK_CONTAINER(functions_frame), functions_box);
 
 
 
 
 
 
 /* Add a test object to the functions/effects box (to be replaced.*/
 create_functions_menu(functions_box);
 create_effects_menu(functions_box);
 
 
 
 
 
 
 gtk_widget_show(functions_box);
 gtk_widget_show(functions_frame);
 
 
 
 
 gtk_widget_show(preview_display);
 gtk_widget_show(frame);
 gtk_widget_show(preview_frame);
 
 gtk_widget_show(preview_box);
 gtk_widget_show(west_box);




 /*  Create the parameter control box  */
 frame = gtk_frame_new("Parameter settings");
 gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
 gtk_box_pack_start(GTK_BOX(north_box), frame, TRUE, TRUE, 0);

 parameter_box = gtk_vbox_new(FALSE, 5);
 gtk_container_add(GTK_CONTAINER(frame), parameter_box);


 oal = create_slider_with_label(parameter_values.lettering01, parameter_box, (GtkSignalFunc)a1_callback, parameter_values.a1, -10.0, 10.0);
 widget_data.slider01 = oal.object;
 widget_data.label01 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering02, parameter_box, (GtkSignalFunc)a2_callback, parameter_values.a2, -10.0, 10.0);
 widget_data.slider02 = oal.object;
 widget_data.label02 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering03, parameter_box, (GtkSignalFunc)a3_callback, parameter_values.a3, -10.0, 10.0);
 widget_data.slider03 = oal.object;
 widget_data.label03 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering04, parameter_box, (GtkSignalFunc)a4_callback, parameter_values.a4, -10.0, 10.0);
 widget_data.slider04 = oal.object;
 widget_data.label04 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering05, parameter_box, (GtkSignalFunc)a5_callback, parameter_values.a5, -10.0, 10.0);
 widget_data.slider05 = oal.object;
 widget_data.label05 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering06, parameter_box, (GtkSignalFunc)a6_callback, parameter_values.a6, -10.0, 10.0);
 widget_data.slider06 = oal.object;
 widget_data.label06 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering07, parameter_box, (GtkSignalFunc)a7_callback, parameter_values.a7, -10.0, 10.0);
 widget_data.slider07 = oal.object;
 widget_data.label07 = oal.label;
 
 oal = create_slider_with_label(parameter_values.lettering08, parameter_box, (GtkSignalFunc)a8_callback, parameter_values.a8, -10.0, 10.0);
 widget_data.slider08 = oal.object;
 widget_data.label08 = oal.label;


 gtk_widget_show(parameter_box);
 gtk_widget_show(frame);
 gtk_widget_show(north_box);
 
 
 
 
 /* Create the south box (contains the cutoff panel)*/
 south_box = gtk_hbox_new(FALSE, 5);
 gtk_box_pack_start(GTK_BOX(main_box), south_box, TRUE, TRUE, 0);
 
 frame = gtk_frame_new("Cutoff function");
 gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
 gtk_box_pack_start(GTK_BOX(south_box), frame, TRUE, TRUE, 0);

 cutoff_box = gtk_hbox_new(FALSE, 5);
 gtk_container_border_width(GTK_CONTAINER(cutoff_box), 10);
 gtk_container_add(GTK_CONTAINER(frame), cutoff_box);
 

 create_cutoff_panel(cutoff_box);
 
 gtk_widget_show(cutoff_box);
 gtk_widget_show(frame);
 gtk_widget_show(south_box);
 
 
 gtk_widget_show(main_box);
 
 gtk_widget_show(dtd.dlg);

 gtk_main ();
 gdk_flush ();
  
 return 1;
}



/* Modify the dialog controls such that they resemble the actual 
   parameter values. The parameters are modified by selecting an effect.
*/
static void update_dialog(void)
{
 GtkAdjustment *adjustment;
 
 adjustment = (GtkAdjustment *)widget_data.slider01;
 adjustment->value = parameter_values.a1;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label01, parameter_values.lettering01);
 
 adjustment = (GtkAdjustment *)widget_data.slider02;
 adjustment->value = parameter_values.a2;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label02, parameter_values.lettering02);
 
 adjustment = (GtkAdjustment *)widget_data.slider03;
 adjustment->value = parameter_values.a3;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label03, parameter_values.lettering03);
 
 adjustment = (GtkAdjustment *)widget_data.slider04;
 adjustment->value = parameter_values.a4;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label04, parameter_values.lettering04);
 
 adjustment = (GtkAdjustment *)widget_data.slider05;
 adjustment->value = parameter_values.a5;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label05, parameter_values.lettering05);
 
 adjustment = (GtkAdjustment *)widget_data.slider06;
 adjustment->value = parameter_values.a6;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label06, parameter_values.lettering06);
 
 adjustment = (GtkAdjustment *)widget_data.slider07;
 adjustment->value = parameter_values.a7;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label07, parameter_values.lettering07);
 
 adjustment = (GtkAdjustment *)widget_data.slider08;
 adjustment->value = parameter_values.a8;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 gtk_label_set(widget_data.label08, parameter_values.lettering08);
 
 
 
 adjustment = (GtkAdjustment *)widget_data.slider09;
 adjustment->value = parameter_values.r;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 
 adjustment = (GtkAdjustment *)widget_data.slider10;
 adjustment->value = parameter_values.dr;
 gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
 
 gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(widget_data.toggle01), parameter_values.use_cutoff);
}


/******************************************************************************/
/*                          Callback functions                                */
/******************************************************************************/

static void close_callback(GtkWidget *widget, gpointer data)
{
 gtk_main_quit();
}

static void ok_callback(GtkWidget *widget, gpointer data)
{
 gtk_widget_destroy(GTK_WIDGET(data));
}

void cancel_callback(GtkWidget *widget, gpointer data)
{
 do_action = -1;
 gtk_main_quit();
}



static void a1_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a1 = adjustment->value;
 update_preview(preview_data.display);
}

static void a2_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a2 = adjustment->value;
 update_preview(preview_data.display);
}

static void a3_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a3 = adjustment->value;
 update_preview(preview_data.display);
}

static void a4_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a4 = adjustment->value;
 update_preview(preview_data.display);
}

static void a5_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a5 = adjustment->value;
 update_preview(preview_data.display);
}

static void a6_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a6 = adjustment->value;
 update_preview(preview_data.display);
}

static void a7_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a7 = adjustment->value;
 update_preview(preview_data.display);
}

static void a8_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.a8 = adjustment->value;
 update_preview(preview_data.display);
}




static void use_cutoff_callback(GtkWidget *widget, gpointer data)
{
 parameter_values.use_cutoff = 0;
 if(GTK_TOGGLE_BUTTON(widget)->active) parameter_values.use_cutoff = 1;
 update_preview(preview_data.display);
}

static void r_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.r = adjustment->value;
 update_preview(preview_data.display);
}

static void dr_callback(GtkAdjustment *adjustment, gpointer data)
{
 parameter_values.dr = adjustment->value;
 update_preview(preview_data.display);
}







static void effect01_callback(GtkWidget *widget, gpointer data)
{ 
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][0].a1;                 
 parameter_values.a2 = EffectsParameters[f][0].a2;                 
 parameter_values.a3 = EffectsParameters[f][0].a3;                 
 parameter_values.a4 = EffectsParameters[f][0].a4;                 
 parameter_values.a5 = EffectsParameters[f][0].a5;                 
 parameter_values.a6 = EffectsParameters[f][0].a6;                 
 parameter_values.a7 = EffectsParameters[f][0].a7;                 
 parameter_values.a8 = EffectsParameters[f][0].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][0].use_cutoff;         
 parameter_values.r = EffectsParameters[f][0].r;                   
 parameter_values.dr = EffectsParameters[f][0].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect02_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][1].a1;                 
 parameter_values.a2 = EffectsParameters[f][1].a2;                 
 parameter_values.a3 = EffectsParameters[f][1].a3;                 
 parameter_values.a4 = EffectsParameters[f][1].a4;                 
 parameter_values.a5 = EffectsParameters[f][1].a5;                 
 parameter_values.a6 = EffectsParameters[f][1].a6;                 
 parameter_values.a7 = EffectsParameters[f][1].a7;                 
 parameter_values.a8 = EffectsParameters[f][1].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][1].use_cutoff;         
 parameter_values.r = EffectsParameters[f][1].r;                   
 parameter_values.dr = EffectsParameters[f][1].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect03_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][2].a1;                 
 parameter_values.a2 = EffectsParameters[f][2].a2;                 
 parameter_values.a3 = EffectsParameters[f][2].a3;                 
 parameter_values.a4 = EffectsParameters[f][2].a4;                 
 parameter_values.a5 = EffectsParameters[f][2].a5;                 
 parameter_values.a6 = EffectsParameters[f][2].a6;                 
 parameter_values.a7 = EffectsParameters[f][2].a7;                 
 parameter_values.a8 = EffectsParameters[f][2].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][2].use_cutoff;         
 parameter_values.r = EffectsParameters[f][2].r;                   
 parameter_values.dr = EffectsParameters[f][2].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect04_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][3].a1;                 
 parameter_values.a2 = EffectsParameters[f][3].a2;                 
 parameter_values.a3 = EffectsParameters[f][3].a3;                 
 parameter_values.a4 = EffectsParameters[f][3].a4;                 
 parameter_values.a5 = EffectsParameters[f][3].a5;                 
 parameter_values.a6 = EffectsParameters[f][3].a6;                 
 parameter_values.a7 = EffectsParameters[f][3].a7;                 
 parameter_values.a8 = EffectsParameters[f][3].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][3].use_cutoff;         
 parameter_values.r = EffectsParameters[f][3].r;                   
 parameter_values.dr = EffectsParameters[f][3].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect05_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][4].a1;                 
 parameter_values.a2 = EffectsParameters[f][4].a2;                 
 parameter_values.a3 = EffectsParameters[f][4].a3;                 
 parameter_values.a4 = EffectsParameters[f][4].a4;                 
 parameter_values.a5 = EffectsParameters[f][4].a5;                 
 parameter_values.a6 = EffectsParameters[f][4].a6;                 
 parameter_values.a7 = EffectsParameters[f][4].a7;                 
 parameter_values.a8 = EffectsParameters[f][4].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][4].use_cutoff;         
 parameter_values.r = EffectsParameters[f][4].r;                   
 parameter_values.dr = EffectsParameters[f][4].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect06_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][5].a1;                 
 parameter_values.a2 = EffectsParameters[f][5].a2;                 
 parameter_values.a3 = EffectsParameters[f][5].a3;                 
 parameter_values.a4 = EffectsParameters[f][5].a4;                 
 parameter_values.a5 = EffectsParameters[f][5].a5;                 
 parameter_values.a6 = EffectsParameters[f][5].a6;                 
 parameter_values.a7 = EffectsParameters[f][5].a7;                 
 parameter_values.a8 = EffectsParameters[f][5].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][5].use_cutoff;         
 parameter_values.r = EffectsParameters[f][5].r;                   
 parameter_values.dr = EffectsParameters[f][5].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect07_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][6].a1;                 
 parameter_values.a2 = EffectsParameters[f][6].a2;                 
 parameter_values.a3 = EffectsParameters[f][6].a3;                 
 parameter_values.a4 = EffectsParameters[f][6].a4;                 
 parameter_values.a5 = EffectsParameters[f][6].a5;                 
 parameter_values.a6 = EffectsParameters[f][6].a6;                 
 parameter_values.a7 = EffectsParameters[f][6].a7;                 
 parameter_values.a8 = EffectsParameters[f][6].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][6].use_cutoff;         
 parameter_values.r = EffectsParameters[f][6].r;                   
 parameter_values.dr = EffectsParameters[f][6].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect08_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][7].a1;                 
 parameter_values.a2 = EffectsParameters[f][7].a2;                 
 parameter_values.a3 = EffectsParameters[f][7].a3;                 
 parameter_values.a4 = EffectsParameters[f][7].a4;                 
 parameter_values.a5 = EffectsParameters[f][7].a5;                 
 parameter_values.a6 = EffectsParameters[f][7].a6;                 
 parameter_values.a7 = EffectsParameters[f][7].a7;                 
 parameter_values.a8 = EffectsParameters[f][7].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][7].use_cutoff;         
 parameter_values.r = EffectsParameters[f][7].r;                   
 parameter_values.dr = EffectsParameters[f][7].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void effect09_callback(GtkWidget *widget, gpointer data)
{
 gint f = parameter_values.current_function;
 
 parameter_values.a1 = EffectsParameters[f][8].a1;                 
 parameter_values.a2 = EffectsParameters[f][8].a2;                 
 parameter_values.a3 = EffectsParameters[f][8].a3;                 
 parameter_values.a4 = EffectsParameters[f][8].a4;                 
 parameter_values.a5 = EffectsParameters[f][8].a5;                 
 parameter_values.a6 = EffectsParameters[f][8].a6;                 
 parameter_values.a7 = EffectsParameters[f][8].a7;                 
 parameter_values.a8 = EffectsParameters[f][8].a8;                 
 parameter_values.use_cutoff = EffectsParameters[f][8].use_cutoff;         
 parameter_values.r = EffectsParameters[f][8].r;                   
 parameter_values.dr = EffectsParameters[f][8].dr;  
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}




static void function01_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[0].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[0].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[0].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[0].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[0].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[0].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[0].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[0].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_R_r;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}


static void function02_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[1].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[1].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[1].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[1].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[1].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[1].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[1].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[1].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_R_phi;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void function03_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[2].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[2].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[2].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[2].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[2].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[2].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[2].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[2].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_Phi_r;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void function04_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[3].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[3].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[3].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[3].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[3].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[3].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[3].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[3].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_Phi_phi;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void function05_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[4].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[4].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[4].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[4].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[4].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[4].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[4].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[4].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_R_r_Phi_phi;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void function06_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[5].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[5].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[5].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[5].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[5].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[5].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[5].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[5].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_R_r_R_phi;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}

static void function07_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[6].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[6].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[6].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[6].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[6].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[6].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[6].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[6].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_Cosines;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}



static void function08_callback(GtkWidget *widget, gpointer data)
{
 strcpy(parameter_values.lettering01, parameter_names[7].name_a1);
 strcpy(parameter_values.lettering02, parameter_names[7].name_a2);
 strcpy(parameter_values.lettering03, parameter_names[7].name_a3);
 strcpy(parameter_values.lettering04, parameter_names[7].name_a4);
 strcpy(parameter_values.lettering05, parameter_names[7].name_a5);
 strcpy(parameter_values.lettering06, parameter_names[7].name_a6);
 strcpy(parameter_values.lettering07, parameter_names[7].name_a7);
 strcpy(parameter_values.lettering08, parameter_names[7].name_a8);
 
 parameter_values.a1 = 0; parameter_values.a5 = 0;
 parameter_values.a2 = 0; parameter_values.a6 = 0;
 parameter_values.a3 = 0; parameter_values.a7 = 0;
 parameter_values.a4 = 0; parameter_values.a8 = 0;
 
 parameter_values.use_cutoff = 0;         
 parameter_values.r = 0.5;                   
 parameter_values.dr = 0.2;  
 
 parameter_values.current_function = FUNCTION_Gauss;
 
 do_update_preview = 0;
 update_dialog();
 do_update_preview = 1;
 update_preview(preview_data.display);
}






/******************************************************************************/
/*                    Image manipulation functions                            */
/******************************************************************************/


void image_action(GDrawable *drawable, Parameters params)
{
 gint width, height;
 gint bytes;
 gint x1, y1, x2, y2;
 GPixelRgn src_rgn, dest_rgn;
 guchar *dest;
 guchar *src;
 
 guchar r, g, b;

 
 gfloat u, v;
 gint image_size;
 
 gint i, j;
 gint count; 
  
 gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

 width = (x2 - x1);
 height = (y2 - y1);
 bytes = drawable->bpp;
 
 image_size = width*height*bytes;
 
 /* Allocate the necessary storage memory */
 src = (guchar *)malloc(image_size);
 dest = (guchar *)malloc(width * bytes);
 
 gimp_pixel_rgn_init(&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
 gimp_pixel_rgn_init(&dest_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

 /* Get the source image */
 get_source_image(src_rgn, src, x1, y1, width, height, bytes);
 
 switch(bytes)
 {
  case 4:   for(j = 0; j < height; j++)
            {
             count = 0;
             for(i = 0; i < width; i++)
             {
              calculate_distortion_function(i, j, width, height, &u, &v);
   
              if((u >= 0) && (u < width) && (v >= 0) && (v < height))
              {
               calculate_pixel_color(u, v, src, width, height, bytes, &r, &g, &b);               
               dest[count] = r; count++;
               dest[count] = g; count++;
               dest[count] = b; count++;
               dest[count] = 255; count++;
              }
              else
              {
               dest[count] = 0; count++;
               dest[count] = 0; count++;
               dest[count] = 0; count++;
               dest[count] = 0; count++;
              }
             }
             gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, j + y1, width);
             gimp_progress_update((double)j / (double)height);
            }
            break;
            
  case 3:   for(j = 0; j < height; j++)
            {
             count = 0;
             for(i = 0; i < width; i++)
             {
              calculate_distortion_function(i, j, width, height, &u, &v);
   
              if((u >= 0) && (u < width) && (v >= 0) && (v < height))
              {
               calculate_pixel_color(u, v, src, width, height, bytes, &r, &g, &b);               
               dest[count] = r; count++;
               dest[count] = g; count++;
               dest[count] = b; count++;           
              }
              else
              {
               dest[count] = 0; count++;
               dest[count] = 0; count++;
               dest[count] = 0; count++;             
              }
             }
             gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, j + y1, width);
             gimp_progress_update((double)j / (double)height);
            }
            break;
            
  case 2:   for(j = 0; j < height; j++)
            {
             count = 0;
             for(i = 0; i < width; i++)
             {
              calculate_distortion_function(i, j, width, height, &u, &v);
   
              if((u >= 0) && (u < width) && (v >= 0) && (v < height))
              {
               calculate_pixel_color(u, v, src, width, height, bytes, &r, &g, &b);               
               dest[count] = r;   count++;          
               dest[count] = 255; count++;
              }
              else
              {
               dest[count] = 0; count++;              
               dest[count] = 0; count++;
              }
             }
             gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, j + y1, width);
             gimp_progress_update((double)j / (double)height);
            }
            break;
            
  case 1:   for(j = 0; j < height; j++)
            {
             count = 0;
             for(i = 0; i < width; i++)
             {
              calculate_distortion_function(i, j, width, height, &u, &v);
   
              if((u >= 0) && (u < width) && (v >= 0) && (v < height))
              {
               calculate_pixel_color(u, v, src, width, height, bytes, &r, &g, &b);               
               dest[count] = r; count++;            
              }
              else
              {
               dest[count] = 0; count++;          
              }
             }
             gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, j + y1, width);
             gimp_progress_update((double)j / (double)height);
            }
            break;
 }

 /*  merge the shadow, update the drawable  */
 gimp_drawable_flush(drawable);
 gimp_drawable_merge_shadow(drawable->id, TRUE);
 gimp_drawable_update(drawable->id, x1, y1, width, height);

 
 free(src);
 free(dest);
}



static void polar_coordinates(gfloat x, gfloat y, gfloat *rho, gfloat *phi)
{
 gfloat r, p;
 
 /* Calculate the radius */
 r = sqrt(x*x + y*y);
 
 /* exceptions first .. */
 if((x == 0.0) && (y == 0.0)) p = 0.0; 
 else if((x < epsilon) && (x > -epsilon) && (y > 0.0)) p = M_PI/2.0;
 else if((x < epsilon) && (x > -epsilon) && (y < 0.0)) p = 3.0*M_PI/2.0;
 else if((x > 0.0) && (y < epsilon) && (y > -epsilon)) p = 0.0;
 else if((x < 0.0) && (y < epsilon) && (y > -epsilon)) p = M_PI;
 
 /* simple cases in the four quadrants */
 else if((x > 0.0) && (y > 0.0)) p = atan(y/x);
 else if((x < 0.0) && (y > 0.0)) p = M_PI - atan(-y/x);
 else if((x < 0.0) && (y < 0.0)) p = M_PI + atan(y/x);
 else if((x > 0.0) && (y < 0.0)) p = 2.0*M_PI - atan(-y/x);
 
 /* Return the values */
 *rho = r;
 *phi = p;
}



/* The distortion functions */

static void distortion_function01(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 gfloat r2;
 
 r2 = x1*x1;
 
 *delta_x1 = parameter_values.a1*x1 + parameter_values.a2*r2 +
             parameter_values.a3*r2*x1 + parameter_values.a4*r2*r2;
}

static void distortion_function02(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 *delta_x1 = parameter_values.a1*cos(x2*parameter_values.a2) + 
             parameter_values.a3*cos(x2*parameter_values.a4);
}

static void distortion_function03(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 gfloat r2;
 
 r2 = x1*x1;
 
 *delta_x2 = parameter_values.a1*x1 + parameter_values.a2*r2 +
             parameter_values.a3*r2*x1 + parameter_values.a4*r2*r2;
}

static void distortion_function04(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 *delta_x2 = parameter_values.a1*cos(x2*parameter_values.a2) + 
             parameter_values.a3*cos(x2*parameter_values.a4);
}


static void distortion_function05(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 gfloat P, Q;
 gfloat r2;
 
 r2 = x1*x1;

 P = parameter_values.a1*x1 + parameter_values.a2*r2 +
     parameter_values.a3*r2*x1 + parameter_values.a4*r2*r2;
     
 Q = parameter_values.a5*cos(x2*parameter_values.a6) + 
             parameter_values.a7*cos(x2*parameter_values.a8);
 
 *delta_x2 = P*Q;
}


static void distortion_function06(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 gfloat P, Q;
 gfloat r2;
 
 r2 = x1*x1;

 P = parameter_values.a1*x1 + parameter_values.a2*r2 +
     parameter_values.a3*r2*x1 + parameter_values.a4*r2*r2;
     
 Q = parameter_values.a5*cos(x2*parameter_values.a6) + 
             parameter_values.a7*cos(x2*parameter_values.a8);
 
 *delta_x2 = P*Q;
}



static void distortion_function07(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 *delta_x1 = 0.1*parameter_values.a1*sin(5.0*parameter_values.a3*x1)*cos(5.0*parameter_values.a4*x1);
 *delta_x2 = 0.1*parameter_values.a2*sin(5.0*parameter_values.a5*x2)*cos(5.0*parameter_values.a6*x2);
}



static void distortion_function08(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
 gfloat x0, y0;
 gfloat lambda1, lambda2;
 
 x0 = parameter_values.a3/10.0;
 y0 = parameter_values.a4/10.0;

 lambda1 = 30.0/(10.1 + parameter_values.a5);
 lambda2 = 30.0/(10.1 + parameter_values.a6);

 
 *delta_x1 = 0.5*parameter_values.a1*(x1 - x0)*exp(-lambda1*(x1 - x0)*(x1 - x0));
 *delta_x2 = 0.5*parameter_values.a2*(x2 - y0)*exp(-lambda2*(x2 - y0)*(x2 - y0));
}



static void distortion_function09(gfloat x1, gfloat x2, gfloat *delta_x1, gfloat *delta_x2)
{
/*
 *delta_x1 = 0.1*parameter_values.a1*sin(5.0*parameter_values.a3*x1)*cos(5.0*parameter_values.a4*x1);
 *delta_x2 = 0.1*parameter_values.a2*sin(5.0*parameter_values.a5*x2)*cos(5.0*parameter_values.a6*x2);
*/
}




static void calculate_distortion_function(gint i, gint j, gint width, gint height, gfloat *u, gfloat *v)
{
 gint x, y;
 gfloat scale_factor;
 gint mx, my;
 gfloat scaled_x, scaled_y;
 gfloat delta_rho, delta_phi;
 gfloat delta_x, delta_y;
 gfloat rho, phi;
 gfloat damp;

 mx = width/2;
 my = height/2;

 /* The scale factor is chosed such that the scaled image fits into a unit square. */
 scale_factor = (gfloat)MAX(width, height);

 x = i - mx;
 y = my - j;
    
 scaled_x = ((float)x)/scale_factor;
 scaled_y = ((float)y)/scale_factor;
 
 switch(parameter_values.current_function)
 {
  case FUNCTION_R_r: 
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       polar_coordinates(scaled_x, scaled_y, &rho, &phi);
 
       distortion_function01(rho, phi, &delta_rho, &delta_phi);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_rho*cos(phi);
       *v = (float)my - (float)y - damp*scale_factor*delta_rho*sin(phi);
       break;
       
       
  case FUNCTION_R_phi:
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       polar_coordinates(scaled_x, scaled_y, &rho, &phi);
 
       distortion_function02(rho, phi, &delta_rho, &delta_phi);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_rho*cos(phi);
       *v = (float)my - (float)y - damp*scale_factor*delta_rho*sin(phi);
       break;
       
       
  case FUNCTION_Phi_r:
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       polar_coordinates(scaled_x, scaled_y, &rho, &phi);
 
       distortion_function03(rho, phi, &delta_rho, &delta_phi);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_phi*sin(phi);
       *v = (float)my - (float)y + damp*scale_factor*delta_phi*cos(phi);
       break;
       
       
  case FUNCTION_Phi_phi:
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       polar_coordinates(scaled_x, scaled_y, &rho, &phi);
 
       distortion_function04(rho, phi, &delta_rho, &delta_phi);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_phi*sin(phi);
       *v = (float)my - (float)y + damp*scale_factor*delta_phi*cos(phi);
       break;
       
       
  case FUNCTION_R_r_Phi_phi:
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       polar_coordinates(scaled_x, scaled_y, &rho, &phi);
 
       distortion_function05(rho, phi, &delta_rho, &delta_phi);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_phi*sin(phi);
       *v = (float)my - (float)y + damp*scale_factor*delta_phi*cos(phi);
       break;
       
  case FUNCTION_R_r_R_phi:
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       polar_coordinates(scaled_x, scaled_y, &rho, &phi);
 
       distortion_function06(rho, phi, &delta_rho, &delta_phi);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_phi*cos(phi);
       *v = (float)my - (float)y - damp*scale_factor*delta_phi*sin(phi);
       break;
       
      
  case FUNCTION_Cosines:
       /* Calculate the (u, v) coordinates of the distorted image
          from the original image */
       distortion_function07(scaled_x, scaled_y, &delta_x, &delta_y);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_x;
       *v = (float)my - (float)y - damp*scale_factor*delta_y;
       break;
       
       
  case FUNCTION_Gauss:
    
       distortion_function08(scaled_x, scaled_y, &delta_x, &delta_y);
 
       /* calculate the damping factor */
       damp = 1.0;
       if(parameter_values.use_cutoff != 0) damp = cutoff_function(scaled_x, scaled_y);
  
       *u = (float)x + (float)mx + damp*scale_factor*delta_x;
       *v = (float)my - (float)y - damp*scale_factor*delta_y;
       break;
 }
}







static gfloat cutoff_function(gfloat u, gfloat v)
{
 gfloat radius;
 gfloat x;
 
 radius = sqrt(u*u + v*v);
 
 if(radius <= parameter_values.r) return(1.0);
 
 x = (radius - parameter_values.r)/(0.2*parameter_values.dr);
 return(exp(-x*x));
}



static void calculate_pixel_color(gfloat u, gfloat v, guchar *source, gint width, gint height, gint bpp, guchar *r, guchar *g, guchar *b)
{
 gint address;
 gfloat ru, rv;
 gint u0, v0;
 gfloat w0, w1, w2, w3;
 
 guchar red[4];
 guchar green[4];
 guchar blue[4];
 
 if(((gint)u < width - 1) && ((gint)v < height - 1))
 {
  ru = u - (int)u;
  rv = v - (int)v;
  
  // Calculate the area weights.
  w0 = (1.0 - ru)*rv;
  w1 = rv*ru;
  w2 = (1.0 - ru)*(1.0 - rv);
  w3 = (1.0 - rv)*ru;
  
  // Get the four pixel colors depending on bpp.
  switch(bpp)
  {
   case 4:    
   case 3:  address = bpp*((int)v*width + (int)u);
            red[0] = source[address];
            green[0] = source[address +1];
            blue[0] = source[address + 2];
            
            address = bpp*((int)v*width + (int)(u + 1.0));
            red[1] = source[address];
            green[1] = source[address + 1];
            blue[1] = source[address + 2];
            
            address = bpp*((int)(v - 1.0)*width + (int)u);
            red[2] = source[address];
            green[2] = source[address + 1];
            blue[2] = source[address + 2];
            
            address = bpp*((int)(v - 1.0)*width + (int)(u + 1.0));
            red[3] = source[address];
            green[3] = source[address + 1];
            blue[3] = source[address + 2];
            
            *r = w0*red[0]   + w1*red[1]   + w2*red[2]   + w3*red[3];
            *g = w0*green[0] + w1*green[1] + w2*green[2] + w3*green[3];
            *b = w0*blue[0]  + w1*blue[1]  + w2*blue[2]  + w3*blue[3];
            break;
            
            
   /* For grayscale images, the grayvalue is stored in the
      red component.  
   */       
   case 2:  
   case 1:  address = bpp*((int)v*width + (int)u);
            red[0] = source[address];
                      
            address = bpp*((int)v*width + (int)(u + 1.0));
            red[1] = source[address];
            
            address = bpp*((int)(v - 1.0)*width + (int)u);
            red[2] = source[address];
            
            address = bpp*((int)(v - 1.0)*width + (int)(u + 1.0));
            red[3] = source[address];
            
            *r = w0*red[0]   + w1*red[1]   + w2*red[2]   + w3*red[3];
            break;
   
  }
 } 
}



static void get_source_image(GPixelRgn source_rgn, guchar *buffer, gint x, gint y, gint width, gint height, gint bytes)
{
 gint offset;
 gint i, j;

 guchar *src_buffer;

 src_buffer = (guchar *)malloc(width * bytes);

 offset = 0;
 for(j = 0; j < height; j++)
 {
  gimp_pixel_rgn_get_row(&source_rgn, src_buffer, x, j + y, width);
  for(i = 0; i < bytes*width; i++) buffer[i + offset] = src_buffer[i];
  offset += bytes*width;
 }
 
 free(src_buffer);
}
