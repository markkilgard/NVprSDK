{
  'targets': [
    {
      'target_name': 'xps',
      'type': 'static_library',
      'dependencies': [
        'core.gyp:core',
        'images.gyp:images',
        'utils.gyp:utils',
      ],
      'include_dirs': [
        '../include/device/xps',
        '../include/utils/win',
        '../src/core', # needed to get SkGlyphCache.h
        '../src/utils', # needed to get SkBitSet.h
      ],
      'sources': [
        '../include/device/xps/SkConstexprMath.h',
        '../include/device/xps/SkXPSDevice.h',

        '../src/device/xps/SkXPSDevice.cpp',
      ],
      'conditions': [
        [ 'skia_os == "win"', {
          'link_settings': {
            'libraries': [
              'T2Embed.lib',
              'FontSub.lib',
            ],
          },
        },{ #else if 'skia_os != "win"'
          'include_dirs!': [
            '../include/utils/win',
          ],
          'sources!': [
            '../include/device/xps/SkXPSDevice.h',

            '../src/device/xps/SkXPSDevice.cpp',
          ],
        }],
      ],
      # This section makes all targets that depend on this target
      # #define SK_SUPPORT_XPS and have access to the xps header files.
      'direct_dependent_settings': {
        'conditions': [
          [ 'skia_os == "win"', {
            'defines': [
              'SK_SUPPORT_XPS',
            ],
          }],
        ],
        'include_dirs': [
          '../include/device/xps',
          '../src/utils', # needed to get SkBitSet.h
        ],
      },
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
