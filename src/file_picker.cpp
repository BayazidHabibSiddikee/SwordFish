// src/file_picker.cpp — Good-looking custom file picker
#include "file_picker.h"
#include "folder_picker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QFrame>
#include <QFont>
#include <QApplication>
#include <QStandardPaths>

// ─── Placeholder sentinel for lazy-load ─────────────────────────────────────
static const QString PH = "__ph__";

// ─── Icon characters + colors per file type ─────────────────────────────────
QString FilePicker::iconChar(const QFileInfo &fi) const {
    if (fi.isDir()) return "📁";
    QString ext = fi.suffix().toLower();
    if (ext == "pdf")  return "📄";
    if (ext == "png" || ext == "jpg" || ext == "jpeg" ||
        ext == "bmp" || ext == "gif" || ext == "webp" || ext == "svg") return "🖼";
    if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov") return "🎬";
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "ogg") return "🎵";
    if (ext == "zip" || ext == "7z"  || ext == "tar" || ext == "gz"  ||
        ext == "bz2" || ext == "xz"  || ext == "rar") return "📦";
    if (ext == "docx" || ext == "doc") return "📝";
    if (ext == "xlsx" || ext == "xls" || ext == "csv") return "📊";
    if (ext == "pptx" || ext == "ppt") return "📋";
    if (ext == "txt" || ext == "md")   return "📃";
    if (ext == "cpp" || ext == "h"  || ext == "py" || ext == "js" ||
        ext == "ts"  || ext == "rs" || ext == "go" || ext == "java") return "💻";
    if (ext == "sh" || ext == "bash")  return "⚙";
    return "📄";
}

// Returns a distinct color for each file category
static QColor fileColor(const QFileInfo &fi) {
    if (fi.isDir())   return QColor("#82b1ff");   // soft blue  — folders

    QString ext = fi.suffix().toLower();

    // Images — violet/pink
    if (ext == "png" || ext == "jpg" || ext == "jpeg" ||
        ext == "bmp" || ext == "gif" || ext == "webp" || ext == "svg")
        return QColor("#f48fb1");

    // Video — amber
    if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov" || ext == "wmv")
        return QColor("#ffcc80");

    // Audio — lime green
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "ogg" || ext == "aac")
        return QColor("#a5d6a7");

    // Archives — orange
    if (ext == "zip" || ext == "7z" || ext == "tar" || ext == "gz" ||
        ext == "bz2" || ext == "xz" || ext == "rar")
        return QColor("#ffab40");

    // PDF — red-orange
    if (ext == "pdf")
        return QColor("#ef9a9a");

    // Word / rich text — cyan
    if (ext == "docx" || ext == "doc" || ext == "odt" || ext == "rtf")
        return QColor("#80deea");

    // Spreadsheets — teal/green
    if (ext == "xlsx" || ext == "xls" || ext == "csv" || ext == "ods")
        return QColor("#80cbc4");

    // Presentations — yellow
    if (ext == "pptx" || ext == "ppt" || ext == "odp")
        return QColor("#fff176");

    // Plain text / markdown — light grey-blue
    if (ext == "txt" || ext == "md" || ext == "rst")
        return QColor("#b0bec5");

    // Code — bright cyan
    if (ext == "cpp" || ext == "c" || ext == "h" || ext == "hpp" ||
        ext == "py"  || ext == "js" || ext == "ts" || ext == "jsx" ||
        ext == "rs"  || ext == "go" || ext == "java" || ext == "kt" ||
        ext == "cs"  || ext == "rb" || ext == "php")
        return QColor("#80d8ff");

    // Shell / config
    if (ext == "sh" || ext == "bash" || ext == "zsh" ||
        ext == "yaml" || ext == "yml" || ext == "toml" || ext == "json" ||
        ext == "xml"  || ext == "ini" || ext == "conf")
        return QColor("#ce93d8");

    // Executables
    if (ext == "exe" || ext == "bin" || ext == "appimage" || ext == "deb" || ext == "rpm")
        return QColor("#ff8a65");

    // Fonts
    if (ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2")
        return QColor("#f0e68c");

    // Default — neutral light
    return QColor("#cfd8dc");
}

// ─── Filter parsing ──────────────────────────────────────────────────────────
// Input:  "PDFs (*.pdf);;Images (*.png *.jpg)"
// Output: [{label:"PDFs (*.pdf)", exts:["pdf"]}, ...]
static QList<FilePicker::FilterGroup> parseFilter(const QString &filter) {
    QList<FilePicker::FilterGroup> groups;
    if (filter.trimmed().isEmpty()) {
        FilePicker::FilterGroup all; all.label = "All files (*)";
        groups.append(all);
        return groups;
    }
    QRegularExpression re(R"(([^;(]+)\(([^)]*)\))");
    for (const QString &part : filter.split(";;")) {
        auto m = re.match(part.trimmed());
        FilePicker::FilterGroup g;
        g.label = part.trimmed();
        if (m.hasMatch()) {
            for (const QString &w : m.captured(2).split(' ', Qt::SkipEmptyParts)) {
                QString ext = w.trimmed();
                if (ext.startsWith("*.")) ext = ext.mid(2);
                if (!ext.isEmpty() && ext != "*") g.exts << ext.toLower();
            }
        }
        if (!g.label.isEmpty()) groups.append(g);
    }
    if (groups.isEmpty()) {
        FilePicker::FilterGroup all; all.label = "All files (*)";
        groups.append(all);
    }
    return groups;
}

bool FilePicker::passesFilter(const QFileInfo &fi) const {
    if (fi.isDir()) return true;  // always show dirs
    if (m_activeFilter < 0 || m_activeFilter >= m_filterGroups.size()) return true;
    const auto &g = m_filterGroups[m_activeFilter];
    if (g.exts.isEmpty()) return true;  // "All files"
    return g.exts.contains(fi.suffix().toLower());
}

QStringList FilePicker::parsedExtensions() const {
    if (m_activeFilter >= 0 && m_activeFilter < m_filterGroups.size())
        return m_filterGroups[m_activeFilter].exts;
    return {};
}

// ─── Constructor / buildUi ───────────────────────────────────────────────────
FilePicker::FilePicker(Mode mode, const QString &title,
                       const QString &startPath, const QString &filter,
                       QWidget *parent)
    : QDialog(parent), m_mode(mode), m_filter(filter)
{
    m_filterGroups = parseFilter(filter);
    setWindowTitle(title.isEmpty() ? (mode == Mode::SaveFile ? "Save File" : "Open File") : title);
    setMinimumSize(780, 520);
    resize(860, 580);

    // Determine start directory
    QString start = startPath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        : startPath;
    QFileInfo startFi(start);
    m_currentDir = startFi.isDir() ? start : startFi.absolutePath();
    if (m_currentDir.isEmpty())
        m_currentDir = QDir::homePath();

    buildUi();
    populatePlaces();
    navigateTo(m_currentDir);

    // Pre-fill name for save mode
    if (mode == Mode::SaveFile && !startPath.isEmpty()) {
        QFileInfo fi(startPath);
        if (!fi.isDir()) m_nameEdit->setText(fi.fileName());
    }
}

void FilePicker::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setSpacing(4);
    root->setContentsMargins(8, 8, 8, 8);

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;

    m_upBtn = new QPushButton("↑ Up");
    m_homeBtn = new QPushButton("⌂ Home");
    m_hiddenBtn = new QPushButton("👁 Hidden");
    m_hiddenBtn->setCheckable(true);

    for (auto *b : {m_upBtn, m_homeBtn, m_hiddenBtn}) {
        b->setFixedHeight(28);
        b->setStyleSheet(
            "QPushButton { background:#2a2a2a; color:#ccc; border:1px solid #444;"
            "  border-radius:4px; padding:0 10px; font-size:12px; }"
            "QPushButton:hover { background:#3a3a3a; }"
            "QPushButton:checked { background:#1565C0; color:#fff; }");
    }

    m_pathLabel = new QLabel(m_currentDir);
    m_pathLabel->setStyleSheet(
        "color:#aaa; font-size:12px; padding:2px 6px;"
        "background:#1e1e1e; border-radius:4px;");
    m_pathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    toolbar->addWidget(m_upBtn);
    toolbar->addWidget(m_homeBtn);
    toolbar->addWidget(m_pathLabel);
    toolbar->addWidget(m_hiddenBtn);
    root->addLayout(toolbar);

    // ── Splitter: places | file tree ─────────────────────────────────────
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(2);

    // Places panel
    m_places = new QListWidget;
    m_places->setFixedWidth(160);
    m_places->setStyleSheet(
        "QListWidget { background:#1a1a2e; border:none; color:#ccc; font-size:13px; }"
        "QListWidget::item { padding:6px 8px; border-radius:4px; }"
        "QListWidget::item:selected { background:#1565C0; color:#fff; }"
        "QListWidget::item:hover:!selected { background:#2a2a4a; }");

    // File tree
    m_tree = new QTreeWidget;
    m_tree->setHeaderLabels({"Name", "Size", "Modified"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setRootIsDecorated(true);
    m_tree->setAnimated(true);
    m_tree->setAlternatingRowColors(true);
    m_tree->setStyleSheet(
        "QTreeWidget { background:#121212; color:#e0e0e0; border:none;"
        "  alternate-background-color:#181818; font-size:13px; }"
        "QTreeWidget::item { padding:3px 4px; }"
        "QTreeWidget::item:selected { background:#1565C0; color:#fff; }"
        "QTreeWidget::item:hover:!selected { background:#1e1e3a; }"
        "QHeaderView::section { background:#1e1e1e; color:#aaa; border:none;"
        "  border-bottom:1px solid #333; padding:4px; font-size:12px; }");
    if (m_mode == Mode::OpenFiles)
        m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    else
        m_tree->setSelectionMode(QAbstractItemView::SingleSelection);

    splitter->addWidget(m_places);
    splitter->addWidget(m_tree);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter);

    // ── Bottom bar ────────────────────────────────────────────────────────
    auto *bottomBar = new QVBoxLayout;
    bottomBar->setSpacing(4);

    // File name row
    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(new QLabel(m_mode == Mode::SaveFile ? "Save as:" : "File:"));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setReadOnly(m_mode != Mode::SaveFile);
    m_nameEdit->setPlaceholderText(m_mode == Mode::SaveFile ? "filename" : "");
    m_nameEdit->setStyleSheet(
        "QLineEdit { background:#1e1e1e; color:#e0e0e0; border:1px solid #444;"
        "  border-radius:4px; padding:4px 8px; font-size:13px; }"
        "QLineEdit:focus { border-color:#1565C0; }");
    nameRow->addWidget(m_nameEdit);
    bottomBar->addLayout(nameRow);

    // Filter + buttons row
    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Filter:"));
    m_filterCombo = new QComboBox;
    m_filterCombo->setMinimumWidth(200);
    m_filterCombo->setStyleSheet(
        "QComboBox { background:#1e1e1e; color:#e0e0e0; border:1px solid #444;"
        "  border-radius:4px; padding:3px 8px; font-size:13px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#1e1e1e; color:#e0e0e0; "
        "  selection-background-color:#1565C0; }");
    for (const auto &g : m_filterGroups)
        m_filterCombo->addItem(g.label);

    filterRow->addWidget(m_filterCombo);
    filterRow->addStretch();

    auto *cancelBtn = new QPushButton("Cancel");
    m_okBtn = new QPushButton(m_mode == Mode::SaveFile ? "Save" : "Open");
    m_okBtn->setDefault(true);
    m_okBtn->setEnabled(false);

    for (auto *b : {cancelBtn, m_okBtn}) {
        b->setFixedHeight(30);
        b->setMinimumWidth(80);
    }
    m_okBtn->setStyleSheet(
        "QPushButton { background:#1565C0; color:#fff; border:none;"
        "  border-radius:4px; font-size:13px; font-weight:bold; padding:0 16px; }"
        "QPushButton:hover { background:#1976D2; }"
        "QPushButton:disabled { background:#333; color:#666; }");
    cancelBtn->setStyleSheet(
        "QPushButton { background:#2a2a2a; color:#ccc; border:1px solid #444;"
        "  border-radius:4px; font-size:13px; padding:0 16px; }"
        "QPushButton:hover { background:#3a3a3a; }");

    filterRow->addWidget(cancelBtn);
    filterRow->addWidget(m_okBtn);
    bottomBar->addLayout(filterRow);

    root->addLayout(bottomBar);

    // Style labels
    setStyleSheet("QDialog { background:#121212; } QLabel { color:#ccc; font-size:13px; }");

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_places, &QListWidget::itemClicked,          this, &FilePicker::onPlaceClicked);
    connect(m_tree,   &QTreeWidget::itemClicked,          this, &FilePicker::onTreeItemClicked);
    connect(m_tree,   &QTreeWidget::itemDoubleClicked,    this, &FilePicker::onTreeItemDoubleClicked);
    connect(m_tree,   &QTreeWidget::itemExpanded,         this, &FilePicker::onTreeItemExpanded);
    connect(m_nameEdit, &QLineEdit::textChanged,          this, &FilePicker::onNameEdited);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilePicker::onFilterChanged);
    connect(m_upBtn,    &QPushButton::clicked, this, &FilePicker::navigateUp);
    connect(m_homeBtn,  &QPushButton::clicked, this, &FilePicker::navigateHome);
    connect(m_hiddenBtn,&QPushButton::clicked, this, [this]() {
        m_showHidden = m_hiddenBtn->isChecked();
        navigateTo(m_currentDir);
    });
    connect(m_okBtn,    &QPushButton::clicked, this, &FilePicker::confirmSelection);
    connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
}

// ─── Places panel ────────────────────────────────────────────────────────────
void FilePicker::populatePlaces() {
    m_places->clear();

    auto addPlace = [this](const QString &icon, const QString &label, const QString &path) {
        if (path.isEmpty() || !QDir(path).exists()) return;
        auto *item = new QListWidgetItem(icon + "  " + label, m_places);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
    };

    m_places->addItem(new QListWidgetItem("── Bookmarks ──"));
    m_places->item(m_places->count()-1)->setFlags(Qt::NoItemFlags);
    m_places->item(m_places->count()-1)->setForeground(QColor("#666"));

    addPlace("⌂", "Home",      QDir::homePath());
    addPlace("🖥", "Desktop",   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    addPlace("📥", "Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    addPlace("📄", "Documents", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    addPlace("🖼", "Pictures",  QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    addPlace("🎵", "Music",     QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    addPlace("🎬", "Videos",    QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));

    // Mounted drives
    bool drivesHeaderAdded = false;
    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) continue;
        QString mp = storage.rootPath();
        if (mp == "/") continue;
        if (!drivesHeaderAdded) {
            auto *sep = new QListWidgetItem("── Drives ──");
            sep->setFlags(Qt::NoItemFlags);
            sep->setForeground(QColor("#666"));
            m_places->addItem(sep);
            drivesHeaderAdded = true;
        }
        QString name = storage.displayName().isEmpty()
            ? QFileInfo(mp).fileName() : storage.displayName();
        addPlace("💾", name, mp);
    }
}

// ─── Tree navigation ─────────────────────────────────────────────────────────
void FilePicker::navigateTo(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) return;
    m_currentDir = QDir::cleanPath(path);
    m_pathLabel->setText(m_currentDir);
    m_upBtn->setEnabled(m_currentDir != "/");
    populateTree(m_currentDir);
    updateOkState();
}

void FilePicker::populateTree(const QString &path) {
    m_tree->clear();

    QDir dir(path);
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (!m_showHidden) filters |= QDir::Hidden;
    // Note: we manually skip hidden below for cleaner control

    const auto entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                           QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fi : entries) {
        if (!m_showHidden && fi.isHidden()) continue;

        // Filter files (dirs always shown so user can navigate)
        if (!fi.isDir() && !passesFilter(fi)) continue;

        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, iconChar(fi) + "  " + fi.fileName());
        item->setData(0, Qt::UserRole, fi.absoluteFilePath());
        item->setData(0, Qt::UserRole + 1, fi.isDir());
        item->setForeground(0, fileColor(fi));

        if (fi.isDir()) {
            // Add placeholder for expand arrow
            QDir sub(fi.absoluteFilePath());
            if (!sub.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
                auto *ph = new QTreeWidgetItem(item);
                ph->setText(0, PH);
            }
        } else {
            // Size
            QString sizeStr;
            qint64 sz = fi.size();
            if      (sz < 1024)           sizeStr = QString::number(sz) + " B";
            else if (sz < 1024*1024)      sizeStr = QString::number(sz/1024) + " KB";
            else if (sz < 1024*1024*1024) sizeStr = QString::number(sz/1024/1024) + " MB";
            else                          sizeStr = QString::number(sz/1024/1024/1024) + " GB";
            item->setText(1, sizeStr);
            item->setForeground(1, QColor("#607d8b"));

            // Date
            item->setText(2, fi.lastModified().toString("yyyy-MM-dd"));
            item->setForeground(2, QColor("#607d8b"));
        }
    }
    m_tree->resizeColumnToContents(1);
    m_tree->resizeColumnToContents(2);
}

void FilePicker::populateChildren(QTreeWidgetItem *parent, const QString &path) {
    // Remove placeholder
    for (int i = parent->childCount() - 1; i >= 0; --i) {
        if (parent->child(i)->text(0) == PH)
            delete parent->takeChild(i);
    }
    QDir dir(path);
    const auto entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                           QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fi : entries) {
        if (!m_showHidden && fi.isHidden()) continue;
        if (!fi.isDir() && !passesFilter(fi)) continue;

        auto *child = new QTreeWidgetItem(parent);
        child->setText(0, iconChar(fi) + "  " + fi.fileName());
        child->setData(0, Qt::UserRole,     fi.absoluteFilePath());
        child->setData(0, Qt::UserRole + 1, fi.isDir());
        child->setForeground(0, fileColor(fi));

        if (fi.isDir()) {
            QDir sub(fi.absoluteFilePath());
            if (!sub.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
                auto *ph = new QTreeWidgetItem(child);
                ph->setText(0, PH);
            }
        } else {
            qint64 sz = fi.size();
            QString sizeStr =
                sz < 1024       ? QString::number(sz) + " B" :
                sz < 1024*1024  ? QString::number(sz/1024) + " KB" :
                                  QString::number(sz/1024/1024) + " MB";
            child->setText(1, sizeStr);
            child->setForeground(1, QColor("#607d8b"));
            child->setText(2, fi.lastModified().toString("yyyy-MM-dd"));
            child->setForeground(2, QColor("#607d8b"));
        }
    }
}

// ─── Slots ────────────────────────────────────────────────────────────────────
void FilePicker::onPlaceClicked(QListWidgetItem *item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) navigateTo(path);
}

void FilePicker::onTreeItemExpanded(QTreeWidgetItem *item) {
    if (!item) return;
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    if (!isDir) return;
    if (item->childCount() == 1 && item->child(0)->text(0) == PH) {
        populateChildren(item, item->data(0, Qt::UserRole).toString());
    }
}

void FilePicker::onTreeItemClicked(QTreeWidgetItem *item, int) {
    if (!item || item->text(0) == PH) return;

    QString path = item->data(0, Qt::UserRole).toString();
    bool isDir   = item->data(0, Qt::UserRole + 1).toBool();

    if (m_mode == Mode::OpenFiles) {
        // Collect all selected files
        m_selected.clear();
        for (auto *sel : m_tree->selectedItems()) {
            if (sel->data(0, Qt::UserRole + 1).toBool()) continue; // skip dirs
            m_selected << sel->data(0, Qt::UserRole).toString();
        }
        if (!m_selected.isEmpty())
            m_nameEdit->setText(QFileInfo(m_selected.first()).fileName() +
                (m_selected.size() > 1 ? QString(" (+%1 more)").arg(m_selected.size()-1) : ""));
    } else {
        m_selected = {path};
        if (!isDir) {
            m_nameEdit->setText(QFileInfo(path).fileName());
            m_currentDir = QFileInfo(path).absolutePath();
        } else {
            m_currentDir = path;
            if (m_mode == Mode::SaveFile)
                m_nameEdit->setPlaceholderText("filename");
        }
    }
    updateOkState();
}

void FilePicker::onTreeItemDoubleClicked(QTreeWidgetItem *item, int) {
    if (!item || item->text(0) == PH) return;
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    if (isDir) {
        navigateTo(item->data(0, Qt::UserRole).toString());
    } else {
        confirmSelection();
    }
}

void FilePicker::onNameEdited(const QString &text) {
    Q_UNUSED(text);
    updateOkState();
}

void FilePicker::navigateUp() {
    QDir d(m_currentDir);
    d.cdUp();
    navigateTo(d.absolutePath());
}

void FilePicker::navigateHome() {
    navigateTo(QDir::homePath());
}

void FilePicker::onFilterChanged(int index) {
    m_activeFilter = index;
    navigateTo(m_currentDir);
}

void FilePicker::updateOkState() {
    if (m_mode == Mode::SaveFile) {
        m_okBtn->setEnabled(!m_nameEdit->text().trimmed().isEmpty());
    } else if (m_mode == Mode::Directory) {
        m_okBtn->setEnabled(!m_currentDir.isEmpty());
    } else {
        m_okBtn->setEnabled(!m_selected.isEmpty());
    }
}

void FilePicker::confirmSelection() {
    if (m_mode == Mode::SaveFile) {
        QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty()) return;
        // Auto-append extension from filter if missing
        auto exts = parsedExtensions();
        if (!exts.isEmpty()) {
            bool hasExt = false;
            for (const auto &e : exts)
                if (name.endsWith("." + e, Qt::CaseInsensitive)) { hasExt = true; break; }
            if (!hasExt) name += "." + exts.first();
        }
        m_selected = {m_currentDir + "/" + name};
    } else if (m_mode == Mode::Directory) {
        m_selected = {m_currentDir};
    }
    if (!m_selected.isEmpty()) accept();
}

// ─── Public accessors ────────────────────────────────────────────────────────
QStringList FilePicker::selectedFiles() const { return m_selected; }
QString     FilePicker::selectedFile()  const {
    return m_selected.isEmpty() ? QString() : m_selected.first();
}

// ─── Static helpers ──────────────────────────────────────────────────────────
QString FilePicker::getOpenFileName(QWidget *parent, const QString &title,
                                    const QString &dir, const QString &filter) {
    FilePicker dlg(Mode::OpenFile, title, dir, filter, parent);
    return dlg.exec() == QDialog::Accepted ? dlg.selectedFile() : QString();
}

QStringList FilePicker::getOpenFileNames(QWidget *parent, const QString &title,
                                         const QString &dir, const QString &filter) {
    FilePicker dlg(Mode::OpenFiles, title, dir, filter, parent);
    return dlg.exec() == QDialog::Accepted ? dlg.selectedFiles() : QStringList();
}

QString FilePicker::getSaveFileName(QWidget *parent, const QString &title,
                                    const QString &suggested, const QString &filter) {
    FilePicker dlg(Mode::SaveFile, title, suggested, filter, parent);
    return dlg.exec() == QDialog::Accepted ? dlg.selectedFile() : QString();
}

QString FilePicker::getExistingDirectory(QWidget *parent, const QString &title,
                                         const QString &dir) {
    FolderPickerDialog dlg(dir.isEmpty() ? QDir::homePath() : dir, parent);
    dlg.setWindowTitle(title.isEmpty() ? "Select Folder" : title);
    return dlg.exec() == QDialog::Accepted ? dlg.selectedPath() : QString();
}
