// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATEWALLETDIALOG_H
#define BITCOIN_QT_CREATEWALLETDIALOG_H

#include <QDialog>

#include <cstddef>
#include <memory>

namespace interfaces {
class ExternalSigner;
} // namespace interfaces

class WalletModel;

namespace Ui {
    class CreateWalletDialog;
}

/** Dialog for creating wallets
 */
class CreateWalletDialog : public QDialog
{
    Q_OBJECT

public:
    //! How the keys of the new wallet are created.
    enum class SeedMode {
        NONE,     //!< No recovery phrase (blank, watch-only or external signer wallet)
        GENERATE, //!< Generate and show a new recovery phrase
        RESTORE,  //!< Restore from an existing recovery phrase
    };

    explicit CreateWalletDialog(QWidget* parent);
    virtual ~CreateWalletDialog();

    void setSigners(const std::vector<std::unique_ptr<interfaces::ExternalSigner>>& signers);

    QString walletName() const;
    bool isEncryptWalletChecked() const;
    bool isDisablePrivateKeysChecked() const;
    bool isMakeBlankWalletChecked() const;
    bool isExternalSignerChecked() const;
    SeedMode seedMode() const;
    size_t mnemonicWordCount() const;

private:
    void updateSeedOptions();

    Ui::CreateWalletDialog *ui;
    bool m_has_signers = false;
};

#endif // BITCOIN_QT_CREATEWALLETDIALOG_H
