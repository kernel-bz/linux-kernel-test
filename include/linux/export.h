#ifndef _TOOLS_LINUX_EXPORT_H_
#define _TOOLS_LINUX_EXPORT_H_

//include/linux/module.h
struct module {
    char name[64];
};

#ifndef __ASSEMBLY__
#ifdef MODULE
extern struct module __this_module;
#define THIS_MODULE (&__this_module)
#else
#define THIS_MODULE ((struct module *)0)
#endif
#endif

//#define EXPORT_SYMBOL(sym)
//#define EXPORT_SYMBOL_GPL(sym)
#define EXPORT_SYMBOL_GPL_FUTURE(sym)
#define EXPORT_UNUSED_SYMBOL(sym)
#define EXPORT_UNUSED_SYMBOL_GPL(sym)

#define EXPORT_SYMBOL_GPL(sym) extern typeof(sym) sym
#define EXPORT_SYMBOL(sym) extern typeof(sym) sym


#endif
