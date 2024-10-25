#ifndef __SYNC_QUEUE_HPP__
#define __SYNC_QUEUE_HPP__
#include "CoreMinimal.h"
#include "UMuxer.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

template<typename EType>
class UMUXER_API SyncQueue
{
public:
     SyncQueue(int capacity = 400)
     :capacity_(capacity)
     ,start_threshold(200)
     ,ending_(false){}

    ~ SyncQueue(){};

    int try_read(EType &element){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        if(q_.empty()){
            if(ending_){
                UE_LOG(LogUMuxer, Verbose, TEXT("queue is empty and finished"));
                //DLOG(INFO) << "queue is empty and finished";
                return -1;
            }
            else{
                UE_LOG(LogUMuxer, Verbose, TEXT("queue is empty ,but may not be finished,to be continued"));
                //DLOG(INFO) << "queue is empty ,but may not be finished,to be continued";
                return 1;
            }
        }

        EType val = q_.front();
        q_.pop();
        if(q_.size() <= capacity_){
            UE_LOG(LogUMuxer, Verbose, TEXT("waking up writer"));
            //DLOG(INFO) << "waking up writer";
            cv_.notify_all();
        }

        element = val;
        return 0;
    }

    int read(EType &element){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        if(q_.empty()){
            UE_LOG(LogUMuxer, Verbose, TEXT("queue is empty ,but may not be finished,to be continued"));
            //DLOG(INFO) << "queue is full ,waiting for reader";
            cv_.wait(lck, [this]() {
                return ending_ || !q_.empty();
                });
            if (ending_ && q_.empty()) {
                return -1;
            }
        }
        EType val = q_.front();
        q_.pop();

        if (q_.size() <= capacity_) {
            UE_LOG(LogUMuxer, Verbose, TEXT("waking up writer"));
            cv_.notify_all();
        }

        element = val;
        return 0;
    }

    int write(EType& element) {
        std::unique_lock<std::mutex> lck(queue_mtx_);
        if (q_.size() == capacity_) {
            cv_.wait(lck);
        }
        q_.push(element);
        cv_.notify_all();
        return 0;
    }

    void force(bool exit){
        std::lock_guard<std::mutex> lck(queue_mtx_);
        ending_ = exit;
        cv_.notify_all();
    }

    //is queue empty
    bool empty(){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        return q_.empty();
    }

    //is queue ended
    bool ended(){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        return ending_;
    }

    //is queue full
    bool full(){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        return q_.size() == capacity_;
    }
    
    //called before read/write operation
    void set_start_threshold(int element_num){
        start_threshold = element_num;
    }

    void clear(){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        std::queue<EType> empty;
        std::swap(q_,empty);
        cv_.notify_all();
    }

    size_t get_size(){
        std::unique_lock<std::mutex> lck(queue_mtx_);
        return q_.size();
    }

private:
    std::queue<EType> q_;
    std::mutex queue_mtx_;
    std::condition_variable cv_;
    size_t capacity_;
    int start_threshold;
    bool ending_;
};

#endif // !__SYNC_QUEUE_HPP__