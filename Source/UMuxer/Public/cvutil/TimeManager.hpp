#ifndef __TIME_MANAGER_HPP__
#define __TIME_MANAGER_HPP__

#include "CoreMinimal.h"
#include "UMuxer.h"

#include <string>
#ifndef _WIN32
#include <sys/time.h>

struct  time_rep
{
    time_t time;
    unsigned short millitm;
};
#else
#include <time.h>
#include <sys/timeb.h>
#   if PLATFORM_WINDOWS
#   include "Windows/AllowWindowsPlatformTypes.h"
#       include <windows.h>
#	include "Windows/HideWindowsPlatformTypes.h"
#   endif
typedef timeb time_rep;
#endif // !_WIN32

class UMUXER_API TimeManager{
public:
    static std::string get_local_time_second() {
        time_t tim;
        struct tm *p;
        time(&tim);
        p = localtime(&tim);

        char time_str[50] = {0};
        sprintf(time_str,"%d-%02d-%02d %02d:%02d:%02d",
            p->tm_year + 1900,
            p->tm_mon + 1,
            p->tm_mday,
            p->tm_hour,
            p->tm_min,
            p->tm_sec);
        return std::string(time_str);
    }
};

class UMUXER_API PreciseTimer
{
public:
    PreciseTimer(const std::string& proc_name = "precise timer" ,bool auto_print = true):
        proc_name_(proc_name),
        auto_print_(auto_print){
#ifdef  _WIN32
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_system_freq_ = (double) freq.QuadPart;
#endif
        restart();
    }

    ~PreciseTimer(){
        if(auto_print_){
            if (count_)
            {
                UE_LOG(LogUMuxer, Log, TEXT("%s time elapsed : %f s"), proc_name_.c_str(), average());
                //LOG(INFO) << proc_name_ << " time elapsed : " << average() << "s";
            }
            else
            {
                UE_LOG(LogUMuxer, Log, TEXT("%s time elapsed : %f s"), proc_name_.c_str(), average());
            }
                //LOG(INGO) << proc_name_ << " time elapsed : " << elapsed() << "s";
        }
    }


    void restart(){
        count_ = 0;
#ifdef _WIN32
        QueryPerformanceCounter(&m_wstart_);
#else   
        clock_gettime(CLOCK_MONOTONIC,&m_start_);
#endif
    }

    double elapsed(){
#ifdef _WIN32
        LARGE_INTEGER tmp;
        QueryPerformanceCounter(&tmp);
        return (tmp.QuadPart - m_wstart_.QuadPart) / m_system_freq_;
#else   
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC,&now);
        return (double)(now.tv_sec - m_start_.tv_sec)
            + (double)(now.tv_nsec - m_start_.tv_nsec ) / 1000000000.0;
#endif
    }

    void count(){
        ++count_;
    }

    double average(){
        return elapsed() / count_;
    }

    void print(){
        if (count_)
        {
            UE_LOG(LogUMuxer, Log, TEXT("%s time elapsed : %f s"), proc_name_.c_str(), average());
            //LOG(INFO) << proc_name_ << "time elapsed: " << average() << "s";
        }
        else
        {
            UE_LOG(LogUMuxer, Log, TEXT("%s time elapsed : %f s"), proc_name_.c_str(), average());
            //LOG(INFO) << proc_name_ << "time elapsed: " << elapsed() << "s";
        }
     }

private:
    std::string     proc_name_;
    bool            auto_print_;
    unsigned long   count_;
#ifndef _WIN32
    struct timespec m_start_;
#else 
    double          m_system_freq_;
    LARGE_INTEGER   m_wstart_;
#endif // !_WIN32
};

#endif // !__TIME_MANAGER_HPP__


