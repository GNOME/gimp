/* ----------------------------------------------------------------------
 * p_plug_in_CentralReflection_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_CentralReflection_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_CentralReflection_Vals 
    {
      gdouble   circle_radius;
      gint      keep_surroundings;
      gint      set_background;
      gint      set_transparent;
      gint      use_antialias;
      gint      flip;
    } t_plug_in_CentralReflection_Vals; 

    t_plug_in_CentralReflection_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_CentralReflection_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_CentralReflection_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_CentralReflection_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_CentralReflection_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_CentralReflection_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_CentralReflection_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_CentralReflection_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.circle_radius, buf_from->circle_radius, buf_to->circle_radius, total_steps, current_step);
    p_delta_gint(&buf.keep_surroundings, buf_from->keep_surroundings, buf_to->keep_surroundings, total_steps, current_step);
    p_delta_gint(&buf.set_background, buf_from->set_background, buf_to->set_background, total_steps, current_step);
    p_delta_gint(&buf.set_transparent, buf_from->set_transparent, buf_to->set_transparent, total_steps, current_step);
    p_delta_gint(&buf.use_antialias, buf_from->use_antialias, buf_to->use_antialias, total_steps, current_step);
    p_delta_gint(&buf.flip, buf_from->flip, buf_to->flip, total_steps, current_step);

    gimp_set_data("plug_in_CentralReflection", &buf, sizeof(buf)); 

    return 0; /* OK */
}
