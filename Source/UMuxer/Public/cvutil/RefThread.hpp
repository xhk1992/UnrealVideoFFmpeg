#ifndef __REF_THREAD_H__
#define __REF_THREAD_H__

#if _MSC_VER >= 1800
#include <thread>

# if PLATFORM_WINDOWS
//#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <process.h>
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
# endif
#elif defined __linux__
#include <thread>
#endif
#include <iostream>
#include <vector>

class RefThread
{
public:
    RefThread(){}

    virtual ~RefThread(){}

    void create_thread(short cpu_id = -1){
        exit_ = false;
#if _MSC_VER >= 1800 || defined __linux__
        if(cpu_id >=0 ){
            std::vector<short> ids(1,cpu_id);
            set_cpu(ids);
        }
        working_thread_ = std::thread(base_thread_entry,this);
#elif defined _WIN32
        working_thread_ = (HANDLE)_beginthread(base_thread_entry,0,this);
#endif
    }

    void join(){
#if _MSC_VER >= 1800 || defined __linux__
        if(working_thread_.joinable()){
            working_thread_.join();
        }else{
            std::cout<<"No joinable thread" <<std::endl;
        }
#elif defined _WIN32
        WaitForSingleObject(working_thread_,INFINITE);
#endif
    }

    inline void stop(){
        exit_ = true;
    }

    void detach(){
        working_thread_.detach();
    }

protected:
    virtual void thread_loop() = 0;
    inline bool is_exit(){
        return exit_;
    }

    void set_cpu(const std::vector<short>& cpuids){
        if(cpuids.empty()) return;
        /*cpu_set_t mask;
        CPU_ZERO(&mask);
        for(auto &id :cpuids){
            CPU_SET(id,&mask);
        }
        pthread_setaffinity_np(pthread_self(),sizeof(mask),&mask);*/
    }

private:
#if _MSC_VER >= 1800 || defined __linux__
    std::thread working_thread_;
#elif defined _WIN32
    HANDLE working_thread_;
#endif
    bool exit_;
    static void base_thread_entry(void *para){
        RefThread *thiz = static_cast<RefThread *>(para);
        thiz->thread_loop();
    }
};

class  RefThreadNull
{
public:
    RefThreadNull() {}
    virtual ~RefThreadNull() {}

    void crete_thread(short cpu_id = -1){
        exit_ = false;
    }

    void create_thread(const std::vector<short>& cpuids){
        exit_ = false;
    }

    void join() {}

    inline void stop(){
        exit_ = true;
    }

    void detach() {}

    virtual void thread_loop() = 0;

    inline bool is_exit() {
        return exit_;
    }

private:
    bool exit_;
};

#endif //!__REF_THREAD_H__

