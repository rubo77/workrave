// Timer.cc --- break timer
//
// Copyright (C) 2001, 2002 Rob Caelers <robc@krandor.org>
// All rights reserved.
//
// Time-stamp: <2002-09-16 15:24:17 caelersr>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"

#include <sstream>
#include <stdio.h>
#include <math.h>

#include "Timer.hh"

#include "TimePredFactory.hh"
#include "TimePred.hh"
#include "TimeSource.hh"

#include "util.h"


//! Constructs a new break timer.
/*!
 *  \param timeSource the Timer wil obtain the current time from this source of
 *                    time.
 */
Timer::Timer(TimeSource *timeSource) :
  activity_timer(true),
  timer_enabled(false),
  activity_state(ACTIVITY_UNKNOWN),
  timer_state(STATE_INVALID),
  previous_timer_state(STATE_INVALID),
  snooze_interval(60),
  limit_enabled(true),
  limit_interval(600),
  autoreset_enabled(true),
  autoreset_interval(120),
  autoreset_interval_predicate(NULL),
  restore_enabled(true),
  elapsed_time(0),
  last_limit_time(0),
  last_start_time(0),
  last_reset_time(0),
  last_stop_time(0),
  next_reset_time(0),
  last_pred_reset_time(0),
  next_pred_reset_time(0),
  next_limit_time(0),
  time_source(timeSource),
  activity_monitor(NULL)
{
}


//! Destructor
Timer::~Timer()
{
  if (autoreset_interval_predicate != NULL)
    {
      delete autoreset_interval_predicate;
    }
}


//! Enable activity event monitoring.
void
Timer::enable()
{
  lock.lock();

  timer_enabled = true;
  stop_timer();

  lock.unlock();
}


//! Disable activity event monitoring.
void
Timer::disable()
{
  lock.lock();

  timer_enabled = false;
  stop_timer();

  lock.unlock();
}


//! Set Limit time in seconds.
/*!
 * \param limitTime time at which this timer reaches its limit.
 *
 */
void
Timer::set_limit(long limitTime)
{
  lock.lock();

  limit_interval = limitTime;
  compute_next_limit_time();

  lock.unlock();
}


//! Enable/Disable limiting
/*!
 * \param b indicates whether the timer limit must be enabled (\c true) or
 *          disabled (\c false).
 */
void
Timer::set_limit_enabled(bool b)
{
  lock.lock();

  limit_enabled = b;
  compute_next_limit_time();

  lock.unlock();
}


void
Timer::set_snooze_interval(time_t t)
{
  snooze_interval = t;
}


//! Enable/Disable auto-reset
/*!
 * \param b indicates whether the timer auto-reset must be enabled (\c true)
 *          or disabled (\c false).
 */
void
Timer::set_auto_reset_enabled(bool b)
{
  lock.lock();

  autoreset_enabled = b;
  compute_next_reset_time();
  
  lock.unlock();
}


//! Set auto-reset time period.
/*!
 * \param resetTime after this amount of idle time the timer will reset itself.
 */
void
Timer::set_auto_reset(long resetTime)
{
  lock.lock();

  autoreset_interval = resetTime;
  compute_next_reset_time();

  lock.unlock();
}


//! Set auto-reset predicate.
/*!
 * \param predicate auto-reset predicate.
 */
void
Timer::set_auto_reset(string predicate)
{
  lock.lock();

  autoreset_interval_predicate = TimePredFactory::create_time_pred(predicate);
  compute_next_predicate_reset_time();

  lock.unlock();
}



//! Compute the time the limit will be reached.
void
Timer::compute_next_limit_time()
{
  lock.lock();
  TRACE_ENTER_MSG("Timer::compute_next_limit_time", timer_id);
  
  if (last_limit_time != 0)
    {
      // Timer already reached limit
      next_limit_time = last_limit_time + snooze_interval;
    }
  else if (timer_enabled && timer_state == STATE_RUNNING && limit_enabled && limit_interval != 0)
    {
      // We are enabled, running and a limit != 0 was set.
      // So update our current Limit.

      // new limit = last start time + limit - elapsed.
      
      next_limit_time = last_start_time + limit_interval - elapsed_time;
    }
  else
    {
      // Just in case....
      next_limit_time = 0;
    }

  TRACE_RETURN(next_limit_time);
  lock.unlock();
}


//! Compute the time the auto-reset must take place.
void
Timer::compute_next_reset_time()
{
  lock.lock();

  if (timer_enabled && timer_state == STATE_STOPPED && last_stop_time != 0 &&
      autoreset_enabled && autoreset_interval != 0)
    {
      // We are enabled, not running and a limit != 0 was set.

      // next reset time = last stop time + auto reset
      next_reset_time = last_stop_time + autoreset_interval;

      if (next_reset_time <= last_reset_time)
        {
          next_reset_time = 0;
        }
    }
  else
    {
      // Just in case....
      next_reset_time = 0;
    }
  
  lock.unlock();
}


void
Timer::compute_next_predicate_reset_time()
{
  TRACE_ENTER_MSG("Timer::compute_next_predicate_reset_time", timer_id);
  lock.lock();

  if (autoreset_interval_predicate)
    {
      if (last_pred_reset_time == 0)
        {
          last_pred_reset_time = time_source->get_time();
          TRACE_MSG("setting last_pred_reset_time " <<  last_pred_reset_time);
        }
        
      autoreset_interval_predicate->set_last(last_pred_reset_time);
      next_pred_reset_time = autoreset_interval_predicate->get_next();
    }
  
  lock.unlock();
  TRACE_EXIT();
}


//! Reset and stop the timer.
void
Timer::reset_timer()
{
  lock.lock();
  TRACE_ENTER_MSG("Timer::reset_timer", timer_id);

  // Full reset.
  elapsed_time = 0;
  last_limit_time = 0;
  last_reset_time = time_source->get_time();
  
  if (timer_state == STATE_RUNNING)
    {
      // Pretend the timer just started.
      last_start_time = time_source->get_time();
      last_stop_time = 0;

      compute_next_limit_time();
      next_reset_time = 0;
    }
  else
    {
      // Timer stopped.
      last_start_time = 0;
      next_reset_time = 0;
      next_limit_time = 0;
      TRACE_MSG("reset next limit time");
    }
      
  next_pred_reset_time = 0;
  compute_next_predicate_reset_time();

  TRACE_EXIT();
  lock.unlock();
}


//! Start the timer.
void
Timer::start_timer()
{
  lock.lock();

  // Set last start and stop times.
  last_start_time = time_source->get_time();
  last_stop_time = 0;
  next_reset_time = 0;
  
  // update state.
  timer_state = STATE_RUNNING;

  // When to generate a limit-reached-event.
  compute_next_limit_time();

  
  lock.unlock();
}


//! Stop the timer.
void
Timer::stop_timer()
{
  lock.lock();

  // Update last stop time.
  last_stop_time = time_source->get_time();

  // Update elapsed time.
  if (last_start_time != 0)
    {
      // But only if we are running...

      elapsed_time += (last_stop_time - last_start_time);
    }
  
  // Reset last start time.
  last_start_time = 0;
  
  // Update state.
  timer_state = STATE_STOPPED;

  // When to reset the timer.
  compute_next_reset_time();

  // 
  compute_next_limit_time();
  
  lock.unlock();
}


//! Snoozes the Timer.
/*!
 *  When the limit of this timer was reached, the Limit Reached event will be
 *  re-sent.
 */
void
Timer::snooze_timer()
{
  TRACE_ENTER_MSG("Timer::snooze", timer_id);
  lock.lock();

  if (get_elapsed_time() >= limit_interval)
    {
      // recompute.
      last_limit_time = time_source->get_time();
      compute_next_limit_time();
    }

  lock.unlock();

  TRACE_EXIT();
}


//! Returns the elapsed idle time.
time_t
Timer::get_elapsed_idle_time() const
{
  lock.lock();
  time_t ret = 0;
  
  if (last_stop_time != 0)
    {
      // We are not running.
      
      ret = time_source->get_time() - last_stop_time;
    }

  lock.unlock();
  return ret;
}


//! Returns the elapsed time.
time_t
Timer::get_elapsed_time() const
{
  lock.lock();

  time_t ret = elapsed_time;

  if (last_start_time != 0)
    {
      // We are running:
      // total elasped = elapes + (last start time - current time)

      ret += (time_source->get_time() - last_start_time);
    }
  else
    {
      // We are not running
      // total elapsed = elapsed
    }

  lock.unlock();
  return ret;
}


//! Activity callback from activity monitor.
void
Timer::activity_notify()
{
  if (timer_enabled)
    {
      if (activity_timer)
        {
          start_timer();
        }
      else
        {
          stop_timer();
        }
    }
}


//! Idle callback from activity monitor.
void
Timer::idle_notify()
{
  if (timer_enabled)
    {
      if (activity_timer)
        {
          stop_timer();
        }
      else
        {
          start_timer();
        }
    }
}


//! Perform timer processing.
void
Timer::process(ActivityState activityState, TimerInfo &info)
{
  lock.lock();

  if (activity_monitor != NULL)
    {
      activityState = activity_monitor->get_current_state();
    }
        
  info.event = TIMER_EVENT_NONE;
  info.idle_time = get_elapsed_idle_time();
  info.elapsed_time = get_elapsed_time();
    
  if (activityState == ACTIVITY_ACTIVE && timer_state != STATE_RUNNING)
    {
      activity_notify();
    }
  else if (activityState != ACTIVITY_ACTIVE && timer_state == STATE_RUNNING)
    {
      idle_notify();
    }
  
  activity_state = activityState;
  
  time_t currentTime= time_source->get_time();

  if (autoreset_interval_predicate && next_pred_reset_time != 0 && currentTime >=  next_pred_reset_time)
    {
      // A next reset time was set and the current time >= reset time.

      reset_timer();

      last_pred_reset_time = time_source->get_time();
      next_pred_reset_time = 0;
      
      compute_next_predicate_reset_time();
      info.event = TIMER_EVENT_RESET;
    }
  else if (next_limit_time != 0 && currentTime >=  next_limit_time)
    {
      // A next limit time was set and the current time >= limit time.
      next_limit_time = 0;
      last_limit_time = time_source->get_time();

      compute_next_limit_time();
      
      info.event = TIMER_EVENT_LIMIT_REACHED;
      // Its very unlikely (but not impossible) that this will overrule
      // the EventStarted. Hey, shit happends.
    }
  else if (next_reset_time != 0 && currentTime >=  next_reset_time)
    {
      // A next reset time was set and the current time >= reset time.
      
      next_reset_time = 0;
      
      bool natural = (limit_interval >=  get_elapsed_time());
      
      reset_timer();
      
      info.event = natural ? TIMER_EVENT_NATURAL_RESET : TIMER_EVENT_RESET;
      // Idem, may overrule the EventStopped.
    }
  else
    {
      switch (timer_state)
        {
        case STATE_RUNNING:
          {
            if (previous_timer_state == STATE_STOPPED)
              {
                info.event = TIMER_EVENT_STARTED;
              }
          }
          break;
          
        case STATE_STOPPED:
          {
            if (previous_timer_state == STATE_RUNNING)
              {
                info.event = TIMER_EVENT_STOPPED;
              }
          }
          break;
          
        default:
          break;
        }
    }
  
  previous_timer_state = timer_state;
  lock.unlock();

  //return event;
}


std::string
Timer::serialize_state() const
{
  stringstream ss;

  if (restore_enabled)
    {
      ss << timer_id << " " 
         << time_source->get_time() << " "
         << get_elapsed_time() << " "
         << last_pred_reset_time;
    }
  
  return ss.str();
}


bool
Timer::deserialize_state(std::string state)
{
  istringstream ss(state);

  time_t saveTime;
  time_t elapsed;
  time_t lastReset;
  time_t now = time_source->get_time();;
  
  ss >> saveTime
     >> elapsed
     >> lastReset;
  
  lock.lock();
  
  last_pred_reset_time = lastReset;
  elapsed_time = 0;

  bool tooOld = ((autoreset_enabled && autoreset_interval != 0) && (now - saveTime >  autoreset_interval));

  if (! tooOld)
    {  
      next_reset_time = now + autoreset_interval;
      elapsed_time = elapsed;
    }

  // overdue, so snooze
  if (get_elapsed_time() >= limit_interval)
    {
      last_limit_time = time_source->get_time();
      compute_next_limit_time();
    }

  compute_next_predicate_reset_time();

  lock.unlock();
  return true;
}
