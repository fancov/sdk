#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_SERVICE_PYTHON

#include <python2.6/Python.h>
#include <dos/dos_types.h>
#include <dos/dos_config.h>
#include <dos/dos_py.h>


/*
 * 函数名: U32 py_init_py()
 * 参数:   无参数
 * 功能:   初始化Python库
 * 返回值: 初始化失败则返回DOS_FAIL，成功则返回DOS_SUCC
 **/
U32 py_init_py()
{
    /*初始化Python*/
    Py_Initialize();
    if (!Py_IsInitialized())
    {
       DOS_ASSERT(0);
       return DOS_FAIL;
    }

    return DOS_SUCC;
}


/*
 * 函数名: U32   py_c_call_py(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
 * 参数:   const char *pszModule:  被调用的Python模块名
 *         const char *pszFunc:    需要调用的Python接口名称
 *         const char *pszPyFormat:Python格式化参数
 * 功能:   C语言调用Python函数接口
 * 返回值: 执行失败则返回DOS_FAIL;执行成功时若存在返回值则返回Python的返回值，否则返回DOS_SUCC
 *  !!!!!!!!!!!!!!!特别说明!!!!!!!!!!!!!!!!!!
 *  参数pszPyFormat格式为Python参数格式，而非C语言参数格式，具体如下:
 *      s: 表示字符串
 *      i: 整型变量
 *      f: 表示浮点数
 *      O: 表示一个Python对象
 *  其中参数格式列表需要使用括号括起来，具体使用见: http://blog.chinaunix.net/uid-22920230-id-3443571.html
 **/
U32   py_c_call_py(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
{
    S8   szPyScriptPath[MAX_PY_SCRIPT_LEN] = {0,};
    U32  ulRet;
    S8   szImportPath[MAX_PY_SCRIPT_LEN] = {0,};
    PyObject *pstPyMod, *pstPyFunc, *pstParam, *pstRetVal;
    va_list vargs;

    if (!pszModule || !pszFunc || !pszPyFormat)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 设置工作路径 */
    ulRet = config_get_py_path(szPyScriptPath, sizeof(szPyScriptPath));
    if (0 > ulRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    logr_info("%s:Line %d: The current python work directory is: %s"
                , dos_get_filename(__FILE__), __LINE__, szPyScriptPath);

    dos_snprintf(szImportPath, sizeof(szImportPath), "sys.path.append(\'%s\')", szPyScriptPath);
    szImportPath[ MAX_PY_SCRIPT_LEN - 1] = '\0';

    PyRun_SimpleString("import sys");
    PyRun_SimpleString(szImportPath);

    /* 导入模块 */
    pstPyMod = PyImport_ImportModule(pszModule);
    if (!pstPyMod)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 查找函数 */
    pstPyFunc = PyObject_GetAttrString(pstPyMod, pszFunc);
    if (!pstPyFunc)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 创建参数 */
    va_start(vargs, pszPyFormat);
    pstParam = Py_VaBuildValue( pszPyFormat, vargs );
    if (!pstParam)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    va_end(vargs);

    /* 函数调用 */
    pstRetVal = PyEval_CallObject(pstPyFunc, pstParam);
    if (!pstRetVal)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstRetVal)
    {
        Py_DECREF(pstRetVal);
        pstRetVal = NULL;
    }

    if (pstParam)
    {
        Py_DECREF(pstParam);
        pstParam= NULL;
    }

    if (pstPyFunc)
    {
        Py_DECREF(pstPyFunc);
        pstPyFunc = NULL;
    }

    if (pstPyMod)
    {
        Py_DECREF(pstPyMod);
        pstPyMod= NULL;
    }

    return DOS_SUCC;
}


/*
 * 函数名: U32  py_deinit_py()
 * 参数:   无参数
 * 功能:   卸载Python库
 * 返回值: 卸载失败则返回DOS_FAIL，成功则返回DOS_SUCC
 **/
U32  py_deinit_py()
{
    /* 卸载Python模块 */
    Py_Finalize();

    return DOS_SUCC;
}

#endif /* endof INCLUDE_SERVICE_PYTHON */

#ifdef __cplusplus
}
#endif /* __cplusplus */

