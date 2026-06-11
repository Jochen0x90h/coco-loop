#pragma once

#include <coco/Loop.hpp>
#include <coco/IntrusiveSortedTaskList.hpp>
#include <linux/io_uring.h>
#include <cassert>
#include <cstdint>
#include <poll.h>
#include <sys/syscall.h>
#include <sys/socket.h> // msghdr
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

/*
    void invoke(TimedTask<Callback> &task, Time time) {
        task.cancelAndSet(time);
        this->sleepTasks1_.add(task);
    }

    void invoke(TimedTask<Callback> &task, Duration duration = {}) {
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
        virtual void onCompletion(io_uring_cqe &cqe, int id) = 0;
    };

    /// @brief Submit a connect operation.
    /// @tparam Type of address, e.g. sockaddr_in or sockaddr_in6
    /// @param handler Handler gets called on completion
    /// @param socket Socket file descriptor to operate on
    /// @param address Address to connect to
    /// @return False if submission queue is full
    template <typename T>
    bool connect(CompletionHandler &handler, int socket, T &address) {
        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        if ((tail - head) >= sq_.mask)
            return false;

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = IORING_OP_CONNECT,
            .flags = IOSQE_IO_LINK,
            .fd = socket,
            .off = sizeof(T),
            .addr = uint64_t(&address),
            .user_data = uint64_t(&handler)};
        sq_.array[index] = index;

        index = (tail + 1) & sq_.mask;
        sq_.entries[index] = {
            .opcode = IORING_OP_POLL_ADD,
            .fd = socket,
            .poll_events = POLLOUT,
            .user_data = uint64_t(&handler)};
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 2, __ATOMIC_RELEASE);

        // submit
        int result = io_uring_enter(ring_, 2, 0, 0);
        assert(result == 2 && "io_uring enter");
        return true;
    }

    /// @brief Submit a stream send/receive operation.
    /// @param handler Handler gets called on completion
    /// @param op Operation (IORING_OP_SEND, IORING_OP_RECV)
    /// @param socket Socket file descriptor to operate on
    /// @param buffer Buffer data
    /// @param length Buffer length
    /// @return False if submission queue is full
    bool transfer(CompletionHandler &handler, uint8_t op, int socket, void *buffer, uint32_t length) {
        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        if ((tail - head) > sq_.mask)
            return false;

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = op,
            .fd = socket,
            .addr = uint64_t(buffer),
            .len = length,
            .user_data = uint64_t(&handler)};
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 1, __ATOMIC_RELEASE);

        // submit
        int result = io_uring_enter(ring_, 1, 0, 0);
        assert(result == 1 && "io_uring enter");
        return true;
    }

    /// @brief Submit a message send/receive operation.
    /// @param handler Handler gets called on completion
    /// @param op Operation (IORING_OP_SENDMSG, ORING_OP_RECVMSG)
    /// @param socket Socket file descriptor to operate on
    /// @param message Message to send (of type msghdr)
    /// @param flags Flags of sendmsg()/recvmsg() (e.g. MSG_DONTWAIT, MSG_OOB, MSG_NOSIGNAL)
    /// @return False if submission queue is full
    bool transfer(CompletionHandler &handler, uint8_t op, int socket, msghdr &message, uint32_t flags) {
        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        if ((tail - head) > sq_.mask)
            return false;

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = op,
            .fd = socket,
            .addr = uint64_t(&message),
            .msg_flags = flags,
            .user_data = uint64_t(&handler)};
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 1, __ATOMIC_RELEASE);

        // submit
        int result = io_uring_enter(ring_, 1, 0, 0);
        assert(result == 1 && "io_uring enter");
        return true;
    }

    /// @brief Submit a file read/write operation.
    /// @param handler Handler gets called on completion
    /// @param op Operation (IORING_OP_READ, IORING_OP_WRITE)
    /// @param file File descriptor to operate on
    /// @param offset Offset into file
    /// @param buffer Buffer data
    /// @param length Buffer length
    /// @return False if submission queue is full
    bool transfer(CompletionHandler &handler, uint8_t op, int file, uint64_t offset, void *buffer, uint32_t length) {
        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        if ((tail - head) > sq_.mask)
            return false;

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = op,
            .fd = file,
            .off = offset,
            .addr = uint64_t(buffer),
            .len = length,
            .user_data = uint64_t(&handler)};
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 1, __ATOMIC_RELEASE);

        // submit
        int result = io_uring_enter(ring_, 1, 0, 0);
        assert(result == 1 && "io_uring enter");
        return true;
    }

    /// @brief Poll for readable/writable.
    /// @param handler Handler gets called on completion
    /// @param fd File descriptor (e.g. regular file or socket)
    /// @param events Events to poll for (e.g. POLLIN, POLLOUT)
    /// @param id Id in range 0-3 to distinguish multiple poll operations for the same handler (e.g. for different file descriptors)
    /// @return False if submission queue is full
    bool poll(CompletionHandler &handler, int fd, int events, int id = 0) {
        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        if ((tail - head) > sq_.mask)
            return false;

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = IORING_OP_POLL_ADD,
            .fd = fd,
            .poll_events = __u16(events),
            .user_data = uint64_t(&handler) | id};
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 1, __ATOMIC_RELEASE);

        // submit
        int result = io_uring_enter(ring_, 1, 0, 0);
        assert(result == 1 && "io_uring enter");
        return true;
    }

    /// @brief Cancel a pending operation for the given handler.
    /// @param handler Handler to cancel an operation for
    /// @return False if submission queue is full
    bool cancel(CompletionHandler &handler) {
        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        if ((tail - head) > sq_.mask)
            return false;

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = IORING_OP_ASYNC_CANCEL,
            .addr = uint64_t(&handler),
            .user_data = 1};
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 1, __ATOMIC_RELEASE);

        // submit
        int result = io_uring_enter(ring_, 1, 0, 0);
        assert(result == 1 && "io_uring enter");
        return true;
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
    IntrusiveSortedTaskList<TimeoutHandler> sleepTasks1_;
    CoroutineTimedTaskList sleepTasks2_;
};

} // namespace coco
