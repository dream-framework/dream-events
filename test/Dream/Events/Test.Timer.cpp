//
//  Timer.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 15/3/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Events/Loop.hpp>
#include <Dream/Events/Source.hpp>

namespace Dream
{
	namespace Events
	{
		UnitTest::Suite TimerTestSuite {
			"Dream::Events::Timer",
			
			{"a timer is scheduled and called correctly",
				[](UnitTest::Examiner & examiner) {
					auto event_loop = ref(new Loop);

					int ticks = 0;

					event_loop->schedule_timer(new TimerSource([&](Loop *, TimerSource *, Event){
						event_loop->stop();
					}, 1.0));
					
					event_loop->schedule_timer(new TimerSource([&](Loop *, TimerSource *, Event){
						ticks += 1;
					}, 0.01, true));

					//event_loop->monitor(FileDescriptorSource::for_standard_in(stdin_callback));

					event_loop->run_forever();
					
					examiner << "Ticker callback called correctly";
					examiner.expect(ticks) == 100;
				}
			},
			
			{"a timer is scheduled and runs for a limited time",
				[](UnitTest::Examiner & examiner) {
					auto event_loop = ref(new Loop);

					int ticks = 0;

					event_loop->schedule_timer(new TimerSource([&](Loop *, TimerSource *, Event){
						ticks += 1;
					}, 0.01, true));

					event_loop->run_until_timeout(0.11);

					examiner << "Ticker callback called correctly within specified timeout";
					examiner.expect(ticks) == 10;
				}
			}
		};
	}
}
