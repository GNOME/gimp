/* ----------------------------------------------------------------------
 * p_plug_in_polar_coords_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_polar_coords_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_polar_coords_Vals 
    {
      gdouble   circle;
      gdouble   angle;
      gint      backwards;
      gint      inverse;
      gint      polrec;
    } t_plug_in_polar_coords_Vals; 

    t_plug_in_polar_coords_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_polar_coords_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_polar_coords_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_polar_coords_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_polar_coords_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_polar_coords_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_polar_coords_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_polar_coords_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.circle, buf_from->circle, buf_to->circle, total_steps, current_step);
    p_delta_gdouble(&buf.angle, buf_from->angle, buf_to->angle, total_steps, current_step);
    p_delta_gint(&buf.backwards, buf_from->backwards, buf_to->backwards, total_steps, current_step);
    p_delta_gint(&buf.inverse, buf_from->inverse, buf_to->inverse, total_steps, current_step);
    p_delta_gint(&buf.polrec, buf_from->polrec, buf_to->polrec, total_steps, current_step);

    gimp_set_data("plug_in_polar_coords", &buf, sizeof(buf)); 

    return 0; /* OK */
}
