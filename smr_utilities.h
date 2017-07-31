#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifndef _SMR_UTILITIES_H
#define _SMR_UTILITIES_H

/* NOTE
    mode to open smr file for reading, on windows this *MUST* be "rb" as just
    opening the file as "r" causes fseek and ftell to skip around to compensate
    for what the system thinks are multi-byte characters (i.e linefeeds - \r\n)
    see: <http://stackoverflow.com/questions/3187693/fread-ftell-apparently-broken-under-windows-works-fine-under-linux>
*/
#define FILE_READ_MODE "rb"

#define gotto() (printf("GOTO: %s [%d]\n", __FUNCTION__, __LINE__))
#define show(v, fmt) (printf(#v " = " fmt "\n", v))

/* ========================================================================= */
FILE * open_file(const char *ifile, const char *mode)
{
    FILE *fp;

#if defined(_WIN32) && ! defined(__MINGW32__)
    fopen_s(&fp, ifile, mode);
#else
    fp = fopen(ifile, mode);
#endif

    return fp;
}
/* ========================================================================= */
char *copy_string(const char *src)
{
    size_t nchar;
    char *dest;

    nchar = strlen(src) + 1;

    dest = malloc(sizeof (char) * nchar);

#if defined(_WIN32) && ! defined(__MINGW32__)
    strcpy_s(dest, nchar, src);
#else
    strcpy(dest, src);
#endif

    dest[nchar-1] = '\0';
    return dest;
}
/* ========================================================================= */
void read_chars(FILE *fp, const char *mode, char *buf, size_t len)
{
#if defined(_WIN32) && ! defined(__MINGW32__)
    fscanf_s(fp, mode, buf, len);
#else
    fscanf(fp, mode, buf);
#endif

}
/* ========================================================================= */
void strtrim(char *str)
{
    size_t len, k, start, end;
    char *ptr;

    ptr = str;
    len = strlen(str);

    start = 0;
    end = len;

    while (isspace(*ptr++))
    {
        ++start;
    }

    ptr = str + (len-1);

    while (isspace(*ptr--))
    {
        --end;
    }

    end -= start;

    for (k = 0; k < len; ++k)
    {
        if (k < end)
        {

            str[k] = str[start];
            ++start;

        }
        else
        {
            str[k] = '\0';
        }
    }
}
/* ------------------------------------------------------------------------- */
char *fill_string(FILE *fp, int32_t pad)
{
    uint8_t n;
    char *pt;
    int32_t loc;

    fread(&n, sizeof (uint8_t), (size_t)1, fp);

    loc = ftell(fp);

    if (n > 0)
    {
        pt = malloc(sizeof (char) * n+1);

        fread(pt, sizeof (char), n, fp);
        pt[n] = '\0';
        strtrim(pt);

    }
    else
    {
        pt = NULL;
    }

    fseek(fp, loc+pad, SEEK_SET);

    return pt;
}
/* ------------------------------------------------------------------------- */
char lower_char(const char c)
{
    return (c > 64 && c < 91) ? c+32 : c;
}
/* ------------------------------------------------------------------------- */
int string_compare_nocase(const char *str1, const char *str2)
{
    unsigned int c1;
    unsigned int c2;

    for(;;)
    {
        c1 = (unsigned int)lower_char(*str1++);
        c2 = (unsigned int)lower_char(*str2++);

        if (c1 == 0 || c1 != c2)
        {
            return c1 - c2;
        }
    }
}
/* ========================================================================= */
#endif
