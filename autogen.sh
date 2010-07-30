#!/bin/sh
# Run this to generate all the initial makefiles, etc.

#srcdir=`dirname $0`
srcdir=src
test -z "$srcdir" && srcdir=.

PKG_NAME="JSBSim"
ACLOCAL_FLAGS="-I . $ACLOCAL_FLAGS"

OSTYPE=`uname -s`
NOCONFIGURE=

if test "$1" = "--no-configure"; then
   echo "Omit the configuration step"
   NOCONFIGURE="true"
fi

if test "$OSTYPE" = "IRIX" -o "$OSTYPE" = "IRIX64"; then
   am_opt="--include-deps";
fi

(test -f $srcdir/JSBSim.cpp \
  && test -f $srcdir/models/flight_control/FGDeadBand.cpp) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile $PKG_NAME."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

LIBTOOL=`(which glibtool || which libtool || echo "libtool") 2> /dev/null`
(grep "^AC_PROG_LIBTOOL" configure.in >/dev/null) && {
  ($LIBTOOL --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile $PKG_NAME."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.2d.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile $PKG_NAME."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test "x$NOCONFIGURE" = "x" -a -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find . -name configure.in -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    macrodirs=`sed -n -e 's,AM_ACLOCAL_INCLUDE(\(.*\)),\1,gp' < $coin`
    ( cd $dr
      echo "deletefiles is $DELETEFILES"
      aclocalinclude="$ACLOCAL_FLAGS"

      if grep "^AC_PROG_LIBTOOL" configure.in >/dev/null; then
	if test -z "$NO_LIBTOOLIZE" ; then 
	  LIBTOOLIZE=`(which glibtoolize || which libtoolize || echo "libtoolize") 2> /dev/null`
	  echo "Running libtoolize..."
	  $LIBTOOLIZE --force --copy
	fi
      fi
      echo "Running aclocal $aclocalinclude ..."
      aclocal $aclocalinclude || {
	echo
	echo "**Error**: aclocal failed. This may mean that you have not"
	echo "installed all of the packages you need, or you may need to"
	echo "set ACLOCAL_FLAGS to include \"-I \$prefix/share/aclocal\""
	echo "for the prefix where you installed the packages whose"
	echo "macros were not found"
	exit 1
      }

      if grep "^AM_CONFIG_HEADER" configure.in >/dev/null; then
	echo "Running autoheader..."
	autoheader || { echo "**Error**: autoheader failed."; exit 1; }
      fi
      echo "Running automake --add-missing --copy $am_opt ..."
      automake --add-missing --copy $am_opt ||
	{ echo "**Error**: automake failed."; exit 1; }
      echo "Running autoconf ..."
      autoconf || { echo "**Error**: autoconf failed."; exit 1; }

      if test "$OSTYPE" = "IRIX" -o "$OSTYPE" = "IRIX64"; then
        echo "Fixing Makefiles for Irix ..."
        for n in `find . -name Makefile.in`; do \
          mv -f $n $n.ar-new; \
          sed 's/$(AR) cru /$(AR) -o /g' $n.ar-new > $n; \
          rm -f $n.ar-new; \
        done;
      fi
    ) || exit 1
  fi
done

conf_flags="--enable-maintainer-mode --enable-compile-warnings" #--enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running configure $conf_flags "$@" ...
  ./configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi

