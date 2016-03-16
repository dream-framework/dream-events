//
//  Notifications.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/3/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Events/Loop.hpp>
#include <Dream/Events/Source.hpp>

namespace Dream
{
	namespace Events
	{
		UnitTest::Suite NotificationsTestSuite {
			"Dream::Events::Notifications",
			
			{"it should send a notification from a different thread",
				[](UnitTest::Examiner & examiner) {
					auto event_loop = ref(new Loop);
					
					int notification_count = 0;
					
					// This notification will be executed on the main thread:
					auto notification = ref(new NotificationSource([&](Loop *, NotificationSource *, Event){
						notification_count += 1;
						
						if (notification_count >= 10)
							event_loop->stop();
					}));
					
					// This thread will send notifications in the background.
					std::thread notification_thread([&](){
						for (int i = 0; i < 10; i += 1) {
							Core::sleep(0.01);
							// If you don't mark a notification as urgent, it won't wake up the event loop until some other event occurs:
							event_loop->post_notification(notification, true);
						}
					});

					// An event loop which exists for a specfic timeframe shouldn't exit because it doesn't have anything to do:
					event_loop->set_stop_when_idle(false);
					event_loop->run_until_timeout(1.0);

					notification_thread.join();

					examiner << "Notifications occurred";
					examiner.expect(notification_count) == 10;
				}
			},
		};
	}
}
