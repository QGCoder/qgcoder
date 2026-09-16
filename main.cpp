#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <QSurfaceFormat>

#include <clocale>
#include <cstdio>
#include <memory>

#include "canonLine.hpp"
#include "mainwin.h"
#include "rs274ngc_interp.hpp"

using namespace Qt::StringLiterals;

namespace {

constexpr auto kAppVersion = "0.1.39";
constexpr auto kAppName = "gcoder";
constexpr auto kAppOrganization = "gcoder.koppi.github.com";

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)
    if (msg.contains("QSocketNotifier"_L1))
        return;
    std::fprintf(stderr, "%s\n", qUtf8Printable(msg));
}

/// --help and --version must work on a machine with no display, so only build a
/// QApplication when we are actually going to show a window.
bool wantsGuiLess(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        const auto arg = QLatin1StringView(argv[i]);
        if (arg == "-h"_L1 || arg == "--help"_L1 || arg == "-v"_L1 || arg == "--version"_L1)
            return true;
    }
    return false;
}

} // namespace

int main(int argc, char **argv)
{
    qRegisterMetaType<QVector<g2m::canonLine *>>("QVector<g2m::canonLine*>");
    qInstallMessageHandler(customMessageHandler);

    const bool guiLess = wantsGuiLess(argc, argv);

    // Not under Emscripten: there the 3D view draws with QPainter and asks for
    // no context of its own, and overriding the default format only gets in
    // the way of the one Qt composes the window with.
#ifndef Q_OS_WASM
    if (!guiLess) {
        // the 3D view needs a core profile; this has to be set before the
        // QApplication creates the first context
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(0);
        format.setSamples(4);
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        QSurfaceFormat::setDefaultFormat(format);
    }
#endif

    const std::unique_ptr<QCoreApplication> app{
        guiLess ? new QCoreApplication(argc, argv) : new QApplication(argc, argv)};

    // the g-code interpreter parses numbers with the C locale
    std::setlocale(LC_NUMERIC, "C");

    QCoreApplication::setOrganizationName(QString::fromLatin1(kAppOrganization));
    QCoreApplication::setApplicationName(QString::fromLatin1(kAppName));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(kAppVersion));

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(u"file"_s, u"The G-code file to open."_s);
    parser.process(*app);

    // The embedded interpreter keeps its parameter file (rs274ngc.var) next to
    // the rest of the application data, and seeds it from its built-in default
    // on first run. Must come after setApplicationName().
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty())
        dataDir = QDir::tempPath();
    QDir().mkpath(dataDir);
    rs274ngc::setParameterFileDirectory(dataDir.toStdString());

    const QStringList files = parser.positionalArguments();

    MainWindow win(nullptr, !files.isEmpty(), files.value(0));
    win.show();

    return QCoreApplication::exec();
}
