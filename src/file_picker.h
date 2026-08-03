#pragma once
// src/file_picker.h — Good-looking custom file picker
// Drop-in replacement for QFileDialog::getOpenFileName/Names/getSaveFileName

#include <QDialog>
#include <QFileInfo>
#include <QStringList>

class QTreeWidget;
class QListWidget;
class QListWidgetItem;
class QTreeWidgetItem;
class QLineEdit;
class QLabel;
class QPushButton;
class QComboBox;
class QSplitter;
class QStackedWidget;

/**
 * FilePicker — a polished file chooser dialog.
 *
 * Modes:
 *   OpenFile        — pick one existing file
 *   OpenFiles       — pick multiple existing files
 *   SaveFile        — choose a path to save to (typed or clicked)
 *   Directory       — pick a folder (wraps FolderPickerDialog-style logic)
 *
 * Static helpers mirror QFileDialog's API so call sites are minimal-change:
 *
 *   FilePicker::getOpenFileName(parent, title, startDir, filter)
 *   FilePicker::getOpenFileNames(parent, title, startDir, filter)
 *   FilePicker::getSaveFileName(parent, title, suggested, filter)
 *   FilePicker::getExistingDirectory(parent, title, startDir)
 */
class FilePicker : public QDialog {
    Q_OBJECT

public:
    enum class Mode { OpenFile, OpenFiles, SaveFile, Directory };

    // Parsed filter groups: "PDFs (*.pdf)" → {"PDFs (*.pdf)", {"pdf"}}
    struct FilterGroup { QString label; QStringList exts; };

    explicit FilePicker(Mode mode,
                        const QString &title      = QString(),
                        const QString &startPath  = QString(),
                        const QString &filter     = QString(),
                        QWidget *parent = nullptr);

    /** Selected path(s). For OpenFiles may be multiple. */
    QStringList selectedFiles() const;
    QString     selectedFile()  const;  // first / only

    // ── Static helpers (QFileDialog-compatible) ───────────────────────────

    static QString     getOpenFileName     (QWidget *parent,
                                            const QString &title   = QString(),
                                            const QString &dir     = QString(),
                                            const QString &filter  = QString());

    static QStringList getOpenFileNames    (QWidget *parent,
                                            const QString &title   = QString(),
                                            const QString &dir     = QString(),
                                            const QString &filter  = QString());

    static QString     getSaveFileName     (QWidget *parent,
                                            const QString &title   = QString(),
                                            const QString &suggested = QString(),
                                            const QString &filter  = QString());

    static QString     getExistingDirectory(QWidget *parent,
                                            const QString &title   = QString(),
                                            const QString &dir     = QString());

private slots:
    void onPlaceClicked(QListWidgetItem *item);
    void onTreeItemExpanded(QTreeWidgetItem *item);
    void onTreeItemClicked(QTreeWidgetItem *item, int col);
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int col);
    void onNameEdited(const QString &text);
    void navigateUp();
    void navigateHome();
    void confirmSelection();
    void onFilterChanged(int index);

private:
    void buildUi();
    void navigateTo(const QString &path);
    void populatePlaces();
    void populateTree(const QString &path);
    void populateChildren(QTreeWidgetItem *parent, const QString &path);
    QString iconChar(const QFileInfo &fi) const;
    bool passesFilter(const QFileInfo &fi) const;
    QStringList parsedExtensions() const;
    void selectSavedFilter(int comboIndex);
    void updateOkState();

    Mode        m_mode;
    QString     m_filter;
    QString     m_currentDir;
    bool        m_showHidden = false;

    QListWidget   *m_places;
    QTreeWidget   *m_tree;
    QLineEdit     *m_nameEdit;
    QComboBox     *m_filterCombo;
    QLabel        *m_pathLabel;
    QPushButton   *m_okBtn;
    QPushButton   *m_hiddenBtn;
    QPushButton   *m_upBtn;
    QPushButton   *m_homeBtn;

    QStringList   m_selected;

    QList<FilterGroup> m_filterGroups;
    int m_activeFilter = 0;
};
