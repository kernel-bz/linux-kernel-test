#!/bin/bash
# $1: ver [v5.x]
# $2: cmd [/menuconfig/log/tag/modules_install/install/clean/distclean/]
# $3: arch [x86_64/riscv/arm64]
# $4: board [hifiveu/origin]

# author: JaeJoon Jung <rgbi3307@nate.com> on the www.kernel.bz

# echo "set tags=$BUILD_PATH/tags" > ~/.vimrc
# Makefile, KBUILD_CFLAGS, -save-temps=obj
# x86_64: CC_HAVE_ASM_GOTO := 1, -DCC_HAVE_ASM_GOTO

CPU_CNT=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print field+2}')
START_TIME=`date +%s`

VER=$1
CMD=$2
ARCH="riscv"
BOARD="origin"
COMPILER="riscv64-unknown-linux-gnu-"

BUILD_PATH="../build/build-$VER-$ARCH-$BOARD"
LOG_PATH="../build/log"
LOG_FILE="$LOG_PATH/log-$VER-$ARCH-$BOARD.txt"
TAG_PATH="$BUILD_PATH/tags"
STRIP="$COMPILER""strip"

if [ ! -d $BUILD_PATH ]; then
    echo "mkdir $BUILD_PATH"
    mkdir $BUILD_PATH
    chmod 755 $BUILD_PATH
fi

if [ ! -d $LOG_PATH ]; then
    echo "mkdir $LOG_PATH"
    mkdir $LOG_PATH
    chmod 755 $LOG_PATH
fi

if [ ! -f $BUILD_PATH/.config ]; then
    echo "make defconfig"
    make ARCH=$ARCH O=$BUILD_PATH defconfig 
fi

if [ $CMD == "log" ]; then
    make O=$BUILD_PATH/ \
        CROSS_COMPILE=$COMPILER \
        ARCH=$ARCH > $LOG_FILE

elif [ $CMD == "logfast" ]; then
    make -j$CPU_CNT O=$BUILD_PATH/ \
        CROSS_COMPILE=$COMPILER \
        ARCH=$ARCH > $LOG_FILE

elif [ $CMD == "tag" ]; then
    make tags -j$CPU_CNT O=$BUILD_PATH/ \
        CROSS_COMPILE=$COMPILER \
        ARCH=$ARCH
    echo "set tags=$TAG_PATH" > ~/.vimrc
else
    make -j$CPU_CNT O=$BUILD_PATH/ \
        CROSS_COMPILE=$COMPILER \
        ARCH=$ARCH $CMD
fi

$STRIP -o $BUILD_PATH/vmlinux-stripped $BUILD_PATH/vmlinux

END_TIME=`date +%s`
echo "Total build-time is $((($END_TIME-$START_TIME)/60)) minutes $((($END_TIME-$START_TIME)%60)) seconds"
