//
//  Loop.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/12/2014.
//  Copyright, 2014, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>
#include <Dream/Events/Loop.hpp>

// MARK: -
// MARK: Unit Tests

#ifdef ENABLE_TESTING
		int ticks;
		void ticker_callback (Loop * event_loop, TimerSource * ts, Event event)
		{
			if (ticks < 100)
				ticks += 1;
		}

		void timeout_callback (Loop * rl, TimerSource *, Event event)
		{
			std::cout << "Timeout callback @ " << rl->stopwatch().time() << std::endl;
		}

		static void stop_callback (Loop * event_loop, TimerSource *, Event event)
		{
			event_loop->stop();
		}

		void stdin_callback (Loop * rl, FileDescriptorSource *, Event event)
		{
			std::cout << "Stdin events: " << event << std::endl;

			if (event & READ_READY) {
				std::string s;
				std::cin >> s;

				std::cout << "Data read: " << s << std::endl;
			}
		}

		UNIT_TEST(TimerSource)
		{
			testing("Timer Sources");

			Ref<Loop> event_loop = new Loop;

			ticks = 0;

			event_loop->schedule_timer(new TimerSource(stop_callback, 1.1));
			event_loop->schedule_timer(new TimerSource(ticker_callback, 0.01, true));

			event_loop->monitor(FileDescriptorSource::for_standard_in(stdin_callback));

			event_loop->run_forever ();

			check(ticks == 100) << "Ticker callback called correctly";

			event_loop = new Loop;

			ticks = 0;

			event_loop->schedule_timer(new TimerSource(ticker_callback, 0.1, true));

			event_loop->run_until_timeout(1.01);

			check(ticks == 10) << "Ticker callback called correctly within specified timeout";
		}

		int notified;
		static void send_notification_after_delay (Ref<Loop> event_loop, Ref<INotificationSource> note)
		{
			int count = 0;
			while (count < 10) {
				Core::sleep(0.1);
				event_loop->post_notification(note);

				count += 1;
			}

			//event_loop->post_notification(note);
		}


		static void notification_received (Loop * rl, NotificationSource * note, Event event)
		{
			notified += 1;

			if (notified >= 10)
				rl->stop();
		}

		UNIT_TEST(Notification)
		{
			testing("Notification Sources");

			Ref<Loop> event_loop = new Loop;
			Ref<NotificationSource> note = new NotificationSource(notification_received);

			// Fail the test after 5 seconds if we are not notified.
			event_loop->schedule_timer(new TimerSource(stop_callback, 2));

			notified = 0;

			std::thread notification_thread(std::bind(send_notification_after_delay, event_loop, note));

			event_loop->run_forever();

			notification_thread.join();

			check(notified == 10) << "Notification occurred";
		}



		UNIT_TEST(EventLoopStop)
		{

		}

#endif

namespace Dream
{
	namespace Events
	{
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
