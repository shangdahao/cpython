
/* Support for dynamic loading of extension modules */

#include "Python.h"

/* ./configure sets HAVE_DYNAMIC_LOADING if dynamic loading of modules is
   supported on this platform. configure will then compile and link in one
   of the dynload_*.c files, as appropriate. We will call a function in
   those modules to get a function pointer to the module's init function.
*/
#ifdef HAVE_DYNAMIC_LOADING

#include "importdl.h"

extern dl_funcptr _PyImport_GetDynLoadFunc(const char *name,
                                           const char *shortname,
                                           const char *pathname, FILE *fp);



// 调用 C 语言编写的扩展模块
PyObject *
_PyImport_LoadDynamicModule(char *name, char *pathname, FILE *fp)
{
    PyObject *m;
    char *lastdot, *shortname, *packagecontext, *oldcontext;
    dl_funcptr p;

    // 在Python的module备份中检查是否有名为name的module
    if ((m = _PyImport_FindExtension(name, pathname)) != NULL) {
        Py_INCREF(m);
        return m;
    }
    lastdot = strrchr(name, '.');
    if (lastdot == NULL) {
        packagecontext = NULL;
        shortname = name;
    }
    else {
        packagecontext = name;
        shortname = lastdot+1;
    }

    // 从 dll/so 中获得module的初始化函数的起始地址
    p = _PyImport_GetDynLoadFunc(name, shortname, pathname, fp);
    if (PyErr_Occurred())
        return NULL;
    if (p == NULL) {
        PyErr_Format(PyExc_ImportError,
           "dynamic module does not define init function (init%.200s)",
                     shortname);
        return NULL;
    }
    oldcontext = _Py_PackageContext;
    _Py_PackageContext = packagecontext;

    // 调用module的初始化函数
    (*p)();
    _Py_PackageContext = oldcontext;
    if (PyErr_Occurred())
        return NULL;

    // 从sys.modules中获得已经被加载的module
    m = PyDict_GetItemString(PyImport_GetModuleDict(), name);
    if (m == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "dynamic module not initialized properly");
        return NULL;
    }
    /* Remember the filename as the __file__ attribute */
    // 设置module的__file__属性
    if (PyModule_AddStringConstant(m, "__file__", pathname) < 0)
        PyErr_Clear(); /* Not important enough to report */

    // 将module加入到Python的module备份中
    if (_PyImport_FixupExtension(name, pathname) == NULL)
        return NULL;
    if (Py_VerboseFlag)
        PySys_WriteStderr(
            "import %s # dynamically loaded from %s\n",
            name, pathname);
    Py_INCREF(m);
    return m;
}

#endif /* HAVE_DYNAMIC_LOADING */
