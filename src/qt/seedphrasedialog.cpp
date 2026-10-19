// Copyright (c) 2026 The Dorkcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/seedphrasedialog.h>

#include <qt/guiutil.h>
#include <random.h>
#include <wallet/mnemonic.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

using namespace wallet;

namespace {
constexpr int WORD_COLUMNS{4};
constexpr size_t VERIFY_WORD_COUNT{3};

SecureString ToSecure(const QString& text)
{
    // BIP39 requires NFKD normalized UTF-8 for both the phrase and the passphrase.
    const QByteArray utf8{text.normalized(QString::NormalizationForm_KD).toUtf8()};
    return SecureString(utf8.constData(), utf8.size());
}

QString FromSecure(const SecureString& text)
{
    return QString::fromUtf8(text.data(), text.size());
}
} // namespace

SeedPhraseDialog::SeedPhraseDialog(Mode mode, QWidget* parent, size_t word_count)
    : QDialog(parent, GUIUtil::dialog_flags), m_mode(mode)
{
    setWindowTitle(mode == Mode::GENERATE ? tr("New Recovery Phrase") : tr("Restore From Recovery Phrase"));
    setMinimumWidth(620);

    auto* layout = new QVBoxLayout(this);
    m_pages = new QStackedWidget(this);
    layout->addWidget(m_pages);

    if (m_mode == Mode::GENERATE) {
        m_mnemonic = GenerateMnemonic(word_count);
        m_words = SplitMnemonic(m_mnemonic);
        m_pages->addWidget(createWordsPage());
        m_pages->addWidget(createVerifyPage());
    } else {
        m_pages->addWidget(createRestorePage());
    }
    layout->addWidget(createPassphraseBox(m_mode == Mode::GENERATE));

    auto* buttons = new QDialogButtonBox(this);
    m_back_button = buttons->addButton(tr("Back"), QDialogButtonBox::ActionRole);
    m_ok_button = buttons->addButton(m_mode == Mode::GENERATE ? tr("Next") : tr("Restore"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    m_back_button->setVisible(false);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SeedPhraseDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SeedPhraseDialog::reject);
    connect(m_back_button, &QPushButton::clicked, [this] {
        m_pages->setCurrentIndex(0);
        updateState();
    });
    connect(this, &QDialog::rejected, this, &SeedPhraseDialog::clearSecrets);

    updateState();
}

SeedPhraseDialog::~SeedPhraseDialog()
{
    clearSecrets();
}

QWidget* SeedPhraseDialog::createWordsPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* warning = new QLabel(tr("Write down these %1 words in the given order and keep them in a safe place. "
                                  "Anyone who knows this recovery phrase can take your coins. "
                                  "It is the only way to restore this wallet if the computer or wallet file is lost.")
                                   .arg(m_words.size()),
                               page);
    warning->setWordWrap(true);
    warning->setStyleSheet("QLabel { font-weight: bold; }");
    layout->addWidget(warning);

    auto* grid_box = new QGroupBox(page);
    auto* grid = new QGridLayout(grid_box);
    const int rows{static_cast<int>((m_words.size() + WORD_COLUMNS - 1) / WORD_COLUMNS)};
    for (size_t i = 0; i < m_words.size(); ++i) {
        auto* word = new QLabel(QStringLiteral("%1. %2").arg(i + 1, 2).arg(FromSecure(m_words[i])), grid_box);
        word->setFont(GUIUtil::fixedPitchFont());
        word->setTextInteractionFlags(Qt::NoTextInteraction);
        // Fill column by column so the numbering reads top to bottom.
        grid->addWidget(word, static_cast<int>(i) % rows, static_cast<int>(i) / rows);
    }
    layout->addWidget(grid_box);

    m_written_checkbox = new QCheckBox(tr("I have written down my recovery phrase"), page);
    connect(m_written_checkbox, &QCheckBox::toggled, this, &SeedPhraseDialog::updateState);
    layout->addWidget(m_written_checkbox);
    return page;
}

QWidget* SeedPhraseDialog::createVerifyPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* info = new QLabel(tr("Confirm that you wrote the recovery phrase down correctly by entering the requested words."), page);
    info->setWordWrap(true);
    layout->addWidget(info);

    FastRandomContext rng;
    std::vector<size_t> positions(m_words.size());
    for (size_t i = 0; i < positions.size(); ++i) positions[i] = i;
    std::shuffle(positions.begin(), positions.end(), rng);
    positions.resize(std::min(VERIFY_WORD_COUNT, positions.size()));
    std::sort(positions.begin(), positions.end());
    m_verify_positions = positions;

    auto* form = new QGridLayout();
    for (size_t i = 0; i < m_verify_positions.size(); ++i) {
        auto* label = new QLabel(tr("Word #%1:").arg(m_verify_positions[i] + 1), page);
        auto* edit = new QLineEdit(page);
        edit->setFont(GUIUtil::fixedPitchFont());
        connect(edit, &QLineEdit::textChanged, this, &SeedPhraseDialog::updateState);
        form->addWidget(label, static_cast<int>(i), 0);
        form->addWidget(edit, static_cast<int>(i), 1);
        m_verify_edits.push_back(edit);
    }
    layout->addLayout(form);
    layout->addStretch();
    return page;
}

QWidget* SeedPhraseDialog::createRestorePage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* info = new QLabel(tr("Enter your 12, 15, 18, 21 or 24 word recovery phrase, separated by spaces. "
                               "After the wallet is created the whole blockchain is rescanned for its transactions, which can take a while."),
                            page);
    info->setWordWrap(true);
    layout->addWidget(info);

    m_phrase_edit = new QPlainTextEdit(page);
    m_phrase_edit->setFont(GUIUtil::fixedPitchFont());
    m_phrase_edit->setTabChangesFocus(true);
    m_phrase_edit->setMinimumHeight(90);
    connect(m_phrase_edit, &QPlainTextEdit::textChanged, this, &SeedPhraseDialog::updateState);
    layout->addWidget(m_phrase_edit);

    m_status_label = new QLabel(page);
    m_status_label->setWordWrap(true);
    layout->addWidget(m_status_label);
    return page;
}

QWidget* SeedPhraseDialog::createPassphraseBox(bool generate)
{
    auto* box = new QWidget(this);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);

    m_use_passphrase = new QCheckBox(generate ? tr("Protect the recovery phrase with an additional passphrase (advanced)")
                                              : tr("The recovery phrase is protected with an additional passphrase"),
                                     box);
    layout->addWidget(m_use_passphrase);

    m_passphrase_fields = new QWidget(box);
    auto* grid = new QGridLayout(m_passphrase_fields);
    grid->setContentsMargins(0, 0, 0, 0);
    auto* warning = new QLabel(tr("The passphrase is NOT the wallet encryption password. Without it the recovery phrase restores a different, empty wallet. "
                                  "If you lose the passphrase your coins cannot be recovered."),
                               m_passphrase_fields);
    warning->setWordWrap(true);
    grid->addWidget(warning, 0, 0, 1, 2);
    m_passphrase_edit = new QLineEdit(m_passphrase_fields);
    m_passphrase_edit->setEchoMode(QLineEdit::Password);
    m_passphrase_confirm_edit = new QLineEdit(m_passphrase_fields);
    m_passphrase_confirm_edit->setEchoMode(QLineEdit::Password);
    grid->addWidget(new QLabel(tr("Passphrase:"), m_passphrase_fields), 1, 0);
    grid->addWidget(m_passphrase_edit, 1, 1);
    grid->addWidget(new QLabel(tr("Repeat passphrase:"), m_passphrase_fields), 2, 0);
    grid->addWidget(m_passphrase_confirm_edit, 2, 1);
    m_passphrase_fields->setVisible(false);
    layout->addWidget(m_passphrase_fields);

    connect(m_use_passphrase, &QCheckBox::toggled, [this](bool checked) {
        if (checked) m_size_without_passphrase = size();
        m_passphrase_fields->setVisible(checked);
        if (!checked) {
            m_passphrase_edit->clear();
            m_passphrase_confirm_edit->clear();
        }
        updateState();
        if (checked) {
            adjustSize();
        } else if (m_size_without_passphrase.isValid()) {
            resize(m_size_without_passphrase);
        }
    });
    connect(m_passphrase_edit, &QLineEdit::textChanged, this, &SeedPhraseDialog::updateState);
    connect(m_passphrase_confirm_edit, &QLineEdit::textChanged, this, &SeedPhraseDialog::updateState);
    return box;
}

bool SeedPhraseDialog::passphraseOk() const
{
    if (!m_use_passphrase->isChecked()) return true;
    return !m_passphrase_edit->text().isEmpty() && m_passphrase_edit->text() == m_passphrase_confirm_edit->text();
}

SecureString SeedPhraseDialog::enteredPhrase() const
{
    return NormalizeMnemonic(ToSecure(m_phrase_edit->toPlainText()));
}

void SeedPhraseDialog::updateState()
{
    bool ok{passphraseOk()};
    if (m_mode == Mode::GENERATE) {
        const bool verify_page{m_pages->currentIndex() == 1};
        m_back_button->setVisible(verify_page);
        m_ok_button->setText(verify_page ? tr("Create") : tr("Next"));
        if (verify_page) {
            for (size_t i = 0; i < m_verify_edits.size(); ++i) {
                const QString entered{m_verify_edits[i]->text().trimmed().toLower()};
                ok = ok && entered == FromSecure(m_words[m_verify_positions[i]]);
            }
        } else {
            ok = ok && m_written_checkbox->isChecked();
        }
    } else {
        const SecureString phrase{enteredPhrase()};
        const size_t word_count{SplitMnemonic(phrase).size()};
        size_t bad_word{0};
        const MnemonicStatus status{CheckMnemonic(phrase, &bad_word)};
        QString text;
        bool error{true};
        if (word_count == 0) {
            text.clear();
        } else if (status == MnemonicStatus::UNKNOWN_WORD) {
            text = tr("Word #%1 is not a valid recovery phrase word.").arg(bad_word + 1);
        } else if (status == MnemonicStatus::INVALID_WORD_COUNT) {
            text = tr("%n word(s) entered.", "", static_cast<int>(word_count));
            error = false;
        } else if (status == MnemonicStatus::INVALID_CHECKSUM) {
            text = tr("Invalid recovery phrase (checksum mismatch). Check the words and their order.");
        } else {
            text = tr("Valid recovery phrase (%n words).", "", static_cast<int>(word_count));
            error = false;
        }
        m_status_label->setText(text);
        m_status_label->setStyleSheet(error ? QStringLiteral("QLabel { color: red; }") : QString());
        ok = ok && status == MnemonicStatus::OK;
    }
    m_ok_button->setEnabled(ok);
}

void SeedPhraseDialog::accept()
{
    if (!m_ok_button->isEnabled()) return;
    if (m_mode == Mode::GENERATE && m_pages->currentIndex() == 0) {
        m_pages->setCurrentIndex(1);
        if (!m_verify_edits.empty()) m_verify_edits.front()->setFocus();
        updateState();
        return;
    }
    if (m_mode == Mode::RESTORE) {
        m_mnemonic = enteredPhrase();
        m_phrase_edit->clear();
    }
    m_passphrase = m_use_passphrase->isChecked() ? ToSecure(m_passphrase_edit->text()) : SecureString{};
    m_passphrase_edit->clear();
    m_passphrase_confirm_edit->clear();
    for (QLineEdit* edit : m_verify_edits) edit->clear();
    m_words.clear();
    QDialog::accept();
}

void SeedPhraseDialog::clearSecrets()
{
    m_mnemonic.clear();
    m_passphrase.clear();
    m_words.clear();
    if (m_phrase_edit) m_phrase_edit->clear();
    if (m_passphrase_edit) m_passphrase_edit->clear();
    if (m_passphrase_confirm_edit) m_passphrase_confirm_edit->clear();
    for (QLineEdit* edit : m_verify_edits) edit->clear();
}
