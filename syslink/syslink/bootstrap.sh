#! /bin/sh

cd `dirname $0`

# on some platforms, you have "g" versions of some of these tools instead,
# ie glibtoolize instead of libtoolize..
find_tool() {
	which $1 2> /dev/null || which g$1 2> /dev/null
}

aclocal=`find_tool aclocal`
libtoolize=`find_tool libtoolize`
autoheader=`find_tool autoheader`
automake=`find_tool automake`
autoconf=`find_tool autoconf`

mkdir -p config && $aclocal && $libtoolize --copy --force && $autoheader && $automake --copy --add-missing --foreign && $autoconf

