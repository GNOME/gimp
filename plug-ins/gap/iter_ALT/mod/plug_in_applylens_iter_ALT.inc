/* ----------------------------------------------------------------------
 * p_plug_in_applylens_iter_ALT 
 * ----------------------------------------------------------------------
 */
static gint p_plug_in_applylens_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_applylens_Vals 
    {
      gdouble   refraction;
      long      keep_surroundings;
      long      set_background;
      long      set_transparent;
    } t_plug_in_applylens_Vals; 

    t_plug_in_applylens_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_applylens_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_applylens_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_applylens_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_applylens_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_applylens_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_applylens_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_applylens_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.refraction, buf_from->refraction, buf_to->refraction, total_steps, current_step);
    p_delta_long(&buf.keep_surroundings, buf_from->keep_surroundings, buf_to->keep_surroundings, total_steps, current_step);
    p_delta_long(&buf.set_background, buf_from->set_background, buf_to->set_background, total_steps, current_step);
    p_delta_long(&buf.set_transparent, buf_from->set_transparent, buf_to->set_transparent, total_steps, current_step);

    gimp_set_data("plug_in_applylens", &buf, sizeof(buf)); 

    return 0; /* OK */
}
