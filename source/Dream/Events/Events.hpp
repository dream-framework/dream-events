//
//  Events/Events.h
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 9/12/08.
//  Copyright (c) 2008 Samuel Williams. All rights reserved.
//
//

#pragma once

#include <Dream/Framework.hpp>
#include <Dream/Core/Buffer.hpp>
#include <Dream/Core/Timer.hpp>

namespace Dream
{
	/// Asynchronous event processing and delegation.
	/// Events and event loops typically help to serialize "asynchronious" events. Because this typically involves multiple threads and multiple run-loops, it
	/// is important to note that functions are not thread-safe (i.e. called from a different thread) unless explicity stated in the documentation. Functions
	/// which are thread-safe have specific mechanisms such as locks in order to provide a specific behaviour, so keep this in mind when using the APIs.
	namespace Events
	{
		using namespace Dream::Core;
		
		/// Event constants as used by Loop
		enum Event {
			READ_READY = 1,
			WRITE_READY = 2,
			TIMEOUT = 16,
			NOTIFICATION = 32
		};
	}
}
