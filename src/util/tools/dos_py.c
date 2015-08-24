#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_SERVICE_PYTHON

#include <dos/dos_types.h>
#include <dos/dos_config.h>
#include <dos/dos_py.h>

PyObject *m_pstRetVal = NULL;

/*
 * 函数名: U32 py_init()
 * 参数:   无参数
 * 功能:   初始化Python库
 * 返回值: 初始化失败则返回DOS_FAIL，成功则返回DOS_SUCC
 **/
U32 py_init()
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
 * 函数名: U32   py_exec_func(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
 * 参数:   const char *pszModule:  被调用的Python模块名
 *         const char *pszFunc:    需要调用的Python接口名称
 *         const char *pszPyFormat:Python格式化参数
 * 功能:   C语言调用Python函数接口
 * 返回值: 执行失败则返回DOS_FAIL;执行成功时若存在返回值则返回Python的返回值，否则返回DOS_SUCC
 *  !!!!!!!!!!!!!!!特别说明!!!!!!!!!!!!!!!!!!
 *  参数pszPyFormat格式为Python参数格式，而非C语言参数格式，具体如下:
 *      "s": (string) [char *]
 *      "z": (string or None) [char *] 将以NULL结尾的C字符串String转换为Python对象，如果字符串为空则返回None
 *      "u": (Unicode string) [Py_UNICODE *] Unicode(UCS-2或UCS-4)字符串转为Python Unicode对象，如果为空则返回None
 *      "i": (integer) [int]
 *      "b": (integer) [char]
 *      "h": (integer) [short int]
 *      "l": (integer) [long int]
 *      "B": (integer) [unsigned char]
 *      "H": (integer) [unsigned short int]
 *      "I": (integer/long) [unsigned int]
 *      "k": (integer/long) [unsigned long]
 *      "d": (float) [double]
 *      "f": (float) [float]
 *      "O": 表示一个Python对象
 *  其中参数格式列表需要使用括号括起来
 *
 *  使用示例
 *  ulRet = py_exec_func("router", "del_route", "(i)", ulGatewayID);
 *  说明: 在这里被调用的PyAPI最好有返回值，以保证返回值m_pstRetVal不为空
 **/
U32  py_exec_func(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
{
    S8   szPyScriptPath[MAX_PY_SCRIPT_LEN] = {0,};
    U32  ulRet = DOS_SUCC;
    S8   szImportPath[MAX_PY_SCRIPT_LEN] = {0,};
    PyObject *pstPyMod = NULL, *pstPyFunc = NULL, *pstParam = NULL;
    va_list vargs;

    if (!pszModule || !pszFunc || !pszPyFormat)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* 设置工作路径 */
    ulRet = config_get_py_path(szPyScriptPath, sizeof(szPyScriptPath));
    if (0 > ulRet)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        return DOS_FAIL;
    }

    dos_snprintf(szImportPath, sizeof(szImportPath), "sys.path.append(\'%s\')", szPyScriptPath);

    PyRun_SimpleString("import sys");
    PyRun_SimpleString(szImportPath);

    /* 导入模块 */
    pstPyMod = PyImport_ImportModule(pszModule);
    if (!pstPyMod)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* 查找函数 */
    pstPyFunc = PyObject_GetAttrString(pstPyMod, pszFunc);
    if (!pstPyFunc)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* 创建参数 */
    va_start(vargs, pszPyFormat);
    pstParam = Py_VaBuildValue( pszPyFormat, vargs);
    if (!pstParam)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }
    va_end(vargs);

    /* 函数调用 */
    m_pstRetVal = PyEval_CallObject(pstPyFunc, pstParam);
    if (!m_pstRetVal)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    goto py_finished;

py_finished:
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
    return ulRet;
}

/*
 * 函数名: const S8*   py_get_version()
 * 参数:   无参数
 * 功能:   获取python解释器版本号
 **/
U32  py_get_version(S8 *pszVersion, U32 ulLen)
{
    U32 ulRet = 0;
    S8  *pszNewVersion = NULL, *pszPos = NULL;

    ulRet = py_exec_func("file_info", "get_interpreter_version", "()");
    if (DOS_SUCC != ulRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(m_pstRetVal))
    {
        logr_error("Python Function \'get_interpreter_version\' returned None.");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszNewVersion = PyString_AsString(m_pstRetVal);
    if (!pszNewVersion)
    {
        DOS_ASSERT(0);
    }

    dos_snprintf(pszVersion, ulLen, "%s", pszNewVersion);

    /* 该句只是为了版本信息打印格式更好看，将第一个换行符打印成空格 */
    pszPos = dos_strchr(pszVersion, '\n');
    *pszPos = ' ';

    if (DOS_ADDR_VALID(m_pstRetVal))
    {
        Py_DECREF(m_pstRetVal);
        m_pstRetVal = NULL;
    }

    return DOS_SUCC;
}

/*
 * 函数名: U32  py_deinit()
 * 参数:   无参数
 * 功能:   卸载Python库
 * 返回值: 卸载失败则返回DOS_FAIL，成功则返回DOS_SUCC
 **/
U32  py_deinit()
{
    /* 卸载Python模块 */
    Py_Finalize();
    return DOS_SUCC;
}

#endif /* endof INCLUDE_SERVICE_PYTHON */

#ifdef __cplusplus
}
#endif /* __cplusplus */

