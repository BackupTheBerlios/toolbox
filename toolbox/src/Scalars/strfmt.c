//===================================================
// $Id: strfmt.c,v 1.1 2004/05/12 22:04:53 plg Exp $
//===================================================
/* Copyright (c) 1999-2004, Paul L. Gatille <paul.gatille@free.fr>
 *
 * This file is part of Toolbox, an object-oriented utility library
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the "Artistic License" which comes with this Kit.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 */

/** 
 * @file strfmt.c string formating
 * 
 * Shamelessly stolen and patched from Str lib (see copyright notices)
 *
 *  Str - String Library
 *  Copyright (c) 1999-2000 Ralf S. Engelschall <rse@engelschall.com>
 *
 *  This file is part of Str, a string handling and manipulation 
 *  library which can be found at http://www.engelschall.com/sw/str/.
 *
 *  Permission to use, copy, modify, and distribute this software for
 *  any purpose with or without fee is hereby granted, provided that
 *  the above copyright notice and this permission notice appear in all
 *  copies.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 *  str_format.c: formatting functions
 */

/*
 *  Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. All advertising materials mentioning features or use of this
 *     software must display the following acknowledgment:
 *     "This product includes software developed by the Apache Group
 *     for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 *  4. The names "Apache Server" and "Apache Group" must not be used to
 *     endorse or promote products derived from this software without
 *     prior written permission. For written permission, please contact
 *     apache@apache.org.
 *
 *  5. Products derived from this software may not be called "Apache"
 *     nor may "Apache" appear in their names without prior written
 *     permission of the Apache Group.
 *
 *  6. Redistributions of any form whatsoever must retain the following
 *     acknowledgment:
 *     "This product includes software developed by the Apache Group
 *     for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 *  THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 *  EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 *  ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 *  OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is a generic printf-style formatting code which is based on
 * Apache's ap_snprintf which it turn is based on and used with the
 * permission of, the SIO stdio-replacement strx_* functions by Panos
 * Tsirigotis <panos@alumni.cs.colorado.edu> for xinetd. The IEEE
 * floating point formatting routines are derived from an anchient
 * FreeBSD version which took it from GNU libc-4.6.27 and modified it
 * to be thread safe. The whole code was finally cleaned up, stripped
 * and extended by Ralf S. Engelschall for use inside the Str library.
 * Especially and Apache and network specific kludges were removed again
 * and instead the formatting engine now can be extended by the caller
 * on-the-fly.
 */

#include <pthread.h>
#include <limits.h>
#include <stdlib.h>     /* for malloc, etc. */
#include <math.h>       /* for modf(3) */
#include <string.h>     /* ... */
#include <ctype.h>     /* ... */
#include <stdio.h>
#include <stdarg.h>

#include "Toolbox.h"
#include "String.h"
#include "limits.h"
#include "strfmt.h"

#define MY_INT_MAX  INT_MAX // 4294967295UL// ((unsigned int)ULONG_MAX) // 2147483647
/* true and false boolean values and corresponding type */
#undef TRUE
#undef FALSE
#undef BOOL
#ifdef __cplusplus
#define BOOL  bool
#define TRUE  true
#define FALSE false
#else
#define BOOL  char
#define TRUE  ((BOOL)(1 == 1))
#define FALSE ((BOOL)(0 == 1))
#endif

/* null values for pointers and characters */
#ifndef NULL
#define NULL ((void *)0)
#endif
#ifndef NUL
#define NUL '\0'
#endif

       
/* explicit support for unsigned char based ctype stuff */
#define str_isalpha(c)  (isalpha(((unsigned char)(c))))
#define str_isdigit(c)  (isdigit(((unsigned char)(c))))
#define str_isxdigit(c) (isxdigit(((unsigned char)(c))))
#define str_islower(c)  (islower(((unsigned char)(c))))
#define str_tolower(c)  (isupper((unsigned char)(c)) ? \
                         tolower((unsigned char)(c)) : (int)(c))
                          
#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct str_vformat_st str_vformat_t;
struct str_vformat_st {
    char *curpos;
    char *endpos;
    union {
        int i; long l;
        double d; void *p;
    } data[6];
    int (*flush)(struct str_vformat_st *);
    char *(*format)(
        struct str_vformat_st *,
        char *, char *, int *,
        char *, int, char *, int, va_list
    );
};
typedef unsigned int str_size_t;
#endif

int str_vformat(str_vformat_t *, const char *, va_list);
char *str_copy(char *as, const char *at, str_size_t n);                         
char *str_span(const char *s, str_size_t n, const char *charset, int mode);
/* types which are locally use */
typedef long                 long_int;
typedef unsigned long      u_long_int;
#if defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG > 0)
typedef long long            quad_int;
typedef unsigned long long u_quad_int;
#else
typedef long                 quad_int;
typedef unsigned long      u_quad_int;
#endif

/* a few handy defines */
#define S_NULL          "(NULL)"
#define S_NULL_LEN      6
#define FLOAT_DIGITS    6
#define EXPONENT_LENGTH 10

/* NUM_BUF_SIZE is the size of the buffer used for arithmetic 
   conversions. This is a magic number; do NOT decrease it! */
#define NUM_BUF_SIZE    512
#define NDIG            80

#define STR_RIGHT        (1 << 0)  /* operate from right end */
#define STR_COMPLEMENT   (1 << 1)  /* use complement */


/* 
 * convert string to decimal. the number of digits is specified by
 * ndigit decpt is set to the position of the decimal point sign is set
 * to 0 for positive, 1 for negative. buf must have at least NDIG bytes.
 */

#define str_ecvt(arg,ndigits,decpt,sign,buf) \
        str_cvt((arg), (ndigits), (decpt), (sign), 1, (buf))

#define str_fcvt(arg,ndigits,decpt,sign,buf) \
        str_cvt((arg), (ndigits), (decpt), (sign), 0, (buf))

static char *
str_cvt(
    double arg, 
    int ndigits, 
    int *decpt, 
    BOOL *sign, 
    int eflag,
    char *buf)
{
    register int r2;
    double fi, fj;
    register char *p, *p1;

    if (ndigits >= NDIG - 1)
        ndigits = NDIG - 2;
    r2 = 0;
    *sign = FALSE;
    p = &buf[0];
    if (arg < 0) {
        *sign = TRUE;
        arg = -arg;
    }
    arg = modf(arg, &fi);
    p1 = &buf[NDIG];

    /* Do integer part */
    if (fi != 0) {
        p1 = &buf[NDIG];
        while (fi != 0) {
            fj = modf(fi / 10, &fi);
            *--p1 = (int) ((fj + .03) * 10) + '0';
            r2++;
        }
        while (p1 < &buf[NDIG])
            *p++ = *p1++;
    }
    else if (arg > 0) {
        while ((fj = arg * 10) < 1) {
            arg = fj;
            r2--;
        }
    }
    p1 = &buf[ndigits];
    if (eflag == 0)
        p1 += r2;
    *decpt = r2;
    if (p1 < &buf[0]) {
        buf[0] = NUL;
        return (buf);
    }
    while (p <= p1 && p < &buf[NDIG]) {
        arg *= 10;
        arg = modf(arg, &fj);
        *p++ = (int) fj + '0';
    }
    if (p1 >= &buf[NDIG]) {
        buf[NDIG - 1] = NUL;
        return (buf);
    }
    p = p1;
    *p1 += 5;
    while (*p1 > '9') {
        *p1 = '0';
        if (p1 > buf)
            ++ * --p1;
        else {
            *p1 = '1';
            (*decpt)++;
            if (eflag == 0) {
                if (p > buf)
                    *p = '0';
                p++;
            }
        }
    }
    *p = NUL;
    return (buf);
}

static char *
str_gcvt(
    double number, 
    int ndigit, 
    char *buf, 
    BOOL altform)
{
    BOOL sign;
    int decpt;
    register char *p1, *p2;
    register int i;
    char buf1[NDIG];

    p1 = str_ecvt(number, ndigit, &decpt, &sign, buf1);
    p2 = buf;
    if (sign)
        *p2++ = '-';
    for (i = ndigit - 1; i > 0 && p1[i] == '0'; i--)
        ndigit--;
    if ((decpt >= 0 && decpt - ndigit > 4)
        || (decpt < 0 && decpt < -3)) { /* use E-style */
        decpt--;
        *p2++ = *p1++;
        *p2++ = '.';
        for (i = 1; i < ndigit; i++)
            *p2++ = *p1++;
        *p2++ = 'e';
        if (decpt < 0) {
            decpt = -decpt;
            *p2++ = '-';
        }
        else
            *p2++ = '+';
        if (decpt / 100 > 0)
            *p2++ = decpt / 100 + '0';
        if (decpt / 10 > 0)
            *p2++ = (decpt % 100) / 10 + '0';
        *p2++ = decpt % 10 + '0';
    }
    else {
        if (decpt <= 0) {
            if (*p1 != '0')
                *p2++ = '.';
            while (decpt < 0) {
                decpt++;
                *p2++ = '0';
            }
        }
        for (i = 1; i <= ndigit; i++) {
            *p2++ = *p1++;
            if (i == decpt)
                *p2++ = '.';
        }
        if (ndigit < decpt) {
            while (ndigit++ < decpt)
                *p2++ = '0';
            *p2++ = '.';
        }
    }
    if (p2[-1] == '.' && !altform)
        p2--;
    *p2 = NUL;
    return (buf);
}

/*
 * The INS_CHAR macro inserts a character in the buffer and flushes the
 * buffer if necessary. It uses the char pointers sp and bep: sp points
 * to the next available character in the buffer, bep points to the
 * end-of-buffer+1. While using this macro, note that the nextb pointer
 * is NOT updated. NOTE: Evaluation of the c argument should not have
 * any side-effects
 */
#define INS_CHAR(c, sp, bep, cc) {  \
    if (sp >= bep) {                \
        vbuff->curpos = sp;         \
        if (vbuff->flush(vbuff))    \
            return -1;              \
        sp = vbuff->curpos;         \
        bep = vbuff->endpos;        \
    }                               \
    *sp++ = (c);                    \
    cc++;                           \
}

/*
 * Convert a string to decimal value 
 */
#define NUM(c) ((c) - '0')
#define STR_TO_DEC(str, num) {    \
    num = NUM( *str++ );          \
    while (str_isdigit(*(str))) { \
        num *= 10 ;               \
        num += NUM( *str++ ) ;    \
    }                             \
}
     
/*
 * This macro does zero padding so that the precision requirement is
 * satisfied. The padding is done by adding '0's to the left of the
 * string that is going to be printed.
 */
#define FIX_PRECISION(adjust, precision, s, s_len)  \
    if (adjust) {                                   \
        while (s_len < precision) {                 \
            *--s = '0';                             \
            s_len++;                                \
        }                                           \
    }

/*
 * This macro does padding. 
 * The padding is done by printing the character ch.
 */
#define PAD(width, len, ch) \
    do {                            \
        INS_CHAR(ch, sp, bep, cc);  \
        width-- ;                   \
    } while (width > len)

/*
 * Prefix the character ch to the string str
 * Increase length. Set the has_prefix flag.
 */
#define PREFIX(str, length, ch) \
    *--str = ch; \
    length++;    \
    has_prefix = TRUE

/*
 * Convert num to its decimal format.
 * Return value:
 *   - a pointer to a string containing the number (no sign)
 *   - len contains the length of the string
 *   - is_negative is set to TRUE or FALSE depending on the sign
 *     of the number (always set to FALSE if is_unsigned is TRUE)
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 * Note: we have 2 versions. One is used when we need to use quads
 * (conv_10_quad), the other when we don't (conv_10). We're assuming the
 * latter is faster.
 */
static char *
conv_10(
    register long_int num, 
    register BOOL is_unsigned,
    register BOOL *is_negative, 
    char *buf_end,
    register int *len)
{
    register char *p = buf_end;
    register u_long_int magnitude;

    if (is_unsigned) {
        magnitude = (u_long_int) num;
        *is_negative = FALSE;
    }
    else {
        *is_negative = (num < 0);
        /* On a 2's complement machine, negating the most negative integer 
           results in a number that cannot be represented as a signed integer.
           Here is what we do to obtain the number's magnitude:
                a. add 1 to the number
                b. negate it (becomes positive)
                c. convert it to unsigned
                d. add 1 */
        if (*is_negative) {
            long_int t = num + 1;

            magnitude = ((u_long_int) - t) + 1;
        }
        else
            magnitude = (u_long_int) num;
    }
    /* We use a do-while loop so that we write at least 1 digit */
    do {
        register u_long_int new_magnitude = magnitude / 10;
        *--p = (char) (magnitude - new_magnitude * 10 + '0');
        magnitude = new_magnitude;
    }
    while (magnitude);
    *len = buf_end - p;
    return (p);
}

static char *
conv_10_quad(
    quad_int num, 
    register BOOL is_unsigned,
    register BOOL *is_negative, 
    char *buf_end,
    register int *len)
{
    register char *p = buf_end;
    u_quad_int magnitude;

    if (is_unsigned) {
        magnitude = (u_quad_int) num;
        *is_negative = FALSE;
    }
    else {
        *is_negative = (num < 0);
        /* On a 2's complement machine, negating the most negative integer 
           result in a number that cannot be represented as a signed integer.
           Here is what we do to obtain the number's magnitude:
                a. add 1 to the number
                b. negate it (becomes positive)
                c. convert it to unsigned
                d. add 1 */
        if (*is_negative) {
            quad_int t = num + 1;
            magnitude = ((u_quad_int) - t) + 1;
        }
        else
            magnitude = (u_quad_int) num;
    }
    /* We use a do-while loop so that we write at least 1 digit */
    do {
        u_quad_int new_magnitude = magnitude / 10;
        *--p = (char)(magnitude - new_magnitude * 10 + '0');
        magnitude = new_magnitude;
    }
    while (magnitude);
    *len = buf_end - p;
    return (p);
}

/*
 * Convert a floating point number to a string formats 'f', 'e' or 'E'.
 * The result is placed in buf, and len denotes the length of the string
 * The sign is returned in the is_negative argument (and is not placed
 * in buf).
 */
static char *
conv_fp(
    register char format, 
    register double num,
    BOOL add_dp, 
    int precision, 
    BOOL *is_negative,
    char *buf, 
    int *len)
{
    register char *s = buf;
    register char *p;
    int decimal_point;
    char buf1[NDIG];

    if (format == 'f')
        p = str_fcvt(num, precision, &decimal_point, is_negative, buf1);
    else  /* either e or E format */
        p = str_ecvt(num, precision + 1, &decimal_point, is_negative, buf1);

    /* Check for Infinity and NaN */
    if (str_isalpha(*p)) {
        *len = strlen(str_copy(buf, p, 0));
        *is_negative = FALSE;
        return (buf);
    }

    if (format == 'f') {
        if (decimal_point <= 0) {
            *s++ = '0';
            if (precision > 0) {
                *s++ = '.';
                while (decimal_point++ < 0)
                    *s++ = '0';
            }
            else if (add_dp)
                *s++ = '.';
        }
        else {
            while (decimal_point-- > 0)
                *s++ = *p++;
            if (precision > 0 || add_dp)
                *s++ = '.';
        }
    }
    else {
        *s++ = *p++;
        if (precision > 0 || add_dp)
            *s++ = '.';
    }

    /* copy the rest of p, the NUL is NOT copied */
    while (*p)
        *s++ = *p++;

    if (format != 'f') {
        char temp[EXPONENT_LENGTH];     /* for exponent conversion */
        int t_len;
        BOOL exponent_is_negative;

        *s++ = format;          /* either e or E */
        decimal_point--;
        if (decimal_point != 0) {
            p = conv_10((long_int) decimal_point, FALSE, &exponent_is_negative, 
                        &temp[EXPONENT_LENGTH], &t_len);
            *s++ = exponent_is_negative ? '-' : '+';
            /* Make sure the exponent has at least 2 digits */
            if (t_len == 1)
                *s++ = '0';
            while (t_len--)
                *s++ = *p++;
        }
        else {
            *s++ = '+';
            *s++ = '0';
            *s++ = '0';
        }
    }

    *len = s - buf;
    return (buf);
}

/*
 * Convert num to a base X number where X is a power of 2. nbits determines X.
 * For example, if nbits is 3, we do base 8 conversion
 * Return value:
 *      a pointer to a string containing the number
 * The caller provides a buffer for the string: that is the buf_end
 * argument which is a pointer to the END of the buffer + 1 (i.e. if the
 * buffer is declared as buf[100], buf_end should be &buf[100]) As with
 * conv_10, we have a faster version which is used when the number isn't
 * quad size.
 */

static const char low_digits[]   = "0123456789abcdef";
static const char upper_digits[] = "0123456789ABCDEF";

static char *
conv_p2(
    register u_long_int num, 
    register int nbits,
    char format, 
    char *buf_end, 
    register int *len)
{
    register int mask = (1 << nbits) - 1;
    register char *p = buf_end;
    register const char *digits = (format == 'X') ? upper_digits : low_digits;

    do {
        *--p = digits[num & mask];
        num >>= nbits;
    } while (num);
    *len = buf_end - p;
    return (p);
}

static char *
conv_p2_quad(
    u_quad_int num, 
    register int nbits,
    char format, 
    char *buf_end, 
    register int *len)
{
    register int mask = (1 << nbits) - 1;
    register char *p = buf_end;
    register const char *digits = (format == 'X') ? upper_digits : low_digits;

    do {
        *--p = digits[num & mask];
        num >>= nbits;
    } while (num);
    *len = buf_end - p;
    return (p);
}

/* 
 * str_vformat(), the generic printf-style formatting routine
 * and heart of this piece of source.
 */
int 
str_vformat(
    str_vformat_t *vbuff, 
    const char *fmt, 
    va_list ap)
{
    register char *sp;
    register char *bep;
    register int cc = 0;
    register int i;

    register char *s = NULL;
		String_t S = NULL;
		Num_t N;
    char *q;
    int s_len;

    register int min_width = 0;
    int precision = 0;
    enum { LEFT, RIGHT } adjust;
    char pad_char;
    char prefix_char;

    double fp_num;
    quad_int i_quad = (quad_int)0;
    u_quad_int ui_quad;
    long_int i_num = (long_int)0;
    u_long_int ui_num;

    char num_buf[NUM_BUF_SIZE];
    char char_buf[2]; /* for printing %% and %<unknown> */

    enum var_type_enum { IS_QUAD, IS_LONG, IS_SHORT, IS_INT };
    enum var_type_enum var_type = IS_INT;

    BOOL alternate_form;
    BOOL print_sign;
    BOOL print_blank;
    BOOL adjust_precision;
    BOOL adjust_width;
    BOOL is_negative;

    char extinfo[20];
    sp = vbuff->curpos;
    bep = vbuff->endpos;

    while (*fmt != NUL) {
        if (*fmt != '%') {
            INS_CHAR(*fmt, sp, bep, cc);
        }
        else {
            /*
             * Default variable settings
             */
            adjust = RIGHT;
            alternate_form = print_sign = print_blank = FALSE;
            pad_char = ' ';
            prefix_char = NUL;
            extinfo[0] = NUL;

            fmt++;

            /*
             * Try to avoid checking for flags, width or precision
             */
            if (!str_islower(*fmt)) {
                /*
                 * Recognize flags: -, #, BLANK, +
                 */
                for (;; fmt++) {
                    if (*fmt == '{') {
                        i = 0;
                        for (fmt++; *fmt != '}' && *fmt != NUL; fmt++) {
                            if (i < sizeof(extinfo)-1)
                                extinfo[i++] = *fmt;
                        }
                        extinfo[i] = NUL;
                    }
                    else if (*fmt == '-')
                        adjust = LEFT;
                    else if (*fmt == '+')
                        print_sign = TRUE;
                    else if (*fmt == '#')
                        alternate_form = TRUE;
                    else if (*fmt == ' ')
                        print_blank = TRUE;
                    else if (*fmt == '0')
                        pad_char = '0';
                    else
                        break;
                }

                /*
                 * Check if a width was specified
                 */
                if (str_isdigit(*fmt)) {
                    STR_TO_DEC(fmt, min_width);
                    adjust_width = TRUE;
                }
                else if (*fmt == '*') {
                    min_width = va_arg(ap, int);
                    fmt++;
                    adjust_width = TRUE;
                    if (min_width < 0) {
                        adjust = LEFT;
                        min_width = -min_width;
                    }
                }
                else
                    adjust_width = FALSE;

                /*
                 * Check if a precision was specified
                 *
                 * XXX: an unreasonable amount of precision may be specified
                 * resulting in overflow of num_buf. Currently we
                 * ignore this possibility.
                 */
                if (*fmt == '.') {
                    adjust_precision = TRUE;
                    fmt++;
                    if (str_isdigit(*fmt)) {
                        STR_TO_DEC(fmt, precision);
                    }
                    else if (*fmt == '*') {
                        precision = va_arg(ap, int);
                        fmt++;
                        if (precision < 0)
                            precision = 0;
                    }
                    else
                        precision = 0;
                }
                else
                    adjust_precision = FALSE;
            }
            else
                adjust_precision = adjust_width = FALSE;

            /*
             * Modifier check
             */
            if (*fmt == 'q') {
                var_type = IS_QUAD;
                fmt++;
            }
            else if (*fmt == 'l') {
                var_type = IS_LONG;
                fmt++;
            }
            else if (*fmt == 'h') {
                var_type = IS_SHORT;
                fmt++;
            }
            else {
                var_type = IS_INT;
            }

            /*
             * Argument extraction and printing.
             * First we determine the argument type.
             * Then, we convert the argument to a string.
             * On exit from the switch, s points to the string that
             * must be printed, s_len has the length of the string
             * The precision requirements, if any, are reflected in s_len.
             *
             * NOTE: pad_char may be set to '0' because of the 0 flag.
             *   It is reset to ' ' by non-numeric formats
             */
            switch (*fmt) {

                /* Unsigned Decimal Integer */
                case 'u':
                    if (var_type == IS_QUAD) {
                        i_quad = va_arg(ap, u_quad_int);
                        s = conv_10_quad(i_quad, 1, &is_negative,
                                         &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    else {
                        if (var_type == IS_LONG)
                            i_num = (long_int)va_arg(ap, u_long_int);
                        else if (var_type == IS_SHORT)
                            i_num = (long_int)(unsigned short)va_arg(ap, unsigned int);
                        else
                            i_num = (long_int)va_arg(ap, unsigned int);
                        s = conv_10(i_num, 1, &is_negative,
                                    &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    FIX_PRECISION(adjust_precision, precision, s, s_len);
                    break;

                /* Signed Decimal Integer */
						    case 'D':
									{
									  N = va_arg(ap, Num_t);
										s = NULL;
										if(TB_VALID(S, TB_NUM)) s = N2sz(N);
										if (s != NULL) {
											s_len = strlen(s);
											if (adjust_precision && precision < s_len)
												s_len = precision;
										} else {
											s = S_NULL;
											s_len = S_NULL_LEN;
										}
										pad_char = ' ';
									}
									break;

                case 'd':
                case 'i':
                    if (var_type == IS_QUAD) {
                        i_quad = va_arg(ap, quad_int);
                        s = conv_10_quad(i_quad, 0, &is_negative,
                                         &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    else {
                        if (var_type == IS_LONG)
                            i_num = (long_int)va_arg(ap, long_int);
                        else if (var_type == IS_SHORT)
                            i_num = (long_int)(short)va_arg(ap, int);
                        else
                            i_num = (long_int)va_arg(ap, int);
                        s = conv_10(i_num, 0, &is_negative,
                                    &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    FIX_PRECISION(adjust_precision, precision, s, s_len);

                    if (is_negative)
                        prefix_char = '-';
                    else if (print_sign)
                        prefix_char = '+';
                    else if (print_blank)
                        prefix_char = ' ';
                    break;

                /* Unsigned Octal Integer */
                case 'o':
                    if (var_type == IS_QUAD) {
                        ui_quad = va_arg(ap, u_quad_int);
                        s = conv_p2_quad(ui_quad, 3, *fmt,
                                         &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    else {
                        if (var_type == IS_LONG)
                            ui_num = (u_long_int) va_arg(ap, u_long_int);
                        else if (var_type == IS_SHORT)
                            ui_num = (u_long_int)(unsigned short)va_arg(ap, unsigned int);
                        else
                            ui_num = (u_long_int)va_arg(ap, unsigned int);
                        s = conv_p2(ui_num, 3, *fmt, &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    FIX_PRECISION(adjust_precision, precision, s, s_len);
                    if (alternate_form && *s != '0') {
                        *--s = '0';
                        s_len++;
                    }
                    break;

                /* Unsigned Hexadecimal Integer */
                case 'x':
                case 'X':
                    if (var_type == IS_QUAD) {
                        ui_quad = va_arg(ap, u_quad_int);
                        s = conv_p2_quad(ui_quad, 4, *fmt,
                                         &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    else {
                        if (var_type == IS_LONG)
                            ui_num = (u_long_int)va_arg(ap, u_long_int);
                        else if (var_type == IS_SHORT)
                            ui_num = (u_long_int)(unsigned short)va_arg(ap, unsigned int);
                        else
                            ui_num = (u_long_int)va_arg(ap, unsigned int);
                        s = conv_p2(ui_num, 4, *fmt, &num_buf[NUM_BUF_SIZE], &s_len);
                    }
                    FIX_PRECISION(adjust_precision, precision, s, s_len);
                    if (alternate_form && i_num != 0) {
                        *--s = *fmt; /* 'x' or 'X' */
                        *--s = '0';
                        s_len += 2;
                    }
                    break;
								/* String_t */		
						    case 'S' :
									S = va_arg(ap, String_t);
									s = S2sz(S);
									if (s != NULL) {
										s_len = strlen(s);
										if (adjust_precision && precision < s_len)
											s_len = precision;
									} else {
										s = S_NULL;
										s_len = S_NULL_LEN;
									}
									pad_char = ' ';
									break;

                /* String */							
                case 's':
                    s = va_arg(ap, char *);
                    if (s != NULL) {
                        s_len = strlen(s);
                        if (adjust_precision && precision < s_len)
                            s_len = precision;
                    }
                    else {
                        s = S_NULL;
                        s_len = S_NULL_LEN;
                    }
                    pad_char = ' ';
                    break;

                /* Double Floating Point (style 1) */
                case 'f':
                case 'e':
                case 'E':
                    fp_num = va_arg(ap, double);
                    /* We use &num_buf[ 1 ], so that we have room for the sign */
                    s = conv_fp(*fmt, fp_num, alternate_form,
                                (adjust_precision == FALSE) ?  FLOAT_DIGITS : precision, 
                                &is_negative, &num_buf[1], &s_len);
                    if (is_negative)
                        prefix_char = '-';
                    else if (print_sign)
                        prefix_char = '+';
                    else if (print_blank)
                        prefix_char = ' ';
                    break;

                /* Double Floating Point (style 2) */
                case 'g':
                case 'G':
                    if (adjust_precision == FALSE)
                        precision = FLOAT_DIGITS;
                    else if (precision == 0)
                        precision = 1;
                    /* We use &num_buf[ 1 ], so that we have room for the sign */
                    s = str_gcvt(va_arg(ap, double), precision, &num_buf[1],
                                 alternate_form);
                    if (*s == '-')
                        prefix_char = *s++;
                    else if (print_sign)
                        prefix_char = '+';
                    else if (print_blank)
                        prefix_char = ' ';
                    s_len = strlen(s);
                    if (alternate_form && (q = str_span(s, 0, ".", 0)) == NULL) {
                        s[s_len++] = '.';
                        s[s_len] = NUL; /* delimit for following str_span() */
                    }
                    if (*fmt == 'G' && (q = str_span(s, 0, "e", 0)) != NULL)
                        *q = 'E';
                    break;

                /* Single Character */
                case 'c':
                    char_buf[0] = (char) (va_arg(ap, int));
                    s = &char_buf[0];
                    s_len = 1;
                    pad_char = ' ';
                    break;

                /* The '%' Character */
                case '%':
                    char_buf[0] = '%';
                    s = &char_buf[0];
                    s_len = 1;
                    pad_char = ' ';
                    break;

                /* Special: Number of already written characters */
                case 'n':
                    if (var_type == IS_QUAD)
                        *(va_arg(ap, quad_int *)) = cc;
                    else if (var_type == IS_LONG)
                        *(va_arg(ap, long *)) = cc;
                    else if (var_type == IS_SHORT)
                        *(va_arg(ap, short *)) = cc;
                    else
                        *(va_arg(ap, int *)) = cc;
                    break;

                /*
                 * Pointer argument type. 
                 */
                case 'p':
#if defined(SIZEOF_LONG_LONG) && (SIZEOF_LONG_LONG == SIZEOF_VOID_P)
                    ui_quad = (u_quad_int) va_arg(ap, void *);
                    s = conv_p2_quad(ui_quad, 4, 'x', &num_buf[NUM_BUF_SIZE], &s_len);
#else
                    ui_num = (u_long_int) va_arg(ap, void *);
                    s = conv_p2(ui_num, 4, 'x', &num_buf[NUM_BUF_SIZE], &s_len);
#endif
                    pad_char = ' ';
                    break;

                case NUL:
                    /*
                     * The last character of the format string was %.
                     * We ignore it.
                     */
                    continue;

                /*
                 * The default case is for unrecognized %'s. We print
                 * %<char> to help the user identify what option is not
                 * understood. This is also useful in case the user
                 * wants to pass the output of format_converter to
                 * another function that understands some other %<char>
                 * (like syslog). Note that we can't point s inside fmt
                 * because the unknown <char> could be preceded by width
                 * etc.
                 */
                default:
                    s = NULL;
                    if (vbuff->format != NULL) {
                        s = vbuff->format(vbuff, 
                            &prefix_char, &pad_char, &s_len, 
                            num_buf, NUM_BUF_SIZE, extinfo, *fmt, ap);
                    }
                    if (s == NULL) {
                        char_buf[0] = '%';
                        char_buf[1] = *fmt;
                        s = char_buf;
                        s_len = 2;
                        pad_char = ' ';
                    }
                    break;
            }

            if (prefix_char != NUL && s != S_NULL && s != char_buf) {
                *--s = prefix_char;
                s_len++;
            }

            if (adjust_width && adjust == RIGHT && min_width > s_len) {
                if (pad_char == '0' && prefix_char != NUL) {
                    INS_CHAR(*s, sp, bep, cc);
                    s++;
                    s_len--;
                    min_width--;
                }
                PAD(min_width, s_len, pad_char);
            }

            /*
             * Print the string s. 
             */
            for (i = s_len; i != 0; i--) {
                INS_CHAR(*s, sp, bep, cc);
                s++;
            }

            if (adjust_width && adjust == LEFT && min_width > s_len)
                PAD(min_width, s_len, pad_char);
        }
        fmt++;
    }
    vbuff->curpos = sp;
    return cc;
}

/*
 * str_format -- format a new string.
 * This is inspired by POSIX sprintf(3), but mainly provides the
 * following differences: first it is actually a snprintf(3) style, i.e.
 * it allows one to specify the maximum number of characters which are
 * allowed to write. Second, it allows one to just count the number of
 * characters which have to be written.
 */

#define STR_FORMAT_BUFLEN 20

static int str_flush_fake(str_vformat_t *out_handle)
{
    out_handle->data[1].i += out_handle->data[2].i;
    out_handle->curpos = (char *)out_handle->data[0].p;
    return 0;
}

static int str_flush_real(str_vformat_t *out_handle)
{
    return -1;
}

int str_format_va(char *s, str_size_t n, const char *fmt, va_list ap)
{
    str_vformat_t handle;
    char buf[STR_FORMAT_BUFLEN];
    int rv;

    if (n == 0)
        return 0;
    if (s == NULL) {
        /* fake formatting, i.e., calculate output length only */
        handle.curpos    = buf;
        handle.endpos    = buf + sizeof(buf) - 1;
        handle.flush     = str_flush_fake;
        handle.format    = NULL;
        handle.data[0].p = buf;
        handle.data[1].i = 0;
        handle.data[2].i = sizeof(buf);
        rv = str_vformat(&handle, fmt, ap);
        if (rv == -1)
            rv = n;
    }
    else {
        /* real formatting, i.e., create output */
        handle.curpos  = s;
        handle.endpos  = s + n - 1;
        handle.flush   = str_flush_real;
        handle.format  = NULL;
        rv = str_vformat(&handle, fmt, ap);
        *(handle.curpos) = NUL;
        if (rv == -1)
            rv = n;
    }
    return rv;
}

int str_format(char *s, str_size_t n, const char *fmt, ...)
{
	int rc;
	va_list ap;

	va_start(ap, fmt);
	rc = str_format_va(s, n, fmt, ap);
	va_end(ap);
	return rc;
}

/** tb_vnasprintf, tb_nasprintf, tb_vasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf - String formatting family.
 *
 * Allocate formated string with variable arg number
 * 
 * Example 1: 
 * \code
 * char *s;
 * int rc;
 * 
 * rc = tb_asprintf(&s, "%-10.8s:%02d", "titi", 8);
 * printf(s);
 * free(s);
 * \endcode
 *
 * format is fully printf compliant (see stdio.h). Toolbox extends standard formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 *
 * Example 2:
 * \code
 * String_t S = tb_String("toto");
 * Num_t    N = tb_Num(55);
 * char *s;
 * tb_asprintf(&s, "%D:<%S>", S, N);   
 * \endcode
 *
 * @param s : pointer to buffer to be allocated
 * @param fmt : format
 * @param ... : variadic args (see stdargs.h)
 * @return strlen size (size without trailing 0x00)
 * @see tb_vnasprintf, tb_nasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf
 * \ingroup String
 */
int tb_asprintf(char **s, const char *fmt, ...) {
	int rc;
	va_list ap;

	va_start(ap, fmt);
	rc = str_format_va(NULL, INT_MAX, fmt, ap);
	*s = calloc(1, rc+1);
	va_start(ap, fmt);
	rc = str_format_va(*s, INT_MAX, fmt, ap);
	va_end(ap);

	return rc;
}

int __tb_asprintf(char **s, const char *fmt, ...) {
	int rc;
	va_list ap;

	va_start(ap, fmt);
	rc = str_format_va(NULL, INT_MAX, fmt, ap);
	*s = tb_xcalloc(1, rc+1);
	va_start(ap, fmt);
	rc = str_format_va(*s, INT_MAX, fmt, ap);
	va_end(ap);

	return rc;
}


/** tb_vnasprintf, tb_nasprintf, tb_vasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf - String formatting family.
 *
 * Allocate formated string with variable arg number
 *
 * Example 1: 
 * \code
 * char *s;
 * int rc;
 * 
 * rc = tb_vasprintf(&s, "%-10.8s:%02d", my_variable_list_of_args);
 * printf(s);
 * free(s);
 * \endcode
 *
 * format is fully printf compliant (see stdio.h). Toolbox extends standard formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 *
 * Example 2:
 * \code
 * String_t S = tb_String("toto");
 * Num_t    N = tb_Num(55);
 * char *s;
 * tb_asprintf(&s, "%D:<%S>", S, N);   
 * \endcode
 * @param s : pointer to buffer to be allocated
 * @param fmt : format
 * @param ap : va_list (see stdargs.h)
 * @return strlen size (size without trailing \0)
 *
 * @see tb_vnasprintf, tb_nasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf
 * \ingroup String
 */
int tb_vasprintf(char **s, const char *fmt, va_list ap) {
	int rc;
	rc = str_format_va(NULL, INT_MAX, fmt, ap);
	*s = calloc(1, rc+1);
	rc = str_format_va(*s, INT_MAX, fmt, ap);

	return rc;
}

int __tb_vasprintf(char **s, const char *fmt, va_list ap) {
	int rc;
	rc = str_format_va(NULL, INT_MAX, fmt, ap);
	*s = tb_xcalloc(1, rc+1);
	rc = str_format_va(*s, INT_MAX, fmt, ap);

	return rc;
}


/** tb_vnasprintf, tb_nasprintf, tb_vasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf - String formatting family.
 * @ingroup String
 * Allocate formated string with variable arg number and max size
 * Example 1: 
 * \code
 * char *s;
 * int rc;
 * 
 * rc = tb_nasprintf(&s, 20, "%-10.8s:%02d", "titi", 56);
 * printf(s);
 * free(s);
 * \endcode
 * format is fully printf compliant (see stdio.h). Toolbox extends standard formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 * 
 * Example 2:
 * \code
 * String_t S = tb_String("toto");
 * Num_t    N = tb_Num(55);
 * char *s;
 * tb_asprintf(&s, "%D:<%S>", S, N);   
 * \endcode

 * @param s : pointer to buffer to be allocated
 * @param max : max length of new buffer
 * @param fmt : format
 * @param ... : variadic args (see stdargs.h)
 * @return strlen size
 * see tb_vnasprintf, tb_vasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf
 * \ingroup String
 */
int tb_nasprintf(char **s, unsigned int max, const char *fmt, ...) {
	int rc;
	va_list ap;

	va_start(ap, fmt);
	rc = str_format_va(NULL, max, fmt, ap);
	*s = calloc(1, rc+1);
	va_start(ap, fmt);
	rc = str_format_va(*s, max, fmt, ap);
	va_end(ap);

	return rc;
}


int __tb_nasprintf(char **s, unsigned int max, const char *fmt, ...) {
	int rc;
	va_list ap;

	va_start(ap, fmt);
	rc = str_format_va(NULL, max, fmt, ap);
	*s = tb_xcalloc(1, rc+1);
	va_start(ap, fmt);
	rc = str_format_va(*s, max, fmt, ap);
	va_end(ap);

	return rc;
}

/** 
 * tb_vnasprintf, tb_nasprintf, tb_vasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf - String formatting family.
 * \ingroup String
 *
 * Allocate formated string with variable arg number and max size
 *
 * Example 1: 
 * \code
 * char *s;
 *
 * int rc;
 *
 * rc = tb_vnasprintf(&s, 20, "%-10.8s:%02d", my_va_list);
 *
 * printf(s);
 * free(s);
 * \endcode
 *
 * format:
 * format is fully printf compliant (see stdio.h). Toolbox extends standard formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 * 
 * Example 2:
 * \code
 * String_t S = tb_String("toto");
 * Num_t N = tb_Num(55); 
 * char *s;
 * 
 * tb_asprintf(&s, "%D:<%S>", S, N);   
 * \endcode
 *
 * @param s : pointer to buffer to be allocated
 * @param max : max length for new buffer
 * @param fmt : format (see formats)
 * @param ap : va_list (see stdargs.h)
 * @return strlen size (size without trailing 0x00)
 * @see tb_nasprintf, tb_vasprintf, tb_asprintf, tb_vsnprintf, tb_snprintf
 * \todo add replacement for [f]printf
 */
int tb_vnasprintf(char **s, unsigned int max, const char *fmt, va_list ap) {
	int rc;
	rc = str_format_va(NULL, max, fmt, ap);

	//	fprintf(stderr, "tb_vnasprintf: need %d bytes\n", rc);

	*s = calloc(1, rc+1);
	rc = str_format_va(*s, max, fmt, ap);
	//	fprintf(stderr, "tb_vnasprintf: done\n");

	return rc;
}

int __tb_vnasprintf(char **s, unsigned int max, const char *fmt, va_list ap) {
	int rc;
	rc = str_format_va(NULL, max, fmt, ap);

	//	fprintf(stderr, "tb_vnasprintf: need %d bytes\n", rc);

	*s = tb_xcalloc(1, rc+1);
	rc = str_format_va(*s, max, fmt, ap);
	//	fprintf(stderr, "tb_vnasprintf: done\n");

	return rc;
}


/**  portable replacement for snprintf (GNU extension to ANSI/POSIX C).
 * \ingroup String
 */
int tb_snprintf(char *str, size_t size, const  char  *format,...) {
	va_list ap;
	int rc;
	va_start(ap, format);
	rc = str_format_va(str, size, format, ap);
	va_end(ap);
	return rc;
}

/**  portable replacement for vsnprintf (GNU extension to ANSI/POSIX C).
 * \ingroup String
 */
int tb_vsnprintf(char *str, size_t size, const  char  *format, va_list ap) {
	int rc;
	rc = str_format_va(str, size, format, ap);
	return rc;
}

#ifdef TEST
int main(int argc, char argv) {

	int rc;
	int     i     = 255;
	char   *s,t[] = "sdjncksqjdnfdskjn";
	double  d     = 2.569;
	/*	
	rc = str_format(NULL, 256, "abababab [%s] %d %f\n", 
									t, i, d);

	fprintf(stderr, "need %d chars\n", rc);
	s = tb_xcalloc(1, rc);

	rc = str_format(s, 256, "abababab [%s] %d %f\n", 
									t, i, d);

	fprintf(stderr, "rc:%d <%s>\n", rc, s);
	*/

	fprintf(stderr, "<%s>\n", tb_asprintf(&s, "abababab [%s] %d %f\n",t, i, d));


	return 0;
}
#endif

/*
 * str_copy -- copy a string.
 * This is inspired by POSIX strncpy(3), but the source and target
 * and overlap and the the target is always NUL-terminated.
 */
char *str_copy(char *as, const char *at, str_size_t n)
{
    register char *s;
    register const char *t;
    char *rv;

    if (as == NULL || at == NULL)
        return NULL;
    if (n == 0) 
        n = strlen(at);
    t = at;
    s = as;
    rv = as;
    if (s > t) { 
        /* must go high to low */
        t += n - 1;
        s += n;
        rv = s;
        *s-- = NUL;
        while (n-- > 0)
            *s-- = *t--;
    }
    else if (s < t) {
        /* must go low to high */
        while (n-- > 0)
            *s++ = *t++;
        *s = NUL;
        rv = s;
    }
    return rv;
}


/*
 * str_span -- span over a set of character in a string.
 * This is a spanning function inspired by POSIX strchr(3) and
 * strrchr(3). But it is more flexible, because it provides
 * left-to-right and right-to-left spanning combined with either
 * performing positive or negative charsets. Additionally it allows one
 * to restrict the number of maximum spanned characters.
 */
char *str_span(const char *s, str_size_t n, const char *charset, int mode)
{
    register const char *sp;
    register const char *cp;
    register char sc, cc;
    char *rv;

    if (s == NULL || charset == NULL)
        return NULL;
    if (n == 0)
        n = strlen(s);
    rv = NULL;
    if (!(mode & STR_RIGHT) && !(mode & STR_COMPLEMENT)) {
        /* span from left to right while chars are in charset */
        sp = s; 
        l1:
        sc = *sp++; n--;
        if (sc != NUL && n >= 0) {
            for (cp = charset; (cc = *cp++) != NUL; )
                if (sc == cc)
                    goto l1;
        }
        rv = (char *)(sp - 1);
    }
    else if ((mode & STR_RIGHT) && !(mode & STR_COMPLEMENT)) {
        /* span from right to left while chars are in charset */
        sp = s;
        while (*sp != NUL && n > 0)
            sp++, n--;
        if (sp > s)
            sp--;
        l2:
        sc = *sp--;
        if (sp >= s) {
            for (cp = charset; (cc = *cp++) != NUL; )
                if (sc == cc)
                    goto l2;
        }
        rv = (char *)(sp + 1);
    }
    else if (!(mode & STR_RIGHT) && (mode & STR_COMPLEMENT)) {
        /* span from left to right while chars are NOT in charset */
        sp = s; 
        l3a:
        sc = *sp++; n--;
        if (sc != NUL && n >= 0) {
            for (cp = charset; (cc = *cp++) != NUL; )
                if (sc == cc)
                    goto l3;
            goto l3a;
        }
        l3:
        rv = (char *)(sp - 1);
    }
    else if ((mode & STR_RIGHT) && (mode & STR_COMPLEMENT)) {
        /* span from right to left while chars are NOT in charset */
        sp = s;
        while (*sp != NUL && n > 0)
            sp++, n--;
        if (sp > s)
            sp--;
        l4a:
        sc = *sp--;
        if (sp >= s) {
            for (cp = charset; (cc = *cp++) != NUL; )
                if (sc == cc)
                    goto l4;
            goto l4a;
        }
        l4:
        rv = (char *)(sp + 1);
    }
    return rv;
}

