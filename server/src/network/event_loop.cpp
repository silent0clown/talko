#include "event_loop.h"
#include "channel.h"
#include "w_sockets.h"

#ifdef WIN32
#include "select_poller.h"
#else
#include "epoll_poller.h"
#endif

#include <sstream>
#include <cstring>

using namespace w_network;

//内部侦听唤醒fd的侦听端口，因此外部可以再使用这个端口
//#define INNER_WAKEUP_LISTEN_PORT 10000

thread_local  EventLoop* loop_in_thread = 0;

const int poll_time_ms = 1;

EventLoop* get_curthread_eventloop()
{
    return loop_in_thread;
}

// 在线程函数中创建eventloop
EventLoop::EventLoop() :
    looping_(false),
    quit_(false),
    event_handling_(false),
    doing_other_tasks_(false),
    thread_id_(std::this_thread::get_id()),
    timer_queue_(new TimerQueue(this)),
    iteration_(0L),
    current_active_channel_(nullptr)
{
    create_wakeup_fd();

#ifdef WIN32
    wakeup_channel_.reset(new Channel(this, m_wakeupFdRecv));
    poller_.reset(new SelectPoller(this));

#else
    wakeup_channel_.reset(new Channel(this, wakeup_fd_));
    poller_.reset(new EpollPoller(this));
#endif

    if (loop_in_thread)
    {
        WHISP_LOG_FALTAL("Another EventLoop  exists in this thread ");
    }
    else
    {
        loop_in_thread = this;
    }
    wakeup_channel_->set_read_callback(std::bind(&EventLoop::handle_read, this));
    // we are always reading the wakeupfd
    wakeup_channel_->enable_reading();
}

EventLoop::~EventLoop()
{
    assert_in_loop_thread();
    WHISP_LOG_DEBUG("EventLoop 0x%x destructs.", this);

    //std::stringstream ss;
    //ss << "eventloop destructs threadid = " << threadId_;
    //std::cout << ss.str() << std::endl;

    wakeup_channel_->disable_all();
    wakeup_channel_->remove();

#ifdef WIN32
    w_sockets::close(wakeup_FdSend_);
    w_sockets::close(m_wakeupFdRecv_);
    w_sockets::close(m_wakeupFdListen_);
#else
    w_sockets::socks_close(wakeup_fd_);
#endif

    //_close(fdpipe_[0]);
    //_close(fdpipe_[1]);

    loop_in_thread = nullptr;
}


void EventLoop::loop()
{
    //assert(!looping_);
    assert_in_loop_thread();
    looping_ = true;
    quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
    WHISP_LOG_DEBUG("EventLoop 0x%x  start looping", this);

    while (!quit_)
    {
        timer_queue_->do_timer();

        active_channels_.clear();
        poll_return_time_ = poller_->poll(poll_time_ms, &active_channels_);
        //if (Logger::logLevel() <= Logger::TRACE)
        //{
        print_active_channels();
        //}
        ++iteration_;
        // TODO sort channel by priority
        event_handling_ = true;
        for (const auto& it : active_channels_)
        {
            current_active_channel_ = it;
            current_active_channel_->handle_event(poll_return_time_);
        }
        current_active_channel_ = nullptr;
        event_handling_ = false;
        do_other_tasks();

        if (frame_functor_)
        {
            frame_functor_();
        }
    }

    WHISP_LOG_DEBUG("EventLoop 0x%0x stop looping", this);
    looping_ = false;


    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string stid = oss.str();
    WHISP_LOG_INFO("Exiting loop, EventLoop object: 0x%x , threadID: %s", this, stid.c_str());
}

void EventLoop::quit()
{
    quit_ = true;
    // There is a chance that loop() just executes while(!quit_) and exists,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (!is_in_loop_thread())
    {
        wakeup();
    }
}

void EventLoop::run_in_loop(const Functor& cb)
{
    if (is_in_loop_thread())
    {
        cb();
    }
    else
    {
        queue_in_loop(cb);
    }
}

void EventLoop::queue_in_loop(const Functor& cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pending_functors_.push_back(cb);
    }

    if (!is_in_loop_thread() || doing_other_tasks_)
    {
        wakeup();
    }
}

void EventLoop::set_frame_functor(const Functor& cb)
{
    frame_functor_ = cb;
}

TimerId EventLoop::run_at(const Timestamp& time, const TimerCallback& cb)
{
    //只执行一次
    return timer_queue_->add_timer(cb, time, 0, 1);
}

TimerId EventLoop::run_after(int64_t delay, const TimerCallback& cb)
{
    Timestamp time(add_time(Timestamp::now(), delay));
    return run_at(time, cb);
}

TimerId EventLoop::run_every(int64_t interval, const TimerCallback& cb)
{
    Timestamp time(add_time(Timestamp::now(), interval));
    //-1表示一直重复下去
    return timer_queue_->add_timer(cb, time, interval, -1);
}

TimerId EventLoop::run_at(const Timestamp& time, TimerCallback&& cb)
{
    return timer_queue_->add_timer(std::move(cb), time, 0, 1);
}

TimerId EventLoop::run_after(int64_t delay, TimerCallback&& cb)
{
    Timestamp time(add_time(Timestamp::now(), delay));
    return run_at(time, std::move(cb));
}

TimerId EventLoop::run_every(int64_t interval, TimerCallback&& cb)
{
    Timestamp time(add_time(Timestamp::now(), interval));
    return timer_queue_->add_timer(std::move(cb), time, interval, -1);
}

void EventLoop::cancel(TimerId timerId, bool off)
{
    return timer_queue_->cancel(timerId, off);
}

void EventLoop::remove(TimerId timerId)
{
    return timer_queue_->remove_timer(timerId);
}

bool EventLoop::update_channel(Channel* channel)
{
    //assert(channel->ownerLoop() == this);
    if (channel->owner_loop() != this)
        return false;

    assert_in_loop_thread();

    return poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel* channel)
{
    //assert(channel->ownerLoop() == this);
    if (channel->owner_loop() != this)
        return;

    assert_in_loop_thread();
    if (event_handling_)
    {
        //assert(currentActiveChannel_ == channel || std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }

    WHISP_LOG_DEBUG("Remove channel, channel = 0x%x, fd = %d", channel, channel->fd());
    poller_->remove_channel(channel);
}

bool EventLoop::has_channel(Channel* channel)
{
    //assert(channel->ownerLoop() == this);
    assert_in_loop_thread();
    return poller_->has_channel(channel);
}

bool EventLoop::create_wakeup_fd()
{
#ifdef WIN32
    //if (_pipe(fdpipe_, 256, O_BINARY) == -1)
    //{
    //    //让程序挂掉
    //    LOGF("Unable to create pipe, EventLoop: 0x%x", this);
    //    return false;
    //}

    m_wakeupFdListen = sockets::createOrDie();
    m_wakeupFdSend = sockets::createOrDie();

    //Windows上需要创建一对socket  
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    //将port设为0，然后进行bind，再接着通过getsockname来获取port，这可以满足获取随机端口的情况。
    bindaddr.sin_port = 0;
    sockets::setReuseAddr(m_wakeupFdListen, true);
    sockets::bindOrDie(m_wakeupFdListen, bindaddr);
    sockets::listenOrDie(m_wakeupFdListen);

    struct sockaddr_in serveraddr;
    int serveraddrlen = sizeof(serveraddr);
    if (getsockname(m_wakeupFdListen, (sockaddr*)&serveraddr, &serveraddrlen) < 0)
    {
        //让程序挂掉
        LOGF("Unable to bind address info, EventLoop: 0x%x", this);
        return false;
    }

    int useport = ntohs(serveraddr.sin_port);
    WHISP_LOG_DEBUG("wakeup fd use port: %d", useport);

    //serveraddr.sin_family = AF_INET;
    //serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //serveraddr.sin_port = htons(INNER_WAKEUP_LISTEN_PORT);   
    if (::connect(m_wakeupFdSend, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
    {
        //让程序挂掉
        LOGF("Unable to connect to wakeup peer, EventLoop: 0x%x", this);
        return false;
    }

    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    m_wakeupFdRecv = ::accept(m_wakeupFdListen, (struct sockaddr*)&clientaddr, &clientaddrlen);
    if (m_wakeupFdRecv < 0)
    {
        //让程序挂掉
        LOGF("Unable to accept wakeup peer, EventLoop: 0x%x", this);
        return false;
    }

    sockets::setNonBlockAndCloseOnExec(m_wakeupFdSend);
    sockets::setNonBlockAndCloseOnExec(m_wakeupFdRecv);

#else
    //Linux上一个eventfd就够了，可以实现读写
    wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd_ < 0)
    {
        //让程序挂掉
        WHISP_LOG_FALTAL("Unable to create wakeup eventfd, EventLoop: 0x%x", this);
        return false;
    }

#endif

    return true;
}

void EventLoop::abort_not_in_loop_thread()
{
    std::stringstream ss;
    ss << "threadid_ = " << thread_id_ << " this_thread::get_id() = " << std::this_thread::get_id();
    WHISP_LOG_FALTAL("EventLoop::abortNotInLoopThread - EventLoop %s", ss.str().c_str());
}

bool EventLoop::wakeup()
{
    uint64_t one = 1;
#ifdef WIN32
    int32_t n = sockets::write(m_wakeupFdSend, &one, sizeof(one));
#else
    int32_t n = w_sockets::socks_write(wakeup_fd_, &one, sizeof(one));
#endif


    if (n != sizeof one)
    {
#ifdef WIN32
        DWORD error = WSAGetLastError();
        WHISP_LOG_SYSERROR("EventLoop::wakeup() writes %d  bytes instead of 8, fd: %d, error: %d", n, m_wakeupFdSend, (int32_t)error);
#else
        int error = errno;
        WHISP_LOG_SYSERROR("EventLoop::wakeup() writes %d  bytes instead of 8, fd: %d, error: %d, errorinfo: %s", n, wakeup_fd_, error, strerror(error));
#endif


        return false;
    }

    return true;
}

bool EventLoop::handle_read()
{
    uint64_t one = 1;
#ifdef WIN32
    int32_t n = sockets::read(m_wakeupFdRecv, &one, sizeof(one));
#else
    int32_t n = w_sockets::socks_read(wakeup_fd_, &one, sizeof(one));
#endif

    if (n != sizeof one)
    {
#ifdef WIN32
        DWORD error = WSAGetLastError();
        WHISP_LOG_SYSERROR("EventLoop::wakeup() read %d  bytes instead of 8, fd: %d, error: %d", n, m_wakeupFdRecv, (int32_t)error);
#else
        int error = errno;
        WHISP_LOG_SYSERROR("EventLoop::wakeup() read %d  bytes instead of 8, fd: %d, error: %d, errorinfo: %s", n, wakeup_fd_, error, strerror(error));
#endif
        return false;
    }

    return true;
}

void EventLoop::do_other_tasks()
{
    std::vector<Functor> functors;
    doing_other_tasks_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }

    doing_other_tasks_ = false;
}

void EventLoop::print_active_channels() const
{
    //TODO: 改成for-each 语法
    //std::vector<Channel*>
    for (const auto& iter : active_channels_)
    {
        WHISP_LOG_DEBUG("{%s}", iter->revents_2_string().c_str());
    }
}