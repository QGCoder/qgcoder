#include "settings_dlg.h"
#include <QDir>

SettingsDialog::SettingsDialog(QWidget *parent, QString& homedir)
:QDialog(parent)
{
QString str;

    // build the dialog from ui
    setupUi(this);
    home_dir = homedir;

}

void SettingsDialog::setValues(QString& tbl, QString& gcode)
{
    le_path2->setText(tooltable = tbl);
    le_path3->setText(gcodefile = gcode);
}

void SettingsDialog::onFileBrowse(int buttonNumber)
{
QString pathStr;
QString filename;
QDir dir;

    if(buttonNumber == 2)
        {
        if(tooltable.isEmpty())
            pathStr = home_dir + "machinekit/configs";
        else
            pathStr = dir.absoluteFilePath(tooltable);
        }
    else
        {
        if(gcodefile.isEmpty())
            pathStr = "/tmp";
        else
            pathStr = dir.absoluteFilePath(gcodefile);
        }

    filename = QFileDialog::getOpenFileName(this, tr("Settings Paths"), pathStr, tr("All files (*)"));
    if(filename.length())
        {
        if(buttonNumber == 2)
            le_path2->setText(filename);
        else
            le_path3->setText(filename);
        }
}

void SettingsDialog::onAccept()
{
    tooltable = le_path2->text();
    gcodefile = le_path3->text();

    QDialog::accept();
}




