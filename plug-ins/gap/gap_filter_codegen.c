/* gap_filter_codegen.c
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - GAP_filter  codegenerator procedures for _iterator_ALT procedures
 * 
 * Note: this code is only used in debug mode,
 *       (for developers (Hackers) to generate code templates
 *       for _iterator_ALT  or _Iterator procedures.)
 */

/* revision history:
 * version 0.99.00  1999.03.14  hof: Codegeneration of File ./gen_filter_iter_code.c
 *                                   splittet into single Files XX_iter_ALT.inc
 *                                   bugfixes in code generation
 * version 0.95.04  1998.06.12  hof: p_delta_drawable (enable use of layerstack anims in drawable iteration)
 * version 0.93.00              hof: generate Iterator Source
 *                                   in one single file (per plugin), ready to compile
 * version 0.91.01; Tue Dec 23  hof: 1.st (pre) release
 */
#include "config.h"
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "libgimp/gimp.h"

/* GAP includes */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

extern int gap_debug;
gint p_gen_code_iter(char  *proc_name);

#define GEN_FORWARDFILE_NAME     "gen_filter_iter_forward.c"
#define GEN_TABFILE_NAME         "gen_filter_iter_tab.c"

void p_remove_codegen_files()
{
   remove(GEN_FORWARDFILE_NAME);
   remove(GEN_TABFILE_NAME);
   
   printf("overwrite  file: %s\n", GEN_FORWARDFILE_NAME);
   printf("overwrite  file: %s\n", GEN_TABFILE_NAME);
}


static char* 
p_type_to_string(GimpPDBArgType t)
{
  switch (t) {
  case GIMP_PDB_INT32:         return "long     ";
  case GIMP_PDB_INT16:         return "short    ";
  case GIMP_PDB_INT8:          return "char     ";
  case GIMP_PDB_FLOAT:         return "gdouble  ";
  case GIMP_PDB_STRING:        return "char     *";
  case GIMP_PDB_INT32ARRAY:    return "INT32ARRAY";
  case GIMP_PDB_INT16ARRAY:    return "INT16ARRAY";
  case GIMP_PDB_INT8ARRAY:     return "INT8ARRAY";
  case GIMP_PDB_FLOATARRAY:    return "FLOATARRAY";
  case GIMP_PDB_STRINGARRAY:   return "STRINGARRAY";
  case GIMP_PDB_COLOR:         return "t_color  ";
  case GIMP_PDB_REGION:        return "REGION";
  case GIMP_PDB_DISPLAY:       return "gint32   ";
  case GIMP_PDB_IMAGE:         return "gint32   ";
  case GIMP_PDB_LAYER:         return "gint32   ";
  case GIMP_PDB_CHANNEL:       return "gint32   ";
  case GIMP_PDB_DRAWABLE:      return "gint32   ";
  case GIMP_PDB_SELECTION:     return "SELECTION";
  case GIMP_PDB_BOUNDARY:      return "BOUNDARY";
  case GIMP_PDB_PATH:          return "PATH";
  case GIMP_PDB_STATUS:        return "STATUS";
  case GIMP_PDB_END:           return "END";
  default:                  return "UNKNOWN?";
  }
}


static void p_get_gendate(char *gendate)
{
  struct      tm *l_t;
  long        l_ti;

  l_ti = time(0L);          /* Get UNIX time */
  l_t  = localtime(&l_ti);  /* konvert time to tm struct */
  sprintf(gendate, "%02d.%02d.%02d %02d:%02d"
	   , l_t->tm_mday
	   , l_t->tm_mon + 1
	   , l_t->tm_year
	   , l_t->tm_hour
	   , l_t->tm_min);
}


static void
p_clean_name(char *name, char *clean_name)
{
  char *l_ptr;
  
  l_ptr = clean_name;
  while(*name != '\0')
  {
    if((*name == '-')
    || (*name == '+')
    || (*name == '/')
    || (*name == '%')
    || (*name == '*')
    || (*name == ':')
    || (*name == '!')
    || (*name == '=')
    || (*name == ';')
    || (*name == '^')
    || (*name == ',')
    || (*name == '[')
    || (*name == ']')
    || (*name == '{')
    || (*name == '}')
    || (*name == '(')
    || (*name == ')')
    || (*name == ' ')
    || (*name == '$')
    || (*name == '<')
    || (*name == '|')
    || (*name == '>')
    || (*name == '?')
    || (*name == '~')
      )
    {
      *l_ptr = '_';
    }
    else
    {
      *l_ptr = *name;
    }
    name++;
    l_ptr++;
  }
  *l_ptr = '\0';
}

gint p_gen_code_iter_ALT(char  *proc_name)
{
  FILE             *l_fp;
  gint              l_idx;
  
  gint              l_nparams;
  gint              l_nreturn_vals;
  GimpPDBProcType   l_proc_type;
  gchar            *l_proc_blurb;
  gchar            *l_proc_help;
  gchar            *l_proc_author;
  gchar            *l_proc_copyright;
  gchar            *l_proc_date;
  GimpParamDef     *l_params;
  GimpParamDef     *l_return_vals;
  gint              l_rc;
  
  gchar             l_filename[512];
  gchar             l_gendate[30];
  gchar             l_clean_proc_name[256];
  gchar             l_clean_par_name[256];


  l_rc = 0;
  p_get_gendate(&l_gendate[0]);
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if (gimp_procedural_db_proc_info (proc_name,
				    &l_proc_blurb,
				    &l_proc_help,
				    &l_proc_author,
				    &l_proc_copyright,
				    &l_proc_date,
				    &l_proc_type,
				    &l_nparams,
				    &l_nreturn_vals,
				    &l_params,
				    &l_return_vals))
    {
     p_clean_name(proc_name, &l_clean_proc_name[0]);
     /* procedure found in PDB */
     if(gap_debug) fprintf(stderr, "DEBUG: found in PDB %s author: %s copyright: %s\n",
                            proc_name, l_proc_author, l_proc_copyright);

     /* check if plugin can be a typical one, that works on one drawable */
     if (l_proc_type != GIMP_PLUGIN)         { l_rc = -1;  }
     if (l_nparams  < 3)                      { l_rc = -1;  }
     if (l_params[0].type !=  GIMP_PDB_INT32)    { l_rc = -1;  }
     if (l_params[1].type !=  GIMP_PDB_IMAGE)    { l_rc = -1;  }
     if (l_params[2].type !=  GIMP_PDB_DRAWABLE) { l_rc = -1;  }

     sprintf(l_filename, "%s_iter_ALT.inc", l_clean_proc_name);
     l_fp = fopen(l_filename, "w");
     if(l_fp != NULL)
     {
       fprintf(l_fp, "/* ----------------------------------------------------------------------\n");
       fprintf(l_fp, " * p_%s_iter_ALT \n", l_clean_proc_name);
       fprintf(l_fp, " * ----------------------------------------------------------------------\n");
       fprintf(l_fp, " */\n");            
       fprintf(l_fp, "gint p_%s_iter_ALT(GimpRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) \n", l_clean_proc_name);
       fprintf(l_fp, "{\n");               
       fprintf(l_fp, "    typedef struct t_%s_Vals \n", l_clean_proc_name);
       fprintf(l_fp, "    {\n");       

       for(l_idx = 3; l_idx < l_nparams; l_idx++)
       {
         p_clean_name(l_params[l_idx].name, &l_clean_par_name[0]);
         
         fprintf(l_fp, "      %s %s;\n",
               p_type_to_string(l_params[l_idx].type), l_clean_par_name);
       }
       fprintf(l_fp, "    } t_%s_Vals; \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    t_%s_Vals  buf, *buf_from, *buf_to; \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(len_struct != sizeof(t_%s_Vals)) \n", l_clean_proc_name);
       fprintf(l_fp, "    {\n");             
       fprintf(l_fp, "      fprintf(stderr, \"ERROR: p_\%s_iter_ALT  stored Data missmatch in size %%d != %%d\\n\",   \n", l_clean_proc_name);
       fprintf(l_fp, "                       (int)len_struct, sizeof(t_%s_Vals) ); \n", l_clean_proc_name);
       fprintf(l_fp, "      return -1;  /* ERROR */ \n");
       fprintf(l_fp, "    }\n");               
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    gimp_get_data(\"%s_ITER_FROM\", g_plugin_data_from); \n", l_clean_proc_name);
       fprintf(l_fp, "    gimp_get_data(\"%s_ITER_TO\",   g_plugin_data_to); \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    buf_from = (t_%s_Vals *)&g_plugin_data_from[0]; \n", l_clean_proc_name);
       fprintf(l_fp, "    buf_to   = (t_%s_Vals *)&g_plugin_data_to[0]; \n", l_clean_proc_name);
       fprintf(l_fp, "    memcpy(&buf, buf_from, sizeof(buf));\n");
       fprintf(l_fp, "\n");

       for(l_idx = 3; l_idx < l_nparams; l_idx++)
       {
         p_clean_name(l_params[l_idx].name, &l_clean_par_name[0]);

         switch(l_params[l_idx].type)
         {
         case GIMP_PDB_INT32:
           fprintf(l_fp, "    p_delta_long(&buf.%s, buf_from->%s, buf_to->%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_INT16:
           fprintf(l_fp, "    p_delta_short(&buf.%s, buf_from->%s, buf_to->%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_INT8:
           fprintf(l_fp, "    p_delta_char(&buf.%s, buf_from->%s, buf_to->%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_FLOAT:
           fprintf(l_fp, "    p_delta_gdouble(&buf.%s, buf_from->%s, buf_to->%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_COLOR:
           fprintf(l_fp, "    p_delta_color(&buf.%s, &buf_from->%s, &buf_to->%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_DRAWABLE:
           fprintf(l_fp, "    p_delta_drawable(&buf.%s, buf_from->%s, buf_to->%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         default:
           break;
         }
       }
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    gimp_set_data(\"%s\", &buf, sizeof(buf)); \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    return 0; /* OK */\n");
       fprintf(l_fp, "}\n");
       
       
       fclose(l_fp);
     }


     /*  free the query information  */
     g_free (l_proc_blurb);
     g_free (l_proc_help);
     g_free (l_proc_author);
     g_free (l_proc_copyright);
     g_free (l_proc_date);
     g_free (l_params);
     g_free (l_return_vals);
  }
  else
  {
     return -1;
  }
  
  p_gen_code_iter(proc_name);

  return l_rc;
}	/* p_gen_code_iter_ALT */

gint p_gen_forward_iter_ALT(char  *proc_name)
{
  FILE           *l_fp;
  char             l_clean_proc_name[256];

  p_clean_name(proc_name, &l_clean_proc_name[0]);
  l_fp = fopen(GEN_FORWARDFILE_NAME, "a");
  if(l_fp != NULL)
  {
    fprintf(l_fp, "static gint p_%s_iter_ALT (GimpRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct);\n",
                   l_clean_proc_name);
    fclose(l_fp);
  }
  return 0;
}

gint p_gen_tab_iter_ALT(char  *proc_name)
{
  FILE           *l_fp;
  char             l_clean_proc_name[256];

  p_clean_name(proc_name, &l_clean_proc_name[0]);
  l_fp = fopen(GEN_TABFILE_NAME, "a");
  if(l_fp != NULL)
  {
    fprintf(l_fp, "   , { \"%s\",  p_%s_iter_ALT }\n",
                  l_clean_proc_name, l_clean_proc_name);
    fclose(l_fp);
  }
  return 0;
}



/* Generate _Itrerator Procedure all in one .c file,
 * ready to compile
 */
gint p_gen_code_iter(char  *proc_name)
{
  FILE             *l_fp;
  gint              l_idx;
  
  gint              l_nparams;
  gint              l_nreturn_vals;
  GimpPDBProcType   l_proc_type;
  gchar            *l_proc_blurb;
  gchar            *l_proc_help;
  gchar            *l_proc_author;
  gchar            *l_proc_copyright;
  gchar            *l_proc_date;
  GimpParamDef     *l_params;
  GimpParamDef     *l_return_vals;
  gint              l_rc;
  
  gchar             l_filename[512];
  gchar             l_gendate[30];
  gchar             l_clean_proc_name[256];
  gchar             l_clean_par_name[256];

  l_rc = 0;
  p_get_gendate(&l_gendate[0]);
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if (gimp_procedural_db_proc_info (proc_name,
				    &l_proc_blurb,
				    &l_proc_help,
				    &l_proc_author,
				    &l_proc_copyright,
				    &l_proc_date,
				    &l_proc_type,
				    &l_nparams,
				    &l_nreturn_vals,
				    &l_params,
				    &l_return_vals))
    {
     p_clean_name(proc_name, &l_clean_proc_name[0]);
     /* procedure found in PDB */
     if(gap_debug) fprintf(stderr, "DEBUG: found in PDB %s\n", proc_name);

     /* check if plugin can be a typical one, that works on one drawable */
     if (l_proc_type != GIMP_PLUGIN)         { l_rc = -1;  }
     if (l_nparams  < 3)                      { l_rc = -1;  }
     if (l_params[0].type !=  GIMP_PDB_INT32)    { l_rc = -1;  }
     if (l_params[1].type !=  GIMP_PDB_IMAGE)    { l_rc = -1;  }
     if (l_params[2].type !=  GIMP_PDB_DRAWABLE) { l_rc = -1;  }
     
     
     sprintf(l_filename, "%s_iter.c", l_clean_proc_name);

     l_fp = fopen(l_filename, "w");
     if(l_fp != NULL)
     {

       fprintf(l_fp, "/* %s\n", l_filename);
       fprintf(l_fp, " * generated by gap_filter_codegen.c\n");
       fprintf(l_fp, " * generation date:  %s\n", l_gendate);
       fprintf(l_fp, " *\n");
       fprintf(l_fp, " * generation source Gimp PDB entry name: %s\n", l_clean_proc_name);
       fprintf(l_fp, " *                            version   : %s\n", l_proc_date);
       fprintf(l_fp, " *\n");
       fprintf(l_fp, " * The generated code will not work if the internal data stucture\n");
       fprintf(l_fp, " * (used to store and retrieve \"LastValues\") is different to the\n");
       fprintf(l_fp, " * PDB Calling Interface.\n");
       fprintf(l_fp, " *\n");
       fprintf(l_fp, " * In that case you will get an Error message like that:\n");
       fprintf(l_fp, " *       ERROR: xxxx_Iterator stored Data missmatch in size N != M\n");
       fprintf(l_fp, " *    if the Iterator is called. \n");
       fprintf(l_fp, " *    (via \"Filter all Layers\" using \"Apply Varying\" Button)\n");
       fprintf(l_fp, " *\n");
       fprintf(l_fp, " *    When you get this Error, you should change this generated code.\n");
       fprintf(l_fp, " *  \n");  
       fprintf(l_fp, " */\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "/* SYTEM (UNIX) includes */ \n");
       fprintf(l_fp, "#include <stdio.h>\n");
       fprintf(l_fp, "#include <string.h>\n");
       fprintf(l_fp, "#include <stdlib.h>\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "/* GIMP includes */\n");
       fprintf(l_fp, "#include \"gtk/gtk.h\"\n");
       fprintf(l_fp, "#include \"libgimp/gimp.h\"\n");
       fprintf(l_fp, "\n");

       fprintf(l_fp, "typedef struct { guchar color[3]; } t_color; \n");
       fprintf(l_fp, "typedef struct { gint color[3]; }   t_gint_color; \n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "static void query(void); \n");
       fprintf(l_fp, "static void run(char *name, int nparam, GimpParam *param, int *nretvals, GimpParam **retvals); \n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "GimpPlugInInfo PLUG_IN_INFO = \n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "  NULL,  /* init_proc */ \n");
       fprintf(l_fp, "  NULL,  /* quit_proc */ \n");
       fprintf(l_fp, "  query, /* query_proc */ \n");
       fprintf(l_fp, "  run,   /* run_proc */ \n");
       fprintf(l_fp, "}; \n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "/* ----------------------------------------------------------------------\n");
       fprintf(l_fp, " * iterator functions for basic datatypes \n");
       fprintf(l_fp, " * ----------------------------------------------------------------------\n");
       fprintf(l_fp, " */\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "static void p_delta_long(long *val, long val_from, long val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "    *val  = val_from + delta;  \n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_short(short *val, short val_from, short val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "    *val  = val_from + delta;\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_gint(gint *val, gint val_from, gint val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "    *val  = val_from + delta;\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_char(char *val, char val_from, char val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "    *val  = val_from + delta;\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_gdouble(double *val, double val_from, double val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "    *val  = val_from + delta;\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_float(float *val, float val_from, float val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "    *val  = val_from + delta;\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_color(t_color *val, t_color *val_from, t_color *val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "    int l_idx;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    for(l_idx = 0; l_idx < 3; l_idx++)\n");
       fprintf(l_fp, "    {\n");
       fprintf(l_fp, "       delta = ((double)(val_to->color[l_idx] - val_from->color[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "       val->color[l_idx]  = val_from->color[l_idx] + delta;\n");
       fprintf(l_fp, "    }\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "static void p_delta_gint_color(t_gint_color *val, t_gint_color *val_from, t_gint_color *val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    double     delta;\n");
       fprintf(l_fp, "    int l_idx;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(total_steps < 1) return;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    for(l_idx = 0; l_idx < 3; l_idx++)\n");
       fprintf(l_fp, "    {\n");
       fprintf(l_fp, "       delta = ((double)(val_to->color[l_idx] - val_from->color[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);\n");
       fprintf(l_fp, "       val->color[l_idx]  = val_from->color[l_idx] + delta;\n");
       fprintf(l_fp, "    }\n");
       fprintf(l_fp, "}\n");

       fprintf(l_fp, "static void p_delta_drawable(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "    gint    l_nlayers;\n");
       fprintf(l_fp, "    gint32 *l_layers_list;\n");
       fprintf(l_fp, "    gint32  l_tmp_image_id;\n");
       fprintf(l_fp, "    gint    l_idx, l_idx_from, l_idx_to;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    l_tmp_image_id = gimp_drawable_image_id(val_from);\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    /* check if from and to values are both valid drawables within the same image */\n");
       fprintf(l_fp, "    if ((l_tmp_image_id > 0)\n");
       fprintf(l_fp, "    &&  (l_tmp_image_id = gimp_drawable_image_id(val_to)))\n");
       fprintf(l_fp, "    {\n");
       fprintf(l_fp, "       l_idx_from = -1;\n");
       fprintf(l_fp, "       l_idx_to   = -1;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "       /* check the layerstack index of from and to drawable */\n");
       fprintf(l_fp, "       l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);\n");
       fprintf(l_fp, "       for (l_idx = l_nlayers -1; l_idx >= 0; l_idx--)\n");
       fprintf(l_fp, "       {\n");
       fprintf(l_fp, "          if( l_layers_list[l_idx] == val_from ) l_idx_from = l_idx;\n");
       fprintf(l_fp, "          if( l_layers_list[l_idx] == val_to )   l_idx_to   = l_idx;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "          if((l_idx_from != -1) && (l_idx_to != -1))\n");
       fprintf(l_fp, "          {\n");
       fprintf(l_fp, "            /* OK found both index values, iterate the index (proceed to next layer) */\n");
       fprintf(l_fp, "            p_delta_gint(&l_idx, l_idx_from, l_idx_to, total_steps, current_step);\n");
       fprintf(l_fp, "            *val = l_layers_list[l_idx];\n");
       fprintf(l_fp, "            break;\n");
       fprintf(l_fp, "          }\n");
       fprintf(l_fp, "       }\n");
       fprintf(l_fp, "       g_free (l_layers_list);\n");
       fprintf(l_fp, "    }\n");
       fprintf(l_fp, "}\n");


       fprintf(l_fp, "\n");
       fprintf(l_fp, "/* ----------------------------------------------------------------------\n");
       fprintf(l_fp, " * p_%s_iter \n", l_clean_proc_name);
       fprintf(l_fp, " * ----------------------------------------------------------------------\n");
       fprintf(l_fp, " */\n");            
       fprintf(l_fp, "gint p_%s_iter(GimpRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) \n", l_clean_proc_name);
       fprintf(l_fp, "{\n");               
       fprintf(l_fp, "    typedef struct t_%s_Vals \n", l_clean_proc_name);
       fprintf(l_fp, "    {\n");       

       for(l_idx = 3; l_idx < l_nparams; l_idx++)
       {
         p_clean_name(l_params[l_idx].name, &l_clean_par_name[0]);

         fprintf(l_fp, "      %s %s;\n",
               p_type_to_string(l_params[l_idx].type), l_clean_par_name);
       }
       fprintf(l_fp, "    } t_%s_Vals; \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    t_%s_Vals  buf, buf_from, buf_to; \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    if(len_struct != sizeof(t_%s_Vals)) \n", l_clean_proc_name);
       fprintf(l_fp, "    {\n");             
       fprintf(l_fp, "      fprintf(stderr, \"ERROR: p_\%s_iter  stored Data missmatch in size %%d != %%d\\n\",   \n", l_clean_proc_name);
       fprintf(l_fp, "                       (int)len_struct, sizeof(t_%s_Vals) ); \n", l_clean_proc_name);
       fprintf(l_fp, "      return -1;  /* ERROR */ \n");
       fprintf(l_fp, "    }\n");               
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    gimp_get_data(\"%s_ITER_FROM\", &buf_from); \n", l_clean_proc_name);
       fprintf(l_fp, "    gimp_get_data(\"%s_ITER_TO\",   &buf_to); \n", l_clean_proc_name);
       fprintf(l_fp, "    memcpy(&buf, &buf_from, sizeof(buf));\n");
       fprintf(l_fp, "\n");

       for(l_idx = 3; l_idx < l_nparams; l_idx++)
       {
         p_clean_name(l_params[l_idx].name, &l_clean_par_name[0]);

         switch(l_params[l_idx].type)
         {
         case GIMP_PDB_INT32:
           fprintf(l_fp, "    p_delta_long(&buf.%s, buf_from.%s, buf_to.%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_INT16:
           fprintf(l_fp, "    p_delta_short(&buf.%s, buf_from.%s, buf_to.%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_INT8:
           fprintf(l_fp, "    p_delta_char(&buf.%s, buf_from.%s, buf_to.%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_FLOAT:
           fprintf(l_fp, "    p_delta_gdouble(&buf.%s, buf_from.%s, buf_to.%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_COLOR:
           fprintf(l_fp, "    p_delta_color(&buf.%s, &buf_from.%s, &buf_to.%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         case GIMP_PDB_DRAWABLE:
           fprintf(l_fp, "    p_delta_drawable(&buf.%s, buf_from.%s, buf_to.%s, total_steps, current_step);\n",
                   l_clean_par_name, l_clean_par_name, l_clean_par_name);
           break;
         default:
           break;
         }
       }
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    gimp_set_data(\"%s\", &buf, sizeof(buf)); \n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "    return 0; /* OK */\n");
       fprintf(l_fp, "}\n");

       fprintf(l_fp, "MAIN ()\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "/* ----------------------------------------------------------------------\n");
       fprintf(l_fp, " * install (query) _Iterator\n");
       fprintf(l_fp, " * ----------------------------------------------------------------------\n");
       fprintf(l_fp, " */\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "static void query ()\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "  char l_blurb_text[300];\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  static GimpParamDef args_iter[] =\n");
       fprintf(l_fp, "  {\n");
       fprintf(l_fp, "    {GIMP_PDB_INT32, \"run_mode\", \"non-interactive\"},\n");
       fprintf(l_fp, "    {GIMP_PDB_INT32, \"total_steps\", \"total number of steps (# of layers-1 to apply the related plug-in)\"},\n");
       fprintf(l_fp, "    {GIMP_PDB_FLOAT, \"current_step\", \"current (for linear iterations this is the layerstack position, otherwise some value inbetween)\"},\n");
       fprintf(l_fp, "    {GIMP_PDB_INT32, \"len_struct\", \"length of stored data structure with id is equal to the plug_in  proc_name\"},\n");
       fprintf(l_fp, "  };\n");
       fprintf(l_fp, "  static int nargs_iter = sizeof(args_iter) / sizeof(args_iter[0]);\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  static GimpParamDef *return_vals = NULL;\n");
       fprintf(l_fp, "  static int nreturn_vals = 0;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  sprintf(l_blurb_text, \"This extension calculates the modified values for one iterationstep for the call of %s\");\n", l_clean_proc_name);
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  gimp_install_procedure(\"%s_Iterator\",\n", l_clean_proc_name);
       fprintf(l_fp, "                         l_blurb_text,\n");
       fprintf(l_fp, "                         \"\",\n");
       fprintf(l_fp, "                         \"Wolfgang Hofer\",\n");
       fprintf(l_fp, "                         \"Wolfgang Hofer\",\n");
       fprintf(l_fp, "                         \"%s\",\n", l_gendate);                   /* generation date */
       fprintf(l_fp, "                         NULL,    /* do not appear in menus */\n");
       fprintf(l_fp, "                         NULL,\n");
       fprintf(l_fp, "                         GIMP_EXTENSION,\n");
       fprintf(l_fp, "                         nargs_iter, nreturn_vals,\n");
       fprintf(l_fp, "                         args_iter, return_vals);\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "}\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "/* ----------------------------------------------------------------------\n");
       fprintf(l_fp, " * run Iterator\n");
       fprintf(l_fp, " * ----------------------------------------------------------------------\n");
       fprintf(l_fp, " */\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "static void\n");
       fprintf(l_fp, "run (char    *name,\n");
       fprintf(l_fp, "     int      n_params,\n");
       fprintf(l_fp, "     GimpParam  *param,\n");
       fprintf(l_fp, "     int     *nreturn_vals,\n");
       fprintf(l_fp, "     GimpParam **return_vals)\n");
       fprintf(l_fp, "{\n");
       fprintf(l_fp, "  static GimpParam values[1];\n");
       fprintf(l_fp, "  GimpRunModeType run_mode;\n");
       fprintf(l_fp, "  GimpPDBStatusType status = GIMP_PDB_SUCCESS;\n");
       fprintf(l_fp, "  gint32     image_id;\n");
       fprintf(l_fp, "  gint32  len_struct;\n");
       fprintf(l_fp, "  gint32  total_steps;\n");
       fprintf(l_fp, "  gdouble current_step;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  gint32     l_rc;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  *nreturn_vals = 1;\n");
       fprintf(l_fp, "  *return_vals = values;\n");
       fprintf(l_fp, "  l_rc = 0;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  run_mode = param[0].data.d_int32;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  if ((run_mode == GIMP_RUN_NONINTERACTIVE) && (n_params == 4))\n");
       fprintf(l_fp, "  {\n");
       fprintf(l_fp, "    total_steps  =  param[1].data.d_int32;\n");
       fprintf(l_fp, "    current_step =  param[2].data.d_float;\n");
       fprintf(l_fp, "    len_struct   =  param[3].data.d_int32;\n");
       fprintf(l_fp, "    l_rc = p_%s_iter(run_mode, total_steps, current_step, len_struct);\n", l_clean_proc_name);
       fprintf(l_fp, "    if(l_rc < 0)\n");
       fprintf(l_fp, "    {\n");
       fprintf(l_fp, "       status = GIMP_PDB_EXECUTION_ERROR;\n");
       fprintf(l_fp, "    }\n");
       fprintf(l_fp, "  }\n");
       fprintf(l_fp, "  else status = GIMP_PDB_CALLING_ERROR;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "  values[0].type = GIMP_PDB_STATUS;\n");
       fprintf(l_fp, "  values[0].data.d_status = status;\n");
       fprintf(l_fp, "\n");
       fprintf(l_fp, "}\n");


       
       
       fclose(l_fp);
     }


     /*  free the query information  */
     g_free (l_proc_blurb);
     g_free (l_proc_help);
     g_free (l_proc_author);
     g_free (l_proc_copyright);
     g_free (l_proc_date);
     g_free (l_params);
     g_free (l_return_vals);
  }
  else
  {
     return -1;
  }
  

  return l_rc;
}	/* p_gen_code_iter */
