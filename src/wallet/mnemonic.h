// Copyright (c) 2026 The Dorkcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_MNEMONIC_H
#define BITCOIN_WALLET_MNEMONIC_H

#include <span.h>
#include <support/allocators/secure.h>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

struct CExtKey;

namespace wallet {

/** BIP39 recovery phrase (mnemonic) support. */

static constexpr size_t MNEMONIC_WORDLIST_SIZE{2048};
//! Number of words generated for new wallets.
static constexpr size_t MNEMONIC_DEFAULT_WORD_COUNT{24};
//! PBKDF2-HMAC-SHA512 iteration count defined by BIP39.
static constexpr unsigned int MNEMONIC_PBKDF2_ROUNDS{2048};

//! The BIP39 English wordlist, sorted.
extern const std::array<const char*, MNEMONIC_WORDLIST_SIZE> MNEMONIC_WORDLIST_ENGLISH;

using SecureBytes = std::vector<unsigned char, secure_allocator<unsigned char>>;

enum class MnemonicStatus {
    OK,
    INVALID_WORD_COUNT, //!< Not 12, 15, 18, 21 or 24 words
    UNKNOWN_WORD,       //!< A word is not in the BIP39 English wordlist
    INVALID_CHECKSUM,
};

//! Return true if word_count is a valid BIP39 phrase length.
bool IsValidMnemonicWordCount(size_t word_count);

//! Return the index of word in the wordlist, if present.
std::optional<size_t> MnemonicWordIndex(const SecureString& word);

//! Lower-case the phrase and collapse whitespace to single spaces.
SecureString NormalizeMnemonic(const SecureString& phrase);

//! Split a phrase into its normalized words.
std::vector<SecureString> SplitMnemonic(const SecureString& phrase);

//! Encode 16, 20, 24, 28 or 32 bytes of entropy as a BIP39 phrase.
SecureString MnemonicFromEntropy(Span<const unsigned char> entropy);

//! Generate a new random BIP39 phrase with the given number of words.
SecureString GenerateMnemonic(size_t word_count = MNEMONIC_DEFAULT_WORD_COUNT);

/**
 * Validate word count, words and checksum of a phrase.
 * @param[out] invalid_word_index  Set to the zero-based index of the first unknown word.
 */
MnemonicStatus CheckMnemonic(const SecureString& phrase, size_t* invalid_word_index = nullptr);

/**
 * Derive the 64-byte BIP39 seed from a phrase and an optional passphrase.
 * Both inputs must already be UTF-8 NFKD normalized; the phrase is additionally
 * passed through NormalizeMnemonic().
 */
SecureBytes MnemonicToSeed(const SecureString& phrase, const SecureString& passphrase);

//! Derive the BIP32 master key for a phrase and optional passphrase.
CExtKey MnemonicToMasterKey(const SecureString& phrase, const SecureString& passphrase);

} // namespace wallet

#endif // BITCOIN_WALLET_MNEMONIC_H
