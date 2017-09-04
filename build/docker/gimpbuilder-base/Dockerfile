# gimpbuilder-base

FROM debian:testing

ENV PREFIX=/export/output
ENV PATH=$PREFIX/bin:$PATH
ENV PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH

RUN apt-get update

# Installing the build environment
RUN apt-get install -y build-essential devscripts fakeroot quilt dh-make automake libdistro-info-perl less nano intltool libglib2.0-dev libjson-c-dev libjson-glib-dev libgexiv2-dev libcairo2-dev libpango1.0-dev libjpeg62-turbo-dev xsltproc gtk-doc-tools libgtk2.0-dev libtiff5-dev libbz2-dev liblzma-dev librsvg2-dev liblcms2-dev python-cairo-dev python-gtk2-dev
