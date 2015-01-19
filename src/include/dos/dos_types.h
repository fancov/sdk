/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_types.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 配置sdk中模块开关宏
 *     History:
 */

#ifndef __TYPES_H____
#define __TYPES_H____

#ifndef S8
#define S8 char
#endif

#ifndef U8
#define U8 unsigned char
#endif

#ifndef S16
#define S16 short
#endif

#ifndef U16
#define U16 unsigned short
#endif

#ifndef S32
#define S32 int
#endif

#ifndef U32
#define U32 unsigned int
#endif

#ifndef S64
#define S64 long
#endif

#ifndef U64
#define U64 unsigned long
#endif

#ifndef F32 
#define F32 float
#endif

#ifndef F64
#define F64 double
#endif

#ifndef BOOL
#define BOOL unsigned int
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef DOS_TRUE
#define DOS_TRUE 1
#endif

#ifndef DOS_FALSE
#define DOS_FALSE 0
#endif

#ifndef DOS_SUCC
#define DOS_SUCC (U32)0
#endif

#ifndef DOS_FAIL
#define DOS_FAIL (U32)(-1)
#endif

#ifndef U8_BUTT
#define U8_BUTT (U8)-1
#endif

#ifndef U16_BUTT
#define U16_BUTT (U16)-1
#endif

#ifndef U32_BUTT
#define U32_BUTT (U32)-1
#endif

#ifndef U64_BUTT
#define U64_BUTT (U64)-1
#endif

#define ELEMENT_OFFSET(type,member) \
/*lint -save -e413*/ ((U64) &((type *)0)->member)/*lint -restore*/

#ifndef DOS_ASSERT
#define DOS_ASSERT(f)\
do{\
logr_error ("Assert happened: func:%s,file=%s,line=%d, param:%d\n", __FUNCTION__, __FILE__, __LINE__ , f);\
dos_assert(__FILE__, __LINE__, f); \
}while(0)
#endif


#endif
