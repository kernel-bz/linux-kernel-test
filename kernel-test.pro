TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    test/cpu/cpus-mask-test.c \
    test/lib/ptr-test.c \
    test/sched/rq-test.c \
    main.c

HEADERS += \
    include/test/test.h
