#ifndef __DOS_STRING__
#define __DOS_STRING__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

DLLEXPORT S32 dos_sscanf(const S8 *pszSrc, const S8 *pszFormat, ...);
DLLEXPORT S32 dos_fscanf(FILE *pStream, const S8 *pszFormat, ...);
DLLEXPORT S32 dos_snprintf(S8 *pszDstBuff, S32 lBuffLength, const S8 *pszFormat, ...);
DLLEXPORT S32 dos_fprintf(FILE *pStream, const S8 *pszFormat, ...);


DLLEXPORT S8* dos_strcpy(S8 *dst, const S8 *src);
DLLEXPORT S8* dos_strncpy(S8 *dst, const S8 *src, S32 lMaxLen);
DLLEXPORT U32 dos_strlen(const S8 *str);
DLLEXPORT S8 *dos_strcat(S8 *dest, const S8 *src);
DLLEXPORT S8 *dos_strchr(const S8 *str, S32 i);
DLLEXPORT S8 *dos_strstr(S8 * source, S8 *key);
DLLEXPORT S32 dos_strcmp(const S8 *s1, const S8 *s2);
DLLEXPORT S32 dos_stricmp(const S8 *s1, const S8 *s2);
DLLEXPORT S32 dos_strncmp(const S8 *s1, const S8 *s2, U32 n);
DLLEXPORT S32 dos_strnicmp(const S8 *s1, const S8 *s2, U32 n);
DLLEXPORT S32 dos_strnlen(const S8 *s, S32 count);
DLLEXPORT S8 dos_toupper(S8 ch);
DLLEXPORT S8 dos_tolower(S8 ch);
DLLEXPORT VOID dos_uppercase(S8 *str);
DLLEXPORT VOID dos_lowercase(S8 *str);
DLLEXPORT S32 dos_is_digit_str(S8 *str);
DLLEXPORT S32 dos_atol(const S8 *szStr, S32 *pnVal);
DLLEXPORT S32 dos_atoul(const S8 *szStr, U32 *pulVal);
DLLEXPORT S32 dos_atoull(const S8 *szStr, U64 *pulVal);
DLLEXPORT S32 dos_atoll(const S8 *szStr, S64 *pulVal);
DLLEXPORT S32 dos_atolx (const S8 *szStr, S32 *pnVal);
DLLEXPORT S32 dos_atoulx(const S8 *szStr, U32 *pulVal);
DLLEXPORT S32 dos_ltoa(S32 nVal, S8 *szStr, U32 ulStrLen);
DLLEXPORT S32 dos_ltoax(S32 nVal, S8 *szStr, U32 ulStrLen);
DLLEXPORT S32 dos_ultoax (U32 ulVal, S8 *szStr, U32 ulStrLen);
DLLEXPORT S8  *dos_ipaddrtostr(U32 ulAddr, S8 *str, U32 ulStrLen);
DLLEXPORT S32 dos_is_printable_str(S8* str);
DLLEXPORT S32 dos_strtoipaddr(S8 *szStr, U32 *pulIpAddr);
DLLEXPORT S8 *dos_strndup(const S8 *s, S32 count);
DLLEXPORT S32 dos_strlastindexof(const S8 *s, const S8 ch, U32 n);

DLLEXPORT U32 dos_htonl(U32 ulSrc);
DLLEXPORT U32 dos_ntohl(U32 ulSrc);
DLLEXPORT U16 dos_htons(U16 usSrc);
DLLEXPORT U16 dos_ntohs(U16 usSrc);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

