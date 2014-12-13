//
//  Events/Source.h
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 9/12/08.
//  Copyright (c) 2008 Samuel Williams. All rights reserved.
//
//

#pragma once

#include "Events.hpp"

#include <functional>

namespace Dream
{
	namespace Events
	{
		class Loop;
		class ISource;

		class ISource : virtual public IObject {
		public:
			virtual void process_events (Loop *, Event) = 0;
		};

		class INotificationSource : virtual public ISource {
		public:
			virtual void process_events (Loop *, Event) = 0;
		};

		class NotificationSource : public Object, virtual public INotificationSource {
			typedef std::function<void (Loop *, NotificationSource *, Event)> CallbackT;

		protected:
			CallbackT _callback;

		public:
			/// This function is called from the main thread as soon as possible after notify() has been called.
			virtual void process_events (Loop *, Event);

			/// @todo loop should really be weak?
			NotificationSource (CallbackT callback);
			virtual ~NotificationSource ();

			static Ref<NotificationSource> stop_loop_notification ();
		};

		class ITimerSource : virtual public ISource {
		public:
			virtual bool repeats () const = 0;
			virtual TimeT next_timeout (const TimeT & last_timeout, const TimeT & current_time) const = 0;
		};

		class TimerSource : public Object, virtual public ITimerSource {
			typedef std::function<void (Loop *, TimerSource *, Event)> CallbackT;

		protected:
			bool _cancelled, _repeats, _strict;
			TimeT _duration;
			CallbackT _callback;

		public:
			/// A strict timer attempts to fire callbacks even if they are in the past.
			/// A non-strict timer might drop events.
			TimerSource (CallbackT callback, TimeT duration, bool repeats = false, bool strict = false);
			~TimerSource ();

			virtual void process_events (Loop *, Event);

			virtual bool repeats () const;
			virtual TimeT next_timeout (const TimeT & last_timeout, const TimeT & current_time) const;

			void cancel ();
		};

		class IFileDescriptorSource : virtual public ISource {
		public:
			virtual FileDescriptorT file_descriptor () const = 0;
			virtual Event mask() const = 0;

			/// Helper functions
			void set_will_block (bool value);
			bool will_block ();

			static void debug_file_descriptor_flags (int fd);
		};

		class FileDescriptorSource : public Object, virtual public IFileDescriptorSource {
			typedef std::function<void (Loop *, FileDescriptorSource *, Event)> CallbackT;

		protected:
			FileDescriptorT _file_descriptor;
			CallbackT _callback;
			Event _mask;

		public:
			FileDescriptorSource(CallbackT callback, FileDescriptorT fd, Event mask);
			virtual ~FileDescriptorSource ();

			virtual FileDescriptorT file_descriptor () const;

			virtual void process_events (Loop *, Event);

			static Ref<FileDescriptorSource> for_standard_in (CallbackT);
			static Ref<FileDescriptorSource> for_standard_out (CallbackT);
			static Ref<FileDescriptorSource> for_standard_error (CallbackT);
		};

		/* Internal class used for processing urgent notifications

		 */
		class NotificationPipeSource : public Object, virtual public IFileDescriptorSource {
		protected:
			FileDescriptorT _file_descriptors[2];

		public:
			NotificationPipeSource ();
			virtual ~NotificationPipeSource ();

			void notify_event_loop () const;

			virtual FileDescriptorT file_descriptor () const;
			virtual void process_events (Loop *, Event);
		};
	}
}
