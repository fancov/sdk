#ifndef __DOS_STRING__
#define __DOS_STRING__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

S32 dos_sscanf(const S8 *pszSrc, const S8 *pszFormat, ...);
S32 dos_fscanf(FILE *pStream, const S8 *pszFormat, ...);
S32 dos_snprintf(S8 *pszDstBuff, S32 lBuffLength, const S8 *pszFormat, ...);
S32 dos_fprintf(FILE *pStream, const S8 *pszFormat, ...);


S8* dos_strcpy(S8 *dst, const S8 *src);
S8* dos_strncpy(S8 *dst, const S8 *src, S32 lMaxLen);
U32 dos_strlen(const S8 *str);
S8 *dos_strcat(S8 *dest, const S8 *src);
S8 *dos_strchr(const S8 *str, S32 i);
S8 *dos_strstr(S8 * source, S8 *key);
S32 dos_strcmp(const S8 *s1, const S8 *s2);
S32 dos_stricmp(const S8 *s1, const S8 *s2);
S32 dos_strncmp(const S8 *s1, const S8 *s2, U32 n);
S32 dos_strnicmp(const S8 *s1, const S8 *s2, U32 n);
S32 dos_strnlen(const S8 *s, S32 count);
S8 dos_toupper(S8 ch); 
S8 dos_tolower(S8 ch);
VOID dos_uppercase(S8 *str);
VOID dos_lowercase(S8 *str);
S32 dos_is_digit_str(S8 *str);
S32 dos_atol(const S8 *szStr, S32 *pnVal);
S32 dos_atoul(const S8 *szStr, U32 *pulVal);
S32 dos_atolx (const S8 *szStr, S32 *pnVal);
S32 dos_atoulx(const S8 *szStr, U32 *pulVal);
S32 dos_ltoa(S32 nVal, S8 *szStr, U32 ulStrLen);
S32 dos_ltoax(S32 nVal, S8 *szStr, U32 ulStrLen);
S32 dos_ultoax (U32 ulVal, S8 *szStr, U32 ulStrLen);
S8  *dos_ipaddrtostr(U32 ulAddr, S8 *str, U32 ulStrLen);
S32 dos_strtoipaddr(S8 *szStr, U32 *pulIpAddr);

U32 dos_htonl(U32 ulSrc);
U32 dos_ntohl(U32 ulSrc);
U16 dos_htons(U16 usSrc);
U16 dos_ntohs(U16 usSrc);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

