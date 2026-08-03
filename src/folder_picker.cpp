// src/folder_picker.cpp — Custom folder picker restricted to ~/subdirs only

#include "folder_picker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidgetItem>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QFont>

// ── Helpers ──────────────────────────────────────────────────────────────

static const QString PLACEHOLDER = "__placeholder__";

// ── Constructor ───────────────────────────────────────────────────────────

FolderPickerDialog::FolderPickerDialog(const QString &startPath, QWidget *parent)
    : QDialog(parent)
    , m_home(QDir::homePath())
{
    setWindowTitle("Select Folder");
    setMinimumSize(480, 420);
    resize(500, 500);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->setContentsMargins(10, 10, 10, 10);

    // ── Path label ──
    m_pathLabel = new QLabel("No folder selected", this);
    m_pathLabel->setWordWrap(true);
    QFont lf = m_pathLabel->font();
    lf.setItalic(true);
    m_pathLabel->setFont(lf);
    root->addWidget(m_pathLabel);

    // ── Tree ──
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setAnimated(true);
    m_tree->setExpandsOnDoubleClick(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setStyleSheet(
        "QTreeWidget { background:#282c34; color:#abb2bf; border:none; font-size:13px; }"
        "QTreeWidget::item { padding:3px 4px; }"
        "QTreeWidget::item:selected { background:#3e4451; color:#abb2bf; }"
        "QTreeWidget::item:hover:!selected { background:#2c313c; }");
    root->addWidget(m_tree);

    // ── Buttons row ──
    auto *btnRow = new QHBoxLayout;
    m_addBtn    = new QPushButton("＋  New folder", this);
    m_removeBtn = new QPushButton("×  Remove",     this);
    m_okBtn     = new QPushButton("✔  Select",      this);
    auto *cancel = new QPushButton("Cancel",        this);

    m_addBtn->setEnabled(false);
    m_removeBtn->setEnabled(false);
    m_okBtn->setEnabled(false);

    m_okBtn->setDefault(true);
    m_okBtn->setStyleSheet(
        "QPushButton { font-weight:bold; background:#61afef; color:#282c34;"
        "  border:none; border-radius:4px; padding:0 12px; }"
        "QPushButton:hover { background:#528bff; }"
        "QPushButton:disabled { background:#21252b; color:#5c6370; }");

    for (auto *b : {m_addBtn, m_removeBtn, cancel}) {
        b->setStyleSheet(
            "QPushButton { background:#3e4451; color:#abb2bf; border:none;"
            "  border-radius:4px; padding:0 10px; }"
            "QPushButton:hover { background:#4b5263; }");
    }

    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addStretch();
    btnRow->addWidget(cancel);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);

    // ── Populate ──
    populateRoot();

    // ── Connections ──
    connect(m_tree, &QTreeWidget::itemClicked,   this, &FolderPickerDialog::onItemClicked);
    connect(m_tree, &QTreeWidget::itemExpanded,  this, &FolderPickerDialog::onItemExpanded);
    connect(m_addBtn,    &QPushButton::clicked,  this, &FolderPickerDialog::addFolder);
    connect(m_removeBtn, &QPushButton::clicked,  this, &FolderPickerDialog::removeFolder);
    connect(m_okBtn,     &QPushButton::clicked,  this, &FolderPickerDialog::confirm);
    connect(cancel,      &QPushButton::clicked,  this, &QDialog::reject);

    // Restore selection to startPath if it's under home
    if (!startPath.isEmpty() && startPath.startsWith(m_home + "/")) {
        auto *item = findOrCreatePath(startPath);
        if (item) {
            m_tree->setCurrentItem(item);
            onItemClicked(item, 0);
        }
    }

    // Dialog + label styling
    setStyleSheet("QDialog { background:#282c34; } QLabel { color:#abb2bf; font-size:13px; }");
}

// ── Private helpers ───────────────────────────────────────────────────────

bool FolderPickerDialog::isBlocked(const QString &path) const {
    QString clean = QDir::cleanPath(path);
    // Block root and /home entirely
    if (clean == "/" || clean == "/home") return true;
    // Block direct children of /home (i.e. other users' homes)
    if (QFileInfo(clean).dir().absolutePath() == "/home" && clean != m_home) return true;
    // Block ~/ itself — only subdirs of ~ are allowed
    if (clean == m_home) return true;
    // Must be under ~/
    if (!clean.startsWith(m_home + "/")) return true;
    return false;
}

void FolderPickerDialog::populateRoot() {
    m_tree->clear();

    QDir homeDir(m_home);
    const auto entries = homeDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                               QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fi : entries) {
        if (fi.isHidden()) continue;  // skip dotfiles at top level
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, fi.fileName());
        item->setData(0, Qt::UserRole, fi.absoluteFilePath());
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

        // Add placeholder so the expand arrow shows
        QDir sub(fi.absoluteFilePath());
        if (!sub.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty()) {
            auto *ph = new QTreeWidgetItem(item);
            ph->setText(0, PLACEHOLDER);
        }
    }
}

void FolderPickerDialog::populateChildren(QTreeWidgetItem *parent, const QString &path) {
    // Remove placeholder if present
    for (int i = parent->childCount() - 1; i >= 0; --i) {
        if (parent->child(i)->text(0) == PLACEHOLDER)
            delete parent->takeChild(i);
    }

    QDir dir(path);
    const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                           QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fi : entries) {
        auto *child = new QTreeWidgetItem(parent);
        child->setText(0, fi.fileName());
        child->setData(0, Qt::UserRole, fi.absoluteFilePath());
        child->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

        QDir sub(fi.absoluteFilePath());
        if (!sub.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty()) {
            auto *ph = new QTreeWidgetItem(child);
            ph->setText(0, PLACEHOLDER);
        }
    }
}

QTreeWidgetItem *FolderPickerDialog::findOrCreatePath(const QString &path) {
    // Walk through the tree expanding nodes to reach the target path
    QString clean = QDir::cleanPath(path);
    if (!clean.startsWith(m_home + "/")) return nullptr;

    // Split relative part into components
    QString rel = clean.mid(m_home.length() + 1);
    QStringList parts = rel.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return nullptr;

    // Find top-level item matching parts[0]
    QTreeWidgetItem *cur = nullptr;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        if (m_tree->topLevelItem(i)->text(0) == parts[0]) {
            cur = m_tree->topLevelItem(i);
            break;
        }
    }
    if (!cur) return nullptr;

    for (int p = 1; p < parts.size(); ++p) {
        // Expand current to load children
        if (cur->childCount() == 1 && cur->child(0)->text(0) == PLACEHOLDER)
            populateChildren(cur, cur->data(0, Qt::UserRole).toString());
        m_tree->expandItem(cur);

        QTreeWidgetItem *next = nullptr;
        for (int i = 0; i < cur->childCount(); ++i) {
            if (cur->child(i)->text(0) == parts[p]) {
                next = cur->child(i);
                break;
            }
        }
        if (!next) return cur;  // Return deepest found
        cur = next;
    }
    return cur;
}

// ── Slots ─────────────────────────────────────────────────────────────────

void FolderPickerDialog::onItemClicked(QTreeWidgetItem *item, int /*col*/) {
    if (!item || item->text(0) == PLACEHOLDER) return;

    QString path = item->data(0, Qt::UserRole).toString();
    m_selected = path;
    m_pathLabel->setText(path);

    bool blocked = isBlocked(path);
    m_okBtn->setEnabled(!blocked);
    m_addBtn->setEnabled(!blocked);

    // Allow removal only for empty, non-top-level dirs
    bool isTopLevel = (item->parent() == nullptr);
    QDir d(path);
    bool isEmpty = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty();
    m_removeBtn->setEnabled(!blocked && !isTopLevel && isEmpty);
}

void FolderPickerDialog::onItemExpanded(QTreeWidgetItem *item) {
    if (!item) return;
    // If only placeholder child, load real children now
    if (item->childCount() == 1 && item->child(0)->text(0) == PLACEHOLDER) {
        QString path = item->data(0, Qt::UserRole).toString();
        populateChildren(item, path);
    }
}

void FolderPickerDialog::addFolder() {
    if (m_selected.isEmpty() || isBlocked(m_selected)) return;

    bool ok;
    QString name = QInputDialog::getText(this, "New Folder",
                                         "Folder name:", QLineEdit::Normal,
                                         "New Folder", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    // Sanitise: no slashes, no leading dots
    name = name.trimmed().remove('/').remove('\\');
    if (name.startsWith('.')) {
        QMessageBox::warning(this, "Invalid Name",
                             "Folder names cannot start with a dot.");
        return;
    }

    QString newPath = m_selected + "/" + name;
    if (QDir().exists(newPath)) {
        QMessageBox::warning(this, "Exists",
                             QString("'%1' already exists.").arg(name));
        return;
    }

    if (!QDir().mkpath(newPath)) {
        QMessageBox::critical(this, "Error",
                              QString("Could not create:\n%1").arg(newPath));
        return;
    }

    // Refresh the currently selected item's children
    auto *cur = m_tree->currentItem();
    if (cur) {
        // Remove placeholder / reload
        while (cur->childCount() > 0) delete cur->takeChild(0);
        populateChildren(cur, m_selected);
        m_tree->expandItem(cur);

        // Select the new item
        for (int i = 0; i < cur->childCount(); ++i) {
            if (cur->child(i)->text(0) == name) {
                m_tree->setCurrentItem(cur->child(i));
                onItemClicked(cur->child(i), 0);
                break;
            }
        }
    }
}

void FolderPickerDialog::removeFolder() {
    if (m_selected.isEmpty() || isBlocked(m_selected)) return;

    auto *cur = m_tree->currentItem();
    if (!cur || cur->parent() == nullptr) return;  // never remove top-level

    QDir d(m_selected);
    if (!d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        QMessageBox::warning(this, "Not Empty",
                             "Only empty folders can be removed.");
        return;
    }

    auto btn = QMessageBox::question(this, "Remove Folder",
                                     QString("Delete:\n%1\n\nThis cannot be undone.")
                                         .arg(m_selected));
    if (btn != QMessageBox::Yes) return;

    if (!d.rmdir(m_selected)) {
        QMessageBox::critical(this, "Error",
                              QString("Could not remove:\n%1").arg(m_selected));
        return;
    }

    // Remove from tree and select parent
    auto *parent = cur->parent();
    parent->removeChild(cur);
    delete cur;

    m_tree->setCurrentItem(parent);
    onItemClicked(parent, 0);
}

void FolderPickerDialog::confirm() {
    if (!m_selected.isEmpty() && !isBlocked(m_selected))
        accept();
}

// ── Public API ────────────────────────────────────────────────────────────

QString FolderPickerDialog::selectedPath() const {
    return m_selected;
}
