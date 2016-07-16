//
//  Thread.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/7/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>
#include <Dream/Events/Thread.hpp>
#include <Dream/Core/Logger.hpp>

namespace Dream
{
	namespace Events
	{
		using namespace Core::Logging;
		
		class TTLNote : public Object, virtual public INotificationSource {
		public:
			std::vector<Ref<Loop>> loops;
			std::size_t count;

			TTLNote ();
			virtual void process_events (Loop *, Event);
		};

		TTLNote::TTLNote () : count(0)
		{
		}

		void TTLNote::process_events(Loop * loop, Event event)
		{
			count += 1;

			if (loops.size() > 0) {
				Ref<Loop> next = loops.back();
				loops.pop_back();

				next->post_notification(this, true);
			}
		}
		
		UnitTest::Suite ThreadTestSuite {
			"Dream::Events::Thread",
			
			{"it can post notifications from remote thread",
				[](UnitTest::Examiner & examiner) {
					Ref<Thread> t1 = new Thread, t2 = new Thread, t3 = new Thread;
					
					Ref<TTLNote> note = new TTLNote;
					note->loops.push_back(t1->loop());
					note->loops.push_back(t2->loop());
					note->loops.push_back(t3->loop());

					t1->start();
					t2->start();
					t3->start();

					t1->loop()->post_notification(note, true);

					sleep(1);

					examiner << "Note count " << note->count << " indicates failure in notification system";
					examiner.expect(note->count) == 4;
				}
			},

			{"it can queue and dequeue items",
				[](UnitTest::Examiner & examiner) {
					Ref<Thread> t1 = new Thread, t2 = new Thread, t3 = new Thread;
					
					// Three threads will generate semi-random integers.
					Ref<Queue<int>> queue = new Queue<int>();

					Ref<TimerSource> timer = new TimerSource([&](...){
						queue->add(1);
					}, 0.001, true);

					t1->loop()->schedule_timer(timer);
					t2->loop()->schedule_timer(timer);
					t3->loop()->schedule_timer(timer);

					t1->start();
					t2->start();
					t3->start();

					queue->add(10);

					/// They will be bulk processed by this loop which will fetch several items at a time.
					int total = 0;
					while (total < 1000) {
						sleep(0.1);

						// Fetch all items currently available.
						std::vector<int> * items = queue->fetch();

						if (items->size() == 0)
							continue;

						log("Fetched", items->size(), "item(s).");

						for(auto & i : *items) {
							total += i;
						}
					}

					examiner << "Total was not incremented correctly.";
					examiner.expect(total) > 1000;
				}
			},
		};
	}
}
