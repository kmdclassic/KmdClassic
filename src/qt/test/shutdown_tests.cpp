// Exercise the real application's shutdown completion slot without starting a node.
#define KOMODO_QT_TEST
#include "../komodoapp.cpp"
#include "qtrpctimer.h"

#include <QCloseEvent>
#include <QPointer>
#include <QSemaphore>
#include <QTemporaryDir>

#include <atomic>
#include <cstdio>
#include <memory>
#include <stdexcept>

static std::atomic<int> timerWarnings{0};

static void messages(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    if (message.contains("Timers cannot be stopped from another thread")) ++timerWarnings;
    if (type != QtDebugMsg) std::fprintf(stderr, "%s\n", message.toUtf8().constData());
}

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static void checkTimers()
{
    QThread worker;
    auto* context = new QObject;
    context->moveToThread(&worker);
    QObject::connect(&worker, &QThread::finished, context, &QObject::deleteLater);
    worker.start();
    struct Cleanup {
        QThread& thread;
        ~Cleanup() { thread.quit(); thread.wait(); }
    } cleanup{worker};
    QtRPCTimerInterface factory;
    std::unique_ptr<RPCTimerBase> timer;
    QSemaphore fired;
    std::atomic<bool> correctThread{false};
    boost::function<void(void)> callback = [&] {
        correctThread = QThread::currentThread() == &worker;
        fired.release();
    };
    QMetaObject::invokeMethod(context, [&] { timer.reset(factory.NewTimer(callback, 0)); }, Qt::BlockingQueuedConnection);
    require(fired.tryAcquire(1, 3000) && correctThread, "RPC timer callback did not run in its owning thread");
    timer.reset(); // Handle deletion from the GUI thread while worker is running.
    QMetaObject::invokeMethod(context, [] {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }, Qt::BlockingQueuedConnection);

    // Cancel before dispatching a zero-delay timeout, and drain deferred deletes.
    QSemaphore cancelNow;
    QSemaphore created;
    QMetaObject::invokeMethod(context, [&] {
        timer.reset(factory.NewTimer(callback, 0));
        created.release();
        cancelNow.acquire();
    }, Qt::QueuedConnection);
    require(created.tryAcquire(1, 3000), "could not create cancellation timer");
    timer.reset();
    cancelNow.release();
    QMetaObject::invokeMethod(context, [] {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }, Qt::BlockingQueuedConnection);
    require(!fired.tryAcquire(), "cancelled RPC timer still fired");

    QMetaObject::invokeMethod(context, [&] { timer.reset(factory.NewTimer(callback, 60000)); }, Qt::BlockingQueuedConnection);
    worker.quit();
    require(worker.wait(3000), "RPC thread failed to stop");
    timer.reset(); // The console thread can finish before StopRPC clears timers.
    require(timerWarnings == 0, "RPC timers were destroyed from the wrong thread");
    std::puts("RPC timers: callback, cross-thread cancellation and stopped-thread cleanup passed");
}

int main()
{
    fPrintToDebugLog = false;
    Q_INIT_RESOURCE(komodo);
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    KomodoApplication app;
    app.setOrganizationName("KmdClassicTests");
    app.setApplicationName("Shutdown");
    qInstallMessageHandler(messages);
    try {
        require(settings.isValid(), "could not isolate settings");
        checkTimers();
        ShutdownWindow window;
        window.show();
        require(!window.close(), "shutdown window must reject premature close");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        bool quitVetoed = false;
        QTimer fallback;
        fallback.setSingleShot(true);
        QObject::connect(&fallback, &QTimer::timeout, [&] { quitVetoed = true; app.exit(); });
        fallback.start(50);
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
        app.exec();
        require(quitVetoed, "test did not reproduce the Qt 6 shutdown-window quit veto");
#endif
        bool timedOut = false;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, [&] { timedOut = true; app.exit(1); });
        timeout.start(3000);
        QTimer::singleShot(0, &app, &KomodoApplication::shutdownResult);
        require(app.exec() == 0 && !timedOut, "shutdown completion failed to exit the GUI event loop");
        timeout.stop();
        window.hide();
        std::puts("Shutdown: protected window and application completion passed");

        // An initialization failure (such as an occupied data directory) must
        // leave the first event loop so main() can run the normal cleanup path.
        std::unique_ptr<const NetworkStyle> style(NetworkStyle::instantiate("regtest"));
        app.createSplashScreen(style.get());
        QPointer<SplashScreen> splash;
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* candidate = qobject_cast<SplashScreen*>(widget)) splash = candidate;
        }
        require(splash && splash->isVisible(), "startup failure test has no visible splash screen");
        timedOut = false;
        timeout.start(3000);
        QTimer::singleShot(0, &app, [&] { app.initializeResult(false); });
        require(app.exec() == EXIT_FAILURE && !timedOut,
                "initialization failure remained in the GUI event loop");
        timeout.stop();
        require(app.getReturnValue() == EXIT_FAILURE, "initialization failure lost its process exit status");
        require(!splash || !splash->isVisible(), "initialization failure left the splash visible");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(!splash, "initialization failure did not release the splash screen");
        timedOut = false;
        timeout.start(3000);
        QTimer::singleShot(0, &app, &KomodoApplication::shutdownResult);
        require(app.exec() == 0 && !timedOut && app.getReturnValue() == EXIT_FAILURE,
                "cleanup after initialization failure did not preserve the failure status");
        std::puts("Startup failure: splash cleanup, event-loop exit and failure status passed");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Shutdown regression: %s\n", e.what());
        return 1;
    }
    return 0;
}
