// Run with a process timeout: a TLS cleanup failure can hang after main().
#include <QCoreApplication>
#include <QThread>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

static std::atomic<int> destroyed{0};
static std::atomic<int> invalidStorage{0};
static thread_local int value = 0;

struct Cleanup {
    ~Cleanup()
    {
        if (value != 42) ++invalidStorage;
        ++destroyed;
    }
};

static void useThreadData()
{
    // Exercise both Qt's per-thread cleanup and GCC's emulated TLS storage.
    QThread::currentThread();
    value = 42;
    static thread_local Cleanup cleanup;
}

class Worker : public QThread {
    void run() override { useThreadData(); }
};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    for (int i = 0; i < 16; ++i) {
        Worker worker;
        worker.start();
        if (!worker.wait(5000)) {
            std::fputs("QThread did not finish\n", stderr);
            std::fflush(stderr);
            std::abort();
        }
        // Qt also adopts threads created outside QThread.
        std::thread adopted(useThreadData);
        adopted.join();
    }
    if (destroyed != 32 || invalidStorage != 0) {
        std::fprintf(stderr, "TLS cleanup failed: destroyed=%d, invalid storage=%d\n",
                     destroyed.load(), invalidStorage.load());
        return 1;
    }
    std::puts("TLS cleanup passed for Qt and adopted threads; returning from main");
    return 0;
}
