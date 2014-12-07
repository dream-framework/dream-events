//
//  Events/Fader.h
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
		class IKnob {
		public:
			virtual ~IKnob ();
			virtual void update (TimeT time) = 0;
		};

		/**
		    A fader object updates a knob with a time value linearly interpolated from 0.0 to 1.0. At the end, it calls
		    the finish callback if it is specified.

		    It is used for providing time based animation/interpolation, and by Ref counting can be used in a set
		    and forget fashion.
		*/
		class Fader : public Object, virtual public ITimerSource {
		public:
			typedef std::function<void (Ptr<Fader> fader)> FinishCallbackT;

		protected:
			Shared<IKnob> _knob;

			int _count, _steps;
			TimeT _increment;

			bool _finished;

			FinishCallbackT _finish_callback;

		public:
			Fader(Shared<IKnob> knob, int steps, TimeT increment);
			virtual ~Fader ();

			void cancel ();
			bool finished () { return _finished; }

			void set_finish_callback (FinishCallbackT finish_callback);

			virtual bool repeats () const;
			virtual TimeT next_timeout (const TimeT & last_timeout, const TimeT & current_time) const;
			virtual void process_events (Loop *, Event);
		};
	}
}
