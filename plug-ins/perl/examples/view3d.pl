#!/usr/bin/perl

use Gimp::Feature qw(pdl);
use PDL;
BEGIN { eval "use PDL::Graphics::TriD"; $@ and Gimp::Feature::missing('PDL TriD (OpenGL) support') }
use Gimp;
use Gimp::Fu;

register
    'view3d',
    'View grayscale drawable in 3D',
    'This script uses PDL::Graphics:TriD to view a grayscale drawable in 3D. You can choose a Cartesian (default) or Polar projection, toggle the drawing of lines, and toggle normal smoothing.',
    'Tom Rathborne', 'GPLv2', '1999-03-11',
    '<Image>/View/3D Surface',
    'RGB*,GRAY*', [
        [ PF_BOOL, 'polar', 'Radial view', 0],
        [ PF_BOOL, 'lines', 'Draw grid lines', 0],
        [ PF_BOOL, 'smooth', 'Smooth surface normals', 1]
    ], [],
sub {
    my ($img, $dwb, $polar, $lines, $smooth) = @_;

    my $w = $dwb->width;
    my $h = $dwb->height;

    my $regn = $dwb->pixel_rgn (0, 0, $w, $h, 0, 0);
    my $surf = $regn->get_rect (0, 0, $w, $h);
    $surf=$surf->slice("(0)") if $surf->getndims>2;

    imag3d [ $polar ? 'POLAR2D' : 'SURF2D', $surf ],
           { 'Lines' => $lines, 'Smooth' => $smooth };

    ();
};

exit main;

