#pragma once 
#include <nanodbc/nanodbc.h>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/event.hpp>
#include <boost/winapi/thread_pool.hpp>
#include <atomic>
#include <cassert>
template <typename Func>
std::pair<boost::winapi::WAITORTIMERCALLBACK_, std::shared_ptr<void>> GetRegisterWaitForSingleObjectFunc(Func&& func)
{
    auto cb = [](boost::winapi::PVOID_ ptr, boost::winapi::BOOLEAN_ timeOut) {
        (*static_cast<Func*>(ptr))(timeOut == TRUE);
    };
    return { cb, std::shared_ptr<void*>(new Func(std::forward<Func>(func)),[](void* data) {
            delete reinterpret_cast<std::decay_t<Func>*>(data); }
        ) };
}

namespace details
{
    class WinEvent
    {
    public:
        explicit WinEvent()
            : event_(boost::winapi::create_anonymous_event(0, 0, 0))
        {
            ++refCount_;
        }

        WinEvent(const WinEvent& other)
            : event_(other.event_)
        {
            refCount_.store(++other.refCount_);
        }

        WinEvent& operator=(const WinEvent& other)
        {
            if (this == &other)
                return *this;
            event_ = other.event_;
            refCount_.store(++other.refCount_);
            return *this;
        }

        ~WinEvent()
        {
            if (1 == refCount_--)
            {
                if (refCount_.fetch_xor(std::numeric_limits<ptrdiff_t>::max()) == 0)
                {
                    boost::winapi::CloseHandle(event_);
                }
            }
        }
        operator boost::winapi::HANDLE_() const { return event_; }

    private:
        boost::winapi::HANDLE_ event_;
        mutable std::atomic_ptrdiff_t refCount_{};
    };
} // namespace details
