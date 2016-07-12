//
//  Monitor.hpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/12/2014.
//  Copyright, 2014, by Samuel Williams. All rights reserved.
//

#pragma once

#include "Events.hpp"

namespace Dream
{
	namespace Events
	{
		class Loop;
		class IFileDescriptorSource;

		/// An exception indicating that the file descriptor has been closed.
		class FileDescriptorClosed {
		};

		/// An interface for various operating system level event-handling mechanisms, e.g. kqueue, poll.
		class IMonitor : virtual public IObject {
		public:
			IMonitor();
			virtual ~IMonitor();
			
			/// Remove a source to be monitored
			virtual void add_source (Ptr<IFileDescriptorSource> source) = 0;

			/// Add a source to be monitored
			virtual void remove_source (Ptr<IFileDescriptorSource> source) = 0;

			/// Count of active file descriptors
			virtual std::size_t source_count () const = 0;

			/// Monitor sources for duration and handle any events that occur.
			/// If timeout >= 0, this call will return at least before this timeout
			/// If timeout == 0, this call does not block
			/// If timeout <= 0, this call blocks indefinitely
			/// Returns the number of events that were processed.
			virtual std::size_t wait_for_events (TimeT timeout, Loop * loop) = 0;
		};
	}
}
