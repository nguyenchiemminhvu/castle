#include "sample_support.hpp"
#include "castle_ext/crypto/xor.hpp"

int main()
{
    const uint8_t key[3] = {0x4BU, 0x45U, 0x59U};
    const uint8_t plain[8] = {'c', 'a', 's', 't', 'l', 'e', '!', '\n'};

    // One-shot, stateless helper.
    uint8_t message[8];
    for (size_t i = 0U; i < sizeof(message); ++i) message[i] = plain[i];
    CASTLE_SAMPLE_CHECK(castle::crypto::xor_encrypt(message, sizeof(message), key, sizeof(key)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(message[0] == static_cast<uint8_t>('c' ^ 0x4BU));
    CASTLE_SAMPLE_CHECK(castle::crypto::xor_decrypt(message, sizeof(message), key, sizeof(key)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(message); ++i) CASTLE_SAMPLE_CHECK(message[i] == plain[i]);

    // Incremental cipher: chunk boundaries do not change the result.
    castle::crypto::xor_cipher<16U> cipher(key);
    CASTLE_SAMPLE_CHECK(cipher.configured());
    uint8_t chunked[8];
    for (size_t i = 0U; i < sizeof(chunked); ++i) chunked[i] = plain[i];
    CASTLE_SAMPLE_CHECK(cipher.encrypt(chunked, 5U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(cipher.encrypt(chunked + 5U, 3U) == castle::status::ok);

    uint8_t whole[8];
    for (size_t i = 0U; i < sizeof(whole); ++i) whole[i] = plain[i];
    CASTLE_SAMPLE_CHECK(castle::crypto::xor_encrypt(whole, sizeof(whole), key, sizeof(key)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(whole); ++i) CASTLE_SAMPLE_CHECK(chunked[i] == whole[i]);

    // Random access: decrypt only the tail by seeking to its stream offset.
    uint8_t tail[3] = {chunked[5], chunked[6], chunked[7]};
    CASTLE_SAMPLE_CHECK(cipher.seek(5U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(cipher.decrypt(tail, sizeof(tail)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(tail); ++i) CASTLE_SAMPLE_CHECK(tail[i] == plain[5U + i]);

    // Wipe the key when finished.
    cipher.clear();
    CASTLE_SAMPLE_CHECK(!cipher.configured());
    return 0;
}
