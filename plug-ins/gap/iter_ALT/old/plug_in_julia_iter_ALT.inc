/* ----------------------------------------------------------------------
 * p_plug_in_julia_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_julia_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_julia_Vals 
    {
      gdouble   xmin;
      gdouble   xmax;
      gdouble   ymin;
      gdouble   ymax;
      gdouble   iter;
      gdouble   cx;
      gdouble   cy;
      gint      colormode;
      gdouble   redstretch;
      gdouble   greenstretch;
      gdouble   bluestretch;
      gint      redmode;
      gint      greenmode;
      gint      bluemode;
    } t_plug_in_julia_Vals; 

    t_plug_in_julia_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_julia_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_julia_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_julia_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_julia_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_julia_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_julia_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_julia_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.xmin, buf_from->xmin, buf_to->xmin, total_steps, current_step);
    p_delta_gdouble(&buf.xmax, buf_from->xmax, buf_to->xmax, total_steps, current_step);
    p_delta_gdouble(&buf.ymin, buf_from->ymin, buf_to->ymin, total_steps, current_step);
    p_delta_gdouble(&buf.ymax, buf_from->ymax, buf_to->ymax, total_steps, current_step);
    p_delta_gdouble(&buf.iter, buf_from->iter, buf_to->iter, total_steps, current_step);
    p_delta_gdouble(&buf.cx, buf_from->cx, buf_to->cx, total_steps, current_step);
    p_delta_gdouble(&buf.cy, buf_from->cy, buf_to->cy, total_steps, current_step);
    p_delta_gint(&buf.colormode, buf_from->colormode, buf_to->colormode, total_steps, current_step);
    p_delta_gdouble(&buf.redstretch, buf_from->redstretch, buf_to->redstretch, total_steps, current_step);
    p_delta_gdouble(&buf.greenstretch, buf_from->greenstretch, buf_to->greenstretch, total_steps, current_step);
    p_delta_gdouble(&buf.bluestretch, buf_from->bluestretch, buf_to->bluestretch, total_steps, current_step);
    p_delta_gint(&buf.redmode, buf_from->redmode, buf_to->redmode, total_steps, current_step);
    p_delta_gint(&buf.greenmode, buf_from->greenmode, buf_to->greenmode, total_steps, current_step);
    p_delta_gint(&buf.bluemode, buf_from->bluemode, buf_to->bluemode, total_steps, current_step);

    gimp_set_data("plug_in_julia", &buf, sizeof(buf)); 

    return 0; /* OK */
}
