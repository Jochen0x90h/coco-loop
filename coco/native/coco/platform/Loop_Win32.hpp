#pragma once

#include <coco/DevicePath.hpp>
#include <coco/Loop.hpp>
#include <coco/IntrusiveSortedTaskList.hpp>
#include <coco/IntrusiveTaskList.hpp>

#include <coco/platform/WindowsDef.hpp>
#include <Windows.h>
#include <coco/platform/WindowsUndef.hpp>


namespace coco {

/// @brief Implementation of the Loop interface using Win32 and io completion ports
///
class Loop_Win32 : public Loop {
public:
    /// @brief Constructor.
    /// @param noWindowMessages Don't process window messages when they are handled e.g. by GLFW (has effect only on Windows)
    Loop_Win32(bool noWindowMessages = false);
    ~Loop_Win32() override;

    // Loop methods
    void run() override;
    [[nodiscard]] Time now() override;
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
    using Loop::sleep;

/*
    void invoke(TimedTask<Callback<>> &task, Time time) {
        task.cancelAndSet(time);
        this->sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback<>> &task, Duration duration = {}) {
        task.cancelAndSet(now() + duration);
        this->sleepTasks1_.add(task);
    }
*/

    /// @brief Timeout handler.
    ///
    class TimeoutHandler : private IntrusiveListNode {
        friend class IntrusiveSortedTaskList<TimeoutHandler>;
    public:
        using IntrusiveListNode::remove;

        virtual ~TimeoutHandler() {}
        virtual void onTimeout() = 0;
        void operator ()() {onTimeout();}

    private:
        Time value;
    };

    void invoke(TimeoutHandler &handler, Time time) {
        this->sleepTasks1_.add(handler, time);
    }

    void invoke(TimeoutHandler &handler, Duration duration = {}) {
        this->sleepTasks1_.add(handler, now() + duration);
    }


    /// @brief IO Completion handler.
    ///
    class CompletionHandler {
    public:
        virtual ~CompletionHandler() {}
        virtual void onCompletion(OVERLAPPED *overlapped) = 0;
    };


    /// @brief Get the handle of the IO completion port.
    /// @return The handle of the IO completion port
    HANDLE port() {return port_;}


    enum class DeviceType {
        UNKNOWN,
        USB,
        COM
    };

    /// @brief Handler for device events.
    /// Windows propagates device events as window messages, therefore a window is needed to receive the messages.
    class DeviceHandler : private IntrusiveListNode {
        friend class Loop_Win32;
        friend class IntrusiveList<DeviceHandler>;
    public:
        virtual ~DeviceHandler() {}
        virtual void onDeviceChange(DeviceType type, bool add, DevicePath path) = 0;
    };

    void addDeviceHandler(DeviceHandler &handler);


    /// @brief Handle events and wait at most the given number of milliseconds for new events
    /// @param wait maximum time to wait in milliseconds
    void handleEvents(int wait = std::numeric_limits<int>::max() / 2);

protected:
    bool noWindowMessages_;

    // frequency for QueryPerformanceCounter
    int64_t frequency_;

    // sleep tasks
    //TimedTaskList<Callback<>> sleepTasks1_;
    IntrusiveSortedTaskList<TimeoutHandler> sleepTasks1_;
    CoroutineTimedTaskList sleepTasks2_;

    // io completion port
    HANDLE port_;

    // device handlers
    HWND window_ = nullptr;
    IntrusiveList<DeviceHandler> deviceHandlers_;
};

} // namespace coco
