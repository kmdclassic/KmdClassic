// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KOMODO_QT_QTRPCTIMER_H
#define KOMODO_QT_QTRPCTIMER_H

#include "rpc/server.h"

#include <QThread>
#include <QTimer>

#include <memory>
#include <mutex>

namespace QtRPCTimerDetail {
struct State {
    std::mutex mutex;
    QTimer* timer = nullptr;
    bool cancelled = false;
};

// This object always dies in its owning Qt thread. The RPC handle can outlive
// that thread, or be cancelled by another thread during replacement/shutdown.
class Timer final : public QTimer
{
public:
    Timer(const std::shared_ptr<State>& state, const boost::function<void(void)>& func) : state(state)
    {
        setSingleShot(true);
        connect(this, &QTimer::timeout, this, [state, func] {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (state->cancelled) return;
            }
            func();
        });
        connect(QThread::currentThread(), &QThread::finished, this, &QObject::deleteLater);
    }

    ~Timer() override
    {
        // Clear the handle before QObject destruction begins. A concurrent
        // cancellation can then safely queue deleteLater, or see no timer.
        std::lock_guard<std::mutex> lock(state->mutex);
        state->timer = nullptr;
    }

private:
    std::shared_ptr<State> state;
};
}

class QtRPCTimerBase : public RPCTimerBase
{
public:
    QtRPCTimerBase(boost::function<void(void)>& func, int64_t millis) :
        state(std::make_shared<QtRPCTimerDetail::State>())
    {
        auto* timer = new QtRPCTimerDetail::Timer(state, func);
        state->timer = timer;
        timer->start(millis);
    }

    ~QtRPCTimerBase() override
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->cancelled = true;
        if (state->timer) state->timer->deleteLater();
    }

private:
    std::shared_ptr<QtRPCTimerDetail::State> state;
};

class QtRPCTimerInterface : public RPCTimerInterface
{
public:
    const char* Name() override { return "Qt"; }
    RPCTimerBase* NewTimer(boost::function<void(void)>& func, int64_t millis) override
    {
        return new QtRPCTimerBase(func, millis);
    }
};

#endif // KOMODO_QT_QTRPCTIMER_H
