#ifndef _RPC_CIRCUIT_BREAKER_H_
#define _RPC_CIRCUIT_BREAKER_H_

#include <string>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "cpputils.h"

using std::string;

namespace cap
{
namespace breaker
{

enum State
{
    STATE_OPEN      = 0,
    STATE_HALFOPEN  = 1,
    STATE_CLOSED    = 2,
};

class Counts
{
public:
    Counts()
    {
        Clear();
    }

public:
    void OnRequest()
    {
        m_requests++;
    }

    void OnSuccess()
    {
        m_total_successes++;
        m_consecutive_successes++;
        m_consecutive_failures = 0;
    }

    void OnFailure()
    {
        m_total_failures++;
        m_consecutive_failures++;
        m_consecutive_successes = 0;
    }

    void Clear()
    {
        m_requests = 0;
        m_total_successes = 0;
        m_total_failures = 0;
        m_consecutive_successes = 0;
        m_consecutive_failures = 0;
    }

public:
	uint32_t m_requests;
	uint32_t m_total_successes;
	uint32_t m_total_failures;
	uint32_t m_consecutive_successes;
	uint32_t m_consecutive_failures;
};

class Settings
{
public:
    Settings(uint32_t interval, uint32_t max_requests, uint32_t timeout)
    :m_interval(interval)
    ,m_max_requests(max_requests)
    ,m_timeout(timeout)
    {
        if (0 == max_requests)
        {
            m_max_requests = 1;
        }

        if (0 == timeout)
        {
            m_timeout = 3;
        }
    }

protected:
    virtual bool ReadyToTrip(Counts& counts)
    {
        return counts.m_consecutive_failures > 5;
    }

	virtual void OnStateChange(State from, State to)
    {

    }

protected:
    uint32_t m_interval;
	uint32_t m_max_requests;
	uint32_t m_timeout;
};

template<typename T, typename V>
class CircuitBreaker : public Settings
{
public:
    CircuitBreaker(uint32_t interval,
                   uint32_t max_requests,
                   uint32_t timeout)
    :Settings(interval, max_requests, timeout)
    ,m_state(STATE_CLOSED)
    ,m_generation(0)
    ,m_expiry(0)
    {
        ToNewGeneration(time(0));
    }

public:
    State state()
    {
        AutoLock<MutexLock> lock(m_lock);

        time_t now = time(0);
        CurrentState(now);
        return m_state;
    }

    const char* StateToString(State state)
    {
        switch (state)
        {
        case STATE_OPEN:
            return "OPEN";
        case STATE_HALFOPEN:
            return "HALFOPEN";
        case STATE_CLOSED:
            return "CLOSED";
        default:
            return "UNKNOWN";
        }

        return "UNKNOWN";
    }

    bool Execute(T& t, V& v)
    {
        bool error = BeforeRequest();
        if (false == error)
        {
            return false;
        }

        bool ret = t.Exec(v);
        AfterRequest(m_generation, ret);
        return ret;
    }

    bool BeforeRequest()
    {
        AutoLock<MutexLock> lock(m_lock);

        time_t now = time(0);
        State state = CurrentState(now);

        if (state == STATE_OPEN)
        {
            return false;
        }
        else if (state == STATE_HALFOPEN && m_counts.m_requests >= m_max_requests)
        {
            return false;
        }

        m_counts.OnRequest();
        return true;
    }

    void AfterRequest(uint64_t before, bool error)
    {
        AutoLock<MutexLock> lock(m_lock);

        time_t now = time(0);
        State state = CurrentState(now);
        if (m_generation != before)
        {
            return;
        }

        if (false != error)
        {
            OnSuccess(state, now);
        }
        else
        {
            OnFailure(state, now);
        }
    }

    void OnSuccess(State state, time_t now)
    {
        switch (state)
        {
        case STATE_CLOSED:
            m_counts.OnSuccess();
            break;
        case STATE_HALFOPEN:
            m_counts.OnSuccess();
            if (m_counts.m_consecutive_successes >= m_max_requests)
            {
                SetState(STATE_CLOSED, now);
            }
            break;
        default:
            break;
        }
    }

    void OnFailure(State state, time_t now)
    {
        switch (state)
        {
        case STATE_CLOSED:
            m_counts.OnFailure();
            if (ReadyToTrip(m_counts))
            {
                SetState(STATE_OPEN, now);
            }
            break;
        case STATE_HALFOPEN:
            SetState(STATE_OPEN, now);
            break;
        default:
            break;
        }
    }

    State CurrentState(time_t now)
    {
        switch (m_state)
        {
        case STATE_CLOSED:
            if (0 != m_expiry && m_expiry < now)
            {
                ToNewGeneration(now);
            }
            break;
        case STATE_OPEN:
            if (m_expiry < now)
            {
                SetState(STATE_HALFOPEN, now);
            }
            break;
        default:
            break;
        }

        return m_state;
    }

    void SetState(State state, time_t now)
    {
        if (m_state == state)
        {
            return;
        }

        State prev = m_state;
        m_state = state;

        ToNewGeneration(now);

        OnStateChange(prev, state);
    }

    void ToNewGeneration(time_t now)
    {
        m_generation++;
        m_counts.Clear();

        switch (m_state)
        {
        case STATE_CLOSED:
            if (m_interval == 0)
            {
                m_expiry = 0;
            }
            else
            {
                m_expiry = now + m_interval;
            }
            break;
        case STATE_OPEN:
            m_expiry = now + m_timeout;
            break;
        default: // StateHalfOpen
            m_expiry = 0;
            break;
        }
    }

private:
	State m_state;
	uint64_t m_generation;
	uint32_t m_expiry;
    Counts m_counts;
    MutexLock m_lock;
};

} // namespace breaker
} // namespace cap

#endif // _RPC_CIRCUIT_BREAKER_H_