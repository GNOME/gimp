/* ----------------------------------------------------------------------
 * p_plug_in_bump_map_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_bump_map_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_bump_map_Vals 
    {
      gint32    bumpmap;
      gdouble   azimuth;
      gdouble   elevation;
      long      depth;
      long      xofs;
      long      yofs;
      long      waterlevel;
      long      ambient;
      long      compensate;
      long      invert;
      long      type;
      long      tiled;
    } t_plug_in_bump_map_Vals; 

    t_plug_in_bump_map_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_bump_map_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_bump_map_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_bump_map_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_bump_map_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_bump_map_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_bump_map_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_bump_map_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_drawable(&buf.bumpmap, buf_from->bumpmap, buf_to->bumpmap, total_steps, current_step);
    p_delta_gdouble(&buf.azimuth, buf_from->azimuth, buf_to->azimuth, total_steps, current_step);
    p_delta_gdouble(&buf.elevation, buf_from->elevation, buf_to->elevation, total_steps, current_step);
    p_delta_long(&buf.depth, buf_from->depth, buf_to->depth, total_steps, current_step);
    p_delta_long(&buf.xofs, buf_from->xofs, buf_to->xofs, total_steps, current_step);
    p_delta_long(&buf.yofs, buf_from->yofs, buf_to->yofs, total_steps, current_step);
    p_delta_long(&buf.waterlevel, buf_from->waterlevel, buf_to->waterlevel, total_steps, current_step);
    p_delta_long(&buf.ambient, buf_from->ambient, buf_to->ambient, total_steps, current_step);
    p_delta_long(&buf.compensate, buf_from->compensate, buf_to->compensate, total_steps, current_step);
    p_delta_long(&buf.invert, buf_from->invert, buf_to->invert, total_steps, current_step);
    p_delta_long(&buf.type, buf_from->type, buf_to->type, total_steps, current_step);

    gimp_set_data("plug_in_bump_map", &buf, sizeof(buf)); 

    return 0; /* OK */
}
