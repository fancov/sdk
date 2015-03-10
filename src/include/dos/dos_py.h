#ifndef __DOS_PY_H__
#define __DOS_PY_H__

#define MAX_PY_SCRIPT_LEN 256  //定义python路径的最大长度
#define MAX_PY_PARAM_CNT  8    //定义最大参数个数

U32   py_init_py();
U32   py_c_call_py(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...);
U32   py_deinit_py();


#endif  // end __DOS_PY_H__
