# gimpbuilder-base

FROM debian:testing

ENV PREFIX=/export/output
ENV PATH=$PREFIX/bin:$PATH
ENV PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH
ENV APT_GET_OPTIONS="-o APT::Install-Recommends=0 -y"

RUN apt-get update

# Installing the build environment
RUN apt-get install $APT_GET_OPTIONS build-essential devscripts fakeroot quilt dh-make automake libdistro-info-perl less nano

# Installing additional dependencies for babl
# none needed

# Installing additional dependencies for GEGL
RUN apt-get install $APT_GET_OPTIONS intltool libglib2.0-dev libjson-c-dev libjson-glib-dev libgexiv2-dev libcairo2-dev libpango1.0-dev libjpeg62-turbo-dev libsuitesparse-dev libspiro-dev libopenexr-dev libwebp-dev

# Installing additional dependencies for GIMP 
RUN apt-get install $APT_GET_OPTIONS xsltproc gtk-doc-tools libgtk2.0-dev libtiff5-dev libbz2-dev liblzma-dev librsvg2-dev liblcms2-dev python-cairo-dev python-gtk2-dev glib-networking libaa1-dev libgs-dev libpoppler-glib-dev libmng-dev libwmf-dev libxpm-dev libasound2-dev