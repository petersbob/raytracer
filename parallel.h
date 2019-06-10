#ifndef PARALLELH
#define PARALLELH

#include <thread>
#include <future>
#include <vector>

class join_threads {
    std::vector<std::thread>& threads;
    public:
        explicit join_threads(std::vector<std::thread>& _threads) : threads(_threads) {}
        ~join_threads() {
            for(unsigned long i=0;i<threads.size();i++) {
                if(threads[i].joinable()) {
                    threads[i].join();
                }
            }
        }
};

void parallel_for_each(int first, int last, const std::function<void(int)> &f){
    unsigned long const length = last-first;

    if (!length) return;

    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;

    unsigned long const hardware_threads= std::thread::hardware_concurrency();

    unsigned long const num_threads=std::min(hardware_threads!=0?hardware_threads:2,max_threads);
    unsigned long const block_size=length/num_threads;

    std::vector<std::future<void>> futures(num_threads-1);
    std::vector<std::thread> threads(num_threads-1);
    join_threads joiner(threads);

    int block_start=first;

    for (unsigned long i=0;i<(num_threads-1);i++) {
        int block_end=block_start;
        block_end += block_size;
        std::packaged_task<void(void)> task(
            [=]() {
                for (unsigned long j=block_start;j<block_end;j++) {
                    f(j);
                }
            });
        futures[i]=task.get_future();
        threads[i]=std::thread(std::move(task));
        block_start=block_end;
    }
    for (unsigned long i=block_start;i<last;i++) f(i);

    for(unsigned long i=0;i<(num_threads-1);i++) {
        futures[i].get();
    }
}

#endif