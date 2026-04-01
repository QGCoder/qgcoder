#include <QtWidgets>
#include <QLoggingCategory>
#include <QMessageLogContext>
#include <QString>
#include <QFile>
#include <QTextStream>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(type);
    Q_UNUSED(context);
    if (msg.contains("QSocketNotifier"))
        return;
    QByteArray ba = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", ba.constData());
}

#include "mainwin.h"

#define APP_VERSION QString("2016")
#define APP_NAME QString("gcoder")
#define APP_NAME_FULL QString("GCoder")
#define APP_ORGANIZATION QString("gcoder.koppi.github.com")

int main(int argv, char **args)
{
    qInstallMessageHandler(customMessageHandler);
    QApplication *app;

    // workaround for https://forum.qt.io/topic/53298/qcommandlineparser-to-select-gui-or-non-gui-mode

    // On Linux: enable printing of version and help without DISPLAY variable set

    bool runCore = false;
    for (int i = 0; i < argv; i++) {
        if (QString(args[i]) == "-h" ||
                QString(args[i]) == "--help" ||
                QString(args[i]) == "-v" ||
                QString(args[i]) == "--version" ) {
            runCore = true;
            break;
        }
    }

    if (runCore) {
        app = static_cast<QApplication*>(new QCoreApplication(argv, args));
    } else {
        app = new QApplication(argv, args);
    }

    // end workaround

    setlocale(LC_NUMERIC,"C");

    //XXX app->setStyleSheet("QPlainTextEdit{ selection-background-color: darkblue } QWidget { font-size: 12pt; font-family: \"Courier\"; background-color: #00003B; color: #FFA700; font: bold }");

    QCoreApplication::setOrganizationName(APP_ORGANIZATION);
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCommandLineParser parser;

    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The G-code file to open.");

    parser.process(*app);

    MainWindow *win;

    if (!parser.positionalArguments().isEmpty()) {
         win = new MainWindow(NULL, !parser.positionalArguments().isEmpty(), parser.positionalArguments().first());
    } else {
        win = new MainWindow();
    }

    win->show();

    int ret = app->exec();
    delete win;
    delete app;
    return ret;
}

