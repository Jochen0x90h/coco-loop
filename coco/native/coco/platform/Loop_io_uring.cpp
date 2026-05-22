#include "Loop_io_uring.hpp"
//#include <iterator>
#include <iostream>
#include <sys/mman.h>
#include <cstring>


namespace coco {

static inline int io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return syscall(__NR_io_uring_setup, entries, p);
}

Loop_io_uring::Loop_io_uring() {
    io_uring_params params;
    memset(&params, 0, sizeof(params));
    //params.flags = IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 10; // 10ms idle time
    //params.sq_thread_cpu  =   // CPU-affinity
    params.features = IORING_FEAT_SINGLE_MMAP;

    int ring_fd = io_uring_setup(32, &params);
    if (ring_fd < 0) {
        std::cerr << "io_uring_setup failed" << std::endl;
        return;
    }

    int sqSize = params.sq_off.array + params.sq_entries * sizeof(__u32);
    int cqSize = params.cq_off.cqes + params.cq_entries * sizeof(io_uring_cqe);
    int ringSize = std::max(sqSize, cqSize);

    // submission and completion queue ring data
    uint8_t *ringData = (uint8_t *)mmap(nullptr, ringSize,
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
        ring_fd, IORING_OFF_SQ_RING);
    if (ringData == MAP_FAILED){
        std::cerr << "mmap failed" << std::endl;
        return;
    }

    // submission queue entries
    void *sqes = mmap(NULL, params.sq_entries * sizeof(struct io_uring_sqe),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
        ring_fd, IORING_OFF_SQES);
    if (sqes == MAP_FAILED) {
        std::cerr << "mmap failed" << std::endl;
        return;
    }

    ring_ = ring_fd;
    ringData_ = ringData;

    sq_.head = (uint32_t *)(ringData + params.sq_off.head);
    sq_.tail = (uint32_t *)(ringData + params.sq_off.tail);
    sq_.mask = params.sq_entries - 1;
    sq_.flags = (uint32_t *)(ringData + params.sq_off.flags);
    sq_.array = (uint32_t *)(ringData + params.sq_off.array);
    sq_.entries = (io_uring_sqe *)sqes;

    cq_.head = (uint32_t *)(ringData + params.cq_off.head);
    cq_.tail = (uint32_t *)(ringData + params.cq_off.tail);
    cq_.mask = params.cq_entries - 1;
    cq_.entries = (io_uring_cqe *)(ringData + params.cq_off.cqes);
}

Loop_io_uring::~Loop_io_uring() {
    int ringSize = std::max((uint8_t *)&sq_.array[sq_.mask + 1], (uint8_t *)&cq_.entries[cq_.mask + 1]) - ringData_;
    munmap(ringData_, ringSize);
    munmap(sq_.entries, (sq_.mask + 1) * sizeof(io_uring_sqe));

    close(ring_);
}

void Loop_io_uring::run() {
    while (!exitFlag_) {
        handleEvents();
    }
    exitFlag_ = false;
}

Loop::Time Loop_io_uring::now() {
    timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    Time t = Time(time.tv_sec * 1000 + time.tv_nsec / 1000000);
    //std::cout << "now   " << t.value << std::endl;
    return t;
}

Awaitable<CoroutineTimedTask> Loop_io_uring::sleep(Time time) {
    //std::cout << "sleep " << time.value << std::endl;
    return {sleepTasks2_, time};
}

int Loop_io_uring::handleEvents(int wait) {
    // determine timeout in milliseconds
    Time currentTime = now();
    Time sleepTime = sleepTasks2_.getFirstTime(sleepTasks1_.getFirstTime(currentTime + wait * 1ms));
    int t = (sleepTime - currentTime).value;
    int result = 0;
    if (t > 0) {
        __kernel_timespec timeout;
        timeout.tv_nsec = (t % 1000) * 1000000;
        timeout.tv_sec = t / 1000;

        uint32_t tail = __atomic_load_n(sq_.tail, __ATOMIC_RELAXED);
        const uint32_t head = __atomic_load_n(sq_.head, __ATOMIC_ACQUIRE);
        assert((tail - head) <= sq_.mask && "io_uring full");

        int index = tail & sq_.mask;
        sq_.entries[index] = {
            .opcode = IORING_OP_TIMEOUT,
            .off = 1, // timer also elapses after one other completion
            .addr = uint64_t(&timeout),
            .len = 1, // one timespec structure
            .user_data = uint64_t(nullptr)}; // timeout is identified by user_data == nullptr
        sq_.array[index] = index;

        // increment tail to make submission visible
        __atomic_store_n(sq_.tail, tail + 1, __ATOMIC_RELEASE);

        // submit and wait for at least one completion
        int result = io_uring_enter(ring_, 1, 1, IORING_ENTER_GETEVENTS);
        assert(result == 1 && "io_uring timeout");
    }

    // call handler of completed operations
    const uint32_t tail = __atomic_load_n(cq_.tail, __ATOMIC_ACQUIRE);
    uint32_t head = __atomic_load_n(cq_.head, __ATOMIC_RELAXED);
    if (head != tail) {
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        do {
            io_uring_cqe &cqe = cq_.entries[head & cq_.mask];

            // get hander (0 is timeout, 1 is cancel)
            auto handler = cqe.user_data;
            if (handler > 1)
                reinterpret_cast<CompletionHandler *>(handler)->onCompletion(cqe);

            ++head;
        } while (head != tail);
        __atomic_store_n(cq_.head, head, __ATOMIC_RELEASE);
    }

    // resume coroutines waiting on sleep() and activate time handlers
    {
        Time currentTime = now();
        sleepTasks1_.doUntil(currentTime, [](TimeoutHandler &handler) {handler.onTimeout();});
        sleepTasks2_.doUntil(currentTime);
    }

    return result;
}

} // namespace coco
