/* ----------------------------------------------------------------------
 * p_plug_in_fractal_trace_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_fractal_trace_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_fractal_trace_Vals 
    {
      gdouble   xmin;
      gdouble   xmax;
      gdouble   ymin;
      gdouble   ymax;
      long      depth;
      long      outside_type;
    } t_plug_in_fractal_trace_Vals; 

    t_plug_in_fractal_trace_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_fractal_trace_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_fractal_trace_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_fractal_trace_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_fractal_trace_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_fractal_trace_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_fractal_trace_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_fractal_trace_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.xmin, buf_from->xmin, buf_to->xmin, total_steps, current_step);
    p_delta_gdouble(&buf.xmax, buf_from->xmax, buf_to->xmax, total_steps, current_step);
    p_delta_gdouble(&buf.ymin, buf_from->ymin, buf_to->ymin, total_steps, current_step);
    p_delta_gdouble(&buf.ymax, buf_from->ymax, buf_to->ymax, total_steps, current_step);
    p_delta_long(&buf.depth, buf_from->depth, buf_to->depth, total_steps, current_step);
    p_delta_long(&buf.outside_type, buf_from->outside_type, buf_to->outside_type, total_steps, current_step);

    gimp_set_data("plug_in_fractal_trace", &buf, sizeof(buf)); 

    return 0; /* OK */
}
