#include "threadpool.hh"
#include <assert.h>

/* Internal helper: thread routine */
static void *worker(void *arg) {
    ThreadPool *tp = static_cast<ThreadPool *>(arg);

    for (;;) {
        pthread_mutex_lock(&tp->mu);
        while (tp->queue.empty() && !tp->stop) {
            pthread_cond_wait(&tp->not_empty, &tp->mu);
        }

        if (tp->stop) {
            pthread_mutex_unlock(&tp->mu);
            return nullptr;
        }

        Work w = tp->queue.front();
        tp->queue.pop_front();
        pthread_mutex_unlock(&tp->mu);
        w.f(w.arg);
    }
}

/* API */
void thread_pool_queue(ThreadPool *tp, void(*f)(void *), void *arg) {
    pthread_mutex_lock(&tp->mu);
    tp->queue.push_back(Work{f, arg});
    pthread_cond_signal(&tp->not_empty);
    pthread_mutex_unlock(&tp->mu);
}

void thread_pool_init(ThreadPool *tp, size_t num_threads) {
    tp->stop = false;
    pthread_mutex_init(&tp->mu, nullptr);
    pthread_cond_init(&tp->not_empty, nullptr);
    tp->threads.resize(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        int rv = pthread_create(&tp->threads[i], nullptr, &worker, tp);
        assert(rv == 0);
    }
}

void thread_pool_destroy(ThreadPool *tp) {
    pthread_mutex_lock(&tp->mu);
    tp->stop = true;
    pthread_cond_broadcast(&tp->not_empty);
    pthread_mutex_unlock(&tp->mu);

    for (size_t i = 0; i < tp->threads.size(); ++i) {
        pthread_join(tp->threads[i], nullptr);
    }
    pthread_mutex_destroy(&tp->mu);
    pthread_cond_destroy(&tp->not_empty);
}