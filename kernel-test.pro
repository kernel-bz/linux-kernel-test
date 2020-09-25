TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += \
        include/

SOURCES += \
    test/cpu/cpus-mask-test.c \
    test/lib/ptr-test.c \
    test/sched/rq-test.c \
    main.c

HEADERS += \
    include/test/test.h \
    include/linux/list.h
