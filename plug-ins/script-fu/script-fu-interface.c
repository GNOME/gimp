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

#include "config.h"

#include <glib.h>		/* Include early for obscure Win32
				   build reasons */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <libgimpbase/gimpbase.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "siod/siod.h"

#include "script-fu-scripts.h"
#include "siod-wrapper.h"

#include "script-fu-intl.h"


#define RESPONSE_RESET         1

#define TEXT_WIDTH           100
#define COLOR_SAMPLE_WIDTH   100
#define COLOR_SAMPLE_HEIGHT   15
#define SLIDER_WIDTH          80

#define MAX_STRING_LENGTH   4096


typedef struct
{
  GtkAdjustment    *adj;
  gdouble           value;
  gdouble           lower;
  gdouble           upper;
  gdouble           step;
  gdouble           page;
  gint              digits;
  SFAdjustmentType  type;
} SFAdjustment;

typedef struct
{
  GtkWidget *file_entry;
  gchar     *filename;
} SFFilename;

typedef struct
{
  gchar                *name;
  gdouble               opacity;
  gint                  spacing;
  GimpLayerModeEffects  paint_mode;
} SFBrush;

typedef struct
{
  GSList  *list;
  guint    history;
} SFOption;

typedef union
{
  gint32         sfa_image;
  gint32         sfa_drawable;
  gint32         sfa_layer;
  gint32         sfa_channel;
  GimpRGB        sfa_color;
  gint32         sfa_toggle;
  gchar         *sfa_value;
  SFAdjustment   sfa_adjustment;
  SFFilename     sfa_file;
  gchar         *sfa_font;
  gchar         *sfa_gradient;
  gchar         *sfa_pattern;
  SFBrush        sfa_brush;
  SFOption       sfa_option;
} SFArgValue;

typedef struct
{
  gchar         *script_name;
  gchar         *pdb_name;
  gchar         *description;
  gchar         *help;
  gchar         *author;
  gchar         *copyright;
  gchar         *date;
  gchar         *img_types;
  gint           num_args;
  SFArgType     *arg_types;
  gchar        **arg_labels;
  SFArgValue    *arg_defaults;
  SFArgValue    *arg_values;
  gboolean       image_based;
  GimpParamDef  *args;     /*  used only temporary until installed  */
} SFScript;

typedef struct
{
  GtkWidget     *dialog;
  GtkWidget    **args_widgets;
  GtkWidget     *status;
  GtkWidget     *about_dialog;
  gchar         *title;
  gchar         *last_command;
  gint           command_count;
  gint           consec_command_count;
} SFInterface;


/* External functions
 */
extern long  nlength (LISP obj);

/*
 *  Local Functions
 */

static void       script_fu_load_script (const GimpDatafileData   *file_data,
                                         gpointer                  user_data);
static gboolean   script_fu_install_script      (gpointer          foo,
                                                 SFScript         *script,
                                                 gpointer          bar);
static gboolean   script_fu_remove_script       (gpointer          foo,
                                                 SFScript         *script,
                                                 gpointer          bar);
static void       script_fu_script_proc         (const gchar      *name,
                                                 gint              nparams,
                                                 const GimpParam  *params,
                                                 gint             *nreturn_vals,
                                                 GimpParam       **return_vals);

static SFScript * script_fu_find_script         (const gchar      *script_name);
static void       script_fu_free_script         (SFScript         *script);
static void       script_fu_interface           (SFScript         *script);
static void       script_fu_interface_quit      (SFScript         *script);
static void       script_fu_error_msg           (const gchar      *command);

static void       script_fu_response            (GtkWidget        *widget,
                                                 gint              response_id,
                                                 SFScript         *script);
static void       script_fu_ok                  (SFScript         *script);
static void       script_fu_reset               (SFScript         *script);
static void       script_fu_about_callback      (GtkWidget        *widget,
                                                 SFScript         *script);
static void       script_fu_menu_callback       (gint32            id,
                                                 gpointer          data);

static void       script_fu_file_entry_callback (GtkWidget        *widget,
                                                 SFFilename       *fil);

static void       script_fu_pattern_callback    (const gchar      *name,
                                                 gint              width,
                                                 gint              height,
                                                 gint              bytes,
                                                 const guchar     *mask_data,
                                                 gboolean          closing,
                                                 gpointer          data);
static void       script_fu_gradient_callback   (const gchar      *name,
                                                 gint              width,
                                                 const gdouble    *mask_data,
                                                 gboolean          closing,
                                                 gpointer          data);
static void       script_fu_font_callback       (const gchar      *name,
                                                 gboolean          closing,
                                                 gpointer          data);
static void       script_fu_brush_callback      (const gchar      *name,
                                                 gdouble           opacity,
                                                 gint              spacing,
                                                 GimpLayerModeEffects  paint_mode,
                                                 gint              width,
                                                 gint              height,
                                                 const guchar     *mask_data,
                                                 gboolean          closing,
                                                 gpointer          data);


/*
 *  Local variables
 */

static GTree       *script_list  = NULL;
static SFInterface *sf_interface = NULL;  /*  there can only be at most one
					      interactive interface  */


/*
 *  Function definitions
 */

void
script_fu_find_scripts (void)
{
  gchar *path_str;

  /*  Make sure to clear any existing scripts  */
  if (script_list != NULL)
    {
      g_tree_foreach (script_list,
                      (GTraverseFunc) script_fu_remove_script,
                      NULL);
      g_tree_destroy (script_list);
    }

  script_list = g_tree_new ((GCompareFunc) strcoll);

  path_str = gimp_gimprc_query ("script-fu-path");

  if (path_str == NULL)
    return;

  gimp_datafiles_read_directories (path_str, G_FILE_TEST_IS_REGULAR,
                                   script_fu_load_script,
                                   NULL);

  g_free (path_str);

  /*  now that all scripts are read in and sorted, tell gimp about them  */
  g_tree_foreach (script_list,
                  (GTraverseFunc) script_fu_install_script,
                  NULL);
}

LISP
script_fu_add_script (LISP a)
{
  GimpParamDef *args;
  SFScript     *script;
  gchar        *val;
  gint          i;
  guchar        r, g, b;
  LISP          color_list;
  LISP          adj_list;
  LISP          brush_list;
  LISP          option_list;
  gchar        *s;

  /*  Check the length of a  */
  if (nlength (a) < 7)
    return my_err ("Too few arguments to script-fu-register", NIL);

  /*  Create a new script  */
  script = g_new0 (SFScript, 1);

  /*  Find the script name  */
  val = get_c_string (car (a));
  script->script_name = g_strdup (val);
  a = cdr (a);

  /* transform the function name into a name containing "_" for each "-".
   * this does not hurt anybody, yet improves the life of many... ;)
   */
  script->pdb_name = g_strdup (val);

  for (s = script->pdb_name; *s; s++)
    if (*s == '-')
      *s = '_';

  /*  Find the script description  */
  val = get_c_string (car (a));
  script->description = g_strdup (val);
  a = cdr (a);

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

  args = g_new (GimpParamDef, script->num_args + 1);
  args[0].type        = GIMP_PDB_INT32;
  args[0].name        = "run_mode";
  args[0].description = "Interactive, non-interactive";

  script->arg_types    = g_new (SFArgType, script->num_args);
  script->arg_labels   = g_new (gchar *, script->num_args);
  script->arg_defaults = g_new0 (SFArgValue, script->num_args);
  script->arg_values   = g_new0 (SFArgValue, script->num_args);

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
		      args[i + 1].type = GIMP_PDB_IMAGE;
		      args[i + 1].name = "image";
		      break;

		    case SF_DRAWABLE:
		      args[i + 1].type = GIMP_PDB_DRAWABLE;
		      args[i + 1].name = "drawable";
		      break;

		    case SF_LAYER:
		      args[i + 1].type = GIMP_PDB_LAYER;
		      args[i + 1].name = "layer";
		      break;

		    case SF_CHANNEL:
		      args[i + 1].type = GIMP_PDB_CHANNEL;
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
		  r = CLAMP (get_c_long (car (color_list)), 0, 255);
		  color_list = cdr (color_list);
		  g = CLAMP (get_c_long (car (color_list)), 0, 255);
		  color_list = cdr (color_list);
		  b = CLAMP (get_c_long (car (color_list)), 0, 255);

		  gimp_rgb_set_uchar (&script->arg_defaults[i].sfa_color,
				      r, g, b);

		  script->arg_values[i].sfa_color =
		    script->arg_defaults[i].sfa_color;

		  args[i + 1].type = GIMP_PDB_COLOR;
		  args[i + 1].name = "color";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_TOGGLE:
		  if (!TYPEP (car (a), tc_flonum))
		    return my_err ("script-fu-register: toggle default must be an integer value", NIL);
		  script->arg_defaults[i].sfa_toggle =
		    (get_c_long (car (a))) ? TRUE : FALSE;
		  script->arg_values[i].sfa_toggle =
		    script->arg_defaults[i].sfa_toggle;

		  args[i + 1].type = GIMP_PDB_INT32;
		  args[i + 1].name = "toggle";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_VALUE:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: value defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_value =
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_value =
		    g_strdup (script->arg_defaults[i].sfa_value);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_STRING:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: string defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_value =
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_value =
		    g_strdup (script->arg_defaults[i].sfa_value);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "string";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_ADJUSTMENT:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: adjustment defaults must be a list", NIL);
		  adj_list = car (a);
		  script->arg_defaults[i].sfa_adjustment.value =
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.lower =
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.upper =
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.step =
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.page =
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.digits =
		    get_c_long (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.type =
		    get_c_long (car (adj_list));
		  script->arg_values[i].sfa_adjustment.adj = NULL;
		  script->arg_values[i].sfa_adjustment.value =
		    script->arg_defaults[i].sfa_adjustment.value;

		  args[i + 1].type = GIMP_PDB_FLOAT;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_FILENAME:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: filename defaults must be string values", NIL);
                  /* fallthrough */
		case SF_DIRNAME:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: dirname defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_file.filename =
		    g_strdup (get_c_string (car (a)));

#ifdef G_OS_WIN32
		  /* Replace POSIX slashes with Win32 backslashes. This
		   * is just so script-fus can be written with only
		   * POSIX directory separators.
		   */
		  val = script->arg_defaults[i].sfa_file.filename;
		  while (*val)
		    {
		      if (*val == '/')
			*val = '\\';
		      val++;
		    }
#endif
		  script->arg_values[i].sfa_file.filename =
		    g_strdup (script->arg_defaults[i].sfa_file.filename);
		  script->arg_values[i].sfa_file.file_entry = NULL;

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = (script->arg_types[i] == SF_FILENAME ?
                                      "filename" : "dirname");
		  args[i + 1].description = script->arg_labels[i];
		 break;

		case SF_FONT:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: font defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_font =
                    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_font =
                    g_strdup (script->arg_defaults[i].sfa_font);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "font";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_PATTERN:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: pattern defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_pattern =
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_pattern =
		    g_strdup (script->arg_defaults[i].sfa_pattern);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "pattern";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_BRUSH:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: brush defaults must be a list", NIL);
		  brush_list = car (a);
		  script->arg_defaults[i].sfa_brush.name =
		    g_strdup (get_c_string (car (brush_list)));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.opacity =
		    get_c_double (car (brush_list));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.spacing =
		    get_c_long (car (brush_list));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.paint_mode =
		    get_c_long (car (brush_list));
		  script->arg_values[i].sfa_brush =
		    script->arg_defaults[i].sfa_brush;
		  /* Need this since we need a copy of the string
		   * in the values area. We could free it later but the
		   * default one must hang around.
		   */
		  script->arg_values[i].sfa_brush.name =
		    g_strdup(script->arg_defaults[i].sfa_brush.name);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "brush";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_GRADIENT:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: gradient defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_gradient =
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_gradient =
		    g_strdup (script->arg_defaults[i].sfa_pattern);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "gradient";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_OPTION:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: option defaults must be a list", NIL);
		  for (option_list = car (a);
		       option_list;
		       option_list = cdr (option_list))
		    {
		      script->arg_defaults[i].sfa_option.list =
			g_slist_append (script->arg_defaults[i].sfa_option.list,
					g_strdup (get_c_string (car (option_list))));
		    }
		  script->arg_defaults[i].sfa_option.history = 0;
		  script->arg_values[i].sfa_option.history = 0;

		  args[i + 1].type = GIMP_PDB_INT32;
		  args[i + 1].name = "option";
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

  script->args = args;
  g_tree_insert (script_list, gettext (script->description), script);

  return NIL;
}

void
script_fu_report_cc (gchar *command)
{
  if (sf_interface == NULL)
    return;

  if (sf_interface->last_command &&
      strcmp (sf_interface->last_command, command) == 0)
    {
      gchar *new_command;

      sf_interface->command_count++;

      new_command = g_strdup_printf ("%s <%d>",
				     command, sf_interface->command_count);
      gtk_label_set_text (GTK_LABEL (sf_interface->status), new_command);
      g_free (new_command);
    }
  else
    {
      sf_interface->command_count = 1;
      gtk_label_set_text (GTK_LABEL (sf_interface->status), command);
      g_free (sf_interface->last_command);
      sf_interface->last_command = g_strdup (command);
    }

  while (gtk_main_iteration ());
}


/*  private functions  */

static void
script_fu_load_script (const GimpDatafileData *file_data,
                       gpointer                user_data)
{
  if (gimp_datafiles_check_extension (file_data->filename, ".scm"))
    {
      gchar *command;
      gchar *qf = g_strescape (file_data->filename, NULL);

      command = g_strdup_printf ("(load \"%s\")", qf);
      g_free (qf);

      if (repl_c_string (command, 0, 0, 1) != 0)
        script_fu_error_msg (command);

#ifdef G_OS_WIN32
      /* No, I don't know why, but this is
       * necessary on NT 4.0.
       */
      Sleep(0);
#endif

      g_free (command);
    }
}

/*
 *  The following function is a GTraverseFunction.  Please
 *  note that it frees the script->args structure.  --Sven
 */
static gboolean
script_fu_install_script (gpointer  foo,
			  SFScript *script,
			  gpointer  bar)
{
  gchar *menu_path = NULL;

  /* Allow scripts with no menus */
  if (strncmp (script->description, "<None>", 6) != 0)
    menu_path = script->description;

  gimp_install_temp_proc (script->pdb_name,
                          script->description,
                          script->help,
                          script->author,
                          script->copyright,
                          script->date,
                          menu_path,
                          script->img_types,
                          GIMP_TEMPORARY,
                          script->num_args + 1, 0,
                          script->args, NULL,
                          script_fu_script_proc);

  g_free (script->args);
  script->args = NULL;

  return FALSE;
}

/*
 *  The following function is a GTraverseFunction.
 */
static gboolean
script_fu_remove_script (gpointer  foo,
			 SFScript *script,
			 gpointer  bar)
{
  script_fu_free_script (script);

  return FALSE;
}

static void
script_fu_script_proc (const gchar      *name,
		       gint              nparams,
		       const GimpParam  *params,
		       gint             *nreturn_vals,
		       GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  SFScript          *script;
  gint               min_args;
  gchar             *escaped;

  run_mode = params[0].data.d_int32;

  if (! (script = script_fu_find_script (name)))
    {
      status = GIMP_PDB_CALLING_ERROR;
    }
  else
    {
      if (script->num_args == 0)
	run_mode = GIMP_RUN_NONINTERACTIVE;

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Determine whether the script is image based (runs on an image) */
	  if (strncmp (script->description, "<Image>", 7) == 0)
	    {
	      script->arg_values[0].sfa_image    = params[1].data.d_image;
	      script->arg_values[1].sfa_drawable = params[2].data.d_drawable;
	      script->image_based = TRUE;
	    }
	  else
	    script->image_based = FALSE;

	  /*  First acquire information with a dialog  */
	  /*  Skip this part if the script takes no parameters */
	  min_args = (script->image_based) ? 2 : 0;
	  if (script->num_args > min_args)
	    {
	      script_fu_interface (script);
	      break;
	    }
	  /*  else fallthrough  */

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != (script->num_args + 1))
	    status = GIMP_PDB_CALLING_ERROR;
	  if (status == GIMP_PDB_SUCCESS)
	    {
	      guchar color[3];
	      gchar *text = NULL;
	      gchar *command;
	      gchar *c;
	      gchar  buffer[MAX_STRING_LENGTH];
	      gint   length;
	      gint   i;

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
		  case SF_FILENAME:
		  case SF_DIRNAME:
		    escaped = g_strescape (params[i + 1].data.d_string, NULL);
		    length += strlen (escaped) + 3;
		    g_free (escaped);
		    break;

		  case SF_ADJUSTMENT:
		    length += G_ASCII_DTOSTR_BUF_SIZE;
		    break;

		  case SF_FONT:
		  case SF_PATTERN:
		  case SF_GRADIENT:
		  case SF_BRUSH:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;

		  case SF_OPTION:
		    length += strlen (params[i + 1].data.d_string) + 1;
		    break;

		  default:
		    break;
		  }

	      c = command = g_new (gchar, length);

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
                          g_snprintf (buffer, sizeof (buffer), "%d",
				      params[i + 1].data.d_image);
                          text = buffer;
                          break;

                        case SF_COLOR:
			  gimp_rgb_get_uchar (&params[i + 1].data.d_color,
					      color, color + 1, color + 2);
                          g_snprintf (buffer, sizeof (buffer), "'(%d %d %d)",
				      color[0], color[1], color[2]);
                          text = buffer;
                          break;

                        case SF_TOGGLE:
                          g_snprintf (buffer, sizeof (buffer), "%s",
				      (params[i + 1].data.d_int32) ? "TRUE"
				                                   : "FALSE");
                          text = buffer;
                          break;

                        case SF_VALUE:
                          text = params[i + 1].data.d_string;
                          break;

                        case SF_STRING:
                        case SF_FILENAME:
                        case SF_DIRNAME:
                          escaped = g_strescape (params[i + 1].data.d_string,
                                                 NULL);
                          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
				      escaped);
                          g_free (escaped);
                          text = buffer;
                          break;

                        case SF_ADJUSTMENT:
                          text = g_ascii_dtostr (buffer, sizeof (buffer),
                                                 params[i + 1].data.d_float);
                          break;

                        case SF_FONT:
                        case SF_PATTERN:
                        case SF_GRADIENT:
                        case SF_BRUSH:
                          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
				      params[i + 1].data.d_string);
                          text = buffer;
                          break;

                        case SF_OPTION:
                          text = params[i + 1].data.d_string;
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
	      if (repl_c_string (command, 0, 0, 1) != 0)
		script_fu_error_msg (command);

	      g_free (command);
	    }
	  break;

	default:
	  break;
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

/* this is a GTraverseFunction */
static gboolean
script_fu_lookup_script (gpointer      *foo,
			 SFScript      *script,
			 gconstpointer *name)
{
  if (strcmp (script->pdb_name, *name) == 0)
    {
      /* store the script in the name pointer and stop the traversal */
      *name = script;
      return TRUE;
    }
  else
    return FALSE;
}

static SFScript *
script_fu_find_script (const gchar *pdb_name)
{
  gconstpointer script = pdb_name;

  g_tree_foreach (script_list,
                  (GTraverseFunc) script_fu_lookup_script,
                  &script);

  if (script == pdb_name)
    return NULL;
  else
    return (SFScript *) script;
}

static void
script_fu_free_script (SFScript *script)
{
  gint i;

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
	    case SF_DIRNAME:
	      g_free (script->arg_defaults[i].sfa_file.filename);
	      g_free (script->arg_values[i].sfa_file.filename);
	      break;

	    case SF_FONT:
	      g_free (script->arg_defaults[i].sfa_font);
	      g_free (script->arg_values[i].sfa_font);
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

	    case SF_OPTION:
	      g_slist_foreach (script->arg_defaults[i].sfa_option.list,
			       (GFunc)g_free, NULL);
	      if (script->arg_defaults[i].sfa_option.list)
		g_slist_free (script->arg_defaults[i].sfa_option.list);
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
script_fu_interface (SFScript *script)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *menu;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *menu_item;
  GSList    *list;
  gchar     *title;
  gchar     *buf;
  gint       start_args;
  gint       i;
  guint      j;

  static gboolean gtk_initted = FALSE;

  /*  Simply return if there is already an interface. This is an
      ugly workaround for the fact that we can not process two
      scripts at a time.  */
  if (sf_interface != NULL)
    return;

  g_return_if_fail (script != NULL);

  if (!gtk_initted)
    {
      INIT_I18N();

      gimp_ui_init ("script-fu", TRUE);

      gtk_initted = TRUE;
    }

  sf_interface = g_new0 (SFInterface, 1);
  sf_interface->args_widgets = g_new0 (GtkWidget *, script->num_args);

  /* strip the first part of the menupath if it contains _("/Script-Fu/") */
  buf = strstr (gettext (script->description), _("/Script-Fu/"));
  if (buf)
    title = g_strdup (buf + strlen (_("/Script-Fu/")));
  else
    title = g_strdup (gettext (script->description));

  /* strip mnemonics from the menupath */
  sf_interface->title = gimp_strip_uline (title);
  g_free (title);

  buf = strstr (sf_interface->title, "...");
  if (buf)
    *buf = '\0';

  buf = g_strdup_printf (_("Script-Fu: %s"), sf_interface->title);

  sf_interface->dialog = dlg =
    gimp_dialog_new (buf, "script-fu",
                     NULL, 0,
                     gimp_standard_help_func, "filters/script-fu.html",

                     GIMP_STOCK_RESET, RESPONSE_RESET,
                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                     NULL);

  g_free (buf);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (script_fu_response),
                    script);

  g_signal_connect_swapped (dlg, "destroy",
                            G_CALLBACK (script_fu_interface_quit),
                            script);

  gtk_window_set_resizable (GTK_WINDOW (dlg), TRUE);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  sf_interface->status = gtk_label_new (sf_interface->title);
  gtk_misc_set_alignment (GTK_MISC (sf_interface->status), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), sf_interface->status, TRUE, TRUE, 0);
  gtk_widget_show (sf_interface->status);

  button = gtk_button_new_with_label (_("About"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (script_fu_about_callback),
		    script);

  /* the script arguments frame */
  frame = gtk_frame_new (_("Script Arguments"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  /* the vbox holding all widgets */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  /*  The argument table  */
  table = gtk_table_new (script->num_args + 1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  start_args = (script->image_based) ? 2 : 0;

  for (i = start_args; i < script->num_args; i++)
    {
      GtkWidget *widget           = NULL;
      gchar     *label_text;
      gfloat     label_yalign     = 0.5;
      gboolean   widget_leftalign = TRUE;

      /*  we add a colon after the label;
          some languages want an extra space here  */
      label_text =  g_strdup_printf (_("%s:"),
                                     gettext (script->arg_labels[i]));

      switch (script->arg_types[i])
	{
	case SF_IMAGE:
	case SF_DRAWABLE:
	case SF_LAYER:
	case SF_CHANNEL:
	  widget = gtk_option_menu_new ();
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
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (widget), menu);
	  break;

	case SF_COLOR:
	  widget = gimp_color_button_new (_("Script-Fu Color Selection"),
                                          COLOR_SAMPLE_WIDTH,
                                          COLOR_SAMPLE_HEIGHT,
                                          &script->arg_values[i].sfa_color,
                                          GIMP_COLOR_AREA_FLAT);

          gimp_color_button_set_update (GIMP_COLOR_BUTTON (widget), TRUE);
	  g_signal_connect (widget, "color_changed",
			    G_CALLBACK (gimp_color_button_get_color),
			    &script->arg_values[i].sfa_color);
	  break;

	case SF_TOGGLE:
	  g_free (label_text);
	  label_text = NULL;
	  widget = gtk_check_button_new_with_mnemonic (gettext (script->arg_labels[i]));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				       script->arg_values[i].sfa_toggle);

	  g_signal_connect (widget, "toggled",
			    G_CALLBACK (gimp_toggle_button_update),
			    &script->arg_values[i].sfa_toggle);
	  break;

	case SF_VALUE:
	case SF_STRING:
	  widget_leftalign = FALSE;

	  widget = gtk_entry_new ();
	  gtk_widget_set_size_request (widget, TEXT_WIDTH, -1);
	  gtk_entry_set_text (GTK_ENTRY (widget),
                              script->arg_values[i].sfa_value);
	  break;

	case SF_ADJUSTMENT:
	  switch (script->arg_defaults[i].sfa_adjustment.type)
	    {
	    case SF_SLIDER:
	      script->arg_values[i].sfa_adjustment.adj = (GtkAdjustment *)
		gimp_scale_entry_new (GTK_TABLE (table), 0, i,
				      label_text, SLIDER_WIDTH, -1,
				      script->arg_values[i].sfa_adjustment.value,
				      script->arg_defaults[i].sfa_adjustment.lower,
				      script->arg_defaults[i].sfa_adjustment.upper,
				      script->arg_defaults[i].sfa_adjustment.step,
				      script->arg_defaults[i].sfa_adjustment.page,
				      script->arg_defaults[i].sfa_adjustment.digits,
				      TRUE, 0.0, 0.0,
				      NULL, NULL);
	      break;

	    case SF_SPINNER:
	      script->arg_values[i].sfa_adjustment.adj = (GtkAdjustment *)
		gtk_adjustment_new (script->arg_values[i].sfa_adjustment.value,
				    script->arg_defaults[i].sfa_adjustment.lower,
				    script->arg_defaults[i].sfa_adjustment.upper,
				    script->arg_defaults[i].sfa_adjustment.step,
				    script->arg_defaults[i].sfa_adjustment.page, 0);
              widget = gtk_spin_button_new (script->arg_values[i].sfa_adjustment.adj,
                                            0,
                                            script->arg_defaults[i].sfa_adjustment.digits);
	      break;

	    default:
	      break;
	    }
	  break;

	case SF_FILENAME:
	case SF_DIRNAME:
	  widget_leftalign = FALSE;

          if (script->arg_types[i] == SF_FILENAME)
            widget = gimp_file_entry_new (_("Script-Fu File Selection"),
                                          script->arg_values[i].sfa_file.filename,
                                          FALSE, TRUE);
          else
            widget = gimp_file_entry_new (_("Script-Fu Folder Selection"),
                                          script->arg_values[i].sfa_file.filename,
                                          TRUE, TRUE);

	  script->arg_values[i].sfa_file.file_entry = widget;

	  g_signal_connect (widget, "filename_changed",
			    G_CALLBACK (script_fu_file_entry_callback),
			    &script->arg_values[i].sfa_file);
	  break;

	case SF_FONT:
	  widget_leftalign = FALSE;

	  widget = gimp_font_select_widget_new (_("Script-Fu Font Selection"),
                                                script->arg_values[i].sfa_font,
                                                script_fu_font_callback,
                                                &script->arg_values[i].sfa_font);
	  break;

	case SF_PATTERN:
	  widget = gimp_pattern_select_widget_new (_("Script-fu Pattern Selection"),
                                                   script->arg_values[i].sfa_pattern,
                                                   script_fu_pattern_callback,
                                                   &script->arg_values[i].sfa_pattern);
	  break;
	case SF_GRADIENT:
	  widget = gimp_gradient_select_widget_new (_("Script-Fu Gradient Selection"),
                                                    script->arg_values[i].sfa_gradient,
                                                    script_fu_gradient_callback,
                                                    &script->arg_values[i].sfa_gradient);
	  break;

	case SF_BRUSH:
	  widget = gimp_brush_select_widget_new (_("Script-Fu Brush Selection"),
                                                 script->arg_values[i].sfa_brush.name,
                                                 script->arg_values[i].sfa_brush.opacity,
                                                 script->arg_values[i].sfa_brush.spacing,
                                                 script->arg_values[i].sfa_brush.paint_mode,
                                                 script_fu_brush_callback,
                                                 &script->arg_values[i].sfa_brush);
	  break;

	case SF_OPTION:
	  widget = gtk_option_menu_new ();
	  menu = gtk_menu_new ();
	  for (list = script->arg_defaults[i].sfa_option.list, j = 0;
	       list;
	       list = g_slist_next (list), j++)
	    {
	      menu_item = gtk_menu_item_new_with_label (gettext ((gchar *)list->data));
	      g_object_set_data (G_OBJECT (menu_item), "gimp-item-data",
                                 GUINT_TO_POINTER (j));
	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	      gtk_widget_show (menu_item);
	    }

	  gtk_option_menu_set_menu (GTK_OPTION_MENU (widget), menu);
	  gtk_option_menu_set_history (GTK_OPTION_MENU (widget),
                                       script->arg_values[i].sfa_option.history);
	  break;

	default:
	  break;
	}

      if (widget)
        gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
                                   label_text, 1.0, label_yalign,
                                   widget, 2, widget_leftalign);

      g_free (label_text);

      sf_interface->args_widgets[i] = widget;
    }

  gtk_widget_show (table);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
}

static void
script_fu_interface_quit (SFScript *script)
{
  gint i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (sf_interface != NULL);

  g_free (sf_interface->title);

  if (sf_interface->about_dialog)
    gtk_widget_destroy (sf_interface->about_dialog);

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_FONT:
  	gimp_font_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_PATTERN:
  	gimp_pattern_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_GRADIENT:
  	gimp_gradient_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_BRUSH:
  	gimp_brush_select_widget_close (sf_interface->args_widgets[i]);
	break;

      default:
	break;
      }

  g_free (sf_interface->args_widgets);
  g_free (sf_interface->last_command);

  g_free (sf_interface);
  sf_interface = NULL;

  /*
   *  We do not call gtk_main_quit() earlier to reduce the possibility
   *  that script_fu_script_proc() is called from gimp_extension_process()
   *  while we are not finished with the current script. This sucks!
   */

  gtk_main_quit ();
}

static void
script_fu_pattern_callback (const gchar  *name,
                            gint          width,
                            gint          height,
                            gint          bytes,
                            const guchar *mask_data,
                            gboolean      closing,
                            gpointer      data)
{
  gchar **pname = data;

  g_free (*pname);
  *pname = g_strdup (name);
}

static void
script_fu_gradient_callback (const gchar   *name,
                             gint           width,
                             const gdouble *mask_data,
                             gboolean       closing,
                             gpointer       data)
{
  gchar **gname = data;

  g_free (*gname);
  *gname = g_strdup (name);
}

static void
script_fu_font_callback (const gchar *name,
                         gboolean     closing,
                         gpointer     data)
{
  gchar **fname = data;

  g_free (*fname);
  *fname = g_strdup (name);
}

static void
script_fu_brush_callback (const gchar          *name,
                          gdouble               opacity,
                          gint                  spacing,
                          GimpLayerModeEffects  paint_mode,
                          gint                  width,
                          gint                  height,
                          const guchar         *mask_data,
                          gboolean              closing,
                          gpointer              data)
{
  SFBrush *brush = data;

  g_free (brush->name);

  brush->name       = g_strdup (name);
  brush->opacity    = opacity;
  brush->spacing    = spacing;
  brush->paint_mode = paint_mode;
}

static void
script_fu_response (GtkWidget *widget,
                    gint       response_id,
                    SFScript  *script)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      script_fu_reset (script);
      break;

    case GTK_RESPONSE_OK:
      script_fu_ok (script);
      /* fallthru */

    default:
      gtk_widget_destroy (sf_interface->dialog);
      break;
    }
}

static void
script_fu_ok (SFScript *script)
{
  GtkWidget *menu_item;
  gchar     *escaped;
  gchar     *text = NULL;
  gchar     *command;
  gchar     *c;
  guchar     r, g, b;
  gchar      buffer[MAX_STRING_LENGTH];
  gint       length;
  gint       i;

  length = strlen (script->script_name) + 3;

  for (i = 0; i < script->num_args; i++)
    {
      GtkWidget *widget = sf_interface->args_widgets[i];

      switch (script->arg_types[i])
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
          length += 12;  /*  Maximum size of integer value  */
          break;

        case SF_COLOR:
          length += 16;  /*  Maximum size of color string: '(XXX XXX XXX)  */
          break;

        case SF_TOGGLE:
          length += 6;   /*  Maximum size of (TRUE, FALSE)  */
          break;

        case SF_VALUE:
          length += strlen (gtk_entry_get_text (GTK_ENTRY (widget))) + 1;
          break;

        case SF_STRING:
          escaped = g_strescape (gtk_entry_get_text (GTK_ENTRY (widget)),
                                 NULL);
          length += strlen (escaped) + 3;
          g_free (escaped);
          break;

        case SF_ADJUSTMENT:
          length += G_ASCII_DTOSTR_BUF_SIZE;
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          escaped = g_strescape (script->arg_values[i].sfa_file.filename,
                                 NULL);
          length += strlen (escaped) + 3;
          g_free (escaped);
          break;

        case SF_FONT:
          length += strlen (script->arg_values[i].sfa_font) + 3;
          break;

        case SF_PATTERN:
          length += strlen (script->arg_values[i].sfa_pattern) + 3;
          break;

        case SF_GRADIENT:
          length += strlen (script->arg_values[i].sfa_gradient) + 3;
          break;

        case SF_BRUSH:
          length += strlen (script->arg_values[i].sfa_brush.name) + 3;
          length += 36;  /*  Maximum size of three ints  */
          /*  for opacity, spacing, mode  */
          break;

        case SF_OPTION:
          length += 12;  /*  Maximum size of integer value  */
          break;

        default:
          break;
        }
    }

  c = command = g_new (gchar, length);

  sprintf (command, "(%s ", script->script_name);
  c += strlen (script->script_name) + 2;
  for (i = 0; i < script->num_args; i++)
    {
      GtkWidget *widget = sf_interface->args_widgets[i];

      switch (script->arg_types[i])
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
          g_snprintf (buffer, sizeof (buffer), "%d",
                      script->arg_values[i].sfa_image);
          text = buffer;
          break;

        case SF_COLOR:
          gimp_rgb_get_uchar (&script->arg_values[i].sfa_color, &r, &g, &b);
          g_snprintf (buffer, sizeof (buffer), "'(%d %d %d)",
                      (gint) r, (gint) g, (gint) b);
          text = buffer;
          break;

        case SF_TOGGLE:
          g_snprintf (buffer, sizeof (buffer), "%s",
                      (script->arg_values[i].sfa_toggle) ? "TRUE" : "FALSE");
          text = buffer;
          break;

        case SF_VALUE:
          text = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
          g_free (script->arg_values[i].sfa_value);
          script->arg_values[i].sfa_value = g_strdup (text);
          break;

        case SF_STRING:
          text = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
          g_free (script->arg_values[i].sfa_value);
          script->arg_values[i].sfa_value = g_strdup (text);
          escaped = g_strescape (text, NULL);
          g_snprintf (buffer, sizeof (buffer), "\"%s\"", escaped);
          g_free (escaped);
          text = buffer;
          break;

        case SF_ADJUSTMENT:
          switch (script->arg_defaults[i].sfa_adjustment.type)
            {
            case SF_SLIDER:
              script->arg_values[i].sfa_adjustment.value =
                script->arg_values[i].sfa_adjustment.adj->value;
              break;

            case SF_SPINNER:
              script->arg_values[i].sfa_adjustment.value =
                gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
              break;

            default:
              break;
            }
          text = g_ascii_dtostr (buffer, sizeof (buffer),
                                 script->arg_values[i].sfa_adjustment.value);
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          escaped = g_strescape (script->arg_values[i].sfa_file.filename,
                                 NULL);
          g_snprintf (buffer, sizeof (buffer), "\"%s\"", escaped);
          g_free (escaped);
          text = buffer;
          break;

        case SF_FONT:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_font);
          text = buffer;
          break;

        case SF_PATTERN:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_pattern);
          text = buffer;
          break;

        case SF_GRADIENT:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_gradient);
          text = buffer;
          break;

        case SF_BRUSH:
          {
            gchar opacity[G_ASCII_DTOSTR_BUF_SIZE];

            g_ascii_dtostr (opacity, sizeof (opacity),
                            script->arg_values[i].sfa_brush.opacity);

            g_snprintf (buffer, sizeof (buffer), "'(\"%s\" %s %d %d)",
                        script->arg_values[i].sfa_brush.name,
                        opacity,
                        script->arg_values[i].sfa_brush.spacing,
                        script->arg_values[i].sfa_brush.paint_mode);
            text = buffer;
          }
          break;

        case SF_OPTION:
          widget = gtk_option_menu_get_menu (GTK_OPTION_MENU (widget));
          menu_item = gtk_menu_get_active (GTK_MENU (widget));
          script->arg_values[i].sfa_option.history =
            GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (menu_item),
                                                 "gimp-item-data"));
          g_snprintf (buffer, sizeof (buffer), "%d",
                      script->arg_values[i].sfa_option.history);
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

  /*  run the command through the interpreter  */
  if (repl_c_string (command, 0, 0, 1) != 0)
    script_fu_error_msg (command);

  g_free (command);
}

static void
script_fu_reset (SFScript *script)
{
  gint i;

  for (i = 0; i < script->num_args; i++)
    {
      GtkWidget *widget = sf_interface->args_widgets[i];

      switch (script->arg_types[i])
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
          break;

        case SF_COLOR:
          script->arg_values[i].sfa_color = script->arg_defaults[i].sfa_color;
          gimp_color_button_set_color (GIMP_COLOR_BUTTON (widget),
                                       &script->arg_values[i].sfa_color);
	break;

        case SF_TOGGLE:
          script->arg_values[i].sfa_toggle = script->arg_defaults[i].sfa_toggle;
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        script->arg_values[i].sfa_toggle);
	break;

        case SF_VALUE:
        case SF_STRING:
          g_free (script->arg_values[i].sfa_value);
          script->arg_values[i].sfa_value =
            g_strdup (script->arg_defaults[i].sfa_value);
          gtk_entry_set_text (GTK_ENTRY (widget),
                              script->arg_values[i].sfa_value);
          break;

        case SF_ADJUSTMENT:
          script->arg_values[i].sfa_adjustment.value =
            script->arg_defaults[i].sfa_adjustment.value;
          gtk_adjustment_set_value (script->arg_values[i].sfa_adjustment.adj,
                                    script->arg_values[i].sfa_adjustment.value);
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          g_free (script->arg_values[i].sfa_file.filename);
          script->arg_values[i].sfa_file.filename =
            g_strdup (script->arg_defaults[i].sfa_file.filename);
          gimp_file_entry_set_filename
            (GIMP_FILE_ENTRY (script->arg_values[i].sfa_file.file_entry),
             script->arg_values[i].sfa_file.filename);
          break;

        case SF_FONT:
          gimp_font_select_widget_set (widget,
                                       script->arg_defaults[i].sfa_font);
          break;

        case SF_PATTERN:
          gimp_pattern_select_widget_set (widget,
                                          script->arg_defaults[i].sfa_pattern);
          break;

        case SF_GRADIENT:
          gimp_gradient_select_widget_set (widget,
                                           script->arg_defaults[i].sfa_gradient);
          break;

        case SF_BRUSH:
          gimp_brush_select_widget_set (widget,
                                        script->arg_defaults[i].sfa_brush.name,
                                        script->arg_defaults[i].sfa_brush.opacity,
                                        script->arg_defaults[i].sfa_brush.spacing,
                                        script->arg_defaults[i].sfa_brush.paint_mode);
          break;

        case SF_OPTION:
          script->arg_values[i].sfa_option.history =
            script->arg_defaults[i].sfa_option.history;
          gtk_option_menu_set_history (GTK_OPTION_MENU (widget),
                                       script->arg_values[i].sfa_option.history);

        default:
          break;
        }
    }
}

static void
script_fu_about_callback (GtkWidget *widget,
                          SFScript  *script)
{
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *label;
  GtkWidget     *scrolled_window;
  GtkWidget     *table;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;

  if (sf_interface->about_dialog == NULL)
    {
      sf_interface->about_dialog = dialog =
        gimp_dialog_new (sf_interface->title, "script-fu-about",
                         sf_interface->dialog, 0,
                         gimp_standard_help_func,
                         "filters/script-fu.html",

                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                         NULL);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      g_signal_connect (dialog, "destroy",
			G_CALLBACK (gtk_widget_destroyed),
			&sf_interface->about_dialog);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
			  TRUE, TRUE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      /* the name */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      label = gtk_label_new (script->script_name);
      gtk_misc_set_padding (GTK_MISC (label), 2, 2);
      gtk_container_add (GTK_CONTAINER (frame), label);
      gtk_widget_show (label);

      /* the help display */
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
      gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 4);
      gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_widget_show (scrolled_window);

      text_buffer = gtk_text_buffer_new (NULL);
      text_view = gtk_text_view_new_with_buffer (text_buffer);
      g_object_unref (text_buffer);

      gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
      gtk_widget_set_size_request (text_view, 240, 120);
      gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
      gtk_widget_show (text_view);

      gtk_text_buffer_set_text (text_buffer, script->help, -1);

      /* author, copyright, etc. */
      table = gtk_table_new (2, 4, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      label = gtk_label_new (script->author);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Author:"), 1.0, 0.5,
				 label, 1, FALSE);

      label = gtk_label_new (script->copyright);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Copyright:"), 1.0, 0.5,
				 label, 1, FALSE);

      label = gtk_label_new (script->date);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
				 _("Date:"), 1.0, 0.5,
				 label, 1, FALSE);

      if (strlen (script->img_types) > 0)
	{
	  label = gtk_label_new (script->img_types);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
				     _("Image Types:"), 1.0, 0.5,
				     label, 1, FALSE);
	}
    }

  gtk_widget_show (sf_interface->about_dialog);
}

static void
script_fu_menu_callback (gint32   id,
                         gpointer data)
{
  *((gint32 *) data) = id;
}

static void
script_fu_file_entry_callback (GtkWidget  *widget,
                               SFFilename *file)
{
  if (file->filename)
    g_free (file->filename);

  file->filename =
    gimp_file_entry_get_filename (GIMP_FILE_ENTRY(file->file_entry));
}

static void
script_fu_error_msg (const gchar *command)
{
  g_message (_("Error while executing\n%s\n%s"),
	     command, siod_get_error_msg ());
}
