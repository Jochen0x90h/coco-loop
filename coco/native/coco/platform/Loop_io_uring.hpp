#pragma once

#include <coco/Loop.hpp>
#include <coco/Callback.hpp>
#include <linux/io_uring.h>
#include <cstdint>
#include <sys/syscall.h>
#include <unistd.h>


namespace coco {

static inline int io_uring_enter(int ring_fd, unsigned to_submit, unsigned min_complete, unsigned flags) {
    return syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete, flags, NULL, 0);
}

/// @brief Implementation of the Loop interface using io_uring
///
class Loop_io_uring : public Loop {
public:

    Loop_io_uring();
    ~Loop_io_uring() override;

    // Loop methods
    void run() override;
    [[nodiscard]] Time now() override;
    [[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;
    using Loop::sleep;


    void invoke(TimedTask<Callback> &task, Time time) {
        task.cancelAndSet(time);
        this->sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback> &task, Duration duration) {
        task.cancelAndSet(now() + duration);
        this->sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback> &task) {
        task.cancelAndSet(now());
        this->sleepTasks1_.add(task);
    }


    /// @brief IO Completion handler.
    ///
    class CompletionHandler {
    public:
        virtual ~CompletionHandler() {}
        virtual void handle() = 0;
    };

    /// @brief Submit an IO operation.
    /// @param op 
    /// @param fd 
    /// @param buffer 
    /// @param length 
    /// @param handler 
    void submit(int op, int fd, void *buffer, int length, CompletionHandler *handler) {
        auto &entry = sq_.entries[*sq_.tail];
        entry.opcode = op;
        entry.flags = 0;
        entry.fd = fd;
        entry.addr = uint64_t(buffer);
        entry.len = length;
        entry.user_data = uint64_t(handler);

        // make submission visible
        *sq_.tail = (*sq_.tail + 1) & sq_.mask;
        __sync_synchronize();

        // submit
        io_uring_enter(ring_, 1, 0, 0);
/*
        // wakeup kernel thread
        if (*sq_.flags & IORING_SQ_NEED_WAKEUP)
            io_uring_enter(ring_, 0, 0, IORING_ENTER_SQ_WAKEUP);
*/
    }

    /// @brief Handle events and wait at most the given number of milliseconds for new events
    /// @param wait maximum time to wait in milliseconds
    int handleEvents(int wait = std::numeric_limits<int>::max() / 2);

protected:
    // io_uring instance
    int ring_ = -1;

    uint8_t *ringData_ = nullptr;

    // submisson queue
    struct SubmissionQueue {
        uint32_t *head;
        uint32_t *tail;
        int mask;
        uint32_t *flags;
        uint32_t *array;
        io_uring_sqe *entries = nullptr;
    };
    SubmissionQueue sq_;

    // completion queue
    struct CompletionQueue {
        uint32_t *head;
        uint32_t *tail;
        int mask;
        io_uring_cqe *entries;
    };
    CompletionQueue cq_;

    // sleep tasks
    TimedTaskList<Callback> sleepTasks1_;
    CoroutineTimedTaskList sleepTasks2_;
};

} // namespace coco
