#include <catch2/catch_all.hpp>

#include <memory>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "slic3r/Utils/Http.hpp"

using namespace Slic3r;

namespace {

struct OpenSslFree
{
    void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); }
    void operator()(X509 *p) const { X509_free(p); }
    void operator()(SSL_CTX *p) const { SSL_CTX_free(p); }
    void operator()(SSL *p) const { SSL_free(p); }
};
template<class T> using OpenSslPtr = std::unique_ptr<T, OpenSslFree>;

// Self-signed RSA-2048 certificate of a printer.
struct PrinterIdentity
{
    OpenSslPtr<EVP_PKEY> key{EVP_RSA_gen(2048)};
    OpenSslPtr<X509>     cert{X509_new()};

    PrinterIdentity()
    {
        REQUIRE(key);
        REQUIRE(cert);
        X509 *x = cert.get();
        X509_set_version(x, 2);
        ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
        X509_gmtime_adj(X509_getm_notBefore(x), 0);
        X509_gmtime_adj(X509_getm_notAfter(x), 3600);
        X509_set_pubkey(x, key.get());
        X509_NAME *name = X509_get_subject_name(x);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("printer.test"), -1, -1, 0);
        X509_set_issuer_name(x, name);
        REQUIRE(X509_sign(x, key.get(), EVP_sha256()) > 0);
    }
};

const PrinterIdentity &printer()
{
    static const PrinterIdentity identity;
    return identity;
}

enum class Rules { OpenSsl3, OpenSsl111, OpenSsl111Twice };

// Handshake in memory; negotiated version or 0.
int handshake(int server_version, const char *server_sigalgs, int client_level, Rules rules)
{
    OpenSslPtr<SSL_CTX> server_ctx(SSL_CTX_new(TLS_server_method()));
    OpenSslPtr<SSL_CTX> client_ctx(SSL_CTX_new(TLS_client_method()));
    REQUIRE(server_ctx);
    REQUIRE(client_ctx);

    // The server allows everything; the client decides.
    SSL_CTX_set_security_level(server_ctx.get(), 0);
    REQUIRE(SSL_CTX_set_cipher_list(server_ctx.get(), "DEFAULT:@SECLEVEL=0") == 1);
    REQUIRE(SSL_CTX_use_certificate(server_ctx.get(), printer().cert.get()) == 1);
    REQUIRE(SSL_CTX_use_PrivateKey(server_ctx.get(), printer().key.get()) == 1);
    REQUIRE(SSL_CTX_set_min_proto_version(server_ctx.get(), server_version) == 1);
    REQUIRE(SSL_CTX_set_max_proto_version(server_ctx.get(), server_version) == 1);
    if (server_sigalgs != nullptr)
        REQUIRE(SSL_CTX_set1_sigalgs_list(server_ctx.get(), server_sigalgs) == 1);

    // TLS 1.0 minimum, as curl and Asio set.
    REQUIRE(SSL_CTX_set_min_proto_version(client_ctx.get(), TLS1_VERSION) == 1);
    SSL_CTX_set_security_level(client_ctx.get(), client_level);
    if (rules != Rules::OpenSsl3)
        Http::tls_openssl111_rules(client_ctx.get());
    if (rules == Rules::OpenSsl111Twice)
        Http::tls_openssl111_rules(client_ctx.get());

    OpenSslPtr<SSL> server(SSL_new(server_ctx.get()));
    OpenSslPtr<SSL> client(SSL_new(client_ctx.get()));
    REQUIRE(server);
    REQUIRE(client);
    BIO *client_bio = nullptr;
    BIO *server_bio = nullptr;
    REQUIRE(BIO_new_bio_pair(&client_bio, 0, &server_bio, 0) == 1);
    SSL_set_bio(client.get(), client_bio, client_bio);
    SSL_set_bio(server.get(), server_bio, server_bio);
    SSL_set_connect_state(client.get());
    SSL_set_accept_state(server.get());

    auto step = [](SSL *ssl, bool &done) {
        if (done)
            return true;
        const int rc = SSL_do_handshake(ssl);
        if (rc == 1) {
            done = true;
            return true;
        }
        const int err = SSL_get_error(ssl, rc);
        return err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE;
    };
    bool client_done = false;
    bool server_done = false;
    bool failed      = false;
    for (int round = 0; round < 50 && !failed && !(client_done && server_done); ++round)
        failed = !step(client.get(), client_done) || !step(server.get(), server_done);
    ERR_clear_error();
    return !failed && client_done && server_done ? SSL_version(client.get()) : 0;
}

} // namespace

TEST_CASE("Printers get the TLS versions and digests OpenSSL 1.1.1 accepted", "[Http][TLS]")
{
    struct Case
    {
        const char *what;
        int         server_version;
        const char *server_sigalgs;
        int         client_level;
        int         openssl3;   // OpenSSL 3 alone
        int         openssl111; // with the 1.1.1 rules
    };
    const Case c = GENERATE(values<Case>({
        {"TLS 1.3", TLS1_3_VERSION, nullptr, 1, TLS1_3_VERSION, TLS1_3_VERSION},
        {"TLS 1.2", TLS1_2_VERSION, nullptr, 1, TLS1_2_VERSION, TLS1_2_VERSION},
        {"TLS 1.1", TLS1_1_VERSION, nullptr, 1, 0, TLS1_1_VERSION},
        {"TLS 1.0", TLS1_VERSION, nullptr, 1, 0, TLS1_VERSION},
        {"TLS 1.2, SHA-1 signature only", TLS1_2_VERSION, "RSA+SHA1", 1, 0, TLS1_2_VERSION},
        {"TLS 1.0 at level 2", TLS1_VERSION, nullptr, 2, 0, TLS1_VERSION},
        {"TLS 1.1 at level 3", TLS1_1_VERSION, nullptr, 3, 0, TLS1_1_VERSION},
        {"TLS 1.2 at level 3", TLS1_2_VERSION, nullptr, 3, TLS1_2_VERSION, TLS1_2_VERSION},
        {"TLS 1.2 at level 4", TLS1_2_VERSION, nullptr, 4, TLS1_2_VERSION, TLS1_2_VERSION},
        // 1.1.1 refused these as well.
        {"TLS 1.0 at level 3", TLS1_VERSION, nullptr, 3, 0, 0},
        {"TLS 1.1 at level 4", TLS1_1_VERSION, nullptr, 4, 0, 0},
        {"SHA-1 signature at level 2", TLS1_2_VERSION, "RSA+SHA1", 2, 0, 0},
    }));
    INFO(c.what);
    CHECK(handshake(c.server_version, c.server_sigalgs, c.client_level, Rules::OpenSsl3) == c.openssl3);
    CHECK(handshake(c.server_version, c.server_sigalgs, c.client_level, Rules::OpenSsl111) == c.openssl111);
    CHECK(handshake(c.server_version, c.server_sigalgs, c.client_level, Rules::OpenSsl111Twice) == c.openssl111);
}

TEST_CASE("The 1.1.1 rules allow servers without secure renegotiation", "[Http][TLS]")
{
    OpenSslPtr<SSL_CTX> ctx(SSL_CTX_new(TLS_client_method()));
    REQUIRE(ctx);
    CHECK((SSL_CTX_get_options(ctx.get()) & SSL_OP_LEGACY_SERVER_CONNECT) == 0);
    Http::tls_openssl111_rules(ctx.get());
    CHECK((SSL_CTX_get_options(ctx.get()) & SSL_OP_LEGACY_SERVER_CONNECT) != 0);
}
