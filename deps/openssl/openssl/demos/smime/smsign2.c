/* S/MIME signing example: 2 signers. OpenSSL 0.9.9 only */
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/err.h>

int main(int argc, char **argv)
{
    BIO *in = NULL, *out = NULL, *tbio = NULL;
    X509 *scert = NULL, *scert2 = NULL;
    EVP_PKEY *skey = NULL, *skey2 = NULL;
    PKCS7 *p7 = NULL;
    int ret = 1;

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    tbio = BIO_new_file("\x73\x69\x67\x6e\x65\x72\x2e\x70\x65\x6d", "\x72");

    if (!tbio)
        goto err;

    scert = PEM_read_bio_X509(tbio, NULL, 0, NULL);

    BIO_reset(tbio);

    skey = PEM_read_bio_PrivateKey(tbio, NULL, 0, NULL);

    BIO_free(tbio);

    tbio = BIO_new_file("\x73\x69\x67\x6e\x65\x72\x32\x2e\x70\x65\x6d", "\x72");

    if (!tbio)
        goto err;

    scert2 = PEM_read_bio_X509(tbio, NULL, 0, NULL);

    BIO_reset(tbio);

    skey2 = PEM_read_bio_PrivateKey(tbio, NULL, 0, NULL);

    if (!scert2 || !skey2)
        goto err;

    in = BIO_new_file("\x73\x69\x67\x6e\x2e\x74\x78\x74", "\x72");

    if (!in)
        goto err;

    p7 = PKCS7_sign(NULL, NULL, NULL, in, PKCS7_STREAM | PKCS7_PARTIAL);

    if (!p7)
        goto err;

    /* Add each signer in turn */

    if (!PKCS7_sign_add_signer(p7, scert, skey, NULL, 0))
        goto err;

    if (!PKCS7_sign_add_signer(p7, scert2, skey2, NULL, 0))
        goto err;

    out = BIO_new_file("\x73\x6d\x6f\x75\x74\x2e\x74\x78\x74", "\x77");
    if (!out)
        goto err;

    /* NB: content included and finalized by SMIME_write_PKCS7 */

    if (!SMIME_write_PKCS7(out, p7, in, PKCS7_STREAM))
        goto err;

    ret = 0;

 err:

    if (ret) {
        fprintf(stderr, "\x45\x72\x72\x6f\x72\x20\x53\x69\x67\x6e\x69\x6e\x67\x20\x44\x61\x74\x61\xa");
        ERR_print_errors_fp(stderr);
    }

    if (p7)
        PKCS7_free(p7);

    if (scert)
        X509_free(scert);
    if (skey)
        EVP_PKEY_free(skey);

    if (scert2)
        X509_free(scert2);
    if (skey)
        EVP_PKEY_free(skey2);

    if (in)
        BIO_free(in);
    if (out)
        BIO_free(out);
    if (tbio)
        BIO_free(tbio);

    return ret;

}
