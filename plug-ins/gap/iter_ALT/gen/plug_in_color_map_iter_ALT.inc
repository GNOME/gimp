/* ----------------------------------------------------------------------
 * p_plug_in_color_map_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_color_map_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_color_map_Vals 
    {
      t_color   srccolor_1;
      t_color   srccolor_2;
      t_color   dstcolor_1;
      t_color   dstcolor_2;
      long      map_mode;
    } t_plug_in_color_map_Vals; 

    t_plug_in_color_map_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_color_map_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_color_map_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_color_map_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_color_map_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_color_map_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_color_map_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_color_map_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_color(&buf.srccolor_1, &buf_from->srccolor_1, &buf_to->srccolor_1, total_steps, current_step);
    p_delta_color(&buf.srccolor_2, &buf_from->srccolor_2, &buf_to->srccolor_2, total_steps, current_step);
    p_delta_color(&buf.dstcolor_1, &buf_from->dstcolor_1, &buf_to->dstcolor_1, total_steps, current_step);
    p_delta_color(&buf.dstcolor_2, &buf_from->dstcolor_2, &buf_to->dstcolor_2, total_steps, current_step);
    p_delta_long(&buf.map_mode, buf_from->map_mode, buf_to->map_mode, total_steps, current_step);

    gimp_set_data("plug_in_color_map", &buf, sizeof(buf)); 

    return 0; /* OK */
}
