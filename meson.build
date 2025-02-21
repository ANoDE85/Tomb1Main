project('Tomb1Main', ['c'],
  default_options: [
    'c_std=c11',
    'warning_level=2',
  ],
)

warning_level = 3
version = meson.project_version()
version_pretty = 'v' + version

git = find_program('git', required: false)
if git.found()
  gitdesc = run_command(git, 'describe', '--abbrev=7', '--tags')
  if gitdesc.returncode() == 0
    version_pretty = gitdesc.stdout().strip()
  endif
endif

c_compiler = meson.get_compiler('c')

is_mingw = c_compiler.get_id() == 'gcc' and host_machine.system() == 'windows'
if is_mingw
  add_project_link_arguments([], language: 'c')
endif

build_opts = [
  '-Wno-unused',
  '-D_GNU_SOURCE',
  '-DVERSION="T1M ' + version_pretty + '"'
]
c_opts = []
add_project_arguments(build_opts + c_opts, language: 'c')

dep_opengl32 = c_compiler.find_library('opengl32')
dep_dinput8 = c_compiler.find_library('dinput8')
dep_dxguid = c_compiler.find_library('dxguid')

dep_avcodec = dependency('libavcodec', static: true)
dep_avformat = dependency('libavformat', static: true)
dep_avutil = dependency('libavutil', static: true)
dep_swscale = dependency('libswscale', static: true)
dep_swresample = dependency('libswresample', static: true)

dep_sdl2 = dependency('SDL2', static: true)

# autogenerated files
resources = []
python3 = find_program('python3', required: false)
if python3.found()
  custom_target(
    'fake_init',
    output: ['fake_init'],
    command: [python3, meson.source_root() + '/scripts/generate_init'],
    build_by_default: true
  )
  version_rc = custom_target(
    'fake_version',
    output: ['version.rc'],
    command: [python3, meson.source_root() + '/scripts/generate_version', '--version', version_pretty, '--output', '@OUTPUT0@'],
    build_always: true
  )
  icon_rc = custom_target(
    'fake_icon',
    output: ['icon.rc'],
    command: [python3, meson.source_root() + '/scripts/generate_version', '--version', version_pretty, '--output', '@OUTPUT0@'],
  )
  if host_machine.system() == 'windows'
    windows = import('windows')
    resources = [
      windows.compile_resources(version_rc),
      windows.compile_resources(icon_rc),
    ]
  endif
endif

sources = [
  'src/3dsystem/3d_gen.c',
  'src/3dsystem/matrix.c',
  'src/3dsystem/phd_math.c',
  'src/config.c',
  'src/filesystem.c',
  'src/game/ai/abortion.c',
  'src/game/ai/alligator.c',
  'src/game/ai/ape.c',
  'src/game/ai/baldy.c',
  'src/game/ai/bat.c',
  'src/game/ai/bear.c',
  'src/game/ai/centaur.c',
  'src/game/ai/cowboy.c',
  'src/game/ai/crocodile.c',
  'src/game/ai/dino.c',
  'src/game/ai/evil_lara.c',
  'src/game/ai/larson.c',
  'src/game/ai/lion.c',
  'src/game/ai/mummy.c',
  'src/game/ai/mutant.c',
  'src/game/ai/natla.c',
  'src/game/ai/pierre.c',
  'src/game/ai/pod.c',
  'src/game/ai/raptor.c',
  'src/game/ai/rat.c',
  'src/game/ai/skate_kid.c',
  'src/game/ai/statue.c',
  'src/game/ai/vole.c',
  'src/game/ai/wolf.c',
  'src/game/box.c',
  'src/game/camera.c',
  'src/game/cinema.c',
  'src/game/clock.c',
  'src/game/collide.c',
  'src/game/control.c',
  'src/game/control_pause.c',
  'src/game/control_util.c',
  'src/game/demo.c',
  'src/game/draw.c',
  'src/game/effects/blood.c',
  'src/game/effects/body_part.c',
  'src/game/effects/bubble.c',
  'src/game/effects/chain_block.c',
  'src/game/effects/dino_stomp.c',
  'src/game/effects/earthquake.c',
  'src/game/effects/explosion.c',
  'src/game/effects/finish_level.c',
  'src/game/effects/flicker.c',
  'src/game/effects/flipmap.c',
  'src/game/effects/flood.c',
  'src/game/effects/lara_effects.c',
  'src/game/effects/missile.c',
  'src/game/effects/powerup.c',
  'src/game/effects/raising_block.c',
  'src/game/effects/ricochet.c',
  'src/game/effects/sand.c',
  'src/game/effects/splash.c',
  'src/game/effects/stairs2slope.c',
  'src/game/effects/turn_180.c',
  'src/game/effects/twinkle.c',
  'src/game/effects/waterfall.c',
  'src/game/fmv.c',
  'src/game/game.c',
  'src/game/gamebuf.c',
  'src/game/gameflow.c',
  'src/game/hair.c',
  'src/game/input.c',
  'src/game/inventry.c',
  'src/game/invfunc.c',
  'src/game/invvars.c',
  'src/game/items.c',
  'src/game/lara.c',
  'src/game/lara1gun.c',
  'src/game/lara2gun.c',
  'src/game/larafire.c',
  'src/game/laramisc.c',
  'src/game/larasurf.c',
  'src/game/laraswim.c',
  'src/game/level.c',
  'src/game/lot.c',
  'src/game/music.c',
  'src/game/objects/boat.c',
  'src/game/objects/bridge.c',
  'src/game/objects/cabin.c',
  'src/game/objects/cog.c',
  'src/game/objects/door.c',
  'src/game/objects/gunshot.c',
  'src/game/objects/keyhole.c',
  'src/game/objects/misc.c',
  'src/game/objects/pickup.c',
  'src/game/objects/puzzle_hole.c',
  'src/game/objects/savegame_crystal.c',
  'src/game/objects/scion.c',
  'src/game/objects/switch.c',
  'src/game/objects/trapdoor.c',
  'src/game/option.c',
  'src/game/option_compass.c',
  'src/game/option_control.c',
  'src/game/option_graphics.c',
  'src/game/option_passport.c',
  'src/game/option_sound.c',
  'src/game/output.c',
  'src/game/overlay.c',
  'src/game/people.c',
  'src/game/picture.c',
  'src/game/random.c',
  'src/game/requester.c',
  'src/game/savegame.c',
  'src/game/screen.c',
  'src/game/settings.c',
  'src/game/setup.c',
  'src/game/shell.c',
  'src/game/sound.c',
  'src/game/sphere.c',
  'src/game/text.c',
  'src/game/traps/damocles_sword.c',
  'src/game/traps/dart.c',
  'src/game/traps/dart_emitter.c',
  'src/game/traps/falling_block.c',
  'src/game/traps/falling_ceiling.c',
  'src/game/traps/flame.c',
  'src/game/traps/lava.c',
  'src/game/traps/lightning_emitter.c',
  'src/game/traps/midas_touch.c',
  'src/game/traps/movable_block.c',
  'src/game/traps/pendulum.c',
  'src/game/traps/rolling_ball.c',
  'src/game/traps/rolling_block.c',
  'src/game/traps/spikes.c',
  'src/game/traps/teeth_trap.c',
  'src/game/traps/thors_hammer.c',
  'src/game/viewport.c',
  'src/gfx/2d/2d_renderer.c',
  'src/gfx/2d/2d_surface.c',
  'src/gfx/3d/3d_renderer.c',
  'src/gfx/3d/vertex_stream.c',
  'src/gfx/blitter.c',
  'src/gfx/context.c',
  'src/gfx/gl/buffer.c',
  'src/gfx/gl/gl_core_3_3.c',
  'src/gfx/gl/program.c',
  'src/gfx/gl/sampler.c',
  'src/gfx/gl/texture.c',
  'src/gfx/gl/utils.c',
  'src/gfx/gl/vertex_array.c',
  'src/gfx/gl/wgl_ext.c',
  'src/gfx/screenshot.c',
  'src/global/vars.c',
  'src/global/vars_platform.c',
  'src/init.c',
  'src/json.c',
  'src/log.c',
  'src/memory.c',
  'src/specific/s_audio.c',
  'src/specific/s_audio_sample.c',
  'src/specific/s_audio_stream.c',
  'src/specific/s_clock.c',
  'src/specific/s_filesystem.c',
  'src/specific/s_fmv.c',
  'src/specific/s_input.c',
  'src/specific/s_misc.c',
  'src/specific/s_output.c',
  'src/specific/s_picture.c',
  'src/specific/s_shell.c',
  resources,
]

dependencies = [
    dep_avcodec,
    dep_avformat,
    dep_avutil,
    dep_dinput8,
    dep_dxguid,
    dep_opengl32,
    dep_sdl2,
    dep_swresample,
    dep_swscale,
  ]

executable(
  'Tomb1Main',
  sources,
  name_prefix: '',
  include_directories: ['src/'],
  dependencies: dependencies,
  link_args: ['-static'],
  gui_app: true,
)
