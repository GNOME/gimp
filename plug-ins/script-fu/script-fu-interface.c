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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>		/* For toupper() */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "siod.h"
#include "script-fu-scripts.h"

#define TEXT_WIDTH  100
#define TEXT_HEIGHT 25
#define COLOR_SAMPLE_WIDTH 100
#define COLOR_SAMPLE_HEIGHT 15
#define SLIDER_WIDTH  100
#define SLIDER_HEIGHT 30
#define SPINNER_WIDTH 75
#define FONT_PREVIEW_WIDTH 100

#define DEFAULT_FONT_SIZE 240

#define MAX_STRING_LENGTH 4096

typedef struct
{
  GtkWidget *preview;
  GtkWidget *dialog;
  gdouble    color[3];
  gdouble    old_color[3];
} SFColor;

typedef struct
{
  GtkAdjustment *adj;
  gfloat         value;
  gfloat         lower;
  gfloat         upper;
  gfloat         step;
  gfloat         page;
  gint           digits;
  SFAdjustmentType type;
} SFAdjustment;

typedef struct
{
  GtkWidget *preview;
  GtkWidget *dialog;
  gchar     *fontname;
} SFFont;

typedef struct
{
  GtkWidget *preview;
  GtkWidget *dialog;
  gchar     *filename;
} SFFilename;

typedef struct 
{
  gchar    *name;
  gdouble  opacity;
  gint     spacing;
  gint     paint_mode;
} SFBrush;


typedef union
{
  gint32        sfa_image;
  gint32        sfa_drawable;
  gint32        sfa_layer;
  gint32        sfa_channel;
  SFColor       sfa_color;
  gint32        sfa_toggle;
  gchar *       sfa_value;
  SFAdjustment  sfa_adjustment;
  SFFont        sfa_font;
  SFFilename    sfa_file;
  gchar *       sfa_pattern;
  gchar *       sfa_gradient;
  SFBrush       sfa_brush;
} SFArgValue;

typedef struct
{
  GtkWidget ** args_widgets;
  gchar *      script_name;
  gchar *      description;
  gchar *      help;
  gchar *      author;
  gchar *      copyright;
  gchar *      date;
  gchar *      img_types;
  gint         num_args;
  SFArgType *  arg_types;
  gchar **     arg_labels;
  SFArgValue * arg_defaults;
  SFArgValue * arg_values;
  gint32       image_based;
} SFScript;

typedef struct
{
  GtkWidget *status;
  GtkWidget *entry;
  GtkWidget *about_dialog;
  SFScript  *script;
} SFInterface;

/* External functions
 */
extern long  nlength      (LISP obj);

/*
 *  Local Functions
 */

static void       script_fu_script_proc      (char     *name,
					      int       nparams,
					      GParam   *params,
					      int      *nreturn_vals,
					      GParam  **return_vals);

static SFScript  *script_fu_find_script      (gchar    *script_name);
static void       script_fu_free_script      (SFScript *script);
static void       script_fu_enable_cc        (void);
static void       script_fu_disable_cc       (gint    err_msg);
static void       script_fu_interface        (SFScript *script);
static void       script_fu_color_preview    (GtkWidget *preview,
					      gdouble   *color);
static void       script_fu_file_preview     (GtkWidget *preview,
					      gchar    *fontname);
static void       script_fu_font_preview     (GtkWidget *preview,
					      gchar    *fontname);
static void       script_fu_cleanup_widgets  (SFScript *script);
static void       script_fu_ok_callback      (GtkWidget *widget,
					      gpointer   data);
static void       script_fu_close_callback   (GtkWidget *widget,
					      gpointer   data);
static void       script_fu_about_callback   (GtkWidget *widget,
					      gpointer   data);
static void       script_fu_reset_callback   (GtkWidget *widget,
					      gpointer   data);
static void       script_fu_menu_callback    (gint32     id,
					      gpointer   data);
static void       script_fu_toggle_update    (GtkWidget *widget,
					      gpointer   data);
static void       script_fu_color_preview_callback (GtkWidget *widget,
						    gpointer   data);
static void       script_fu_color_preview_changed  (GtkWidget *widget,
						    gpointer   data);
static void       script_fu_color_preview_cancel   (GtkWidget *widget,
						    gpointer   data);
static gint       script_fu_color_preview_delete   (GtkWidget *widget,
						    GdkEvent  *event,
						    gpointer   data);
static void       script_fu_file_preview_callback  (GtkWidget *widget,
						    gpointer   data);
static void       script_fu_file_dialog_ok         (GtkWidget *widget,
						    gpointer   data);
static void       script_fu_file_dialog_cancel     (GtkWidget *widget,
						    gpointer   data);
static gint       script_fu_file_dialog_delete     (GtkWidget *widget,
						    GdkEvent  *event,
						    gpointer   data);
static void       script_fu_font_preview_callback  (GtkWidget *widget,
						    gpointer   data);
static void       script_fu_font_dialog_ok         (GtkWidget *widget,
						    gpointer   data);
static void       script_fu_font_dialog_cancel     (GtkWidget *widget,
						    gpointer   data);
static gint       script_fu_font_dialog_delete     (GtkWidget *widget,
						    GdkEvent  *event,
						    gpointer   data);
static void       script_fu_about_dialog_close     (GtkWidget *widget,
						    gpointer   data);
static gint       script_fu_about_dialog_delete    (GtkWidget *widget,
						    GdkEvent  *event,
						    gpointer   data);
static void       script_fu_pattern_preview        (gchar    *name,
						    gint     width,
						    gint     height,
						    gint     bytes,
						    gchar *  mask_data,
						    gint     closing,
						    gpointer udata);

static void       script_fu_gradient_preview       (gchar    *name,
						    gint     width,
						    gdouble *  mask_data,
						    gint     closing,
						    gpointer udata);

static void       script_fu_brush_preview          (char *, /* Name */
						    gdouble, /* opacity */
						    gint,    /* spacing */
						    gint,    /* paint_mode */
						    gint,    /* width */
						    gint,    /* height */
						    gchar *, /* mask data */
						    gint,    /* dialog closing */
						    gpointer /* user data */);


/*
 *  Local variables
 */

static SFInterface sf_interface =
{
  NULL,  /*  status (current command) */
  NULL   /*  active script    */
};

static struct stat filestat;
static gint   current_command_enabled = FALSE;
static gint   command_count = 0;
static gint   consec_command_count = 0;
static gchar *last_command = NULL;
static GList *script_list = NULL;

extern char   siod_err_msg[];

/*
 *  Function definitions
 */

void
script_fu_find_scripts ()
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar *path_str;
  gchar *home;
  gchar *local_path;
  gchar *path;
  gchar *filename;
  gchar *token;
  gchar *next_token;
  gchar *command;
  gint   my_err;
  DIR   *dir;
  struct dirent *dir_ent;

  /*  Make sure to clear any existing scripts  */
  if (script_list != NULL)
    {
      GList *list;
      SFScript *script;

      list = script_list;
      while (list)
	{
	  script = (SFScript *) list->data;
	  script_fu_free_script (script);
	  list = list->next;
	}

      if (script_list)
	g_list_free (script_list);
      script_list = NULL;
    }

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "script-fu-path",
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      path_str = return_vals[1].data.d_string;

      if (path_str == NULL)
	return;

      /* Set local path to contain temp_path, where (supposedly)
       * there may be working files.
       */
      home = getenv ("HOME");
      local_path = g_strdup (path_str);

      /* Search through all directories in the local path */

      next_token = local_path;

      token = strtok (next_token, G_SEARCHPATH_SEPARATOR_S);

      while (token)
	{
	  if (*token == '~')
	    {
	      path = g_malloc (strlen (home) + strlen (token) + 2);
	      sprintf (path, "%s%s", home, token + 1);
	    }
	  else
	    {
	      path = g_malloc (strlen (token) + 2);
	      strcpy (path, token);
	    } /* else */

	  /* Check if directory exists and if it has any items in it */
	  my_err = stat (path, &filestat);

	  if (!my_err && S_ISDIR (filestat.st_mode))
	    {
	      if (path[strlen (path) - 1] != '/')
		strcat (path, "/");

	      /* Open directory */
	      dir = opendir (path);

	      if (!dir)
		  g_message ("error reading script directory \"%s\"", path);
	      else
		{
		  while ((dir_ent = readdir (dir)))
		    {
		      filename = g_malloc (strlen(path) + strlen (dir_ent->d_name) + 1);

		      sprintf (filename, "%s%s", path, dir_ent->d_name);

		      if (strcmp (filename + strlen (filename) - 4, ".scm") == 0)
			{
			  /* Check the file and see that it is not a sub-directory */
			  my_err = stat (filename, &filestat);

			  if (!my_err && S_ISREG (filestat.st_mode))
			    {
#ifdef __EMX__
			      _fnslashify(filename);
#endif
			      command = g_new (char, strlen ("(load \"\")") + strlen (filename) + 1);
			      sprintf (command, "(load \"%s\")", filename);

			      repl_c_string (command, 0, 0, 1);

			      g_free (command);
			    }
			}

		      g_free (filename);
		    } /* while */

		  closedir (dir);
		} /* else */
	    } /* if */

	  g_free (path);

	  token = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
	} /* while */

      g_free(local_path);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

LISP
script_fu_add_script (LISP a)
{
  SFScript *script;
  GParamDef *args;
  char *val;
  int i;
  gdouble color[3];
  LISP color_list;
  LISP adj_list;
  LISP brush_list;
  gchar *menu_path = NULL;

  /*  Check the length of a  */
  if (nlength (a) < 7)
    return my_err ("Too few arguments to script-fu-register", NIL);

  /*  Create a new script  */
  script = g_new (SFScript, 1);

  /*  Find the script name  */
  val = get_c_string (car (a));
  script->script_name = g_strdup (val);
  a = cdr (a);

  /*  Find the script description  */
  val = get_c_string (car (a));
  script->description = g_strdup (val);
  a = cdr (a);

  /* Allow scripts with no menus */
  if (strncmp(val, "<None>", 6) != 0)
      menu_path = script->description;

  /*  Find the script help  */
  val = get_c_string (car (a));
  script->help = g_strdup (val);
  a = cdr (a);

  /*  Find the script author  */
  val = get_c_string (car (a));
  script->author = g_strdup (val);
  a = cdr (a);

  /*  Find the script copyright  */
  val = get_c_string (car (a));
  script->copyright = g_strdup (val);
  a = cdr (a);

  /*  Find the script date  */
  val = get_c_string (car (a));
  script->date = g_strdup (val);
  a = cdr (a);

  /*  Find the script image types  */
  if (TYPEP (a, tc_cons))
    {
      val = get_c_string (car (a));
      a = cdr (a);
    }
  else
    {
      val = get_c_string (a);
      a = NIL;
    }
  script->img_types = g_strdup (val);

  /*  Check the supplied number of arguments  */
  script->num_args = nlength (a) / 3;

  args = g_new (GParamDef, script->num_args + 1);
  args[0].type = PARAM_INT32;
  args[0].name = "run_mode";
  args[0].description = "Interactive, non-interactive";

  script->args_widgets = NULL;
  script->arg_types = g_new (SFArgType, script->num_args);
  script->arg_labels = g_new (char *, script->num_args);
  script->arg_defaults = g_new (SFArgValue, script->num_args);
  script->arg_values = g_new (SFArgValue, script->num_args);

  if (script->num_args > 0)
    {
      for (i = 0; i < script->num_args; i++)
	{
	  if (a != NIL)
	    {
	      if (!TYPEP (car (a), tc_flonum))
		return my_err ("script-fu-register: argument types must be integer values", NIL);
	      script->arg_types[i] = get_c_long (car (a));
	      a = cdr (a);
	    }
	  else
	    return my_err ("script-fu-register: missing type specifier", NIL);

	  if (a != NIL)
	    {
	      if (!TYPEP (car (a), tc_string))
		return my_err ("script-fu-register: argument labels must be strings", NIL);
	      script->arg_labels[i] = g_strdup (get_c_string (car (a)));
	      a = cdr (a);
	    }
	  else
	    return my_err ("script-fu-register: missing arguments label", NIL);

	  if (a != NIL)
	    {
	      switch (script->arg_types[i])
		{
		case SF_IMAGE:
		case SF_DRAWABLE:
		case SF_LAYER:
		case SF_CHANNEL:
		  if (!TYPEP (car (a), tc_flonum))
		    return my_err ("script-fu-register: drawable defaults must be integer values", NIL);
		  script->arg_defaults[i].sfa_image = get_c_long (car (a));
		  script->arg_values[i].sfa_image = script->arg_defaults[i].sfa_image;

		  switch (script->arg_types[i])
		    {
		    case SF_IMAGE:
		      args[i + 1].type = PARAM_IMAGE;
		      args[i + 1].name = "image";
		      break;
		    case SF_DRAWABLE:
		      args[i + 1].type = PARAM_DRAWABLE;
		      args[i + 1].name = "drawable";
		      break;
		    case SF_LAYER:
		      args[i + 1].type = PARAM_LAYER;
		      args[i + 1].name = "layer";
		      break;
		    case SF_CHANNEL:
		      args[i + 1].type = PARAM_CHANNEL;
		      args[i + 1].name = "channel";
		      break;
		    default:
		      break;
		    }

		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_COLOR:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: color defaults must be a list of 3 integers", NIL);
		  color_list = car (a);
		  color[0] = (gdouble) get_c_long (car (color_list)) / 255.0;
		  color_list = cdr (color_list);
		  color[1] = (gdouble) get_c_long (car (color_list)) / 255.0;
		  color_list = cdr (color_list);
		  color[2] = (gdouble) get_c_long (car (color_list)) / 255.0;
		  memcpy (script->arg_defaults[i].sfa_color.color, color, sizeof (gdouble) * 3);
		  memcpy (script->arg_values[i].sfa_color.color, color, sizeof (gdouble) * 3);
		  script->arg_values[i].sfa_color.preview = NULL;
		  script->arg_values[i].sfa_color.dialog = NULL;

		  args[i + 1].type = PARAM_COLOR;
		  args[i + 1].name = "color";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_TOGGLE:
		  if (!TYPEP (car (a), tc_flonum))
		    return my_err ("script-fu-register: toggle default must be an integer value", NIL);
		  script->arg_defaults[i].sfa_toggle = (get_c_long (car (a))) ? TRUE : FALSE;
		  script->arg_values[i].sfa_toggle = script->arg_defaults[i].sfa_toggle;

		  args[i + 1].type = PARAM_INT32;
		  args[i + 1].name = "toggle";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_VALUE:  
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: value defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_value = g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_value =  g_strdup (script->arg_defaults[i].sfa_value);

		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_STRING:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: string defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_value = g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_value =  g_strdup (script->arg_defaults[i].sfa_value);

		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "string";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_ADJUSTMENT:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: adjustment defaults must be a list", NIL);
		  adj_list = car (a);
		  script->arg_defaults[i].sfa_adjustment.value = get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.lower = get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.upper = get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.step = get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.page = get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.digits = get_c_long (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.type = get_c_long (car (adj_list));
		  script->arg_values[i].sfa_adjustment.adj = NULL;
		  script->arg_values[i].sfa_adjustment.value = script->arg_defaults[i].sfa_adjustment.value;

		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_FILENAME:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: filename defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_file.filename = g_strdup (get_c_string (car (a)));
		 script->arg_values[i].sfa_file.filename =  g_strdup (script->arg_defaults[i].sfa_file.filename);
		 script->arg_values[i].sfa_file.preview = NULL;
		 script->arg_values[i].sfa_file.dialog = NULL;

		 args[i + 1].type = PARAM_STRING;
		 args[i + 1].name = "filename";
		 args[i + 1].description = script->arg_labels[i];
		 break;

		case SF_FONT:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: font defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_font.fontname = g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_font.fontname =  g_strdup (script->arg_defaults[i].sfa_font.fontname);
		  script->arg_values[i].sfa_font.preview = NULL;
		  script->arg_values[i].sfa_font.dialog = NULL;
		  
		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "font";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_PATTERN:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: pattern defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_pattern = g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_pattern =  g_strdup (script->arg_defaults[i].sfa_pattern);

		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "pattern";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_BRUSH:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: brush defaults must be a list", NIL);
		  brush_list = car (a);
		  script->arg_defaults[i].sfa_brush.name = g_strdup (get_c_string (car (brush_list)));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.opacity = get_c_double (car (brush_list));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.spacing = get_c_long (car (brush_list));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.paint_mode = get_c_long (car (brush_list));
		  script->arg_values[i].sfa_brush = script->arg_defaults[i].sfa_brush;
		  /* Need this since we need a copy of the string
		   * in the values area. We could free it later but the
		   * default one must hang around.
		   */

		  script->arg_values[i].sfa_brush.name = g_strdup(script->arg_defaults[i].sfa_brush.name);

		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "brush";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_GRADIENT:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: gradient defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_gradient = g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_gradient =  g_strdup (script->arg_defaults[i].sfa_pattern);
		  
		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "gradient";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		default:
		  break;
		}

	      a = cdr (a);
	    }
	  else
	    return my_err ("script-fu-register: missing default argument", NIL);
	}
    }

  gimp_install_temp_proc (script->script_name,
			  script->description,
			  script->help,
			  script->author,
			  script->copyright,
			  script->date,
			  menu_path,
			  script->img_types,
			  PROC_TEMPORARY,
			  script->num_args + 1, 0,
			  args, NULL,
			  script_fu_script_proc);

  g_free (args);

  script_list = g_list_append (script_list, script);

  return NIL;
}

void
script_fu_report_cc (gchar *command)
{
  if (last_command && strcmp (last_command, command) == 0)
    {
      char *new_command;

      new_command = g_new (gchar, strlen (command) + 10);
      sprintf (new_command, "%s <%d>", command, ++consec_command_count);
      if (current_command_enabled == TRUE)
	{
	  gtk_entry_set_text (GTK_ENTRY (sf_interface.status), new_command);
	}
      g_free (new_command);
      g_free (last_command);
    }
  else
    {
      consec_command_count = 1;
      if (current_command_enabled == TRUE)
	gtk_entry_set_text (GTK_ENTRY (sf_interface.status), command);
      if (last_command)
	g_free (last_command);
    }
  last_command = g_strdup (command);
  command_count++;

  if (current_command_enabled == TRUE)
    gdk_flush ();
}

static void
script_fu_script_proc (char     *name,
		       int       nparams,
		       GParam   *params,
		       int      *nreturn_vals,
		       GParam  **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;
  SFScript *script;
  int min_args;

  run_mode = params[0].data.d_int32;

  if (! (script = script_fu_find_script (name)))
    status = STATUS_CALLING_ERROR;
  else
    {
      if (script->num_args == 0)
	run_mode = RUN_NONINTERACTIVE;

      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  /*  Determine whether the script is image based (runs on an image)  */
	  if (strncmp (script->description, "<Image>", 7) == 0)
	    {
	      script->arg_values[0].sfa_image = params[1].data.d_image;
	      script->arg_values[1].sfa_drawable = params[2].data.d_drawable;
	      script->image_based = TRUE;
	    }
	  else
	    script->image_based = FALSE;

	  /*  First acquire information with a dialog  */
	  /*  Skip this part if the script takes no parameters */ 
	  min_args = (script->image_based) ? 2 : 0;
	  if (script->num_args > min_args) {
	    script_fu_interface (script); 
	    break;
	  }

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != (script->num_args + 1))
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      gint err_msg;
	      char *text = NULL;
	      char *command, *c;
	      char buffer[MAX_STRING_LENGTH];
	      int length;
	      int i;

	      length = strlen (script->script_name) + 3;

	      for (i = 0; i < script->num_args; i++)
		switch (script->arg_types[i])
		  {
		  case SF_IMAGE:
		  case SF_DRAWABLE:
		  case SF_LAYER:
		  case SF_CHANNEL:
		    length += 12;  /*  Maximum size of integer value will not exceed this many characters  */
		    break;
		  case SF_COLOR:
		    length += 16;  /*  Maximum size of color string: '(XXX XXX XXX)  */
		    break;
		  case SF_TOGGLE:
		    length += 6;   /*  Maximum size of (TRUE, FALSE)  */
		    break;
		  case SF_VALUE:
		    length += strlen (params[i + 1].data.d_string) + 1;
		    break;
		  case SF_STRING:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;
		  case SF_FILENAME:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;
		  case SF_ADJUSTMENT:
		    length += strlen (params[i + 1].data.d_string) + 1;
		    break;
		  case SF_FONT:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;
		  case SF_PATTERN:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;
		  case SF_GRADIENT:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;
		  case SF_BRUSH:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;
		  default:
		    break;
		  }

	      c = command = g_new (char, length);

	      if (script->num_args)
	      {
	      sprintf (command, "(%s ", script->script_name);
	      c += strlen (script->script_name) + 2;
	      for (i = 0; i < script->num_args; i++)
		{
		  switch (script->arg_types[i])
		    {
		    case SF_IMAGE:
		    case SF_DRAWABLE:
		    case SF_LAYER:
		    case SF_CHANNEL:
		      sprintf (buffer, "%d", params[i + 1].data.d_image);
		      text = buffer;
		      break;
		    case SF_COLOR:
		      sprintf (buffer, "'(%d %d %d)",
			       params[i + 1].data.d_color.red,
			       params[i + 1].data.d_color.green,
			       params[i + 1].data.d_color.blue);
		      text = buffer;
		      break;
		    case SF_TOGGLE:
		      sprintf (buffer, "%s", (params[i + 1].data.d_int32) ? "TRUE" : "FALSE");
		      text = buffer;
		      break;
		    case SF_VALUE:
		      text = params[i + 1].data.d_string;
		      break;
		    case SF_STRING:
		    case SF_FILENAME:
		      g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", params[i + 1].data.d_string);
		      text = buffer;
		      break;
		    case SF_ADJUSTMENT:
		      text = params[i + 1].data.d_string;
		      break;
		    case SF_FONT:
		      g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", params[i + 1].data.d_string);
		      text = buffer;
		      break;  
		    case SF_PATTERN:
		      g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", params[i + 1].data.d_string);
		      text = buffer;
		      break;
		    case SF_GRADIENT:
		      g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", params[i + 1].data.d_string);
		      text = buffer;
		      break;
		    case SF_BRUSH:
		      g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", params[i + 1].data.d_string);
		      text = buffer;
		      break;
		    default:
		      break;
		    }

		  if (i == script->num_args - 1)
		    sprintf (c, "%s)", text);
		  else
		    sprintf (c, "%s ", text);
		  c += strlen (text) + 1;
		}
	      }
	      else
		sprintf (command, "(%s)", script->script_name);

	      /*  run the command through the interpreter  */
	      err_msg = (repl_c_string (command, 0, 0, 1) != 0) ? TRUE : FALSE;

	      g_free (command);
	    }
	  break;

	default:
	  break;
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static SFScript *
script_fu_find_script (gchar *script_name)
{
  GList *list;
  SFScript *script;

  list = script_list;
  while (list)
    {
      script = (SFScript *) list->data;
      if (! strcmp (script->script_name, script_name))
	return script;

      list = list->next;
    }

  return NULL;
}

static void
script_fu_free_script (SFScript *script)
{
  int i;

  /*  Uninstall the temporary procedure for this script  */
  gimp_uninstall_temp_proc (script->script_name);

  if (script)
    {
      g_free (script->script_name);
      g_free (script->description);
      g_free (script->help);
      g_free (script->author);
      g_free (script->copyright);
      g_free (script->date);
      g_free (script->img_types);
      g_free (script->arg_types);

      for (i = 0; i < script->num_args; i++)
	{
	  g_free (script->arg_labels[i]);
	  switch (script->arg_types[i])
	    {
	    case SF_IMAGE:
	    case SF_DRAWABLE:
	    case SF_LAYER:
	    case SF_CHANNEL:
	    case SF_COLOR:
	      break;
	    case SF_VALUE:
	    case SF_STRING:
	      g_free (script->arg_defaults[i].sfa_value);
	      g_free (script->arg_values[i].sfa_value);
	      break;
	    case SF_ADJUSTMENT:
	      break;
	    case SF_FILENAME:
	      g_free (script->arg_defaults[i].sfa_file.filename);
	      g_free (script->arg_values[i].sfa_file.filename);
	      break;
	    case SF_FONT:
	      g_free (script->arg_defaults[i].sfa_font.fontname);
	      g_free (script->arg_values[i].sfa_font.fontname);
	      break;
	    case SF_PATTERN:
	      g_free (script->arg_defaults[i].sfa_pattern);
	      g_free (script->arg_values[i].sfa_pattern);
	      break;
	    case SF_GRADIENT:
	      g_free (script->arg_defaults[i].sfa_gradient);
	      g_free (script->arg_values[i].sfa_gradient);
	      break;
	    case SF_BRUSH:
	      g_free (script->arg_defaults[i].sfa_brush.name);
	      g_free (script->arg_values[i].sfa_brush.name);
	      break;
	    default:
	      break;
	    }
	}

      g_free (script->arg_labels);
      g_free (script->arg_defaults);
      g_free (script->arg_values);

      g_free (script);
    }
}

static void
script_fu_enable_cc ()
{
  current_command_enabled = TRUE;
}

static void
script_fu_disable_cc (gint err_msg)
{
  if (err_msg)
    g_message ("Script-Fu Error\n%s\n"
              "If this happens while running a logo script,\n"
              "you might not have the font it wants installed on your system",
              siod_err_msg);

  current_command_enabled = FALSE;

  if (last_command)
    g_free (last_command);
  last_command = NULL;
  command_count = 0;
  consec_command_count = 0;
}

static void
script_fu_interface (SFScript *script)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *menu;
  GtkWidget *table;
  GtkWidget *main_box;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *bbox;
  guchar *title;
  guchar *buf;
  gchar **argv;
  gint argc;
  int start_args;
  int i;
  guchar *color_cube;

  static gint gtk_initted = FALSE;

  if (!gtk_initted)
    {
      argc = 1;
      argv = g_new (gchar *, 1);
      argv[0] = g_strdup ("script-fu");
      
      gtk_init (&argc, &argv);
      gtk_rc_parse (gimp_gtkrc ());
      
      gdk_set_use_xshm (gimp_use_xshm ());
  
      gtk_preview_set_gamma (gimp_gamma ());
      gtk_preview_set_install_cmap (gimp_install_cmap());
      color_cube = gimp_color_cube ();
      gtk_preview_set_color_cube (color_cube[0], color_cube[1], color_cube[2], color_cube[3]);
      
      gtk_widget_set_default_visual (gtk_preview_get_visual());
      gtk_widget_set_default_colormap (gtk_preview_get_cmap());
      
      gtk_initted = TRUE;
    }

  sf_interface.about_dialog = NULL;
  sf_interface.script = script;
  
  title = g_new (guchar, strlen ("Script-Fu: ") + strlen (script->description));   
  buf = strstr (script->description, "Script-Fu/");
  if (buf)
    sprintf ((char *)title, "Script-Fu: %s", (buf + 10));
  else 
    sprintf ((char *)title, "Script-Fu: %s", script->description);

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_quit_add_destroy (1, GTK_OBJECT (dlg));
  gtk_window_set_title (GTK_WINDOW (dlg), (const gchar *)title);
  gtk_window_set_wmclass (GTK_WINDOW (dlg), "script_fu", "Gimp");
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      (GtkSignalFunc) script_fu_close_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) script_fu_close_callback,
		      NULL);
  
  /* the vbox holding all widgets */
  main_box = gtk_vbox_new (0,2);
  gtk_container_add (GTK_CONTAINER (dlg), main_box);
  gtk_container_border_width (GTK_CONTAINER (main_box), 2);
 
  /* the script arguments frame */
  frame = gtk_frame_new ("Script Arguments");
  gtk_container_border_width (GTK_CONTAINER (frame), 0);
  gtk_box_pack_start (GTK_BOX (main_box), frame, TRUE, TRUE, 0);

  /* the vbox holding all widgets */
  vbox = gtk_vbox_new (0,2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);

  /*  The argument table  */
  table = gtk_table_new (script->num_args + 1, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  script->args_widgets = g_new (GtkWidget *, script->num_args);

  start_args = (script->image_based) ? 2 : 0;

  for (i = start_args; i < script->num_args; i++)
    {
      label = gtk_label_new (script->arg_labels[i]);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label,
			0, 1, i, i + 1, GTK_FILL, GTK_FILL, 4, 2);
      gtk_widget_show (label);

      switch (script->arg_types[i])
	{
	case SF_IMAGE:
	case SF_DRAWABLE:
	case SF_LAYER:
	case SF_CHANNEL:
	  script->args_widgets[i] = gtk_option_menu_new ();
	  switch (script->arg_types[i])
	    {
	    case SF_IMAGE:
	      menu = gimp_image_menu_new (NULL, script_fu_menu_callback,
					  &script->arg_values[i].sfa_image,
					  script->arg_values[i].sfa_image);
	      break;
	    case SF_DRAWABLE:
	      menu = gimp_drawable_menu_new (NULL, script_fu_menu_callback,
					     &script->arg_values[i].sfa_drawable,
					     script->arg_values[i].sfa_drawable);
	      break;
	    case SF_LAYER:
	      menu = gimp_layer_menu_new (NULL, script_fu_menu_callback,
					  &script->arg_values[i].sfa_layer,
					  script->arg_values[i].sfa_layer);
	      break;
	    case SF_CHANNEL:
	      menu = gimp_channel_menu_new (NULL, script_fu_menu_callback,
					    &script->arg_values[i].sfa_channel,
					    script->arg_values[i].sfa_channel);
	      break;
	    default:
	      menu = NULL;
	      break;
	    }
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (script->args_widgets[i]), menu);
	  break;

	case SF_COLOR:
	  script->args_widgets[i] = gtk_button_new();

	  script->arg_values[i].sfa_color.preview = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (script->arg_values[i].sfa_color.preview),
			    COLOR_SAMPLE_WIDTH, COLOR_SAMPLE_HEIGHT);
	  gtk_container_add (GTK_CONTAINER (script->args_widgets[i]),
			     script->arg_values[i].sfa_color.preview);
	  gtk_widget_show (script->arg_values[i].sfa_color.preview);

	  script_fu_color_preview (script->arg_values[i].sfa_color.preview,
				   script->arg_values[i].sfa_color.color);

	  gtk_signal_connect (GTK_OBJECT (script->args_widgets[i]), "clicked",
			      (GtkSignalFunc) script_fu_color_preview_callback,
			      &script->arg_values[i].sfa_color);
	  break;

	case SF_TOGGLE:
	  gtk_label_set_text (GTK_LABEL (label), "Script Toggle");
	  script->args_widgets[i] = gtk_check_button_new_with_label (script->arg_labels[i]);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (script->args_widgets[i]),
				       script->arg_values[i].sfa_toggle);
	  gtk_signal_connect (GTK_OBJECT (script->args_widgets[i]), "toggled",
			      (GtkSignalFunc) script_fu_toggle_update,
			      &script->arg_values[i].sfa_toggle);
	  break;

	case SF_VALUE:
	case SF_STRING:
	  script->args_widgets[i] = gtk_entry_new ();
	  gtk_widget_set_usize (script->args_widgets[i], TEXT_WIDTH, 0);
	  gtk_entry_set_text (GTK_ENTRY (script->args_widgets[i]),
			      script->arg_values[i].sfa_value);
	  break;

	case SF_ADJUSTMENT:
	  script->arg_values[i].sfa_adjustment.adj = 
	    (GtkAdjustment *) gtk_adjustment_new (script->arg_values[i].sfa_adjustment.value, 
						  script->arg_defaults[i].sfa_adjustment.lower, 
						  script->arg_defaults[i].sfa_adjustment.upper, 
						  script->arg_defaults[i].sfa_adjustment.step, 
						  script->arg_defaults[i].sfa_adjustment.page, 0);
	  switch (script->arg_defaults[i].sfa_adjustment.type)
	    {
	    case SF_SLIDER:
	      script->args_widgets[i] = gtk_hscale_new (script->arg_values[i].sfa_adjustment.adj);
	      gtk_widget_set_usize (GTK_WIDGET (script->args_widgets[i]), 
				    SLIDER_WIDTH, SLIDER_HEIGHT);
	      gtk_scale_set_digits (GTK_SCALE (script->args_widgets[i]), 
				    script->arg_defaults[i].sfa_adjustment.digits);
	      gtk_scale_set_draw_value (GTK_SCALE (script->args_widgets[i]), TRUE);
	      gtk_range_set_update_policy (GTK_RANGE (script->args_widgets[i]), 
					   GTK_UPDATE_DELAYED);
	      break;

	    case SF_SPINNER:
	      script->args_widgets[i] = gtk_spin_button_new (script->arg_values[i].sfa_adjustment.adj, 0, 0);
	      gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (script->args_widgets[i]), TRUE);
	      gtk_widget_set_usize (script->args_widgets[i], SPINNER_WIDTH, 0);
	      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (script->args_widgets[i]),
					  script->arg_defaults[i].sfa_adjustment.digits);
	      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (script->args_widgets[i]), TRUE);
	      break;
	    default: /* this shouldn't happen */
	      script->args_widgets[i] = NULL;
	      break;
	    }
	  break;

	case SF_FILENAME:
	  script->args_widgets[i] = gtk_button_new();
	  script->arg_values[i].sfa_file.preview = gtk_label_new ("");
	  gtk_widget_set_usize (script->args_widgets[i], TEXT_WIDTH, 0);
	  gtk_container_add (GTK_CONTAINER (script->args_widgets[i]),
			     script->arg_values[i].sfa_file.preview);
	  gtk_widget_show (script->arg_values[i].sfa_file.preview);
 
	  script_fu_file_preview (script->arg_values[i].sfa_file.preview,
				  script->arg_values[i].sfa_file.filename);

	  gtk_signal_connect (GTK_OBJECT (script->args_widgets[i]), "clicked",
			      (GtkSignalFunc) script_fu_file_preview_callback,
			      &script->arg_values[i].sfa_file);
	  break;

	case SF_FONT:
	  script->args_widgets[i] = gtk_button_new();
	  script->arg_values[i].sfa_font.preview = gtk_label_new ("");
	  gtk_widget_set_usize (script->args_widgets[i], FONT_PREVIEW_WIDTH, 0);
	  gtk_container_add (GTK_CONTAINER (script->args_widgets[i]),
			     script->arg_values[i].sfa_font.preview);
	  gtk_widget_show (script->arg_values[i].sfa_font.preview);

	  script_fu_font_preview (script->arg_values[i].sfa_font.preview,
				  script->arg_values[i].sfa_font.fontname);

	  gtk_signal_connect (GTK_OBJECT (script->args_widgets[i]), "clicked",
			      (GtkSignalFunc) script_fu_font_preview_callback,
			      &script->arg_values[i].sfa_font);	  
	  break;

	case SF_PATTERN:
	  script->args_widgets[i] = gimp_pattern_select_widget("Script-fu Pattern Selection",
							       script->arg_values[i].sfa_pattern, 
							       script_fu_pattern_preview,
							       &script->arg_values[i].sfa_pattern);
	  break;
	case SF_GRADIENT:
	  script->args_widgets[i] = gimp_gradient_select_widget("Script-Fu Gradient Selection",
								script->arg_values[i].sfa_gradient, 
								script_fu_gradient_preview,
								&script->arg_values[i].sfa_gradient);
	  break;

	case SF_BRUSH:
	  script->args_widgets[i] = 
	    gimp_brush_select_widget("Script-Fu Brush Selection",
				     script->arg_values[i].sfa_brush.name, 
				     script->arg_values[i].sfa_brush.opacity, 
				     script->arg_values[i].sfa_brush.spacing, 
				     script->arg_values[i].sfa_brush.paint_mode, 
				     script_fu_brush_preview,
				     &script->arg_values[i].sfa_brush);
	  break;

	default:
	  break;
	}

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), script->args_widgets[i],
                          ((script->arg_types[i] == SF_VALUE) || (script->arg_types[i] == SF_STRING) ||  (script->arg_types[i] == SF_FILENAME) ||(script->arg_types[i] == SF_FONT)),
                          ((script->arg_types[i] == SF_VALUE) || (script->arg_types[i] == SF_STRING) || (script->arg_types[i] == SF_FILENAME) || (script->arg_types[i] == SF_FONT)), 0);
      gtk_widget_show (hbox);

      gtk_table_attach (GTK_TABLE (table), hbox, /* script->args_widgets[i], */
			1, 2, i, i + 1,
                        GTK_FILL | (((script->arg_types[i] == SF_VALUE) || (script->arg_types[i] == SF_STRING) || (script->arg_types[i] == SF_FILENAME) || (script->arg_types[i] == SF_FONT))
				    ? GTK_EXPAND : 0),
                        GTK_FILL, 4, 2);
      gtk_widget_show (script->args_widgets[i]);
    }

  gtk_widget_show (table);

  /*  Reset to defaults */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 4);

  button = gtk_button_new_with_label (" Reset to Defaults ");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) script_fu_reset_callback,
                      NULL);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /*  Action area  */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_EDGE);
  gtk_box_pack_start (GTK_BOX (main_box), bbox, FALSE, TRUE, 0);

  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) script_fu_ok_callback,
                      NULL);
  gtk_container_add (GTK_CONTAINER (bbox), button);  
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) script_fu_close_callback,
                      NULL);
  gtk_container_add (GTK_CONTAINER (bbox), button);  
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("About");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) script_fu_about_callback,
                      title);
  gtk_container_add (GTK_CONTAINER (bbox), button);  
  gtk_widget_show (button);

  gtk_widget_show (bbox);

  /* The statusbar (well it's a faked statusbar...) */
  sf_interface.status = gtk_entry_new ();
  gtk_entry_set_editable (GTK_ENTRY (sf_interface.status), FALSE);
  gtk_box_pack_end (GTK_BOX (main_box), sf_interface.status, FALSE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (sf_interface.status), title);
  gtk_widget_show (sf_interface.status);

  gtk_widget_show (main_box);
  gtk_widget_show (dlg);

  gtk_main ();
  
  g_free (script->args_widgets);
  g_free (title);
  gdk_flush ();
}

static void
script_fu_color_preview (GtkWidget *preview,
			 gdouble   *color)
{
  gint i;
  guchar buf[3 * COLOR_SAMPLE_WIDTH];

  for (i = 0; i < COLOR_SAMPLE_WIDTH; i++)
    {
      buf[3*i] = (guint) (255.999 * color[0]);
      buf[3*i+1] = (guint) (255.999 * color[1]);
      buf[3*i+2] = (guint) (255.999 * color[2]);
    }
  for (i = 0; i < COLOR_SAMPLE_HEIGHT; i++)
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, COLOR_SAMPLE_WIDTH);

  gtk_widget_draw (preview, NULL);
}


static void
script_fu_pattern_preview (gchar    *name,
			   gint     width,
			   gint     height,
			   gint     bytes,
			   gchar *  mask_data,
			   gint     closing,
			   gpointer udata)
{
  gchar ** pname = (gchar **) udata;
  g_free(*pname);
  *pname = g_strdup(name);
}

static void
script_fu_gradient_preview(gchar    *name,
			   gint     width,
			   gdouble *  mask_data,
			   gint     closing,
			   gpointer udata)
{
  gchar ** pname = (gchar **) udata;
  g_free(*pname);
  *pname = g_strdup(name);
}

static void      
script_fu_brush_preview(char * name, /* Name */
			gdouble opacity, /* opacity */
			gint spacing,    /* spacing */
			gint paint_mode,    /* paint_mode */
			gint width,    /* width */
			gint height,    /* height */
			gchar * mask_data, /* mask data */
			gint closing,    /* dialog closing */
			gpointer udata/* user data */)
{
  SFBrush *brush = (SFBrush *)udata;
  g_free(brush->name);
  brush->name = g_strdup(name);
  brush->opacity = opacity;
  brush->spacing = spacing;
  brush->paint_mode = paint_mode;
}



static void
script_fu_file_preview (GtkWidget *preview,
			gchar     *data)
{
  if (data == NULL) 
    return;

  gtk_label_set_text (GTK_LABEL (preview), g_basename((gchar *) data));
}



static void
script_fu_font_preview (GtkWidget *preview,
			gchar     *data)
{
  GdkFont *font;
  gchar *fontname;
  gchar *family;
  gchar *label;

  if (data == NULL) 
    return;

  fontname = g_strdup (data);

  /* Check if the fontname is valid and the font is present */
  font = gdk_font_load (fontname);

  if (font != NULL)
    {
      g_free (font);

      strtok (fontname, "-");
      family = strtok (NULL, "-");
      *family = toupper (*family);
  
      label = g_new (guchar, strlen (family) + 1);
      sprintf (label, "%s", family);
      gtk_label_set_text (GTK_LABEL (preview), label);
      g_free (label);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (preview), "NOT SET");
    }
  
  g_free (fontname);
}

static void
script_fu_cleanup_widgets (SFScript *script)
{
  int i;

  if (sf_interface.about_dialog != NULL)
    {
      gtk_widget_destroy (sf_interface.about_dialog);
      sf_interface.about_dialog = NULL;
    }

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_COLOR:
	if (script->arg_values[i].sfa_color.dialog != NULL)
	  {
	    gtk_widget_destroy (script->arg_values[i].sfa_color.dialog);
	    script->arg_values[i].sfa_color.dialog = NULL;
	  }
	break;
      case SF_FILENAME:
	if (script->arg_values[i].sfa_file.dialog != NULL)
	  {
	    gtk_widget_destroy (script->arg_values[i].sfa_file.dialog);
	    script->arg_values[i].sfa_file.dialog = NULL;
	  }
	break;
      case SF_FONT:
	if (script->arg_values[i].sfa_font.dialog != NULL)
	  {
	    gtk_widget_destroy (script->arg_values[i].sfa_font.dialog);
	    script->arg_values[i].sfa_font.dialog = NULL;
	  }
	break;
      case SF_PATTERN:
  	gimp_pattern_select_widget_close_popup(script->args_widgets[i]); 
	break;
      case SF_GRADIENT:
  	gimp_gradient_select_widget_close_popup(script->args_widgets[i]); 
	break;
      case SF_BRUSH:
  	gimp_brush_select_widget_close_popup(script->args_widgets[i]); 
	break;
      default:
	break;
      }
}

static void
script_fu_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  SFScript *script;
  gint err_msg;
  char *text = NULL;
  char *command, *c;
  char buffer[MAX_STRING_LENGTH];
  int length;
  int i;
  GdkFont *font;

  if ((script = sf_interface.script) == NULL)
    return;

  /* Check if choosen fonts are there */
  for (i = 0; i < script->num_args; i++)
    if (script->arg_types[i] == SF_FONT)
      {
	font = gdk_font_load (script->arg_values[i].sfa_font.fontname);
	if (font == NULL)
	  {
	    g_message ("At least one font you've choosen is invalid.\nPlease check your settings.\n");
	    return;
	  }
	g_free (font);
      }
  
  length = strlen (script->script_name) + 3;

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_IMAGE:
      case SF_DRAWABLE:
      case SF_LAYER:
      case SF_CHANNEL:
	length += 12;  /*  Maximum size of integer value will not exceed this many characters  */
	break;
      case SF_COLOR:
	length += 16;  /*  Maximum size of color string: '(XXX XXX XXX)  */
	break;
      case SF_TOGGLE:
	length += 6;   /*  Maximum size of (TRUE, FALSE)  */
	break;
      case SF_VALUE:
	length += strlen (gtk_entry_get_text (GTK_ENTRY (script->args_widgets[i]))) + 1;
	break;
      case SF_STRING:
	length += strlen (gtk_entry_get_text (GTK_ENTRY (script->args_widgets[i]))) + 3;
	break;
      case SF_ADJUSTMENT:
	length += 24;  /*  Maximum size of float value should not exceed this many characters  */
	break;
      case SF_FILENAME:
	length += strlen (script->arg_values[i].sfa_file.filename) + 3;
	break;
      case SF_FONT:
	length += strlen (script->arg_values[i].sfa_font.fontname) + 3;
	break;
      case SF_PATTERN:
	length += strlen (script->arg_values[i].sfa_pattern) + 3;
	break;
      case SF_GRADIENT:
	length += strlen (script->arg_values[i].sfa_gradient) + 3;
	break;
      case SF_BRUSH:
	length += strlen (script->arg_values[i].sfa_brush.name) + 3;
	length += 36; /* Maximum size of three ints for opacity, spacing,mode*/
	break;
      default:
	break;
      }

  c = command = g_new (char, length);

  sprintf (command, "(%s ", script->script_name);
  c += strlen (script->script_name) + 2;
  for (i = 0; i < script->num_args; i++)
    {
      switch (script->arg_types[i])
	{
	case SF_IMAGE:
	case SF_DRAWABLE:
	case SF_LAYER:
	case SF_CHANNEL:
	  sprintf (buffer, "%d", script->arg_values[i].sfa_image);
	  text = buffer;
	  break;
	case SF_COLOR:
	  sprintf (buffer, "'(%d %d %d)",
		   (gint32) (script->arg_values[i].sfa_color.color[0] * 255.999),
		   (gint32) (script->arg_values[i].sfa_color.color[1] * 255.999),
		   (gint32) (script->arg_values[i].sfa_color.color[2] * 255.999));
	  text = buffer;
	  break;
	case SF_TOGGLE:
	  sprintf (buffer, "%s", (script->arg_values[i].sfa_toggle) ? "TRUE" : "FALSE");
	  text = buffer;
	  break;
	case SF_VALUE:
	  text = gtk_entry_get_text (GTK_ENTRY (script->args_widgets[i]));
	  g_free (script->arg_values[i].sfa_value);
	  script->arg_values[i].sfa_value = g_strdup (text); 
	  break;
	case SF_STRING:
	  text = gtk_entry_get_text (GTK_ENTRY (script->args_widgets[i]));
	  g_free (script->arg_values[i].sfa_value);
	  script->arg_values[i].sfa_value = g_strdup (text); 
	  g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", text);
	  text = buffer;
	  break;
	case SF_ADJUSTMENT:
	  switch (script->arg_defaults[i].sfa_adjustment.type)
	    {
	    case SF_SLIDER:
	      script->arg_values[i].sfa_adjustment.value = script->arg_values[i].sfa_adjustment.adj->value;
	      g_snprintf (buffer, 24, "%f", script->arg_values[i].sfa_adjustment.value);
	      text = buffer;
	      break;
	    case SF_SPINNER:
	      script->arg_values[i].sfa_adjustment.value = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (script->args_widgets[i]));
	      g_snprintf (buffer, 24, "%f", script->arg_values[i].sfa_adjustment.value);		       text = buffer;
	      break;
	    default:
	      break;
	    }
	  break;
	case SF_FILENAME:
	  g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", script->arg_values[i].sfa_file.filename);
	  text = buffer;
	  break;
	case SF_FONT:
	  g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"", script->arg_values[i].sfa_font.fontname);
	  text = buffer;
	  break;
	case SF_PATTERN:
	  g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"",script->arg_values[i].sfa_pattern);
	  text = buffer;
	  break;
	case SF_GRADIENT:
	  g_snprintf (buffer, MAX_STRING_LENGTH, "\"%s\"",script->arg_values[i].sfa_gradient);
	  text = buffer;
	  break;
	case SF_BRUSH:
	  g_snprintf (buffer, MAX_STRING_LENGTH, 
		      "'(\"%s\" %f %d %d)",
		      script->arg_values[i].sfa_brush.name,
		      script->arg_values[i].sfa_brush.opacity,
		      script->arg_values[i].sfa_brush.spacing,
		      script->arg_values[i].sfa_brush.paint_mode);
	  text = buffer;
	  break;
	default:
	  break;
	}

      if (i == script->num_args - 1)
	sprintf (c, "%s)", text);
      else
	sprintf (c, "%s ", text);
      c += strlen (text) + 1;
    }

  /*  enable the current command field  */
  script_fu_enable_cc ();

  /*  run the command through the interpreter  */
  err_msg = (repl_c_string (command, 0, 0, 1) != 0) ? TRUE : FALSE;

  /*  disable the current command field  */
  script_fu_disable_cc (err_msg);

  /* Clean up flying widgets before terminating Gtk */
  script_fu_cleanup_widgets(script);

  gtk_main_quit ();

  g_free (command);
}

static void
script_fu_close_callback (GtkWidget *widget,
			  gpointer   data)
{
  if (sf_interface.script != NULL)
    script_fu_cleanup_widgets (sf_interface.script);

  gtk_main_quit ();
}

static void
script_fu_about_callback (GtkWidget *widget,
			  gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *text;
  GtkWidget *vscrollbar;
  gchar     *title;

  if (sf_interface.about_dialog == NULL)
    {
      title = (gchar *)data;
     
      dialog = gtk_dialog_new ();
      gtk_window_set_title (GTK_WINDOW (dialog), title);
      gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
      gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			  (GtkSignalFunc) script_fu_about_dialog_delete,
			  dialog);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (frame), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);
      
      vbox = gtk_vbox_new (0, 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_border_width (GTK_CONTAINER (vbox), 2);
      gtk_widget_show (vbox);
      
      /* the name */
      hbox = gtk_hbox_new (0, 2);
      gtk_container_border_width (GTK_CONTAINER (hbox), 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
      
      label = gtk_label_new (sf_interface.script->script_name);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);
 
      /* the help display */
      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
      gtk_widget_show (table);

      text = gtk_text_new (NULL, NULL);
      gtk_text_set_editable (GTK_TEXT (text), FALSE);
      gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);      
      gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_set_usize (text, 200, 60);
      gtk_widget_show (text);

      vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
      gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
			GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (vscrollbar);

      gtk_widget_realize (text);
      gtk_text_freeze (GTK_TEXT (text));
      gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL,
		       sf_interface.script->help, -1);
   
      /* author, copyright, etc. */
      table = gtk_table_new (2, 4, FALSE);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_widget_show (table);
      
      label = gtk_label_new ("Author: ");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
      gtk_widget_show (label);

      label = gtk_label_new (sf_interface.script->author);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
      gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
      gtk_widget_show (label);

      label = gtk_label_new ("Copyright: ");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
      gtk_widget_show (label);

      label = gtk_label_new (sf_interface.script->copyright);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
      gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 1, 2);
      gtk_widget_show (label);

      label = gtk_label_new ("Date: ");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
      gtk_widget_show (label);

      label = gtk_label_new (sf_interface.script->date);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
      gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 2, 3);
      gtk_widget_show (label);
      
      if (strlen (sf_interface.script->img_types) > 0)
	{
	  label = gtk_label_new ("Image types: ");
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
	  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
	  gtk_widget_show (label);

	  label = gtk_label_new (sf_interface.script->img_types);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 3, 4);
	  gtk_widget_show (label);
	}

      gtk_widget_show (frame);
      gtk_text_thaw (GTK_TEXT (text));

      /*  action area  */
      button = gtk_button_new_with_label ("Close");
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) script_fu_about_dialog_close,
			  dialog);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      sf_interface.about_dialog = dialog;
    }
  gtk_window_position (GTK_WINDOW (sf_interface.about_dialog), 
		       GTK_WIN_POS_MOUSE);  
  gtk_widget_show (GTK_WIDGET (sf_interface.about_dialog));
}

static void
script_fu_reset_callback (GtkWidget *widget,
			  gpointer   data)
{
  SFScript *script;
  int i,j;

  if ((script = sf_interface.script) == NULL)
    return;

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_IMAGE:
      case SF_DRAWABLE:
      case SF_LAYER:
      case SF_CHANNEL:
	break;
      case SF_COLOR:
	for (j = 0; j < 4; j++)
	  {
	    script->arg_values[i].sfa_color.color[j] = script->arg_defaults[i].sfa_color.color[j];
	  }
	script_fu_color_preview (script->arg_values[i].sfa_color.preview, 
				 script->arg_values[i].sfa_color.color);
	break;
      case SF_TOGGLE:
	script->arg_values[i].sfa_toggle = script->arg_defaults[i].sfa_toggle;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (script->args_widgets[i]),
				     script->arg_values[i].sfa_toggle);
	break;
      case SF_VALUE:
      case SF_STRING:
	g_free (script->arg_values[i].sfa_value);
	script->arg_values[i].sfa_value = g_strdup (script->arg_defaults[i].sfa_value); 
	gtk_entry_set_text (GTK_ENTRY (script->args_widgets[i]), 
			    script->arg_values[i].sfa_value);
	break;
      case SF_ADJUSTMENT:
	script->arg_values[i].sfa_adjustment.value = script->arg_defaults[i].sfa_adjustment.value;
	gtk_adjustment_set_value (script->arg_values[i].sfa_adjustment.adj, 
				  script->arg_values[i].sfa_adjustment.value);
	break;
      case SF_FILENAME:
	g_free (script->arg_values[i].sfa_file.filename);
	script->arg_values[i].sfa_file.filename = g_strdup (script->arg_defaults[i].sfa_file.filename);
	if (script->arg_values[i].sfa_file.dialog)
	  {
	    gtk_file_selection_set_filename (GTK_FILE_SELECTION (script->arg_values[i].sfa_file.dialog), script->arg_values[i].sfa_file.filename);
	  }	
	script_fu_file_preview (script->arg_values[i].sfa_file.preview,
				script->arg_values[i].sfa_file.filename);
	break;
      case SF_FONT:
	g_free (script->arg_values[i].sfa_font.fontname);
	script->arg_values[i].sfa_font.fontname = g_strdup (script->arg_defaults[i].sfa_font.fontname);
	if (script->arg_values[i].sfa_font.dialog)
	  {
	    gtk_font_selection_dialog_set_font_name (GTK_FONT_SELECTION_DIALOG (script->arg_values[i].sfa_font.dialog), script->arg_values[i].sfa_font.fontname);
	  }	
	script_fu_font_preview (script->arg_values[i].sfa_font.preview,
				script->arg_values[i].sfa_font.fontname);
	break;
      case SF_PATTERN:
  	gimp_pattern_select_widget_set_popup(script->args_widgets[i],script->arg_defaults[i].sfa_pattern);  
	break;
      case SF_GRADIENT:
  	gimp_gradient_select_widget_set_popup(script->args_widgets[i],script->arg_defaults[i].sfa_gradient);  
	break;
      case SF_BRUSH:
  	gimp_brush_select_widget_set_popup(script->args_widgets[i],
					   script->arg_defaults[i].sfa_brush.name,
					   script->arg_defaults[i].sfa_brush.opacity, 
					   script->arg_defaults[i].sfa_brush.spacing, 
					   script->arg_defaults[i].sfa_brush.paint_mode);  
	break;
      default:
	break;
      }
}

static void
script_fu_menu_callback  (gint32     id,
			  gpointer   data)
{
  *((gint32 *) data) = id;
}

static void
script_fu_toggle_update (GtkWidget *widget,
			 gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}


static void
script_fu_color_preview_callback (GtkWidget *widget,
				  gpointer   data)
{
  GtkColorSelectionDialog *csd;
  SFColor *color;

  color = (SFColor *) data;
  color->old_color[0] = color->color[0];
  color->old_color[1] = color->color[1];
  color->old_color[2] = color->color[2];
      
  if (!color->dialog)
    {
      color->dialog = gtk_color_selection_dialog_new ("Script-Fu Color Picker");
      csd = GTK_COLOR_SELECTION_DIALOG (color->dialog);

      gtk_widget_destroy (csd->help_button);

      gtk_signal_connect_object (GTK_OBJECT (csd->ok_button), "clicked",
				 (GtkSignalFunc) gtk_widget_hide,
				 GTK_OBJECT (color->dialog));
      gtk_signal_connect (GTK_OBJECT (csd), "delete_event",
			  (GtkSignalFunc) script_fu_color_preview_delete,
			  color);
      gtk_signal_connect (GTK_OBJECT (csd->cancel_button), "clicked",
			  (GtkSignalFunc) script_fu_color_preview_cancel,
			  color);
      gtk_signal_connect (GTK_OBJECT (csd->colorsel), "color_changed",
			  (GtkSignalFunc) script_fu_color_preview_changed,
			  color);

      /* call here so the old color is set */
      gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel),
				     color->color);
    }
  else
    csd = GTK_COLOR_SELECTION_DIALOG (color->dialog);

  gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel),
				 color->color);

  gtk_window_position (GTK_WINDOW (color->dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_show (color->dialog);
}

static void
script_fu_color_preview_changed (GtkWidget *widget,
				 gpointer data)
{
  SFColor *color;

  color = (SFColor *) data;
  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (color->dialog)->colorsel),
				 color->color);
  script_fu_color_preview (color->preview, color->color);
}

static void
script_fu_color_preview_cancel (GtkWidget *widget,
				gpointer data)
{
  SFColor *color;

  color = (SFColor *) data;

  gtk_widget_hide(color->dialog);

  color->color[0] = color->old_color[0];
  color->color[1] = color->old_color[1];
  color->color[2] = color->old_color[2];

  script_fu_color_preview (color->preview, color->color);
}

static gint
script_fu_color_preview_delete (GtkWidget *widget,
				GdkEvent *event,
				gpointer data)
{
  script_fu_color_preview_cancel (widget, data);
  return TRUE;
}

static void
script_fu_file_preview_callback (GtkWidget *widget,
				 gpointer   data)
{
  GtkFileSelection *fs;
  SFFilename *file;

  file = (SFFilename *) data;

  if (!file->dialog)
    {
      file->dialog = gtk_file_selection_new ("Script-Fu File Selection");
      fs = GTK_FILE_SELECTION (file->dialog);

      gtk_signal_connect (GTK_OBJECT (fs->ok_button), "clicked",
			  (GtkSignalFunc) script_fu_file_dialog_ok,
			  file);
      gtk_signal_connect (GTK_OBJECT (fs), "delete_event",
			  (GtkSignalFunc) script_fu_file_dialog_delete,
			  file);
      gtk_signal_connect (GTK_OBJECT (fs->cancel_button), "clicked",
			  (GtkSignalFunc) script_fu_file_dialog_cancel,
			  file);
    }
  else
    fs = GTK_FILE_SELECTION (file->dialog);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (fs), file->filename);
  gtk_window_position (GTK_WINDOW (file->dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_show (file->dialog);
}

static void
script_fu_file_dialog_ok (GtkWidget *widget,
			  gpointer data)
{
  SFFilename *file;
  gchar  *filename;

  file = (SFFilename *) data;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (file->dialog));
  if (filename != NULL)
    {
      g_free (file->filename);
      file->filename = g_strdup(filename);
    }
  gtk_widget_hide (file->dialog);

  script_fu_file_preview (file->preview, file->filename);
}

static void
script_fu_file_dialog_cancel (GtkWidget *widget,
			      gpointer data)
{
  SFFilename *file;
  file = (SFFilename *) data;

  gtk_widget_hide (file->dialog);
}

static gint
script_fu_file_dialog_delete (GtkWidget *widget,
			      GdkEvent *event,
			      gpointer data)
{
   script_fu_file_dialog_cancel (widget, data);
   return TRUE;
 }

static void
script_fu_font_preview_callback (GtkWidget *widget,
				 gpointer   data)
{
  GtkFontSelectionDialog *fsd;
  SFFont *font;

  font = (SFFont *) data;

  if (!font->dialog)
    {
      font->dialog = gtk_font_selection_dialog_new ("Script-Fu Font Selection");
      fsd = GTK_FONT_SELECTION_DIALOG (font->dialog);

      gtk_signal_connect (GTK_OBJECT (fsd->ok_button), "clicked",
			  (GtkSignalFunc) script_fu_font_dialog_ok,
			  font);
      gtk_signal_connect (GTK_OBJECT (fsd), "delete_event",
			  (GtkSignalFunc) script_fu_font_dialog_delete,
			  font);
      gtk_signal_connect (GTK_OBJECT (fsd->cancel_button), "clicked",
			  (GtkSignalFunc) script_fu_font_dialog_cancel,
			  font);
    }
  else
    fsd = GTK_FONT_SELECTION_DIALOG (font->dialog);

  gtk_font_selection_dialog_set_font_name (GTK_FONT_SELECTION_DIALOG (fsd),
					   font->fontname);
  gtk_window_position (GTK_WINDOW (font->dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_show (font->dialog);
}

static void
script_fu_font_dialog_ok (GtkWidget *widget,
			  gpointer data)
{
  SFFont *font;
  gchar  *fontname;

  font = (SFFont *) data;

  
  fontname = gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (font->dialog));
  if (fontname != NULL)
    {
      g_free (font->fontname);
      font->fontname = fontname;
    }
  gtk_widget_hide (font->dialog);

  script_fu_font_preview (font->preview, font->fontname);
}

static void
script_fu_font_dialog_cancel (GtkWidget *widget,
			      gpointer data)
{
  SFFont *font;

  font = (SFFont *) data;

  gtk_widget_hide (font->dialog);
}

static gint
script_fu_font_dialog_delete (GtkWidget *widget,
			      GdkEvent *event,
			      gpointer data)
{
  script_fu_font_dialog_cancel (widget, data);
  return TRUE;
}


static void
script_fu_about_dialog_close (GtkWidget *widget,
			      gpointer data)
{
  if (data != NULL)
    gtk_widget_hide (GTK_WIDGET (data));
}

static gint
script_fu_about_dialog_delete (GtkWidget *widget,
			       GdkEvent *event,
			       gpointer data)
{
  script_fu_about_dialog_close (widget, data);
  return TRUE;
}






