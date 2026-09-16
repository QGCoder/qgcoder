#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H

#include <QDialog>
#include <QString>

#include "ui_settings.h"

class SettingsDialog : public QDialog, private Ui_Settings
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent, const QString &homedir);

    void setValues(const QString &tbl, const QString &gcode);

    QString tooltable;
    QString gcodefile;

private slots:
    void onFileBrowse2() { onFileBrowse(2); }
    void onFileBrowse3() { onFileBrowse(3); }
    void onAccept();

private:
    void onFileBrowse(int buttonNumber);

    QString home_dir;
};

#endif // SETTINGS_DLG_H
