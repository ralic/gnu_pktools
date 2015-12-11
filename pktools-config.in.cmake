#!/bin/sh

usage()
{
    cat <<EOF
Usage: pktools-config [OPTIONS]
Options:
     [--prefix]
     [--version]
     [--libs]
     [--cflags]
     [--ldflags]
     [--includes]
EOF
    exit $1
}
if test $# -eq 0; then
  usage 1 1>&2
fi
while test $# -gt 0; do
case "$1" in
    -*=*) optarg=`echo "$1" | sed 's/[-_a-zA-Z0-9]*=//'` ;;
    *) optarg= ;;
esac
case $1 in
    --prefix)
      echo @CMAKE_INSTALL_PREFIX@
     ;;
    --version)
      echo @PKTOOLS_PACKAGE_VERSION@
     ;;
    --cflags)
      echo -I@CMAKE_INSTALL_PREFIX@/include 
      ;;
    --libs)
      echo -L@CMAKE_INSTALL_PREFIX@/@PROJECT_LIBRARY_DIR@ -l@PKTOOLS_ALGORITHMS_LIB_NAME@  -l@PKTOOLS_FILECLASSES_LIB_NAME@ -l@PKTOOLS_IMAGECLASSES_LIB_NAME@ 
      ;;
    --ldflags)
      echo @CMAKE_INSTALL_PREFIX@/@PROJECT_LIBRARY_DIR@
      ;;
    --includes)
      echo @CMAKE_INSTALL_PREFIX@/include
      ;;
  esac
  shift
done

