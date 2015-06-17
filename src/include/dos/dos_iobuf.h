

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct tagIOBuf
{
    U32 ulSize;          /* 当前BUFFER的总长度 */
    U32 ulLength;        /* 当前BUFFER使用的长度 */

    U8  *pszBuffer;      /* 缓存指针 */
}IO_BUF_CB;

/* 静态初始化IO_BUF_CB  */
#define IO_BUF_INIT       {0, 0, NULL}

/* 系统允许的最大缓存长度 */
#define IO_BUF_MAX_SIZE   10 * 1024

/**
 * 函数: dos_iobuf_init
 * 功能: 初始化pstIOBufCB所指向的缓存，默认长度为ulLength
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 要初始化的缓存结构
 *       U32 ulLength: 初始化长度
 * 返回值
 *      成功返回DOS_SUCC，失败返回DOS_FAUIL
 * 当长度不为0时，需要判断返回值
 */
U32 dos_iobuf_init(IO_BUF_CB *pstIOBufCB, U32 ulLength);

/**
 * 函数: dos_iobuf_free
 * 功能: 释放pstIOBufCB所指向的缓存
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 要释放的缓存结构
 * 返回值
 *      成功返回DOS_SUCC，失败返回DOS_FAUIL
 */
U32 dos_iobuf_free(IO_BUF_CB *pstIOBufCB);

/**
 * 函数: dos_iobuf_dup
 * 功能: 将pstIOBufCBSrc所指向的缓存copy到pstIOBufCBDst中，包括数据
 * 参数:
 *      IO_BUF_CB *pstIOBufCBSrc : 源数据
 *      IO_BUF_CB *pstIOBufCBDst : 目的缓存
 * 返回值
 *      成功返回DOS_SUCC，失败返回DOS_FAUIL
 */
U32 dos_iobuf_dup(IO_BUF_CB *pstIOBufCBSrc, IO_BUF_CB *pstIOBufCBDst);

/**
 * 函数: dos_iobuf_resize
 * 功能: 重新初始化pstIOBufCB所指向的缓存，并改变其长度为ulength
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 要初始化的缓存结构
 *       U32 ulLength: 初始化长度
 * 返回值
 *      成功返回DOS_SUCC，失败返回DOS_FAUIL
 */
U32 dos_iobuf_resize(IO_BUF_CB *pstIOBufCB, U32 ulength);

/**
 * 函数: dos_iobuf_append
 * 功能: 向缓存pstIOBufCB追加数据
 * 参数:
 *      IO_BUF_CB *pstIOBufCB: 被长度缓存
 *      const VOID *pData : 将要被添加的数据
 *      U32 ulength       : 缓存的长度
 * 返回值
 *      成功返回DOS_SUCC，失败返回DOS_FAUIL
 */
U32 dos_iobuf_append(IO_BUF_CB *pstIOBufCB, const VOID *pData, U32 ulength);

/**
 * 函数: dos_iobuf_append_size
 * 功能: 给缓存pstIOBufCB扩容，增量为ulength
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 要扩容的缓存
 *       U32 ulLength: 扩荣大小
 * 返回值
 *      成功返回DOS_SUCC，失败返回DOS_FAUIL
 */
U32 dos_iobuf_append_size(IO_BUF_CB *pstIOBufCB, U32 ulength);

/**
 * 函数: dos_iobuf_get_data
 * 功能: 获取缓存pstIOBufCB的数据域
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 需要操作的数据
 * 返回值
 *      成功返回数据指针，失败返回NULL
 */
U8 *dos_iobuf_get_data(IO_BUF_CB *pstIOBufCB);

/**
 * 函数: dos_iobuf_get_data
 * 功能: 获取缓存pstIOBufCB的被使用的长度
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 需要操作的数据
 * 返回值
 *      成功返回缓存长度。失败返回U32_BUTT
 */
U32 dos_iobuf_get_length(IO_BUF_CB *pstIOBufCB);

/**
 * 函数: dos_iobuf_get_data
 * 功能: 获取缓存pstIOBufCB的总大小
 * 参数:
 *       IO_BUF_CB *pstIOBufCB: 需要操作的数据
 * 返回值
 *      成功返回数据长度。失败返回U32_BUTT
 */
U32 dos_iobuf_get_size(IO_BUF_CB *pstIOBufCB);


#ifdef __cplusplus
}
#endif /* __cplusplus */



