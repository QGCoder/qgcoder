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

void SettingsDialog::setValues(QString& rs, QString& tbl, QString& gcode)
{
    le_path1->setText(rs274 = rs);
    le_path2->setText(tooltable = tbl);
    le_path3->setText(gcodefile = gcode);
}

void SettingsDialog::onFileBrowse(int buttonNumber)
{
QString pathStr;
QString filename;
QDir dir;

    if( buttonNumber == 1)
        {
        if(rs274.isEmpty())
            pathStr = "/usr/bin";
        else
            pathStr = dir.absoluteFilePath(rs274);
        }
    else if(buttonNumber == 2)
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
        if( buttonNumber == 1)
            le_path1->setText(filename);
        else if(buttonNumber == 2)
            le_path2->setText(filename);
        else
            le_path3->setText(filename);
        }
}

void SettingsDialog::onAccept()
{
    rs274 = le_path1->text();
    tooltable = le_path2->text();
    gcodefile = le_path3->text();

    QDialog::accept();
}




