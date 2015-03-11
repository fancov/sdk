#ifndef __DOS_PY_H__
#define __DOS_PY_H__

#define MAX_PY_SCRIPT_LEN 256  //定义python路径的最大长度
#define MAX_PY_PARAM_CNT  8    //定义最大参数个数

#if INCLUDE_SERVICE_PYTHON

U32   py_init();
U32   py_exec_func(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...);
U32   py_deinit();

#endif /* INCLUDE_SERVICE_PYTHON */


#endif  // end __DOS_PY_H__

