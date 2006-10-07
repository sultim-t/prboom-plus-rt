require 'rake/loaders/makefile'

@cc = 'cc'
@linker = 'cc'
@makedepend = '/usr/X11R6/bin/makedepend'
@frameworkPaths = %w(~/Library/Frameworks /Library/Frameworks /System/Library/Frameworks)

@commonflags = ''
@cflags = ''
@cxxflags = ''
@objcflags = ''

@includes = ''
@defines = ''
@ldflags = ''
@libs = ''
@systemFrameworks = []
@frameworks = []
@cleanfiles = []

begin
	unless NOT_UNIVERSAL
		@commonflags += ' -arch ppc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk '
		@ldflags += ' -arch ppc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk '
	end
rescue NameError
	NOT_UNIVERSAL = false
	retry
end

###########
# Cleanup #
###########

task(:clean) do |task|
	rm_rf(@cleanfiles)
	rm_rf(BUNDLEDIR)
end

################
# Installation #
################

def installRule(dir, file)
	target = File::join(dir, File::basename(file))
	file(target => [dir, file]) do |t|
		cp(file, target)
	end
end

def installTask(taskName, dir, files)
	if files.is_a?(String)
		files = [files]
	end

	array = []
	for f in files do
		array << File::join(dir, File::basename(f))
		installRule(dir, f)
	end
	task(taskName => array)
end

def installTaskRecursive(taskName, dest, src)
	target = File::join(dest, File::basename(src))
	file(target => [dest, src]) do |t|
		rm_rf(t.name)
		cp_r(t.prerequisites[1], t.prerequisites[0])
	end
	task(taskName => target)
end

def bundleDir(dir)
	file(dir) do |t|
		sh("/Developer/Tools/SetFile -a B #{dir}")
	end
end

def setCurrentVersion(taskName)
	task(taskName => "#{VERSIONSDIR}/#{FRAMEWORKVERSION}") do |t|
		for file in Dir::glob("#{t.prerequisites[0]}/*") do
			filename = File::basename(file)
			ln_s("Contents/Versions/#{FRAMEWORKVERSION}/#{filename}", "#{BUNDLEDIR}/#{filename}", :force => true)
		end
	end
end

begin
	if FRAMEWORK
		directory(BUNDLEDIR = "#{NAME}.framework")
		directory(CONTENTSDIR = "#{BUNDLEDIR}/Contents")
		directory(VERSIONSDIR = "#{CONTENTSDIR}/Versions")
		directory(VERSIONDIR = "#{VERSIONSDIR}/#{FRAMEWORKVERSION}")
		directory(RESOURCEDIR = "#{VERSIONDIR}/Resources")
		directory(HEADERDIR = "#{VERSIONDIR}/Headers")

		@ldflags += " -dynamiclib -install_name @executable_path/../Frameworks/#{VERSIONDIR}/#{NAME}"
	else
		directory(BUNDLEDIR = "#{NAME}.app")
		directory(CONTENTSDIR = "#{BUNDLEDIR}/Contents")
		directory(RESOURCEDIR = "#{CONTENTSDIR}/Resources")
		directory(BINDIR = "#{CONTENTSDIR}/MacOS")
		directory(FRAMEWORKDIR = "#{CONTENTSDIR}/Frameworks")
	end
rescue NameError => e
	if e.name == :FRAMEWORK
		FRAMEWORK = false
		retry
	else
		raise e
	end
end

bundleDir(BUNDLEDIR)
task(:bundle => BUNDLEDIR)

##############
# Frameworks #
##############

def installFrameworks(task)
	for name in [@frameworks, @systemFrameworks].flatten
		framework = nil
		for path in @frameworkPaths
			try = "#{File::expand_path(path)}/#{name}.framework"
			if File::directory?(try)
				framework = try
				break
			end
		end

		if not framework
			puts "Framework #{name} missing!"
			exit(1)
		end

		@libs += " -framework #{name} "
		@includes += " -I#{framework}/Headers "

		if @frameworks.include?(name)
			installTaskRecursive(task, FRAMEWORKDIR, framework)
			bundleDir("#{FRAMEWORKDIR}/#{name}.framework")
		end
	end

	for dir in @frameworkPaths
		@cflags += " -F#{File::expand_path(dir)} "
		@ldflags += " -F#{File::expand_path(dir)} "
	end
end

###############
# Compilation #
###############

def cTask(object, source)
	file(object => source) do |task|
		sh("#{@cc} #{@commonflags} #{@cflags} #{@includes} #{@defines} -o \"#{task.name}\" -c #{task.prerequisites[0]}")
	end
end

def objcTask(object, source)
	file(object => source) do |task|
		sh("#{@cc} #{@commonflags} #{@objcflags} #{@includes} #{@defines} -o \"#{task.name}\" -c #{task.prerequisites[0]}")
	end
end

def objcxxTask(object, source)
	file(object => source) do |task|
		sh("#{@cc} #{@commonflags} #{@cxxflags} #{@objcflags} #{@includes} #{@defines} -o \"#{task.name}\" -c #{task.prerequisites[0]}")
	end
end

def cxxTask(object, source)
	file(object => source) do |task|
		sh("#{@cc} #{@commonflags} #{@cxxflags} #{@includes} #{@defines} -o \"#{task.name}\" -c #{task.prerequisites[0]}")
	end
end

def buildBinary(task, path, file, sources)
	objects = []
	target = "#{path}/#{file}"
	for source in sources
		object = "#{File::dirname(source)}/#{File::basename(source, '.*')}.o"
		if ['.m'].include?(File::extname(source))
			objcTask(object, source)
		elsif ['.M', '.mm'].include?(File::extname(source))
			objcxxTask(object, source)
		elsif ['.cxx', '.cc', '.cpp', '.C'].include?(File::extname(source))
			cxxTask(object, source)
		else
			cTask(object, source)
		end
		objects.push(object)
		@cleanfiles.push(object)

		depfile = "#{File::dirname(source)}/.#{File::basename(source, '.*')}.dep.mf"
		file(depfile => source) do |task|
			prefix = File::join(File::dirname(task.name), '')
			sh("#{@makedepend} -p#{prefix} -f- -- #{@includes} #{@defines} -- #{task.prerequisites} > #{task.name} 2> /dev/null")
		end
		import depfile
		@cleanfiles.push(depfile)
	end

	file(target => [path, *objects]) do |task|
		sh("#{@linker} #{@ldflags} -o \"#{task.name}\" #{task.prerequisites[1..-1].join(' ')} #{@libs}")
	end

	task(task => target)
end
