/// \file
/// The startup / settings dialog: the scratch g-code file and the tool table.

#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H

#include <QDialog>
#include <QString>

#include "ui_settings.h"

/// \brief Asks for the two paths qgcoder cannot work without.
///
/// The scratch g-code file is where the editor pane is written before the
/// interpreter reads it; the tool table is optional and falls back to the
/// interpreter's built-in default when left empty. The values are only copied
/// out of the line edits on accept, so cancelling leaves them as they were.
class SettingsDialog : public QDialog, private Ui_Settings
{
    Q_OBJECT

public:
    /// \param parent  dialog parent
    /// \param homedir where the file browser starts looking, with a trailing '/'
    SettingsDialog(QWidget *parent, const QString &homedir);

    /// Fill the dialog in with the paths currently in use.
    /// \param tbl   tool table path, may be empty
    /// \param gcode scratch g-code file path
    void setValues(const QString &tbl, const QString &gcode);

    /// tool table path as accepted; empty means use the built-in default
    QString tooltable;
    /// scratch g-code file path as accepted
    QString gcodefile;

private slots:
    /// browse for the tool table
    void onFileBrowse2() { onFileBrowse(2); }
    /// browse for the scratch g-code file
    void onFileBrowse3() { onFileBrowse(3); }
    /// copy both line edits into \ref tooltable / \ref gcodefile and accept
    void onAccept();

private:
    /// Run a file dialog for one of the two paths.
    /// \param buttonNumber 2 for the tool table, anything else for the g-code file
    void onFileBrowse(int buttonNumber);

    /// starting directory for the file browser
    QString home_dir;
};

#endif // SETTINGS_DLG_H
