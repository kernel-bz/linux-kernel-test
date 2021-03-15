TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

#linux/time.h sys/time.h
DEFINES += _POSIX_SOURCE HAVE_STRUCT_TIMESPEC __timeval_defined \
        __timespec_defined _SYS_TIME_H \
        HAS_CAA_GET_CYCLES UATOMIC_NO_LINK_ERROR

LIBS += -Lpthread

QMAKE_CFLAGS += -Wno-unused-parameter -finstrument-functions

SOURCES += \
        main.c

HEADERS +=

DISTFILES += \
    ../pointer/struct-s-data.txt \
    ../pointer/pcpu-test.txt \
    ../pointer/rcu-ptr.txt \
    ../study/pointer/pcpu-test.txt \
    ../study/pointer/rcu-ptr.txt \
    ../study/pointer/struct-s-data.txt \
    ../study/study-history.txt \
    ../study/subjects/completion-wait.txt \
    ../study/subjects/rcu-struct.txt
