/* crypto/des/pcbc_enc.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "\x54\x68\x69\x73\x20\x70\x72\x6f\x64\x75\x63\x74\x20\x69\x6e\x63\x6c\x75\x64\x65\x73\x20\x73\x6f\x66\x74\x77\x61\x72\x65\x20\x77\x72\x69\x74\x74\x65\x6e\x20\x62\x79\x20\x54\x69\x6d\x20\x48\x75\x64\x73\x6f\x6e\x20\x28\x74\x6a\x68\x40\x63\x72\x79\x70\x74\x73\x6f\x66\x74\x2e\x63\x6f\x6d\x29"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
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
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include "des_locl.h"

void DES_pcbc_encrypt(const unsigned char *input, unsigned char *output,
                      long length, DES_key_schedule *schedule,
                      DES_cblock *ivec, int enc)
{
    register DES_LONG sin0, sin1, xor0, xor1, tout0, tout1;
    DES_LONG tin[2];
    const unsigned char *in;
    unsigned char *out, *iv;

    in = input;
    out = output;
    iv = &(*ivec)[0];

    if (enc) {
        c2l(iv, xor0);
        c2l(iv, xor1);
        for (; length > 0; length -= 8) {
            if (length >= 8) {
                c2l(in, sin0);
                c2l(in, sin1);
            } else
                c2ln(in, sin0, sin1, length);
            tin[0] = sin0 ^ xor0;
            tin[1] = sin1 ^ xor1;
            DES_encrypt1((DES_LONG *)tin, schedule, DES_ENCRYPT);
            tout0 = tin[0];
            tout1 = tin[1];
            xor0 = sin0 ^ tout0;
            xor1 = sin1 ^ tout1;
            l2c(tout0, out);
            l2c(tout1, out);
        }
    } else {
        c2l(iv, xor0);
        c2l(iv, xor1);
        for (; length > 0; length -= 8) {
            c2l(in, sin0);
            c2l(in, sin1);
            tin[0] = sin0;
            tin[1] = sin1;
            DES_encrypt1((DES_LONG *)tin, schedule, DES_DECRYPT);
            tout0 = tin[0] ^ xor0;
            tout1 = tin[1] ^ xor1;
            if (length >= 8) {
                l2c(tout0, out);
                l2c(tout1, out);
            } else
                l2cn(tout0, tout1, out, length);
            xor0 = tout0 ^ sin0;
            xor1 = tout1 ^ sin1;
        }
    }
    tin[0] = tin[1] = 0;
    sin0 = sin1 = xor0 = xor1 = tout0 = tout1 = 0;
}
