//
//  Events/Interpolator.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 31/12/10.
//  Copyright (c) 2010 Samuel Williams. All rights reserved.
//
//

#include "Interpolator.hpp"

namespace Dream
{
	namespace Events
	{
		Interpolator::Interpolator(int steps, TimeT increment) : _count(0), _steps(steps), _increment(increment), _finished(false)
		{
		}

		Interpolator::~Interpolator ()
		{
		}

		void Interpolator::update(const TimeT time)
		{
			
		}
		
		void Interpolator::finish()
		{
			
		}

		void Interpolator::cancel ()
		{
			_finished = true;
		}

		bool Interpolator::repeats () const
		{
			if (_finished) return false;

			return _count < _steps;
		}

		TimeT Interpolator::next_timeout (const TimeT & last_timeout, const TimeT & current_time) const
		{
			return last_timeout + _increment;
		}

		void Interpolator::process_events (Loop *, Event event)
		{
			if (event == TIMEOUT) {
				_count += 1;

				TimeT time = TimeT(_count) / TimeT(_steps);

				this->update(time);

				if (_count == _steps) {
					_finished = true;
					this->finish();
				}
			}
		}
	}
}
