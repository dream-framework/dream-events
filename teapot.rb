
#
#  This file is part of the "Teapot" project, and is released under the MIT license.
#

teapot_version "1.0"

define_target "dream-events" do |target|
	target.build do |environment|
		source_root = target.package.path + 'source'
		
		copy headers: source_root.glob('Dream/**/*.hpp')
		
		build static_library: "DreamEvents", source_files: source_root.glob('Dream/**/*.cpp')
	end
	
	target.depends :platform
	target.depends "Language/C++11"
	
	target.depends "Build/Files"
	target.depends "Build/Clang"
	
	target.depends "Library/Dream"
	
	target.provides "Library/DreamEvents" do
		append linkflags "-lDreamEvents"
	end
end

define_target "dream-events-tests" do |target|
	target.build do |environment|
		test_root = target.package.path + 'test'
		
		run tests: "DreamEvents", source_files: test_root.glob('Dream/**/*.cpp')
	end
	
	target.depends "Library/UnitTest"
	target.depends "Library/DreamEvents"

	target.provides "Test/DreamEvents"
end

define_configuration "test" do |configuration|
	configuration[:source] = "https://github.com/kurocha"
	
	configuration.require "platforms"
	configuration.require "build-files"
	
	configuration.require "dream"
	configuration.require "euclid"
	configuration.require "unit-test"
	
	configuration.require "language-cpp-class"
end
