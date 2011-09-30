#!/bin/bash
# For any further help please refer the README file.

DIR=`dirname $0`
HOST=arm-none-linux-gnueabi
FILE="Paths.txt"
arg=$1

#If Paths.txt file exits
if [ -f $FILE ]; then
	echo ""
	echo "Current Path Values are: "
	echo "========================="
	exec 3<&0
	exec 0<"$FILE"
	while read -r line
	do
		# skip the comment line from Paths.txt file
		case $line in
		\;*)
			continue;;
		esac

		i=$((line_cnt++))
		# process other lines
		case $line_cnt in
		1)
			export PREFIX=$line
			echo PREFIX            is ${PREFIX};;
		2)
			export TILER_USERSPACE=$line
			echo TILER_USERSPACE   is ${TILER_USERSPACE};;

		3)
			export KRNLSRC=$line
			echo KRNLSRC	       is ${KRNLSRC};;
		4)
			export TOOLBIN=$line
			echo TOOLBIN           is ${TOOLBIN};;
		*)
			continue;;
		esac
	done
	exec 0<&3
	echo "	"
	echo "Edit Paths.txt if required. Enter y to build, n to Exit."
	read Options
	if [[ $Options != y ]]; then
	exit 0
	fi
else
	# Gather requirements
	# path to target filesystem
	echo "Enter PREFIX (currently '$PREFIX'):\c"
	read VALUE
	export PREFIX=${VALUE:=$PREFIX}

	# path to tiler-userspace git root
	echo "Enter path to tiler-userspace (currently '$TILER_USERSPACE'):\c"
	read VALUE
	export TILER_USERSPACE=${VALUE:=$TILER_USERSPACE}

	# path to kernel-syslink git root
	echo "Enter path to kernel-syslink (currently '$KRNLSRC'):\c"
	read VALUE
	export KRNLSRC=${VALUE:=$KRNLSRC}

	echo "Enter tool path (currently '$TOOL'):\c"
	read VALUE
	TOOLBIN=${VALUE:=$TOOLBIN}

	#Creating the Paths.txt file for further inputs.
	cat > $PREFIX/Paths.txt <<EOF
	;Path of userspace-syslink
	${PREFIX}
	;Path of tiler-userspace
	${TILER_USERSPACE}
	;Path of kernel-syslink
	${KRNLSRC}
	;Path of toolchain
	${TOOLBIN}
EOF
	echo TOOLBIN           is ${TOOLBIN}
	echo PREFIX            is ${PREFIX}
	echo TILER_USERSPACE   is ${TILER_USERSPACE}
	echo KRNLSRC	       is ${KRNLSRC}
fi
	# path to userspace-space git root
	export USERSPACE_SYSLINK=`readlink -f $DIR`

export PATH=${TOOLBIN}:$PATH
export PKG_CONFIG_PATH=$PREFIX/target/lib/pkgconfig

#.. find libgcc
TOOL=`which ${HOST}-gcc`
TOOLDIR=`dirname $TOOL`
LIBGCC=$TOOLDIR/../lib/gcc/$HOST/*/libgcc.a
if [ ! -e $LIBGCC ]
then
	echo "Could not find libgcc.a"
exit
fi
echo Found libgcc.a in $LIBGCC
LIBRT=$TOOLDIR/../$HOST/libc/lib/librt*.so
if [ ! -e $LIBRT ]
then
	echo "Could not find librt.so"
exit
fi
echo Found librt.so in $LIBRT
LIBPTHREAD=$TOOLDIR/../$HOST/libc/lib/libpthread*.so
if [ ! -e $LIBPTHREAD ]
then
	echo "Could not find libpthread.so"
exit
fi
echo Found libpthread.so in $LIBPTHREAD

#... Uncomment below if you want to enable DEBUG option.
# ENABLE_DEBUG=--enable-debug

#.. uncomment to include our unit tests as well
ENABLE_UNIT_TESTS=--enable-unit-tests
ENABLE_TESTS=--enable-tests

#.. uncomment to export the tilermgr.h header - this is currently needed by
#   syslink
ENABLE_TILERMGR=--enable-tilermgr

function build_syslink()
{
	# Building memmgr
	echo "							   "
	echo "*****************************************************"
	echo "        Building tiler memmgr APIs and Samples	   "
	echo "*****************************************************"
	echo "							   "
	cd ${TILER_USERSPACE}
	./bootstrap.sh
	./configure --prefix ${PREFIX}/target --bindir ${PREFIX}/target/bin \
	--host ${HOST} --build i686-pc-linux-gnu ${ENABLE_TILERMGR} ${ENABLE_TESTS}
	if [[ "$arg" == "--clean" ]]; then
		make clean > /dev/null 2>&1
        fi
	make
	if [[ $? -ne 0 ]] ; then
	    exit 0
	fi
	make install
	if [[ $? -ne 0 ]] ; then
	    exit 0
	fi
	# Building syslink
	#.. need libgcc.a, librt.so and libpthread.so
	mkdir -p ${PREFIX}/target/lib
	cp $LIBGCC ${PREFIX}/target/lib
	cp `dirname $LIBRT`/librt*.so* ${PREFIX}/target/lib
	cp `dirname $LIBPTHREAD`/libpthread*.so* ${PREFIX}/target/lib
	cd ${PREFIX}/syslink
	echo "							  "
	echo "****************************************************"
	echo "      Building Syslink APIs and Samples		  "
	echo "****************************************************"
	echo "							  "
	./bootstrap.sh
	./configure --prefix ${PREFIX}/target --bindir ${PREFIX}/target/syslink \
	--host ${HOST} ${ENABLE_DEBUG}  --build i686-pc-linux-gnu
	export TILER_INC_PATH=${TILER_USERSPACE}
	if [[ "$arg" == "--clean" ]]; then
		make clean > /dev/null 2>&1
	fi
	make
	if [[ $? -ne 0 ]] ; then
	    exit 0
	fi
	make install
	if [[ $? -ne 0 ]] ; then
	    exit 0
	fi

	cd ${PREFIX}/syslink/d2c
	echo "							  "
	echo "****************************************************"
	echo "                    Building D2C                    "
	echo "****************************************************"
	echo "							  "
	./bootstrap.sh
	./configure --prefix ${PREFIX}/target --host ${HOST} ${ENABLE_TESTS}
	make clean
	make
	make install
	if [[ $? -ne 0 ]] ; then
	    exit 0
	fi
	make install
	if [[ $? -ne 0 ]] ; then
	    exit 0
	fi
	cd -

	cd ${PREFIX}
}

echo "	"
echo "Following Build options are available:"
echo "--------------------------------------"
echo "1--------------> Build Syslink Only"
echo "Any other Option to exit Build system"
echo "	"
echo "Enter your option:"
read VALUE
case $VALUE in
        1)
		build_syslink ;; # End of case 1

	*)	echo " Exiting from the build system... "
		exit 0
		;;
	esac
