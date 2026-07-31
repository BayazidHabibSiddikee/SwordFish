#pragma once
// src/folder_picker.h — Custom folder picker restricted to ~/subdirs only
// Shows a tree of allowed directories with + (add) and × (remove) buttons.

#include <QDialog>
#include <QString>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>

/**
 * FolderPickerDialog
 *
 * A modal dialog that lets the user browse and select a folder.
 * Rules enforced:
 *  - Root (/) is blocked
 *  - /home is blocked
 *  - Direct children of ~/ (e.g. ~/Downloads, ~/Documents) are the top
 *    level entries shown
 *  - The user may navigate deeper into those subdirectories
 *  - + button: creates a new subfolder inside the currently selected dir
 *  - × button: removes the selected folder (only empty, non-system dirs)
 *
 * Usage:
 *   FolderPickerDialog dlg(currentPath, this);
 *   if (dlg.exec() == QDialog::Accepted)
 *       QString chosen = dlg.selectedPath();
 */
class FolderPickerDialog : public QDialog {
    Q_OBJECT

public:
    explicit FolderPickerDialog(const QString &startPath = QString(),
                                QWidget *parent = nullptr);

    /** The path the user confirmed, or empty if cancelled. */
    QString selectedPath() const;

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemExpanded(QTreeWidgetItem *item);
    void addFolder();
    void removeFolder();
    void confirm();

private:
    void populateRoot();
    void populateChildren(QTreeWidgetItem *parent, const QString &path);
    bool isBlocked(const QString &path) const;
    QTreeWidgetItem *findOrCreatePath(const QString &path);

    QTreeWidget  *m_tree;
    QPushButton  *m_addBtn;
    QPushButton  *m_removeBtn;
    QPushButton  *m_okBtn;
    QLabel       *m_pathLabel;
    QString       m_selected;
    QString       m_home;
};
