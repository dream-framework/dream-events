//
//  Events/Interpolator.h
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 31/12/10.
//  Copyright (c) 2010 Samuel Williams. All rights reserved.
//
//

#pragma once

#include "Source.hpp"

#include <functional>

namespace Dream
{
	namespace Events
	{
		/** An Interpolator object updates a value over time. At the end, it calls the finish callback if it is specified.

			It is used for providing time based animation/interpolation, and by Ref counting can be used in a set and forget fashion.
		*/
		class Interpolator : public Object, virtual public ITimerSource {
		protected:
			int _count, _steps;
			TimeT _increment;

			bool _finished;

		public:
			Interpolator(int steps, TimeT increment);
			virtual ~Interpolator ();

			void cancel ();
			bool finished () { return _finished; }

			virtual void update(const TimeT time);
			virtual void finish();

			virtual bool repeats () const;
			virtual TimeT next_timeout (const TimeT & last_timeout, const TimeT & current_time) const;
			virtual void process_events (Loop *, Event);
		};
	}
}
