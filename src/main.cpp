#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QSocketNotifier>
#include <QTimer>
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>

namespace {

int crashPipe[2] = {-1, -1};

void crashSignalHandler(int sig) {
    if (crashPipe[1] >= 0) {
        write(crashPipe[1], &sig, sizeof(sig));
    }
    spdlog::default_logger()->flush();
    signal(sig, SIG_DFL);
    raise(sig);
}

void setupCrashHandler() {
    if (pipe(crashPipe) != 0) return;
    fcntl(crashPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(crashPipe[1], F_SETFL, O_NONBLOCK);

    signal(SIGSEGV, crashSignalHandler);
    signal(SIGABRT, crashSignalHandler);
}

} // namespace

int main(int argc, char* argv[]) {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/nada";
    QDir().mkpath(logDir);

    auto logger = spdlog::rotating_logger_mt("nada", (logDir + "/nada.log").toStdString(), 5 * 1024 * 1024, 3);
    spdlog::set_default_logger(logger);
#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::debug);
#endif

    setupCrashHandler();

    QApplication app(argc, argv);
    app.setApplicationName("Nada");
    app.setOrganizationName("Nada Audio");
    app.setFont(Theme::uiFont());
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setPalette(Theme::darkPalette());
    app.setStyleSheet(Theme::globalQss());
    app.setWindowIcon(QIcon(":/icons/app_icon.svg"));

    QSocketNotifier* crashNotifier = new QSocketNotifier(crashPipe[0], QSocketNotifier::Read, &app);
    QObject::connect(crashNotifier, &QSocketNotifier::activated, [&](int) {
        int sig = 0;
        read(crashPipe[0], &sig, sizeof(sig));
        QMessageBox::critical(nullptr, "Nada Crashed",
            QString("Nada encountered a fatal error (%1).\nLog: %2/nada.log")
                .arg(sig == SIGSEGV ? "segfault" : "abort")
                .arg(logDir));
        QTimer::singleShot(0, qApp, &QApplication::quit);
    });

    MainWindow window;
    window.show();

    return app.exec();
}
