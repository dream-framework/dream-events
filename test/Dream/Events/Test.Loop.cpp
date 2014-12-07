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
			
			{"it should have some real tests",
				[](UnitTest::Examiner & examiner) {
					examiner.expect(false) == true;
				}
			},
		};
	}
}
