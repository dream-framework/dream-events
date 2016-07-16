//
//  Events/Thread.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 13/09/11.
//  Copyright (c) 2011 Samuel Williams. All rights reserved.
//

#include "Thread.hpp"

#include <Dream/Core/Logger.hpp>

#include <string.h>

namespace Dream {
	namespace Events {
		using namespace Logging;
		
		std::string system_error_description(int error_number)
		{
			const std::size_t MAX_LENGTH = 1024;
			char buffer[MAX_LENGTH];

			if (strerror_r(error_number, buffer, MAX_LENGTH) == 0)
				return buffer;
			else
				return "Unknown failure";
		}

		Thread::Thread ()
		{
			_loop = new Loop;
			_loop->set_stop_when_idle(false);
		}

		Thread::~Thread ()
		{
			stop();
		}

		Ref<Loop> Thread::loop()
		{
			return _loop;
		}

		void Thread::start ()
		{
			if (!_thread)
				_thread = new std::thread(std::bind(&Thread::run, this));
		}

		void Thread::run ()
		{
			log_debug("-> Starting thread-based event-loop", this);

			// Lock the loop to ensure it isn't released by another thread:
			Ref<Loop> loop = _loop;

			loop->run_forever();

			log_debug("<- Exiting thread-based event-loop", this);
		}

		void Thread::stop ()
		{
			if (_thread) {
				_loop->stop();
				_thread->join();

				_thread = NULL;
			}
		}
	}
}
