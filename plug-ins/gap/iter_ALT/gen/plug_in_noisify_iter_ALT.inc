/* ----------------------------------------------------------------------
 * p_plug_in_noisify_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_noisify_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_noisify_Vals 
    {
      long      independent;
      gdouble   noise_1;
      gdouble   noise_2;
      gdouble   noise_3;
      gdouble   noise_4;
    } t_plug_in_noisify_Vals; 

    t_plug_in_noisify_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_noisify_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_noisify_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_noisify_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_noisify_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_noisify_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_noisify_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_noisify_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.independent, buf_from->independent, buf_to->independent, total_steps, current_step);
    p_delta_gdouble(&buf.noise_1, buf_from->noise_1, buf_to->noise_1, total_steps, current_step);
    p_delta_gdouble(&buf.noise_2, buf_from->noise_2, buf_to->noise_2, total_steps, current_step);
    p_delta_gdouble(&buf.noise_3, buf_from->noise_3, buf_to->noise_3, total_steps, current_step);
    p_delta_gdouble(&buf.noise_4, buf_from->noise_4, buf_to->noise_4, total_steps, current_step);

    gimp_set_data("plug_in_noisify", &buf, sizeof(buf)); 

    return 0; /* OK */
}
