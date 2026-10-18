// Copyright (c) 2026 The Dorkcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SEEDPHRASEDIALOG_H
#define BITCOIN_QT_SEEDPHRASEDIALOG_H

#include <support/allocators/secure.h>

#include <QDialog>

#include <vector>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QWidget;
QT_END_NAMESPACE

/** Shows a newly generated BIP39 recovery phrase, or lets the user enter an
 *  existing one, plus an optional BIP39 passphrase. */
class SeedPhraseDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode {
        GENERATE,
        RESTORE,
    };

    SeedPhraseDialog(Mode mode, QWidget* parent);
    ~SeedPhraseDialog();

    //! Normalized recovery phrase; valid after the dialog was accepted.
    const SecureString& mnemonic() const { return m_mnemonic; }
    //! Optional BIP39 passphrase (NFKD normalized), empty when not used.
    const SecureString& mnemonicPassphrase() const { return m_passphrase; }

public Q_SLOTS:
    void accept() override;

private:
    QWidget* createWordsPage();
    QWidget* createVerifyPage();
    QWidget* createRestorePage();
    QWidget* createPassphraseBox(bool generate);
    void updateState();
    bool passphraseOk() const;
    SecureString enteredPhrase() const;
    void clearSecrets();

    const Mode m_mode;
    SecureString m_mnemonic;
    SecureString m_passphrase;
    std::vector<SecureString> m_words;
    std::vector<size_t> m_verify_positions;

    QStackedWidget* m_pages{nullptr};
    QCheckBox* m_written_checkbox{nullptr};
    std::vector<QLineEdit*> m_verify_edits;
    QPlainTextEdit* m_phrase_edit{nullptr};
    QLabel* m_status_label{nullptr};
    QCheckBox* m_use_passphrase{nullptr};
    QWidget* m_passphrase_fields{nullptr};
    QLineEdit* m_passphrase_edit{nullptr};
    QLineEdit* m_passphrase_confirm_edit{nullptr};
    QPushButton* m_back_button{nullptr};
    QPushButton* m_ok_button{nullptr};
};

#endif // BITCOIN_QT_SEEDPHRASEDIALOG_H
