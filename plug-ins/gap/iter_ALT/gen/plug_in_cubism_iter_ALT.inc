/* ----------------------------------------------------------------------
 * p_plug_in_cubism_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_cubism_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_cubism_Vals 
    {
      gdouble   tile_size;
      gdouble   tile_saturation;
      long      bg_color;
    } t_plug_in_cubism_Vals; 

    t_plug_in_cubism_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_cubism_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_cubism_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_cubism_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_cubism_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_cubism_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_cubism_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_cubism_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.tile_size, buf_from->tile_size, buf_to->tile_size, total_steps, current_step);
    p_delta_gdouble(&buf.tile_saturation, buf_from->tile_saturation, buf_to->tile_saturation, total_steps, current_step);
    p_delta_long(&buf.bg_color, buf_from->bg_color, buf_to->bg_color, total_steps, current_step);

    gimp_set_data("plug_in_cubism", &buf, sizeof(buf)); 

    return 0; /* OK */
}
