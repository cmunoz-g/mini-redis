#include "threadpool.hh"
#include <cassert>

void thread_pool_queue(ThreadPool *tp, void(*f)(void *), void *arg) {
    pthread_mutex_lock(&tp->mu);
    tp->queue.push_back(Work{f, arg});
    pthread_cond_signal(&tp->not_empty);
    pthread_mutex_unlock(&tp->mu);
}

static void *worker(void *arg) {
    ThreadPool *tp = static_cast<ThreadPool *>(arg);

    for (;;) {
        pthread_mutex_lock(&tp->mu);
        while (tp->queue.empty()) {
            pthread_cond_wait(&tp->not_empty, &tp->mu);
        }

        Work w = tp->queue.front();
        tp->queue.pop_front();
        pthread_mutex_unlock(&tp->mu);
        w.f(w.arg);
    }
}

void thread_pool_init(ThreadPool *tp, size_t num_threads) {
    pthread_mutex_init(&tp->mu, nullptr);
    pthread_cond_init(&tp->not_empty, nullptr);
    tp->threads.resize(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        int rv = pthread_create(&tp->threads[i], nullptr, &worker, tp);
        assert(rv == 0); // best way to check for errors ?
    }
}

void thread_pool_destroy(ThreadPool *tp) {
    pthread_mutex_destroy(&tp->mu);
    pthread_cond_destroy(&tp->not_empty);
}