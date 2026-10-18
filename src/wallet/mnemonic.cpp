// Copyright (c) 2026 The Dorkcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/mnemonic.h>

#include <crypto/hmac_sha512.h>
#include <crypto/sha256.h>
#include <key.h>
#include <random.h>
#include <support/cleanse.h>
#include <util/check.h>

#include <algorithm>
#include <cstring>

namespace wallet {
namespace {

bool IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

char ToLowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
}

bool GetBit(const SecureBytes& data, size_t bit)
{
    return (data[bit / 8] >> (7 - bit % 8)) & 1;
}

void SetBit(SecureBytes& data, size_t bit)
{
    data[bit / 8] |= uint8_t(1 << (7 - bit % 8));
}

unsigned char ChecksumByte(Span<const unsigned char> entropy)
{
    unsigned char hash[CSHA256::OUTPUT_SIZE];
    CSHA256().Write(entropy.data(), entropy.size()).Finalize(hash);
    const unsigned char result{hash[0]};
    memory_cleanse(hash, sizeof(hash));
    return result;
}

} // namespace

bool IsValidMnemonicWordCount(size_t word_count)
{
    return word_count >= 12 && word_count <= 24 && word_count % 3 == 0;
}

std::optional<size_t> MnemonicWordIndex(const SecureString& word)
{
    const auto it = std::lower_bound(MNEMONIC_WORDLIST_ENGLISH.begin(), MNEMONIC_WORDLIST_ENGLISH.end(), word,
        [](const char* entry, const SecureString& w) { return std::strcmp(entry, w.c_str()) < 0; });
    if (it == MNEMONIC_WORDLIST_ENGLISH.end() || word != *it) return std::nullopt;
    return size_t(it - MNEMONIC_WORDLIST_ENGLISH.begin());
}

std::vector<SecureString> SplitMnemonic(const SecureString& phrase)
{
    std::vector<SecureString> words;
    SecureString current;
    for (const char c : phrase) {
        if (IsSpace(c)) {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ToLowerAscii(c));
        }
    }
    if (!current.empty()) words.push_back(current);
    return words;
}

SecureString NormalizeMnemonic(const SecureString& phrase)
{
    SecureString result;
    for (const SecureString& word : SplitMnemonic(phrase)) {
        if (!result.empty()) result.push_back(' ');
        result.append(word);
    }
    return result;
}

SecureString MnemonicFromEntropy(Span<const unsigned char> entropy)
{
    Assert(entropy.size() >= 16 && entropy.size() <= 32 && entropy.size() % 4 == 0);

    SecureBytes data(entropy.begin(), entropy.end());
    data.push_back(ChecksumByte(entropy));

    const size_t word_count{entropy.size() * 8 * 3 / 32};
    SecureString phrase;
    for (size_t i = 0; i < word_count; ++i) {
        size_t index{0};
        for (size_t bit = 0; bit < 11; ++bit) {
            index = (index << 1) | GetBit(data, i * 11 + bit);
        }
        if (i > 0) phrase.push_back(' ');
        phrase.append(MNEMONIC_WORDLIST_ENGLISH[index]);
    }
    return phrase;
}

SecureString GenerateMnemonic(size_t word_count)
{
    Assert(IsValidMnemonicWordCount(word_count));
    SecureBytes entropy(word_count * 4 / 3);
    GetStrongRandBytes(entropy);
    return MnemonicFromEntropy(entropy);
}

MnemonicStatus CheckMnemonic(const SecureString& phrase, size_t* invalid_word_index)
{
    const std::vector<SecureString> words{SplitMnemonic(phrase)};

    SecureBytes data((words.size() * 11 + 7) / 8);
    for (size_t i = 0; i < words.size(); ++i) {
        const std::optional<size_t> index{MnemonicWordIndex(words[i])};
        if (!index) {
            if (invalid_word_index) *invalid_word_index = i;
            return MnemonicStatus::UNKNOWN_WORD;
        }
        for (size_t bit = 0; bit < 11; ++bit) {
            if ((*index >> (10 - bit)) & 1) SetBit(data, i * 11 + bit);
        }
    }
    if (!IsValidMnemonicWordCount(words.size())) return MnemonicStatus::INVALID_WORD_COUNT;

    const size_t entropy_bytes{words.size() * 4 / 3};
    const size_t checksum_bits{words.size() / 3};
    const unsigned char expected{ChecksumByte(Span{data}.first(entropy_bytes))};
    const unsigned char actual{data[entropy_bytes]};
    const unsigned char mask{uint8_t(0xff << (8 - checksum_bits))};
    if ((expected & mask) != (actual & mask)) return MnemonicStatus::INVALID_CHECKSUM;
    return MnemonicStatus::OK;
}

SecureBytes MnemonicToSeed(const SecureString& phrase, const SecureString& passphrase)
{
    // PBKDF2-HMAC-SHA512 with a single 64-byte output block.
    const SecureString password{NormalizeMnemonic(phrase)};
    SecureString salt{"mnemonic"};
    salt.append(passphrase);
    static const unsigned char block_index[4]{0, 0, 0, 1};

    const auto* pw{reinterpret_cast<const unsigned char*>(password.data())};
    unsigned char u[CHMAC_SHA512::OUTPUT_SIZE];
    CHMAC_SHA512(pw, password.size())
        .Write(reinterpret_cast<const unsigned char*>(salt.data()), salt.size())
        .Write(block_index, sizeof(block_index))
        .Finalize(u);

    SecureBytes seed(u, u + sizeof(u));
    for (unsigned int round = 1; round < MNEMONIC_PBKDF2_ROUNDS; ++round) {
        CHMAC_SHA512(pw, password.size()).Write(u, sizeof(u)).Finalize(u);
        for (size_t i = 0; i < sizeof(u); ++i) seed[i] ^= u[i];
    }
    memory_cleanse(u, sizeof(u));
    return seed;
}

CExtKey MnemonicToMasterKey(const SecureString& phrase, const SecureString& passphrase)
{
    const SecureBytes seed{MnemonicToSeed(phrase, passphrase)};
    CExtKey master_key;
    master_key.SetSeed(MakeByteSpan(seed));
    return master_key;
}

} // namespace wallet
