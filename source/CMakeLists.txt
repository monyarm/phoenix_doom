set(INCLUDE_PATHS
    "."
)

# Platform agnostic sources
set(SOURCE_FILES
    "Audio/Audio.cpp"
    "Audio/Audio.h"
    "Audio/AudioData.h"
    "Audio/AudioDataMgr.cpp"
    "Audio/AudioDataMgr.h"
    "Audio/AudioLoader.cpp"
    "Audio/AudioLoader.h"
    "Audio/AudioOutputDevice.cpp"
    "Audio/AudioOutputDevice.h"
    "Audio/AudioSystem.cpp"
    "Audio/AudioSystem.h"
    "Audio/AudioVoice.h"
    "Audio/Sound.cpp"
    "Audio/Sound.h"
    "Audio/Sounds.h"
    "Base/Angle.h"
    "Base/BitInputStream.h"
    "Base/ByteInputStream.h"
    "Base/ControllerInput.cpp"
    "Base/ControllerInput.h"
    "Base/Endian.h"
    "Base/FatalErrors.cpp"
    "Base/FatalErrors.h"
    "Base/FileInputStream.cpp"
    "Base/FileInputStream.h"
    "Base/FileUtils.cpp"
    "Base/FileUtils.h"
    "Base/Finally.h"
    "Base/Fixed.h"
    "Base/FMath.h"
    "Base/FourCID.h"
    "Base/IniUtils.cpp"
    "Base/IniUtils.h"
    "Base/Input.cpp"
    "Base/Input.h"
    "Base/Macros.h"
    "Base/Mem.h"
    "Base/MouseButton.h"
    "Base/Random.cpp"
    "Base/Random.h"
    "Base/Resource.h"
    "Base/ResourceMgr.cpp"
    "Base/ResourceMgr.h"
    "Base/Tables.cpp"
    "Base/Tables.h"
    "Game/Cheats.cpp"
    "Game/Cheats.h"
    "Game/Config.cpp"
    "Game/Config.h"
    "Game/Controls.cpp"
    "Game/Controls.h"
    "Game/Data.cpp"
    "Game/Data.h"
    "Game/DoomDefines.h"
    "Game/DoomMain.cpp"
    "Game/DoomMain.h"
    "Game/DoomRez.h"
    "Game/Game.cpp"
    "Game/Game.h"
    "Game/GameDataFS.cpp"
    "Game/GameDataFS.h"
    "Game/Prefs.cpp"
    "Game/Prefs.h"
    "Game/Resources.cpp"
    "Game/Resources.h"
    "Game/Tick.cpp"
    "Game/Tick.h"
    "Game/TickCounter.cpp"
    "Game/TickCounter.h"
    "GFX/Blit.h"
    "GFX/CelImages.cpp"
    "GFX/CelImages.h"
    "GFX/ImageData.h"
    "GFX/Renderer.cpp"
    "GFX/Renderer.h"
    "GFX/Renderer_BspTraversal.cpp"
    "GFX/Renderer_FlatDraw.cpp"
    "GFX/Renderer_Internal.h"
    "GFX/Renderer_PostFx.cpp"
    "GFX/Renderer_SpriteDraw.cpp"
    "GFX/Renderer_WallDraw.cpp"
    "GFX/Renderer_WallPrep.cpp"
    "GFX/Renderer_WeaponDraw.cpp"
    "GFX/Sprites.cpp"
    "GFX/Sprites.h"
    "GFX/Textures.cpp"
    "GFX/Textures.h"
    "GFX/Video.cpp"
    "GFX/Video.h"
    "Map/Ceiling.cpp"
    "Map/Ceiling.h"
    "Map/Change.cpp"
    "Map/Change.h"
    "Map/Doors.cpp"
    "Map/Doors.h"
    "Map/Floor.cpp"
    "Map/Floor.h"
    "Map/Lights.cpp"
    "Map/Lights.h"
    "Map/Map.cpp"
    "Map/Map.h"
    "Map/MapData.cpp"
    "Map/MapData.h"
    "Map/MapUtil.cpp"
    "Map/MapUtil.h"
    "Map/Platforms.cpp"
    "Map/Platforms.h"
    "Map/Setup.cpp"
    "Map/Setup.h"
    "Map/Sight.cpp"
    "Map/Sight.h"
    "Map/Specials.cpp"
    "Map/Specials.h"
    "Map/Switch.cpp"
    "Map/Switch.h"
    "Things/Base.cpp"
    "Things/Base.h"
    "Things/Enemy.cpp"
    "Things/Enemy.h"
    "Things/Info.cpp"
    "Things/Info.h"
    "Things/Interactions.cpp"
    "Things/Interactions.h"
    "Things/MapObj.cpp"
    "Things/MapObj.h"
    "Things/Move.cpp"
    "Things/Move.h"
    "Things/Player.h"
    "Things/PlayerSprites.cpp"
    "Things/PlayerSprites.h"
    "Things/Shoot.cpp"
    "Things/Shoot.h"
    "Things/Slide.cpp"
    "Things/Slide.h"
    "Things/States.h"
    "Things/Teleport.cpp"
    "Things/Teleport.h"
    "Things/User.cpp"
    "Things/User.h"
    "ThreeDO/CDImageFileInputStream.cpp"
    "ThreeDO/CDImageFileInputStream.h"
    "ThreeDO/CelUtils.cpp"
    "ThreeDO/CelUtils.h"
    "ThreeDO/ChunkedStreamFileUtils.cpp"
    "ThreeDO/ChunkedStreamFileUtils.h"
    "ThreeDO/MovieDecoder.cpp"
    "ThreeDO/MovieDecoder.h"
    "ThreeDO/OperaFS.cpp"
    "ThreeDO/OperaFS.h"
    "UI/Automap.cpp"
    "UI/Automap.h"
    "UI/FinaleScreen.cpp"
    "UI/FinaleScreen.h"
    "UI/IntermissionScreen.cpp"
    "UI/IntermissionScreen.h"
    "UI/IntroLogos.cpp"
    "UI/IntroLogos.h"
    "UI/IntroMovies.cpp"
    "UI/IntroMovies.h"
    "UI/MainMenu.cpp"
    "UI/MainMenu.cpp"
    "UI/MainMenu.h"
    "UI/MainMenu.h"
    "UI/OptionsMenu.cpp"
    "UI/OptionsMenu.h"
    "UI/StatusBarUI.cpp"
    "UI/StatusBarUI.h"
    "UI/TitleScreens.cpp"
    "UI/TitleScreens.h"
    "UI/UIUtils.cpp"
    "UI/UIUtils.h"
    "UI/WipeFx.cpp"
    "UI/WipeFx.h"
    "Main.cpp"
)

set(OTHER_FILES
)

# Platform specific command line arguments
if (PLATFORM_MAC)
    #list(APPEND SOURCE_FILES "Main_Mac.mm")
    if (COMPILER_GCC) 
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-x>)
    elseif(COMPILER_CLANG)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:--language>)
    endif()
    if (COMPILER_GCC OR COMPILER_CLANG)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:objective-c++>)
    endif()
endif()

# Platform and compiler specific defines
if (COMPILER_MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)  # MSVC: Don't complain about using regular 'std::fopen()' etc.
endif()

# Build the executable and setup IDE folders
add_executable(${GAME_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})
setup_source_groups("${SOURCE_FILES}" "${HEADER_FILES}" "${OTHER_FILES}")

# Specify include dirs and link libraries
include_directories(${INCLUDE_PATHS})
target_link_libraries(${GAME_NAME} ${LIBSDL_NAME})

# Other platform or compiler specific settings, flags or switches
if (PLATFORM_WINDOWS)
    set_property(TARGET ${GAME_NAME} PROPERTY WIN32_EXECUTABLE true)
endif()

if (PLATFORM_MAC)
    SET_TARGET_PROPERTIES(${GAME_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if (PLATFORM_LINUX)
    target_compile_options(${GAME_NAME} PRIVATE -pthread)
endif()

if (COMPILER_MSVC)    
    # Enable all warnings
    target_compile_options(${GAME_NAME} PRIVATE /W4)
    # Statically link against the CRT
    set_property(TARGET ${GAME_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif (COMPILER_GCC OR COMPILER_CLANG)
    target_compile_options(${GAME_NAME} PRIVATE -pedantic -Wall)    # Enable all warnings and non standard C++ warnings
endif()
