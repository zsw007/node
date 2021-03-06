/* crypto/md32_common.h */
/* ====================================================================
 * Copyright (c) 1999-2007 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "\x4f\x70\x65\x6e\x53\x53\x4c\x20\x54\x6f\x6f\x6c\x6b\x69\x74" and "\x4f\x70\x65\x6e\x53\x53\x4c\x20\x50\x72\x6f\x6a\x65\x63\x74" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "\x4f\x70\x65\x6e\x53\x53\x4c"
 *    nor may "\x4f\x70\x65\x6e\x53\x53\x4c" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

/*-
 * This is a generic 32 bit "collector" for message digest algorithms.
 * Whenever needed it collects input character stream into chunks of
 * 32 bit values and invokes a block function that performs actual hash
 * calculations.
 *
 * Porting guide.
 *
 * Obligatory macros:
 *
 * DATA_ORDER_IS_BIG_ENDIAN or DATA_ORDER_IS_LITTLE_ENDIAN
 *      this macro defines byte order of input stream.
 * HASH_CBLOCK
 *      size of a unit chunk HASH_BLOCK operates on.
 * HASH_LONG
 *      has to be at lest 32 bit wide, if it's wider, then
 *      HASH_LONG_LOG2 *has to* be defined along
 * HASH_CTX
 *      context structure that at least contains following
 *      members:
 *              typedef struct {
 *                      ...
 *                      HASH_LONG       Nl,Nh;
 *                      either {
 *                      HASH_LONG       data[HASH_LBLOCK];
 *                      unsigned char   data[HASH_CBLOCK];
 *                      };
 *                      unsigned int    num;
 *                      ...
 *                      } HASH_CTX;
 *      data[] vector is expected to be zeroed upon first call to
 *      HASH_UPDATE.
 * HASH_UPDATE
 *      name of "Update" function, implemented here.
 * HASH_TRANSFORM
 *      name of "Transform" function, implemented here.
 * HASH_FINAL
 *      name of "Final" function, implemented here.
 * HASH_BLOCK_DATA_ORDER
 *      name of "block" function capable of treating *unaligned* input
 *      message in original (data) byte order, implemented externally.
 * HASH_MAKE_STRING
 *      macro convering context variables to an ASCII hash string.
 *
 * MD5 example:
 *
 *      #define DATA_ORDER_IS_LITTLE_ENDIAN
 *
 *      #define HASH_LONG               MD5_LONG
 *      #define HASH_LONG_LOG2          MD5_LONG_LOG2
 *      #define HASH_CTX                MD5_CTX
 *      #define HASH_CBLOCK             MD5_CBLOCK
 *      #define HASH_UPDATE             MD5_Update
 *      #define HASH_TRANSFORM          MD5_Transform
 *      #define HASH_FINAL              MD5_Final
 *      #define HASH_BLOCK_DATA_ORDER   md5_block_data_order
 *
 *                                      <appro@fy.chalmers.se>
 */

#include <openssl/crypto.h>

#if !defined(DATA_ORDER_IS_BIG_ENDIAN) && !defined(DATA_ORDER_IS_LITTLE_ENDIAN)
# error "\x44\x41\x54\x41\x5f\x4f\x52\x44\x45\x52\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif

#ifndef HASH_CBLOCK
# error "\x48\x41\x53\x48\x5f\x43\x42\x4c\x4f\x43\x4b\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif
#ifndef HASH_LONG
# error "\x48\x41\x53\x48\x5f\x4c\x4f\x4e\x47\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif
#ifndef HASH_CTX
# error "\x48\x41\x53\x48\x5f\x43\x54\x58\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif

#ifndef HASH_UPDATE
# error "\x48\x41\x53\x48\x5f\x55\x50\x44\x41\x54\x45\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif
#ifndef HASH_TRANSFORM
# error "\x48\x41\x53\x48\x5f\x54\x52\x41\x4e\x53\x46\x4f\x52\x4d\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif
#ifndef HASH_FINAL
# error "\x48\x41\x53\x48\x5f\x46\x49\x4e\x41\x4c\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif

#ifndef HASH_BLOCK_DATA_ORDER
# error "\x48\x41\x53\x48\x5f\x42\x4c\x4f\x43\x4b\x5f\x44\x41\x54\x41\x5f\x4f\x52\x44\x45\x52\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#endif

/*
 * Engage compiler specific rotate intrinsic function if available.
 */
#undef ROTATE
#ifndef PEDANTIC
# if defined(_MSC_VER)
#  define ROTATE(a,n)   _lrotl(a,n)
# elif defined(__ICC)
#  define ROTATE(a,n)   _rotl(a,n)
# elif defined(__MWERKS__)
#  if defined(__POWERPC__)
#   define ROTATE(a,n)  __rlwinm(a,n,0,31)
#  elif defined(__MC68K__)
    /* Motorola specific tweak. <appro@fy.chalmers.se> */
#   define ROTATE(a,n)  ( n<24 ? __rol(a,n) : __ror(a,32-n) )
#  else
#   define ROTATE(a,n)  __rol(a,n)
#  endif
# elif defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
  /*
   * Some GNU C inline assembler templates. Note that these are
   * rotates by *constant* number of bits! But that's exactly
   * what we need here...
   *                                    <appro@fy.chalmers.se>
   */
#  if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
#   define ROTATE(a,n)  ({ register unsigned int ret;   \
                                asm (                   \
                                "\x72\x6f\x6c\x6c\x20\x25\x31\x2c\x25\x30"            \
                                : "\x3d\x72"(ret)             \
                                : "\x49"(n), "\x30"((unsigned int)(a))        \
                                : "\x63\x63");                \
                           ret;                         \
                        })
#  elif defined(_ARCH_PPC) || defined(_ARCH_PPC64) || \
        defined(__powerpc) || defined(__ppc__) || defined(__powerpc64__)
#   define ROTATE(a,n)  ({ register unsigned int ret;   \
                                asm (                   \
                                "\x72\x6c\x77\x69\x6e\x6d\x20\x25\x30\x2c\x25\x31\x2c\x25\x32\x2c\x30\x2c\x33\x31"  \
                                : "\x3d\x72"(ret)             \
                                : "\x72"(a), "\x49"(n));      \
                           ret;                         \
                        })
#  elif defined(__s390x__)
#   define ROTATE(a,n) ({ register unsigned int ret;    \
                                asm ("\x72\x6c\x6c\x20\x25\x30\x2c\x25\x31\x2c\x25\x32"     \
                                : "\x3d\x72"(ret)             \
                                : "\x72"(a), "\x49"(n));      \
                          ret;                          \
                        })
#  endif
# endif
#endif                          /* PEDANTIC */

#ifndef ROTATE
# define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#endif

#if defined(DATA_ORDER_IS_BIG_ENDIAN)

# ifndef PEDANTIC
#  if defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
#   if ((defined(__i386) || defined(__i386__)) && !defined(I386_ONLY)) || \
      (defined(__x86_64) || defined(__x86_64__))
#    if !defined(B_ENDIAN)
    /*
     * This gives ~30-40% performance improvement in SHA-256 compiled
     * with gcc [on P4]. Well, first macro to be frank. We can pull
     * this trick on x86* platforms only, because these CPUs can fetch
     * unaligned data without raising an exception.
     */
#     define HOST_c2l(c,l)        ({ unsigned int r=*((const unsigned int *)(c)); \
                                   asm ("\x62\x73\x77\x61\x70\x6c\x20\x25\x30":"\x3d\x72"(r):"\x30"(r));    \
                                   (c)+=4; (l)=r;                       })
#     define HOST_l2c(l,c)        ({ unsigned int r=(l);                  \
                                   asm ("\x62\x73\x77\x61\x70\x6c\x20\x25\x30":"\x3d\x72"(r):"\x30"(r));    \
                                   *((unsigned int *)(c))=r; (c)+=4; r; })
#    endif
#   elif defined(__aarch64__)
#    if defined(__BYTE_ORDER__)
#     if defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
#      define HOST_c2l(c,l)      ({ unsigned int r;              \
                                   asm ("\x72\x65\x76\x20\x20\x20\x20\x25\x77\x30\x2c\x25\x77\x31"        \
                                        :"\x3d\x72"(r)                \
                                        :"\x72"(*((const unsigned int *)(c))));\
                                   (c)+=4; (l)=r;               })
#      define HOST_l2c(l,c)      ({ unsigned int r;              \
                                   asm ("\x72\x65\x76\x20\x20\x20\x20\x25\x77\x30\x2c\x25\x77\x31"        \
                                        :"\x3d\x72"(r)                \
                                        :"\x72"((unsigned int)(l)));\
                                   *((unsigned int *)(c))=r; (c)+=4; r; })
#     elif defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
#      define HOST_c2l(c,l)      ((l)=*((const unsigned int *)(c)), (c)+=4, (l))
#      define HOST_l2c(l,c)      (*((unsigned int *)(c))=(l), (c)+=4, (l))
#     endif
#    endif
#   endif
#  endif
#  if defined(__s390__) || defined(__s390x__)
#   define HOST_c2l(c,l) ((l)=*((const unsigned int *)(c)), (c)+=4, (l))
#   define HOST_l2c(l,c) (*((unsigned int *)(c))=(l), (c)+=4, (l))
#  endif
# endif

# ifndef HOST_c2l
#  define HOST_c2l(c,l)   (l =(((unsigned long)(*((c)++)))<<24),          \
                         l|=(((unsigned long)(*((c)++)))<<16),          \
                         l|=(((unsigned long)(*((c)++)))<< 8),          \
                         l|=(((unsigned long)(*((c)++)))    )           )
# endif
# ifndef HOST_l2c
#  define HOST_l2c(l,c)   (*((c)++)=(unsigned char)(((l)>>24)&0xff),      \
                         *((c)++)=(unsigned char)(((l)>>16)&0xff),      \
                         *((c)++)=(unsigned char)(((l)>> 8)&0xff),      \
                         *((c)++)=(unsigned char)(((l)    )&0xff),      \
                         l)
# endif

#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)

# ifndef PEDANTIC
#  if defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
#   if defined(__s390x__)
#    define HOST_c2l(c,l)        ({ asm ("\x6c\x72\x76\x20\x20\x20\x20\x25\x30\x2c\x25\x31"                  \
                                   :"\x3d\x64"(l) :"\x6d"(*(const unsigned int *)(c)));\
                                   (c)+=4; (l);                         })
#    define HOST_l2c(l,c)        ({ asm ("\x73\x74\x72\x76\x20\x20\x20\x25\x31\x2c\x25\x30"                  \
                                   :"\x3d\x6d"(*(unsigned int *)(c)) :"\x64"(l));\
                                   (c)+=4; (l);                         })
#   endif
#  endif
#  if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
#   ifndef B_ENDIAN
    /* See comment in DATA_ORDER_IS_BIG_ENDIAN section. */
#    define HOST_c2l(c,l)        ((l)=*((const unsigned int *)(c)), (c)+=4, l)
#    define HOST_l2c(l,c)        (*((unsigned int *)(c))=(l), (c)+=4, l)
#   endif
#  endif
# endif

# ifndef HOST_c2l
#  define HOST_c2l(c,l)   (l =(((unsigned long)(*((c)++)))    ),          \
                         l|=(((unsigned long)(*((c)++)))<< 8),          \
                         l|=(((unsigned long)(*((c)++)))<<16),          \
                         l|=(((unsigned long)(*((c)++)))<<24)           )
# endif
# ifndef HOST_l2c
#  define HOST_l2c(l,c)   (*((c)++)=(unsigned char)(((l)    )&0xff),      \
                         *((c)++)=(unsigned char)(((l)>> 8)&0xff),      \
                         *((c)++)=(unsigned char)(((l)>>16)&0xff),      \
                         *((c)++)=(unsigned char)(((l)>>24)&0xff),      \
                         l)
# endif

#endif

/*
 * Time for some action:-)
 */

int HASH_UPDATE(HASH_CTX *c, const void *data_, size_t len)
{
    const unsigned char *data = data_;
    unsigned char *p;
    HASH_LONG l;
    size_t n;

    if (len == 0)
        return 1;

    l = (c->Nl + (((HASH_LONG) len) << 3)) & 0xffffffffUL;
    /*
     * 95-05-24 eay Fixed a bug with the overflow handling, thanks to Wei Dai
     * <weidai@eskimo.com> for pointing it out.
     */
    if (l < c->Nl)              /* overflow */
        c->Nh++;
    c->Nh += (HASH_LONG) (len >> 29); /* might cause compiler warning on
                                       * 16-bit */
    c->Nl = l;

    n = c->num;
    if (n != 0) {
        p = (unsigned char *)c->data;

        if (len >= HASH_CBLOCK || len + n >= HASH_CBLOCK) {
            memcpy(p + n, data, HASH_CBLOCK - n);
            HASH_BLOCK_DATA_ORDER(c, p, 1);
            n = HASH_CBLOCK - n;
            data += n;
            len -= n;
            c->num = 0;
            /*
             * We use memset rather than OPENSSL_cleanse() here deliberately.
             * Using OPENSSL_cleanse() here could be a performance issue. It
             * will get properly cleansed on finalisation so this isn't a
             * security problem.
             */
            memset(p, 0, HASH_CBLOCK); /* keep it zeroed */
        } else {
            memcpy(p + n, data, len);
            c->num += (unsigned int)len;
            return 1;
        }
    }

    n = len / HASH_CBLOCK;
    if (n > 0) {
        HASH_BLOCK_DATA_ORDER(c, data, n);
        n *= HASH_CBLOCK;
        data += n;
        len -= n;
    }

    if (len != 0) {
        p = (unsigned char *)c->data;
        c->num = (unsigned int)len;
        memcpy(p, data, len);
    }
    return 1;
}

void HASH_TRANSFORM(HASH_CTX *c, const unsigned char *data)
{
    HASH_BLOCK_DATA_ORDER(c, data, 1);
}

int HASH_FINAL(unsigned char *md, HASH_CTX *c)
{
    unsigned char *p = (unsigned char *)c->data;
    size_t n = c->num;

    p[n] = 0x80;                /* there is always room for one */
    n++;

    if (n > (HASH_CBLOCK - 8)) {
        memset(p + n, 0, HASH_CBLOCK - n);
        n = 0;
        HASH_BLOCK_DATA_ORDER(c, p, 1);
    }
    memset(p + n, 0, HASH_CBLOCK - 8 - n);

    p += HASH_CBLOCK - 8;
#if   defined(DATA_ORDER_IS_BIG_ENDIAN)
    (void)HOST_l2c(c->Nh, p);
    (void)HOST_l2c(c->Nl, p);
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
    (void)HOST_l2c(c->Nl, p);
    (void)HOST_l2c(c->Nh, p);
#endif
    p -= HASH_CBLOCK;
    HASH_BLOCK_DATA_ORDER(c, p, 1);
    c->num = 0;
    OPENSSL_cleanse(p, HASH_CBLOCK);

#ifndef HASH_MAKE_STRING
# error "\x48\x41\x53\x48\x5f\x4d\x41\x4b\x45\x5f\x53\x54\x52\x49\x4e\x47\x20\x6d\x75\x73\x74\x20\x62\x65\x20\x64\x65\x66\x69\x6e\x65\x64\x21"
#else
    HASH_MAKE_STRING(c, md);
#endif

    return 1;
}

#ifndef MD32_REG_T
# if defined(__alpha) || defined(__sparcv9) || defined(__mips)
#  define MD32_REG_T long
/*
 * This comment was originaly written for MD5, which is why it
 * discusses A-D. But it basically applies to all 32-bit digests,
 * which is why it was moved to common header file.
 *
 * In case you wonder why A-D are declared as long and not
 * as MD5_LONG. Doing so results in slight performance
 * boost on LP64 architectures. The catch is we don't
 * really care if 32 MSBs of a 64-bit register get polluted
 * with eventual overflows as we *save* only 32 LSBs in
 * *either* case. Now declaring 'em long excuses the compiler
 * from keeping 32 MSBs zeroed resulting in 13% performance
 * improvement under SPARC Solaris7/64 and 5% under AlphaLinux.
 * Well, to be honest it should say that this *prevents*
 * performance degradation.
 *                              <appro@fy.chalmers.se>
 */
# else
/*
 * Above is not absolute and there are LP64 compilers that
 * generate better code if MD32_REG_T is defined int. The above
 * pre-processor condition reflects the circumstances under which
 * the conclusion was made and is subject to further extension.
 *                              <appro@fy.chalmers.se>
 */
#  define MD32_REG_T int
# endif
#endif
