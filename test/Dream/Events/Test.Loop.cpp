//
//  Loop.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/12/2014.
//  Copyright, 2014, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>
#include <Dream/Events/Loop.hpp>
#include <Dream/Core/Logger.hpp>

namespace Dream
{
	namespace Events
	{
		static void stop_callback (Loop * event_loop, TimerSource *, Event event)
		{
			event_loop->stop();
		}
		
		UnitTest::Suite LoopTestSuite {
			"Dream::Events::Loop",
			
			{"the event loop should stop when asked",
				[](UnitTest::Examiner & examiner) {
					bool timer_stopped = false;

					Ref<Loop> event_loop = new Loop;
					event_loop->set_stop_when_idle(false);

					event_loop->schedule_timer(new TimerSource([&](Loop * event_loop, TimerSource *, Event event){
							timer_stopped = true;
							event_loop->stop();
						}, 1.0));

					std::thread stop_thread([&](){
							// We are stopping the event loop from a "remote" thread:
							Core::sleep(0.1);
							event_loop->stop();
						});

					// Will be stopped after 1 seconds from the above thread if the stop after delay fails:
					event_loop->run_forever();

					stop_thread.join();

					examiner.expect(timer_stopped) == false;
				}
			},
			
			{"the notification was sent and received",
				[](UnitTest::Examiner & examiner) {
					Ref<Loop> event_loop = new Loop;
					std::size_t notified = 0;
					
					Ref<NotificationSource> note = new NotificationSource([&](Loop * rl, NotificationSource * note, Event event)
						{
							notified += 1;

							if (notified >= 10)
								rl->stop();
						});

					// Fail the test after 2 seconds if we are not notified.
					event_loop->schedule_timer(new TimerSource(stop_callback, 2));

					std::size_t count = 0;
					std::thread notification_thread([&]()
						{
							while (count < 10) {
								Core::sleep(0.01);
								
								event_loop->post_notification(note, true);
								
								count += 1;
							}
						});

					event_loop->run_forever();

					notification_thread.join();
					
					examiner.expect(count) == 10;
					examiner.expect(notified) == 10;
				}
			},

			{"the timer source fires",
				[](UnitTest::Examiner & examiner) {
					Ref<Loop> event_loop = new Loop;

					std::size_t ticks = 0;
					auto ticker_callback = [&](Loop * event_loop, TimerSource * ts, Event event)
						{
							if (ticks < 100)
								ticks += 1;
						};

					event_loop->schedule_timer(new TimerSource(stop_callback, 1.1));
					event_loop->schedule_timer(new TimerSource(ticker_callback, 0.01, true));

					event_loop->run_forever ();

					examiner << "ticks accumulated correctly";
					examiner.expect(ticks) == 100;

					event_loop = new Loop;

					ticks = 0;

					event_loop->schedule_timer(new TimerSource(ticker_callback, 0.1, true));

					event_loop->run_until_timeout(1.01);

					examiner << "ticks accumulated correctly within specified timeout";
					examiner.expect(ticks) == 10;
				}
			},
			
			{"can read from stdin",
				[](UnitTest::Examiner & examiner) {
					Ref<Loop> event_loop = new Loop;
					
					event_loop->schedule_timer(new TimerSource(stop_callback, 1.1));
					
					bool stdin_ready = false;
					
					auto stdin_callback = [&](Loop * rl, FileDescriptorSource *, Event event)
						{
							log_debug("Stdin events:", event);

							if (event & READ_READY) {
								stdin_ready = true;
							}
						};
					
					event_loop->monitor(FileDescriptorSource::for_standard_in(stdin_callback));
					
					event_loop->run_until_timeout(5.0);
					
					examiner.expect(stdin_ready) == true;
				}
			}
		};
	}
}
