solution "Tools"

    configurations {"Release", "Debug"}

    flags { "Symbols" }

    location "build"
    targetdir "build"

    defines {"_CRT_SECURE_NO_WARNINGS"}

    defines("BUILDER_DEFAULT_RUNTIME=x64")

    defines("LIVEUPDATE_ENABLE")
    defines("PUTKI_ENABLE_LOG")
    defines("KOSMOS_ENABLE_LOG")

    if os.get() == "windows" then
        flags {"StaticRuntime"}
    end

    configuration {"linux", "gmake"}
        buildoptions {"-fPIC"}
        buildoptions ("-std=c++11")

    configuration "Debug"
        defines {"DEBUG"}
    configuration "Release"
        flags {"Optimize"}

    configuration {}

    ------------------------------------
    -- Putki must always come first   --
    ------------------------------------

    dofile "ext/putki/libs.lua"
    dofile "ext/kosmos/libs.lua"
    dofile "ext/ccg-ui/libs.lua"

    project "laxion-putki-lib"
        language "C++"
        targetname "laxion-putki-lib"
        kind "StaticLib"

        -- putki last here
        kosmos_use_builder_lib()
        ccgui_use_builder_lib()
        putki_use_builder_lib()

        putki_typedefs_builder("src/types", true)

    project "laxion-databuilder"

        kind "ConsoleApp"
        language "C++"
        targetname "laxion-databuilder"

        files { "src/builder/**.*" }
        links { "laxion-putki-lib" }
        includedirs { "src" }

        kosmos_use_builder_lib()
        putki_use_builder_lib()

        putki_typedefs_builder("src/types", false)

    project "laxion-data-dll"

        kind "SharedLib"
        language "C++"
        targetname "laxion-data-dll"

        files { "src/builder/dll-main.cpp" }
        links { "laxion-putki-lib"}
        includedirs { "src" }

        putki_typedefs_builder("src/types", false)
        kosmos_use_builder_lib()
        putki_use_builder_lib()
