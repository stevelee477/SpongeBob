/** Redundance check. **/
#ifndef GLOBAL_HEADER
#define GLOBAL_HEADER
#include <stdint.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sys/syscall.h>
#include <unistd.h>

using namespace std;

/* A global queue manager. */
template <typename T>
class Queue {
private:
    std::vector<T> queue;
    std::mutex m;
    std::condition_variable cond;
    uint8_t offset = 0;
public:
    Queue(){}
    ~Queue(){}
    T pop() {
        std::unique_lock<std::mutex> mlock(m);
        while (queue.empty()) {
            cond.wait(mlock);
        }
        auto item = queue.front();
        queue.erase(queue.begin());
        return item;
    }
    T PopPolling() {
        while (offset == 0);
        auto item = queue.front();
        queue.erase(queue.begin());
         __sync_fetch_and_sub(&offset, 1);
         return item;
    }
    void push(T item) {
        std::unique_lock<std::mutex> mlock(m);
        queue.push_back(item);
        mlock.unlock();
        cond.notify_one();
    }
    void PushPolling(T item) {
        queue.push_back(item);
        __sync_fetch_and_add(&offset, 1);
    }
};

/** Redundance check. **/
#endif
