/// \file
/// \see SettingsDialog

#include "settings_dlg.h"

#include <QAbstractButton>
#include <QDir>
#include <QFileDialog>

using namespace Qt::StringLiterals;

/// Builds the dialog from settings.ui and wires the four buttons up. Under
/// Emscripten the two browse buttons are hidden rather than connected.
SettingsDialog::SettingsDialog(QWidget *parent, const QString &homedir)
    : QDialog(parent)
    , home_dir(homedir)
{
    // build the dialog from ui
    setupUi(this);

#ifdef Q_OS_WASM
    // Browsing means a nested event loop, and there is nothing to browse: both
    // paths name files in Emscripten's in-memory file system, so they can only
    // be typed.
    pb_browse2->hide();
    pb_browse3->hide();
#else
    connect(pb_browse2, &QAbstractButton::clicked, this, &SettingsDialog::onFileBrowse2);
    connect(pb_browse3, &QAbstractButton::clicked, this, &SettingsDialog::onFileBrowse3);
#endif
    connect(pushButton_2, &QAbstractButton::clicked, this, &SettingsDialog::onAccept);
    connect(pushButton, &QAbstractButton::clicked, this, &QDialog::reject);
}

/// Seeds both members and both line edits, so that cancelling the dialog
/// leaves the caller's values untouched.
void SettingsDialog::setValues(const QString &tbl, const QString &gcode)
{
    tooltable = tbl;
    gcodefile = gcode;
    le_path2->setText(tooltable);
    le_path3->setText(gcodefile);
}

/// Opens the file dialog at the current path if there is one, and otherwise at
/// the machinekit config directory for the tool table or the temporary
/// directory for the g-code file. Cancelling leaves the line edit alone.
void SettingsDialog::onFileBrowse(int buttonNumber)
{
    const QDir dir;
    QString pathStr;

    if (buttonNumber == 2)
        pathStr = tooltable.isEmpty() ? home_dir + "machinekit/configs"_L1 : dir.absoluteFilePath(tooltable);
    else
        pathStr = gcodefile.isEmpty() ? QDir::tempPath() : dir.absoluteFilePath(gcodefile);

    const QString filename =
        QFileDialog::getOpenFileName(this, tr("Settings Paths"), pathStr, tr("All files (*)"));
    if (filename.isEmpty())
        return;

    if (buttonNumber == 2)
        le_path2->setText(filename);
    else
        le_path3->setText(filename);
}

/// Takes the text of both line edits as the result and closes the dialog.
void SettingsDialog::onAccept()
{
    tooltable = le_path2->text();
    gcodefile = le_path3->text();

    QDialog::accept();
}
