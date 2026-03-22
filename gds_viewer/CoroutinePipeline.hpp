#pragma once

#include <QCoreApplication>
#include <QDebug>
#include <QFutureWatcher>
#include <QPointer>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include <atomic>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace coroutine_pipeline::detail {

inline QObject* dispatcherObject() noexcept {
    return QCoreApplication::instance();
}

struct ContinuationState {
    explicit ContinuationState(std::coroutine_handle<> handle_) noexcept
        : handle(handle_) {}

    std::coroutine_handle<> handle;
    std::atomic_bool completed{false};
};

inline void disconnectIfValid(const std::shared_ptr<QMetaObject::Connection>& connection) {
    if (connection && *connection) {
        QObject::disconnect(*connection);
    }
}

inline void postResume(const std::shared_ptr<ContinuationState>& continuation,
                       QObject* dispatcher,
                       const std::shared_ptr<QMetaObject::Connection>& connection = {}) {
    QTimer::singleShot(0, dispatcher, [continuation, connection]() mutable {
        disconnectIfValid(connection);

        bool expected = false;
        if (continuation->completed.compare_exchange_strong(expected, true)) {
            continuation->handle.resume();
        }
    });
}

inline void postDestroy(const std::shared_ptr<ContinuationState>& continuation,
                        QObject* dispatcher,
                        const std::shared_ptr<QMetaObject::Connection>& connection = {}) {
    QTimer::singleShot(0, dispatcher, [continuation, connection]() mutable {
        disconnectIfValid(connection);

        bool expected = false;
        if (continuation->completed.compare_exchange_strong(expected, true)) {
            continuation->handle.destroy();
        }
    });
}

template <typename Result>
struct BackgroundJobResult {
    std::shared_ptr<Result> value;
    std::exception_ptr exception;
};

template <>
struct BackgroundJobResult<void> {
    std::exception_ptr exception;
};

template <typename Result>
struct BackgroundSharedState {
    QPointer<QObject> context;
    std::shared_ptr<QMetaObject::Connection> contextDestroyedConnection =
        std::make_shared<QMetaObject::Connection>();
    BackgroundJobResult<Result> result;
};

template <typename Result>
decltype(auto) consumeResult(BackgroundJobResult<Result>& result) {
    if (result.exception) {
        std::rethrow_exception(result.exception);
    }

    if constexpr (std::is_void_v<Result>) {
        return;
    } else {
        return std::move(*result.value);
    }
}

} // namespace coroutine_pipeline::detail

class DetachedTask {
public:
    struct promise_type {
        DetachedTask get_return_object() noexcept {
            return {};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        struct FinalAwaiter {
            bool await_ready() const noexcept {
                return false;
            }

            template <class Promise>
            void await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
                handle.destroy();
            }

            void await_resume() const noexcept {}
        };

        FinalAwaiter final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            try {
                if (auto exception = std::current_exception()) {
                    std::rethrow_exception(exception);
                }
            } catch (const std::exception& ex) {
                qCritical() << "DetachedTask unhandled exception:" << ex.what();
            } catch (...) {
                qCritical() << "DetachedTask unhandled non-std exception";
            }
        }
    };
};

class GuiYieldAwaitable {
public:
    explicit GuiYieldAwaitable(QObject* context) noexcept
        : context_(context) {}

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) const {
        QObject* dispatcher = coroutine_pipeline::detail::dispatcherObject();
        if (dispatcher == nullptr || context_.isNull()) {
            handle.destroy();
            return;
        }

        auto continuation = std::make_shared<coroutine_pipeline::detail::ContinuationState>(handle);
        auto connection = std::make_shared<QMetaObject::Connection>();

        if (QObject* contextObject = context_.data()) {
            *connection = QObject::connect(
                contextObject,
                &QObject::destroyed,
                dispatcher,
                [continuation, dispatcher, connection]() mutable {
                    coroutine_pipeline::detail::postDestroy(continuation, dispatcher, connection);
                });
        }

        coroutine_pipeline::detail::postResume(continuation, dispatcher, connection);
    }

    void await_resume() const noexcept {}

private:
    QPointer<QObject> context_;
};

inline GuiYieldAwaitable nextGuiCycle(QObject* context) {
    return GuiYieldAwaitable(context);
}

template <typename Function>
class BackgroundAwaitable {
public:
    using FunctionType = std::decay_t<Function>;
    using Result = std::invoke_result_t<FunctionType&>;

    static_assert(!std::is_reference_v<Result>,
                  "runInBackground() does not support reference return types; "
                  "return a value, pointer, or std::reference_wrapper explicitly.");

    using JobResult = coroutine_pipeline::detail::BackgroundJobResult<Result>;
    using SharedState = coroutine_pipeline::detail::BackgroundSharedState<Result>;

    BackgroundAwaitable(QObject* context, Function&& function)
        : context_(context)
        , function_(std::forward<Function>(function))
        , state_(std::make_shared<SharedState>()) {
        state_->context = context;
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        QObject* dispatcher = coroutine_pipeline::detail::dispatcherObject();
        if (dispatcher == nullptr || context_.isNull()) {
            handle.destroy();
            return;
        }

        auto continuation = std::make_shared<coroutine_pipeline::detail::ContinuationState>(handle);
        auto state = state_;

        if (QObject* contextObject = context_.data()) {
            *state->contextDestroyedConnection = QObject::connect(
                contextObject,
                &QObject::destroyed,
                dispatcher,
                [continuation, dispatcher, connection = state->contextDestroyedConnection]() mutable {
                    coroutine_pipeline::detail::postDestroy(continuation, dispatcher, connection);
                });
        }

        auto* watcher = new QFutureWatcher<JobResult>(dispatcher);
        QObject::connect(
            watcher,
            &QFutureWatcher<JobResult>::finished,
            dispatcher,
            [watcher, state, continuation, dispatcher]() mutable {
                state->result = watcher->result();
                watcher->deleteLater();

                if (state->context.isNull()) {
                    coroutine_pipeline::detail::postDestroy(
                        continuation,
                        dispatcher,
                        state->contextDestroyedConnection);
                    return;
                }

                coroutine_pipeline::detail::postResume(
                    continuation,
                    dispatcher,
                    state->contextDestroyedConnection);
            });

        watcher->setFuture(QtConcurrent::run(
            [function = std::move(function_)]() mutable -> JobResult {
                JobResult result;

                try {
                    if constexpr (std::is_void_v<Result>) {
                        std::invoke(function);
                    } else {
                        result.value = std::make_shared<Result>(std::invoke(function));
                    }
                } catch (...) {
                    result.exception = std::current_exception();
                }

                return result;
            }));
    }

    decltype(auto) await_resume() {
        return coroutine_pipeline::detail::consumeResult<Result>(state_->result);
    }

private:
    QPointer<QObject> context_;
    FunctionType function_;
    std::shared_ptr<SharedState> state_;
};

template <typename Function>
auto runInBackground(QObject* context, Function&& function) {
    return BackgroundAwaitable<Function>(context, std::forward<Function>(function));
}
