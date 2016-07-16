//
//  Events/Loop.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 9/12/08.
//  Copyright (c) 2008 Samuel Williams. All rights reserved.
//
//

#include "Loop.hpp"

#include <Dream/Core/Logger.hpp>
#include <Dream/Core/System.hpp>

#include "Thread.hpp"

#include <iostream>
#include <limits>
#include <fcntl.h>
#include <unistd.h>

#if defined(TARGET_OS_LINUX)
// PollMonitor
	#include <poll.h>
#elif defined(TARGET_OS_MAC)
// KQueueMonitor
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
#else
	#error "Couldn't find file descriptor event subsystem."
#endif

namespace Dream
{
	namespace Events
	{
		static const bool DEBUG = false;

// MARK: -
// MARK: Helper Functions

		/// Returns the READ/WRITE events that may occur for a file descriptor.
		/// @returns READ_EVENT if file is open READ ONLY.
		/// @returns WRITE_EVENT if file is open WRITE ONLY.
		/// @returns READ_EVENT|WRITE_EVENT if file is READ/WRITE.
		static int events_for_file_descriptor (int fd)
		{
			if (fd == STDIN_FILENO) return READ_READY;

			if (fd == STDOUT_FILENO || fd == STDERR_FILENO) return WRITE_READY;

			int file_mode = fcntl(fd, F_GETFL) & O_ACCMODE;
			int events;

			DREAM_ASSERT(file_mode != -1);

			switch (file_mode) {
			case O_RDONLY:
				events = READ_READY;
				break;
			case O_WRONLY:
				events = WRITE_READY;
				break;
			case O_RDWR:
				events = READ_READY|WRITE_READY;
				break;
			default:
				events = 0;
			}

			return events;
		}

// MARK: File Descriptor Monitor Implementations
// MARK: -
		typedef std::set<Ref<IFileDescriptorSource>> FileDescriptorHandlesT;

#if defined(TARGET_OS_MAC)
		class KQueueMonitor : public Object, virtual public IMonitor {
		protected:
			FileDescriptor _kqueue;
			std::set<FileDescriptor> _removed_file_descriptors;

			FileDescriptorHandlesT _file_descriptor_handles;

		public:
			KQueueMonitor ();
			virtual ~KQueueMonitor ();

			virtual void add_source (Ptr<IFileDescriptorSource> source);
			virtual void remove_source (Ptr<IFileDescriptorSource> source);

			virtual std::size_t source_count () const;

			virtual std::size_t wait_for_events (TimeT timeout, Loop * loop);
		};

		KQueueMonitor::KQueueMonitor ()
		{
			_kqueue = kqueue();
		}

		KQueueMonitor::~KQueueMonitor ()
		{
			close(_kqueue);
		}

		void KQueueMonitor::add_source (Ptr<IFileDescriptorSource> source)
		{
			SystemError::reset();

			FileDescriptor fd = source->file_descriptor();
			_file_descriptor_handles.insert(source);

			int mode = events_for_file_descriptor(fd);

			struct kevent change[2];
			int c = 0;

			if (mode & READ_READY)
				EV_SET(&change[c++], fd, EVFILT_READ, EV_ADD, 0, 0, (void*)source.get());

			if (mode & WRITE_READY)
				EV_SET(&change[c++], fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)source.get());

			int result = kevent(_kqueue, change, c, NULL, 0, NULL);

			if (result == -1) {
				SystemError::check("kevent()");
			}
		}

		std::size_t KQueueMonitor::source_count () const
		{
			return _file_descriptor_handles.size();
		}

		void KQueueMonitor::remove_source (Ptr<IFileDescriptorSource> source)
		{
			SystemError::reset();

			FileDescriptor fd = source->file_descriptor();

			struct kevent change[2];
			int c = 0;

			int mode = events_for_file_descriptor(fd);

			if (mode & READ_READY)
				EV_SET(&change[c++], fd, EVFILT_READ, EV_DELETE, 0, 0, 0);

			if (mode & WRITE_READY)
				EV_SET(&change[c++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);

			int result = kevent(_kqueue, change, c, NULL, 0, NULL);

			if (result == -1) {
				SystemError::check("kevent()");
			}

			_removed_file_descriptors.insert(fd);
			_file_descriptor_handles.erase(source);
		}

		std::size_t KQueueMonitor::wait_for_events (TimeT timeout, Loop * loop)
		{
			SystemError::reset();

			const unsigned KQUEUE_SIZE = 32;
			int count;

			struct kevent events[KQUEUE_SIZE];
			timespec kevent_timeout;

			_removed_file_descriptors.clear();

			if (timeout > 0.0) {
				kevent_timeout.tv_sec = timeout;
				kevent_timeout.tv_nsec = (timeout - kevent_timeout.tv_sec) * 1000000000;
			} else {
				kevent_timeout.tv_sec = 0;
				kevent_timeout.tv_nsec = 0;
			}

			if (timeout < 0)
				count = kevent(_kqueue, NULL, 0, events, KQUEUE_SIZE, NULL);
			else
				count = kevent(_kqueue, NULL, 0, events, KQUEUE_SIZE, &kevent_timeout);

			if (count == -1) {
				SystemError::check("kevent()");
			} else {
				for (unsigned i = 0; i < count; i += 1) {
					//std::cerr << this << " event[" << i << "] for fd: " << events[i].ident << " filter: " << events[i].filter << std::endl;

					// Discard events for descriptors which have already been removed
					if (_removed_file_descriptors.find(events[i].ident) != _removed_file_descriptors.end())
						continue;

					IFileDescriptorSource * s = (IFileDescriptorSource *)events[i].udata;

					if (events[i].flags & EV_ERROR) {
						log_error("Error processing fd:", s->file_descriptor());
					}

					try {
						if (events[i].filter == EVFILT_READ)
							s->process_events(loop, READ_READY);

						if (events[i].filter == EVFILT_WRITE)
							s->process_events(loop, WRITE_READY);
					} catch (FileDescriptorClosed & ex) {
						remove_source(s);
					} catch (std::runtime_error & ex) {
						log_error("Exception thrown by runloop: ", ex.what());
						log_error("Removing file descriptor: ", s->file_descriptor());

						remove_source(s);
					}
				}
			}

			return count;
		}

		typedef KQueueMonitor SystemMonitor;
#endif

// MARK: -

#if defined(TARGET_OS_LINUX)
		class PollMonitor : public Object, virtual public IMonitor {
		protected:
			// Used to provide O(1) delete time within process_events handler
			bool _delete_current_file_descriptor_handle;
			Ref<IFileDescriptorSource> _current_file_descriptor_source;

			FileDescriptorHandlesT _file_descriptor_handles;

		public:
			PollMonitor ();
			virtual ~PollMonitor ();

			virtual void add_source (Ptr<IFileDescriptorSource> source);
			virtual void remove_source (Ptr<IFileDescriptorSource> source);

			virtual std::size_t source_count () const;

			virtual std::size_t wait_for_events (TimeT timeout, Loop * loop);
		};

		PollMonitor::PollMonitor ()
		{
		}

		PollMonitor::~PollMonitor ()
		{
		}

		void PollMonitor::add_source (Ptr<IFileDescriptorSource> source)
		{
			_file_descriptor_handles.insert(source);
		}

		void PollMonitor::remove_source (Ptr<IFileDescriptorSource> source)
		{
			if (_current_file_descriptor_source == source) {
				_delete_current_file_descriptor_handle = true;
			} else {
				_file_descriptor_handles.erase(source);
			}
		}

		std::size_t PollMonitor::source_count () const
		{
			return _file_descriptor_handles.size();
		}

		std::size_t PollMonitor::wait_for_events (TimeT timeout, Loop * loop)
		{
			SystemError::reset();
			
			// Number of events which have been processed
			int count = 0;

			std::vector<FileDescriptorHandlesT::iterator> handles;
			std::vector<struct pollfd> pollfds;

			handles.reserve(_file_descriptor_handles.size());
			pollfds.reserve(_file_descriptor_handles.size());

			for (FileDescriptorHandlesT::iterator i = _file_descriptor_handles.begin(); i != _file_descriptor_handles.end(); i++) {
				struct pollfd pfd;

				pfd.fd = (*i)->file_descriptor();
				pfd.events = POLLIN|POLLOUT;

				handles.push_back(i);
				pollfds.push_back(pfd);

				//std::cerr << "Monitoring " << (*i)->class_name() << " fd: " << (*i)->file_descriptor() << std::endl;
			}

			int result = 0;

			if (timeout > 0.0) {
				// Convert timeout to milliseconds
				timeout = std::min(timeout, (TimeT)(std::numeric_limits<int>::max() / 1000));
				// Granularity of poll is not very good, so it might run through the loop multiple times
				// in order to reach a timeout. When timeout is less than 1ms, timeout * 1000 = 0, i.e.
				// non-blocking poll.
				result = poll(&pollfds[0], pollfds.size(), (timeout * 1000) + 1);
			} else if (timeout == 0) {
				result = poll(&pollfds[0], pollfds.size(), 0);
			} else {
				result = poll(&pollfds[0], pollfds.size(), -1);
			}

			if (result < 0) {
				SystemError::check("poll()");
			}

			_delete_current_file_descriptor_handle = false;

			if (result > 0) {
				for (unsigned i = 0; i < pollfds.size(); i += 1) {
					int e = 0;

					if (pollfds[i].revents & POLLIN)
						e |= READ_READY;

					if (pollfds[i].revents & POLLOUT)
						e |= WRITE_READY;

					if (pollfds[i].revents & POLLNVAL) {
						log_error("Invalid file descriptor:", pollfds[i].fd);
					}

					if (e == 0) continue;

					count += 1;
					_current_file_descriptor_source = *(handles[i]);

					try {
						_current_file_descriptor_source->process_events(loop, Event(e));
					} catch (std::runtime_error & ex) {
						//std::cerr << "Exception thrown by runloop " << this << ": " << ex.what() << std::endl;
						//std::cerr << "Removing file descriptor " << _current_file_descriptor_source->file_descriptor() << " ..." << std::endl;

						_delete_current_file_descriptor_handle = true;
					}

					if (_delete_current_file_descriptor_handle) {
						// O(1) delete time if deleting current handle
						_file_descriptor_handles.erase(handles[i]);
						_delete_current_file_descriptor_handle = false;
					}
				}
			}

			return count;
		}

		typedef PollMonitor SystemMonitor;
#endif

// MARK: -
// MARK: class Loop

		Loop::Loop () : _stop_when_idle(true), _rate_limit(20)
		{
			// Setup file descriptor monitor
			_monitor = new SystemMonitor;

			// Setup timers
			_stopwatch.start();

			// Create and open an urgent notification pipe
			_urgent_notification_pipe = new NotificationPipeSource;
			monitor(_urgent_notification_pipe);
		}

		Loop::~Loop ()
		{
			// Remove the internal urgent notification pipe
			//std::cerr << "Stop monitoring urgent notification pipe..." << std::endl;
			//stop_monitoring_file_descriptor(_urgent_notification_pipe);

			//foreach(Ref<IFileDescriptorSource> source, _file_descriptor_handles)
			//{
			//	std::cerr << "FD Still Scheduled: " << source->class_name() << source->file_descriptor() << std::endl;
			//}
		}

		void Loop::set_stop_when_idle (bool stop_when_idle)
		{
			_stop_when_idle = stop_when_idle;
		}

		const Stopwatch & Loop::stopwatch () const
		{
			return _stopwatch;
		}

// MARK: -

		/// Used to schedule a timer to the loop via a notification.
		class ScheduleTimerNotificationSource : public Object, virtual public INotificationSource {
		protected:
			Ref<ITimerSource> _timer_source;

		public:
			ScheduleTimerNotificationSource(Ref<ITimerSource> timer_source);
			virtual ~ScheduleTimerNotificationSource ();

			virtual void process_events (Loop * event_loop, Event event);
		};

		ScheduleTimerNotificationSource::ScheduleTimerNotificationSource(Ref<ITimerSource> timer_source) : _timer_source(timer_source)
		{
		}

		ScheduleTimerNotificationSource::~ScheduleTimerNotificationSource ()
		{
		}

		void ScheduleTimerNotificationSource::process_events (Loop * event_loop, Event event)
		{
			// Add the timer into the run-loop.
			if (event == NOTIFICATION)
				event_loop->schedule_timer(_timer_source);
		}

		void Loop::schedule_timer (Ref<ITimerSource> source)
		{
			if (std::this_thread::get_id() == _current_thread) {
				TimerHandle th;

				TimeT current_time = _stopwatch.time();
				th.timeout = source->next_timeout(current_time, current_time);

				th.source = source;

				_timer_handles.push(th);
			} else {
				if (DEBUG) log_debug("Posting notification to remote event loop");

				// Add the timer via a notification which is passed across the thread.
				Ref<ScheduleTimerNotificationSource> note = new ScheduleTimerNotificationSource(source);

				// If the event loop is currently running forever with a timeout of -1, the notification will never be processed unless it is set to urgent. It might be better to have a default timeout for the runloop and expose this behaviour to the client library rather than just posting all schedule timer notifcations as urgent.
				this->post_notification(note, true);
			}
		}

// MARK: -

		void Loop::post_notification (Ref<INotificationSource> note, bool urgent)
		{
			// Lock the event loop notification queue
			// Add note to the end of the queue
			// Interrupt event loop thread if urgent

			if (std::this_thread::get_id() == _current_thread) {
				note->process_events(this, NOTIFICATION);
			} else {
				{
					// Enqueue the notification to be processed
					std::lock_guard<std::mutex> lock(_notifications.lock);
					_notifications.sources.push(note);
				}

				if (urgent) {
					// Interrupt event loop thread so that it processes notifications more quickly
					_urgent_notification_pipe->notify_event_loop();
				}
			}
		}

		void Loop::monitor (Ptr<IFileDescriptorSource> source)
		{
			DREAM_ASSERT(source->file_descriptor() != -1);
			//std::cerr << this << " monitoring fd: " << fd << std::endl;
			//IFileDescriptorSource::debug_file_descriptor_flags(fd);

			_monitor->add_source(source);
		}

		void Loop::stop_monitoring_file_descriptor (Ptr<IFileDescriptorSource> source)
		{
			DREAM_ASSERT(source->file_descriptor() != -1);

			if (DEBUG) log_debug("Event loop removing file descriptor:", source->file_descriptor());

			//IFileDescriptorSource::debug_file_descriptor_flags(fd);

			_monitor->remove_source(source);
		}

		/// If there is a timeout, returns true and the timeout in `at_time`.
		/// If there isn't a timeout, returns false and -1 in `at_time`.
		bool Loop::next_timeout (TimeT & at_time)
		{
			if (_timer_handles.empty()) {
				at_time = -1;
				return false;
			} else {
				at_time = _timer_handles.top().timeout - _stopwatch.time();
				return true;
			}
		}

		void Loop::stop ()
		{
			if (std::this_thread::get_id() != _current_thread) {
				post_notification(NotificationSource::stop_loop_notification(), true);
			} else {
				_running = false;
			}
		}

		Loop::Notifications::Notifications ()
		{
		}

		void Loop::Notifications::swap ()
		{
			// This function assumes that the structure has been locked..
			std::swap(sources, processing);
		}

		void Loop::process_notifications ()
		{
			// Escape quickly - if this is not thread-safe, we still shouldn't have a problem unless
			// the data-structure itself gets corrupt, but this shouldn't be possible because empty() is const.
			// If we get a false positive, we still check below by locking the structure properly.
			if (_notifications.sources.empty())
				return;

			{
				std::lock_guard<std::mutex> lock(_notifications.lock);

				// Grab all pending notifications
				_notifications.swap();
			}

			if (DEBUG) log_debug("Processing", _notifications.processing.size(), "notifications");

			unsigned rate = _rate_limit;

			while (!_notifications.processing.empty() && (rate-- || _rate_limit == 0)) {
				Ref<INotificationSource> note = _notifications.processing.front();
				_notifications.processing.pop();

				note->process_events(this, NOTIFICATION);
			}

			if (rate == 0 && _rate_limit != 0) {
				log_warning("Rate limiting notifications!");
				log("Rescheduling", _notifications.processing.size(), "notifications.");
				
				while (!_notifications.processing.empty()) {
					Ref<INotificationSource> note = _notifications.processing.front();
					_notifications.processing.pop();

					post_notification(note, false);
				}
			}
		}

		TimeT Loop::process_timers()
		{
			TimeT timeout = -1;
			unsigned rate = _rate_limit;

			// next_timeout returns true if the timeout should be processed, and updates timeout with the time it was due
			while (next_timeout(timeout)) {
				if (DEBUG) log_debug("Timeout at:", timeout);

				if (timeout > 0.0) {
					// The timeout was in the future:
					break;
				}

				if (_rate_limit > 0) {
					// Are we rate-limiting timeouts?
					rate -= 1;

					if (rate == 0) {
						if (DEBUG) log_warning("Timers have been rate limited!");

						// The timeout was rate limited, the timer needs to be called again on the next iteration of the event loop:
						return 0.0;
					}
				}

				// Check if the timeout is late:
				if (timeout < -0.1 && DEBUG)
					log_warning("Timeout was late:", timeout);

				TimerHandle th = _timer_handles.top();
				_timer_handles.pop();

				th.source->process_events(this, TIMEOUT);

				if (th.source->repeats()) {
					// Calculate the next time to schedule.
					th.timeout = th.source->next_timeout(th.timeout, _stopwatch.time());
					_timer_handles.push(th);
				}
			}

			if (DEBUG) log_debug("process_timers finished with timeout:", timeout);

			return timeout;
		}

		void Loop::process_file_descriptors (TimeT timeout)
		{
			if (DEBUG) log_debug("process_file_descriptors timeout:", timeout);

			// Timeout is now the amount of time we have to process other events until another timeout will need to fire.
			if (_monitor->source_count())
				_monitor->wait_for_events(timeout, this);
			else if (timeout > 0.0)
				Core::sleep(timeout);
		}

		void Loop::run_one_iteration (bool use_timer_timeout, TimeT timeout)
		{
			if (DEBUG) log_debug("Loop::run_one_iteration use_timer_timeout:", use_timer_timeout, "timeout:", timeout);

			TimeT time_until_next_timer_event = process_timers();

			// Process notifications before waiting for IO... [optional - reduce notification latency]
			process_notifications();

			// We have 1 "hidden" source: _urgent_notification_pipe..
			if (_stop_when_idle && _monitor->source_count() == 1 && _timer_handles.size() == 0)
				stop();

			// A timer may have stopped the runloop. We should check here before we possibly block indefinitely.
			if (_running == false)
				return;

			// If the timeout specified was too big, we set it till the time the next event will occur, so that this function (will/should) be called again shortly and process the timeout as appropriate.
			if (use_timer_timeout || timeout > time_until_next_timer_event) {
				timeout = time_until_next_timer_event;

				if (DEBUG) log_debug("Loop::run_one_iteration timeout:", timeout);
			}

			process_file_descriptors(timeout);

			// Process any outstanding notifications after IO... [required]
			process_notifications();
		}

		void Loop::run_once(bool block)
		{
			_running = true;
			_current_thread = std::this_thread::get_id();

			run_one_iteration(false, block ? -1 : 0);

			_running = false;
		}

		void Loop::run_forever()
		{
			log_debug("-> Entering runloop:", this);

			_running = true;
			_current_thread = std::this_thread::get_id();

			while (_running) {
				run_one_iteration(true, -1);
			}

			log_debug("<- Exiting runloop:", this);
		}

		TimeT Loop::run_until_timeout (TimeT timeout)
		{
			DREAM_ASSERT(timeout > 0);

			using Core::EggTimer;

			_running = true;
			_current_thread = std::this_thread::get_id();

			EggTimer timer(timeout);

			timer.start();
			while (_running && (timeout = timer.remaining_time()) > 0) {
				run_one_iteration(false, timeout);
			}

			return timer.remaining_time();
		}
	}
}
