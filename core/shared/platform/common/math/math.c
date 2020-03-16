/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include "platform_common.h"

#define __FDLIBM_STDC__

typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

typedef union u32double_tag {
    int *pint;
    double *pdouble;
} U32DOUBLE;

static inline int *
pdouble2pint(double *pdouble)
{
    U32DOUBLE u;
    u.pdouble = pdouble;
    return u.pint;
}

typedef union
{
    double value;
    struct
    {
        u_int32_t lsw;
        u_int32_t msw;
    } parts;
    struct
    {
        u_int64_t w;
    } xparts;
} ieee_double_shape_type_little;

typedef union
{
    double value;
    struct
    {
        u_int32_t msw;
        u_int32_t lsw;
    } parts;
    struct
    {
        u_int64_t w;
    } xparts;
} ieee_double_shape_type_big;

typedef union {
    double	d;
    struct {
        unsigned int	manl	:32;
        unsigned int	manh	:20;
        unsigned int	exp	:11;
        unsigned int	sign	:1;
    } bits;
} IEEEd2bits_L;

typedef union {
    double	d;
    struct {
        unsigned int	sign	:1;
        unsigned int	exp	:11;
        unsigned int	manh	:20;
        unsigned int	manl	:32;
    } bits;
} IEEEd2bits_B;

typedef union {
    float   f;
    struct {
        unsigned int    man :23;
        unsigned int    exp :8;
        unsigned int    sign    :1;
    } bits;
} IEEEf2bits_L;

typedef union {
    float   f;
    struct {
        unsigned int    sign    :1;
        unsigned int    exp :8;
        unsigned int    man :23;
    } bits;
} IEEEf2bits_B;

static union {
    int a;
    char b;
} __ue = { .a = 1 };

#define is_little_endian() (__ue.b == 1)

#define __HIL(x) *(1+pdouble2pint(&x))
#define __LOL(x) *(pdouble2pint(&x))
#define __HIB(x) *(pdouble2pint(&x))
#define __LOB(x) *(1+pdouble2pint(&x))

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS_L(ix0,ix1,d)				\
        do {								\
            ieee_double_shape_type_little ew_u;	                 	\
            ew_u.value = (d);						\
            (ix0) = ew_u.parts.msw;					\
            (ix1) = ew_u.parts.lsw;					\
        } while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS_L(d,ix0,ix1)				\
        do {								\
            ieee_double_shape_type_little iw_u;	                 	\
            iw_u.parts.msw = (ix0);					\
            iw_u.parts.lsw = (ix1);					\
            (d) = iw_u.value;						\
        } while (0)

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS_B(ix0,ix1,d)				\
        do {								\
            ieee_double_shape_type_big ew_u;	                 	\
            ew_u.value = (d);						\
            (ix0) = ew_u.parts.msw;					\
            (ix1) = ew_u.parts.lsw;					\
        } while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS_B(d,ix0,ix1)				\
        do {								\
            ieee_double_shape_type_big iw_u;	                 	\
            iw_u.parts.msw = (ix0);					\
            iw_u.parts.lsw = (ix1);					\
            (d) = iw_u.value;						\
        } while (0)

/* Get the more significant 32 bit int from a double.  */
#define GET_HIGH_WORD_L(i,d)					\
        do {								\
            ieee_double_shape_type_little gh_u;			        \
            gh_u.value = (d);						\
            (i) = gh_u.parts.msw;						\
        } while (0)

/* Get the more significant 32 bit int from a double.  */
#define GET_HIGH_WORD_B(i,d)					\
        do {								\
            ieee_double_shape_type_big gh_u;			        \
            gh_u.value = (d);						\
            (i) = gh_u.parts.msw;						\
        } while (0)

/* Set the more significant 32 bits of a double from an int.  */
#define SET_HIGH_WORD_L(d,v)					\
        do {								\
            ieee_double_shape_type_little sh_u;				\
            sh_u.value = (d);						\
            sh_u.parts.msw = (v);						\
            (d) = sh_u.value;						\
        } while (0)

/* Set the more significant 32 bits of a double from an int.  */
#define SET_HIGH_WORD_B(d,v)					\
        do {								\
            ieee_double_shape_type_big sh_u;				\
            sh_u.value = (d);						\
            sh_u.parts.msw = (v);						\
            (d) = sh_u.value;						\
        } while (0)

/*
 * A union which permits us to convert between a float and a 32 bit
 * int.
 */
typedef union
{
    float value;
    /* FIXME: Assumes 32 bit int.  */
    unsigned int word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */
#define GET_FLOAT_WORD(i,d)                 \
    do {                                    \
        ieee_float_shape_type gf_u;         \
        gf_u.value = (d);                   \
        (i) = gf_u.word;                    \
    } while (0)

/* Set a float from a 32 bit int.  */
#define SET_FLOAT_WORD(d,i)                 \
    do {                                    \
        ieee_float_shape_type sf_u;         \
        sf_u.word = (i);                    \
        (d) = sf_u.value;                   \
    } while (0)

/* Macro wrappers.  */
#define EXTRACT_WORDS(ix0,ix1,d) do {   \
    if (is_little_endian())             \
        EXTRACT_WORDS_L(ix0,ix1,d);     \
    else                                \
        EXTRACT_WORDS_B(ix0,ix1,d);     \
} while (0)

#define INSERT_WORDS(d,ix0,ix1) do {    \
    if (is_little_endian())             \
       INSERT_WORDS_L(d,ix0,ix1);       \
    else                                \
        INSERT_WORDS_B(d,ix0,ix1);      \
} while (0)

#define GET_HIGH_WORD(i,d)              \
    do {                                \
        if (is_little_endian())         \
        GET_HIGH_WORD_L(i,d);           \
        else                            \
        GET_HIGH_WORD_B(i,d);           \
    } while (0)

#define SET_HIGH_WORD(d,v)              \
    do {                                \
        if (is_little_endian())         \
        SET_HIGH_WORD_L(d,v);           \
        else                            \
        SET_HIGH_WORD_B(d,v);           \
    } while (0)

#define __HI(x) (is_little_endian() ? __HIL(x) : __HIB(x))

#define __LO(x) (is_little_endian() ? __LOL(x) : __LOB(x))

/*
 * Attempt to get strict C99 semantics for assignment with non-C99 compilers.
 */
#if FLT_EVAL_METHOD == 0 || __GNUC__ == 0
#define	STRICT_ASSIGN(type, lval, rval)	((lval) = (rval))
#else
#define	STRICT_ASSIGN(type, lval, rval) do {	\
        volatile type __lval;			\
        \
        if (sizeof(type) >= sizeof(long double))	\
        (lval) = (rval);		\
        else {					\
            __lval = (rval);		\
            (lval) = __lval;		\
        }					\
} while (0)
#endif

#ifdef __FDLIBM_STDC__
static const double huge = 1.0e300;
#else
static double huge = 1.0e300;
#endif

#ifdef __STDC__
static const double
#else
static double
#endif
tiny  = 1.0e-300;

#ifdef __STDC__
static const double
#else
static double
#endif
one=  1.00000000000000000000e+00; /* 0x3FF00000, 0x00000000 */

#ifdef __STDC__
static const double
#else
static double
#endif
TWO52[2]={
        4.50359962737049600000e+15, /* 0x43300000, 0x00000000 */
        -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

static double freebsd_sqrt(double x);
static double freebsd_floor(double x);
static double freebsd_ceil(double x);
static double freebsd_fabs(double x);
static double freebsd_rint(double x);
static int freebsd_isnan(double x);

static double freebsd_sqrt(double x)		/* wrapper sqrt */
{
    double z;
    int32_t sign = (int)0x80000000;
    int32_t ix0,s0,q,m,t,i;
    u_int32_t r,t1,s1,ix1,q1;

    EXTRACT_WORDS(ix0,ix1,x);

    /* take care of Inf and NaN */
    if((ix0&0x7ff00000)==0x7ff00000) {
        return x*x+x;		/* sqrt(NaN)=NaN, sqrt(+inf)=+inf
					   sqrt(-inf)=sNaN */
    }
    /* take care of zero */
    if(ix0<=0) {
        if(((ix0&(~sign))|ix1)==0) return x;/* sqrt(+-0) = +-0 */
        else if(ix0<0)
            return (x-x)/(x-x);		/* sqrt(-ve) = sNaN */
    }
    /* normalize x */
    m = (ix0>>20);
    if(m==0) {				/* subnormal x */
        while(ix0==0) {
            m -= 21;
            ix0 |= (ix1>>11); ix1 <<= 21;
        }
        for(i=0;(ix0&0x00100000)==0;i++) ix0<<=1;
        m -= i-1;
        ix0 |= (ix1>>(32-i));
        ix1 <<= i;
    }
    m -= 1023;	/* unbias exponent */
    ix0 = (ix0&0x000fffff)|0x00100000;
    if(m&1){	/* odd m, double x to make it even */
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
    }
    m >>= 1;	/* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix0 += ix0 + ((ix1&sign)>>31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0;	/* [q,q1] = sqrt(x) */
    r = 0x00200000;		/* r = moving bit from right to left */

    while(r!=0) {
        t = s0+r;
        if(t<=ix0) {
            s0   = t+r;
            ix0 -= t;
            q   += r;
        }
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
        r>>=1;
    }

    r = sign;
    while(r!=0) {
        t1 = s1+r;
        t  = s0;
        if((t<ix0)||((t==ix0)&&(t1<=ix1))) {
            s1  = t1+r;
            if(((t1&sign)==sign)&&(s1&sign)==0) s0 += 1;
            ix0 -= t;
            if (ix1 < t1) ix0 -= 1;
            ix1 -= t1;
            q1  += r;
        }
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
        r>>=1;
    }

    /* use floating add to find out rounding direction */
    if((ix0|ix1)!=0) {
        z = one-tiny; /* trigger inexact flag */
        if (z>=one) {
            z = one+tiny;
            if (q1==(u_int32_t)0xffffffff) { q1=0; q += 1;}
            else if (z>one) {
                if (q1==(u_int32_t)0xfffffffe) q+=1;
                q1+=2;
            } else
                q1 += (q1&1);
        }
    }
    ix0 = (q>>1)+0x3fe00000;
    ix1 =  q1>>1;
    if ((q&1)==1) ix1 |= sign;
    ix0 += (m <<20);

    INSERT_WORDS(z,ix0,ix1);

    return z;
}

static double freebsd_floor(double x)
{
    int32_t i0,i1,j0;
    u_int32_t i,j;

    EXTRACT_WORDS(i0,i1,x);

    j0 = ((i0>>20)&0x7ff)-0x3ff;
    if(j0<20) {
        if(j0<0) { 	/* raise inexact if x != 0 */
            if(huge+x>0.0) {/* return 0*sign(x) if |x|<1 */
                if(i0>=0) {i0=i1=0;}
                else if(((i0&0x7fffffff)|i1)!=0)
                { i0=0xbff00000;i1=0;}
            }
        } else {
            i = (0x000fffff)>>j0;
            if(((i0&i)|i1)==0) return x; /* x is integral */
            if(huge+x>0.0) {	/* raise inexact flag */
                if(i0<0) i0 += (0x00100000)>>j0;
                i0 &= (~i); i1=0;
            }
        }
    } else if (j0>51) {
        if(j0==0x400) return x+x;	/* inf or NaN */
        else return x;		/* x is integral */
    } else {
        i = ((u_int32_t)(0xffffffff))>>(j0-20);
        if((i1&i)==0) return x;	/* x is integral */
        if(huge+x>0.0) { 		/* raise inexact flag */
            if(i0<0) {
                if(j0==20) i0+=1;
                else {
                    j = i1+(1<<(52-j0));
                    if(j<i1) i0 +=1 ; 	/* got a carry */
                    i1=j;
                }
            }
            i1 &= (~i);
        }
    }

    INSERT_WORDS(x,i0,i1);

    return x;
}

static double freebsd_ceil(double x)
{
    int32_t i0,i1,j0;
    u_int32_t i,j;
    EXTRACT_WORDS(i0,i1,x);
    j0 = ((i0>>20)&0x7ff)-0x3ff;
    if(j0<20) {
        if(j0<0) { 	/* raise inexact if x != 0 */
            if(huge+x>0.0) {/* return 0*sign(x) if |x|<1 */
                if(i0<0) {i0=0x80000000;i1=0;}
                else if((i0|i1)!=0) { i0=0x3ff00000;i1=0;}
            }
        } else {
            i = (0x000fffff)>>j0;
            if(((i0&i)|i1)==0) return x; /* x is integral */
            if(huge+x>0.0) {	/* raise inexact flag */
                if(i0>0) i0 += (0x00100000)>>j0;
                i0 &= (~i); i1=0;
            }
        }
    } else if (j0>51) {
        if(j0==0x400) return x+x;	/* inf or NaN */
        else return x;		/* x is integral */
    } else {
        i = ((u_int32_t)(0xffffffff))>>(j0-20);
        if((i1&i)==0) return x;	/* x is integral */
        if(huge+x>0.0) { 		/* raise inexact flag */
            if(i0>0) {
                if(j0==20) i0+=1;
                else {
                    j = i1 + (1<<(52-j0));
                    if(j<i1) i0+=1;	/* got a carry */
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    INSERT_WORDS(x,i0,i1);
    return x;
}

static double freebsd_rint(double x)
{
    int32_t i0,j0,sx;
    u_int32_t i,i1;
    double w,t;
    EXTRACT_WORDS(i0,i1,x);
    sx = (i0>>31)&1;
    j0 = ((i0>>20)&0x7ff)-0x3ff;
    if(j0<20) {
        if(j0<0) {
            if(((i0&0x7fffffff)|i1)==0) return x;
            i1 |= (i0&0x0fffff);
            i0 &= 0xfffe0000;
            i0 |= ((i1|-i1)>>12)&0x80000;
            SET_HIGH_WORD(x,i0);
            STRICT_ASSIGN(double,w,TWO52[sx]+x);
            t =  w-TWO52[sx];
            GET_HIGH_WORD(i0,t);
            SET_HIGH_WORD(t,(i0&0x7fffffff)|(sx<<31));
            return t;
        } else {
            i = (0x000fffff)>>j0;
            if(((i0&i)|i1)==0) return x; /* x is integral */
            i>>=1;
            if(((i0&i)|i1)!=0) {
                /*
                 * Some bit is set after the 0.5 bit.  To avoid the
                 * possibility of errors from double rounding in
                 * w = TWO52[sx]+x, adjust the 0.25 bit to a lower
                 * guard bit.  We do this for all j0<=51.  The
                 * adjustment is trickiest for j0==18 and j0==19
                 * since then it spans the word boundary.
                 */
                if(j0==19) i1 = 0x40000000; else
                    if(j0==18) i1 = 0x80000000; else
                        i0 = (i0&(~i))|((0x20000)>>j0);
            }
        }
    } else if (j0>51) {
        if(j0==0x400) return x+x;	/* inf or NaN */
        else return x;		/* x is integral */
    } else {
        i = ((u_int32_t)(0xffffffff))>>(j0-20);
        if((i1&i)==0) return x;	/* x is integral */
        i>>=1;
        if((i1&i)!=0) i1 = (i1&(~i))|((0x40000000)>>(j0-20));
    }
    INSERT_WORDS(x,i0,i1);
    STRICT_ASSIGN(double,w,TWO52[sx]+x);
    return w-TWO52[sx];
}

static int freebsd_isnan(double d)
{
    if (is_little_endian()) {
        IEEEd2bits_L u;
        u.d = d;
        return (u.bits.exp == 2047 && (u.bits.manl != 0 || u.bits.manh != 0));
    }
    else {
        IEEEd2bits_B u;
        u.d = d;
        return (u.bits.exp == 2047 && (u.bits.manl != 0 || u.bits.manh != 0));
    }
}

static double freebsd_fabs(double x)
{
    u_int32_t high;
    GET_HIGH_WORD(high,x);
    SET_HIGH_WORD(x,high&0x7fffffff);
    return x;
}

static const float huge_f = 1.0e30F;

static const float
TWO23[2]={
    8.3886080000e+06, /* 0x4b000000 */
    -8.3886080000e+06, /* 0xcb000000 */
};

static float
freebsd_truncf(float x)
{
    int32_t i0,j0;
    u_int32_t i;
    GET_FLOAT_WORD(i0,x);
    j0 = ((i0>>23)&0xff)-0x7f;
    if(j0<23) {
        if(j0<0) {  /* raise inexact if x != 0 */
            if(huge_f+x>0.0F)     /* |x|<1, so return 0*sign(x) */
                i0 &= 0x80000000;
        } else {
            i = (0x007fffff)>>j0;
            if((i0&i)==0) return x; /* x is integral */
            if(huge_f+x>0.0F)     /* raise inexact flag */
                i0 &= (~i);
        }
    } else {
        if(j0==0x80) return x+x;    /* inf or NaN */
        else return x;      /* x is integral */
    }
    SET_FLOAT_WORD(x,i0);
    return x;
}

static float
freebsd_rintf(float x)
{
    int32_t i0,j0,sx;
    float w,t;
    GET_FLOAT_WORD(i0,x);
    sx = (i0>>31)&1;
    j0 = ((i0>>23)&0xff)-0x7f;
    if(j0<23) {
        if(j0<0) {
            if((i0&0x7fffffff)==0) return x;
            STRICT_ASSIGN(float,w,TWO23[sx]+x);
            t =  w-TWO23[sx];
            GET_FLOAT_WORD(i0,t);
            SET_FLOAT_WORD(t,(i0&0x7fffffff)|(sx<<31));
            return t;
        }
        STRICT_ASSIGN(float,w,TWO23[sx]+x);
        return w-TWO23[sx];
    }
    if(j0==0x80) return x+x;    /* inf or NaN */
    else return x;          /* x is integral */
}

static float
freebsd_ceilf(float x)
{
    int32_t i0,j0;
    u_int32_t i;

    GET_FLOAT_WORD(i0,x);
    j0 = ((i0>>23)&0xff)-0x7f;
    if(j0<23) {
        if(j0<0) {  /* raise inexact if x != 0 */
            if(huge_f+x>(float)0.0) {/* return 0*sign(x) if |x|<1 */
                if(i0<0) {i0=0x80000000;}
                else if(i0!=0) { i0=0x3f800000;}
            }
        } else {
            i = (0x007fffff)>>j0;
            if((i0&i)==0) return x; /* x is integral */
            if(huge_f+x>(float)0.0) { /* raise inexact flag */
                if(i0>0) i0 += (0x00800000)>>j0;
                i0 &= (~i);
            }
        }
    } else {
        if(j0==0x80) return x+x;    /* inf or NaN */
        else return x;      /* x is integral */
    }
    SET_FLOAT_WORD(x,i0);
    return x;
}

static float
freebsd_floorf(float x)
{
    int32_t i0,j0;
    u_int32_t i;
    GET_FLOAT_WORD(i0,x);
    j0 = ((i0>>23)&0xff)-0x7f;
    if(j0<23) {
        if(j0<0) {  /* raise inexact if x != 0 */
            if(huge_f+x>(float)0.0) {/* return 0*sign(x) if |x|<1 */
                if(i0>=0) {i0=0;}
                else if((i0&0x7fffffff)!=0)
                { i0=0xbf800000;}
            }
        } else {
            i = (0x007fffff)>>j0;
            if((i0&i)==0) return x; /* x is integral */
            if(huge_f+x>(float)0.0) { /* raise inexact flag */
                if(i0<0) i0 += (0x00800000)>>j0;
                i0 &= (~i);
            }
        }
    } else {
        if(j0==0x80) return x+x;    /* inf or NaN */
        else return x;      /* x is integral */
    }
    SET_FLOAT_WORD(x,i0);
    return x;
}

static float
freebsd_fminf(float x, float y)
{
    if (is_little_endian()) {
        IEEEf2bits_L u[2];

        u[0].f = x;
        u[1].f = y;

        /* Check for NaNs to avoid raising spurious exceptions. */
        if (u[0].bits.exp == 255 && u[0].bits.man != 0)
            return (y);
        if (u[1].bits.exp == 255 && u[1].bits.man != 0)
            return (x);

        /* Handle comparisons of signed zeroes. */
        if (u[0].bits.sign != u[1].bits.sign)
            return (u[u[1].bits.sign].f);
    }
    else {
        IEEEf2bits_B u[2];

        u[0].f = x;
        u[1].f = y;

        /* Check for NaNs to avoid raising spurious exceptions. */
        if (u[0].bits.exp == 255 && u[0].bits.man != 0)
            return (y);
        if (u[1].bits.exp == 255 && u[1].bits.man != 0)
            return (x);

        /* Handle comparisons of signed zeroes. */
        if (u[0].bits.sign != u[1].bits.sign)
            return (u[u[1].bits.sign].f);
    }

    return (x < y ? x : y);
}

static float
freebsd_fmaxf(float x, float y)
{
    if (is_little_endian()) {
        IEEEf2bits_L u[2];

        u[0].f = x;
        u[1].f = y;

        /* Check for NaNs to avoid raising spurious exceptions. */
        if (u[0].bits.exp == 255 && u[0].bits.man != 0)
            return (y);
        if (u[1].bits.exp == 255 && u[1].bits.man != 0)
            return (x);

        /* Handle comparisons of signed zeroes. */
        if (u[0].bits.sign != u[1].bits.sign)
            return (u[u[0].bits.sign].f);
    }
    else {
        IEEEf2bits_B u[2];

        u[0].f = x;
        u[1].f = y;

        /* Check for NaNs to avoid raising spurious exceptions. */
        if (u[0].bits.exp == 255 && u[0].bits.man != 0)
            return (y);
        if (u[1].bits.exp == 255 && u[1].bits.man != 0)
            return (x);

        /* Handle comparisons of signed zeroes. */
        if (u[0].bits.sign != u[1].bits.sign)
            return (u[u[0].bits.sign].f);
    }

    return (x > y ? x : y);
}

double sqrt(double x)
{
    return freebsd_sqrt(x);
}

double floor(double x)
{
    return freebsd_floor(x);
}

double ceil(double x)
{
    return freebsd_ceil(x);
}

double fmin(double x, double y)
{
    return x < y ? x : y;
}

double fmax(double x, double y)
{
    return x > y ? x : y;
}

double rint(double x)
{
    return freebsd_rint(x);
}

double fabs(double x)
{
    return freebsd_fabs(x);
}

int isnan(double x)
{
    return freebsd_isnan(x);
}

double trunc(double x)
{
    return (x > 0) ? freebsd_floor(x) : freebsd_ceil(x);
}

int signbit(double x)
{
    return ((__HI(x) & 0x80000000) >> 31);
}

float
truncf(float x)
{
    return freebsd_truncf(x);
}

float
rintf(float x)
{
    return freebsd_rintf(x);
}

float
ceilf(float x)
{
    return freebsd_ceilf(x);
}

float
floorf(float x)
{
    return freebsd_floorf(x);
}

float
fminf(float x, float y)
{
    return freebsd_fminf(x, y);
}

float
fmaxf(float x, float y)
{
    return freebsd_fmaxf(x, y);
}

