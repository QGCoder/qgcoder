
#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H

#include <QtGui>
#include <QSettings>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDebug>

#include "ui_settings.h"


class SettingsDialog  : public QDialog , private Ui_Settings
{
    Q_OBJECT
public:

    SettingsDialog(QWidget* parent, QString& homedir);

    void setValues(QString& rs, QString& tbl, QString& gcode);
    QString rs274;
    QString tooltable;
    QString gcodefile;

private:
    void onFileBrowse(int buttonNumber);
    QString home_dir;

private slots:
    virtual void onFileBrowse1() {onFileBrowse(1);}
    virtual void onFileBrowse2() {onFileBrowse(2);}
    virtual void onFileBrowse3() {onFileBrowse(3);}

    virtual void onAccept();
};


#endif
