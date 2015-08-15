solution "Laxion"

	location "build"
	targetdir "build"

        platforms { "x64" }

	configurations {"Debug", "Release", "Ship"}
	
	configuration {"Debug or Release"}
		defines("LIVEUPDATE_ENABLE")
		defines("PUTKI_ENABLE_LOG")
		defines("KOSMOS_ENABLE_LOG")
		
	configuration {"Ship"}
		defines("SHIP")
		defines("PUTKI_NO_RT_PATH_PREFIX")
		
	configuration {"Release or Ship"}
		flags {"Optimize"}
	
	configuration {}
		defines("KOSMOS_GL3")
	
	flags { "Symbols" }
	libdirs {"/usr/lib"}

	dofile("ext/putki/runtime.lua")
	dofile("ext/kosmos/runtime.lua")
	
project "laxion-runtime"
        kind "ConsoleApp"
        language "C++"
        targetname "laxion"

        kosmos_use_runtime_lib()
        putki_use_runtime_lib()
        
        putki_typedefs_runtime("src/types", true)

        files { "src/**.cpp" }
        files { "src/**.h" }

        excludes { "src/builder/**.*" }

        includedirs { "src" }

    configuration {"windows"}
            excludes {"src/**_osx*"}
    
    configuration {"macosx"}
            excludes {"src/**_win32*"}
            files {"src/**.mm"}
            
    links {"AppKit.framework", "QuartzCore.framework", "OpenGL.framework"}
