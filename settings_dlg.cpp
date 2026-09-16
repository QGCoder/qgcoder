#include "settings_dlg.h"

#include <QAbstractButton>
#include <QDir>
#include <QFileDialog>

using namespace Qt::StringLiterals;

SettingsDialog::SettingsDialog(QWidget *parent, const QString &homedir)
    : QDialog(parent)
    , home_dir(homedir)
{
    // build the dialog from ui
    setupUi(this);

    connect(pb_browse2, &QAbstractButton::clicked, this, &SettingsDialog::onFileBrowse2);
    connect(pb_browse3, &QAbstractButton::clicked, this, &SettingsDialog::onFileBrowse3);
    connect(pushButton_2, &QAbstractButton::clicked, this, &SettingsDialog::onAccept);
    connect(pushButton, &QAbstractButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::setValues(const QString &tbl, const QString &gcode)
{
    tooltable = tbl;
    gcodefile = gcode;
    le_path2->setText(tooltable);
    le_path3->setText(gcodefile);
}

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

void SettingsDialog::onAccept()
{
    tooltable = le_path2->text();
    gcodefile = le_path3->text();

    QDialog::accept();
}
