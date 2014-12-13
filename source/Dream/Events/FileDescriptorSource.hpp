//
//  FileDescriptorSource.hpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/12/2014.
//  Copyright, 2014, by Samuel Williams. All rights reserved.
//

#pragma once

namespace Dream
{
	namespace Events
	{
		// This is an immutable reference to a file descriptor, but the interesting events (e.g. read, write) may 
		class FileDescriptorSource
		{
		public:
			FileDescriptorSource(FileDescriptorT file_descriptor);
			virtual ~FileDescriptorSource();
			
			const FileDescriptorT & file_descriptor() const { return _file_descriptor; }
			
			// READ_READY or WRITE_READY or both.
			virtual Events events() const = 0;
			
		private:
			FileDescriptorT _file_descriptor;
		};
	}
}
