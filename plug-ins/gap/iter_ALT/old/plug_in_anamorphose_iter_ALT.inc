/* ----------------------------------------------------------------------
 * p_plug_in_anamorphose_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_anamorphose_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_anamorphose_Vals 
    {
      gdouble   cone_radius;
      gdouble   base_angle;
      long      keep_surroundings;
      long      set_background;
      long      set_transparent;
      long      use_antialias;
      long      flip;
      long      ndouble;
    } t_plug_in_anamorphose_Vals; 

    t_plug_in_anamorphose_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_anamorphose_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_anamorphose_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_anamorphose_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_anamorphose_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_anamorphose_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_anamorphose_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_anamorphose_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.cone_radius, buf_from->cone_radius, buf_to->cone_radius, total_steps, current_step);
    p_delta_gdouble(&buf.base_angle, buf_from->base_angle, buf_to->base_angle, total_steps, current_step);
    p_delta_long(&buf.keep_surroundings, buf_from->keep_surroundings, buf_to->keep_surroundings, total_steps, current_step);
    p_delta_long(&buf.set_background, buf_from->set_background, buf_to->set_background, total_steps, current_step);
    p_delta_long(&buf.set_transparent, buf_from->set_transparent, buf_to->set_transparent, total_steps, current_step);
    p_delta_long(&buf.use_antialias, buf_from->use_antialias, buf_to->use_antialias, total_steps, current_step);
    p_delta_long(&buf.flip, buf_from->flip, buf_to->flip, total_steps, current_step);
    p_delta_long(&buf.ndouble, buf_from->ndouble, buf_to->ndouble, total_steps, current_step);

    gimp_set_data("plug_in_anamorphose", &buf, sizeof(buf)); 

    return 0; /* OK */
}
