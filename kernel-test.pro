TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c \
    cpu/cpus-mask-test.c

HEADERS += \
    include/test.h
