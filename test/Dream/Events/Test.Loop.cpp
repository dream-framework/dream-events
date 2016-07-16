//
//  Loop.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/12/2014.
//  Copyright, 2014, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>
#include <Dream/Events/Loop.hpp>

namespace Dream
{
	namespace Events
	{
		UnitTest::Suite LoopTestSuite {
			"Dream::Events::Loop",
			
			{"the event loop should stop when called from a different thread",
				[](UnitTest::Examiner & examiner) {
					bool timer_stopped = false;

					Ref<Loop> event_loop = new Loop;
					event_loop->set_stop_when_idle(false);

					event_loop->schedule_timer(new TimerSource([&](Loop * event_loop, TimerSource *, Event event){
						// This shouldn't happen, the stop thread below should occur first.
						timer_stopped = true;
						event_loop->stop();
					}, 1.0));

					std::thread stop_thread([&](){
						// We are stopping the event loop from a "remote" thread:
						Core::sleep(0.4);
						event_loop->stop();
					});

					// Will be stopped after 1 seconds from the above thread if the stop after delay fails:
					event_loop->run_forever();

					stop_thread.join();

					examiner.expect(timer_stopped) == false;
				}
			},
		};
	}
}
