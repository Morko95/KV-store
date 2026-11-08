#include <cstddef>
#include "thread_queue.hpp"
#include <memory>

template <typename task_type, size_t thread_num, size_t buffer_capacity>
class InboundQueue{
    public:
        std::array<std::unique_ptr<SpscRingBuffer<task_type, buffer_capacity>>, thread_num*thread_num> queues;

        bool put(size_t curr_thread_idx, size_t target_thread_idx, task_type task){
            return queues[target_thread_idx*thread_num + target_thread_idx]->push(std::move(task));
        }

};
