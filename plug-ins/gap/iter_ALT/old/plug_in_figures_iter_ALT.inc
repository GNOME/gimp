/* ----------------------------------------------------------------------
 * p_plug_in_figures_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_figures_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_figures_Vals 
    {
      gint      min_width;
      gint      max_width;
      gint      min_height;
      gint      max_height;
      float     density;
    } t_plug_in_figures_Vals; 

    t_plug_in_figures_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_figures_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_figures_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_figures_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_figures_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_figures_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_figures_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_figures_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_float(&buf.density, buf_from->density, buf_to->density, total_steps, current_step);
    p_delta_gint(&buf.min_width, buf_from->min_width, buf_to->min_width, total_steps, current_step);
    p_delta_gint(&buf.max_width, buf_from->max_width, buf_to->max_width, total_steps, current_step);
    p_delta_gint(&buf.min_height, buf_from->min_height, buf_to->min_height, total_steps, current_step);
    p_delta_gint(&buf.max_height, buf_from->max_height, buf_to->max_height, total_steps, current_step);

    gimp_set_data("plug_in_figures", &buf, sizeof(buf)); 

    return 0; /* OK */
}
