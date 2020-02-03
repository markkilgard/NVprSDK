{
  'targets': [
    {
      'target_name': 'FileReaderApp',
      'type': 'executable',
      'mac_bundle' : 1,
      
      'include_dirs' : [
        '../include/pipe',
        '../experimental/FileReaderApp',
        '../experimental/SimpleCocoaApp',
      ],
      'sources': [
        '../experimental/FileReaderApp/ReaderView.cpp',
        '../src/pipe/SkGPipeRead.cpp',
      ],
      'sources!': [
        '../src/utils/mac/SkOSWindow_Mac.cpp',
      ],
      'dependencies': [
        'core.gyp:core',
        'effects.gyp:effects',
        'opts.gyp:opts',
        'ports.gyp:ports',
        'utils.gyp:utils',
        'views.gyp:views',
        'xml.gyp:xml',
      ],
      'conditions' : [
        # Only supports Mac currently
        ['skia_os == "mac"', {
          'sources': [
            '../experimental/SimpleCocoaApp/SkNSWindow.mm',
            '../experimental/SimpleCocoaApp/SkNSView.mm',
            '../experimental/FileReaderApp/FileReaderApp-Info.plist',
            '../experimental/FileReaderApp/FileReaderAppDelegate.mm',
            '../experimental/FileReaderApp/FileReaderApp_Prefix.pch',
            '../experimental/FileReaderApp/FileReaderWindow.mm',
            '../experimental/FileReaderApp/main.m',
            '../include/utils/mac/SkCGUtils.h',
            '../src/utils/mac/SkCreateCGImageRef.cpp',
          ],
          'link_settings': {
            'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
            '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
            '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
            ],
            'libraries!': [
            # Currently skia mac apps rely on Carbon and AGL for UI. Future
            # apps should use Cocoa instead and dependencies on Carbon and AGL
            # should eventually be removed
            '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
            '$(SDKROOT)/System/Library/Frameworks/AGL.framework',
            ],
          },
          'xcode_settings' : {
            'INFOPLIST_FILE' : '../experimental/FileReaderApp/FileReaderApp-Info.plist',
          },
          'mac_bundle_resources' : [
            '../experimental/FileReaderApp/English.lproj/InfoPlist.strings',
            '../experimental/FileReaderApp/English.lproj/MainMenu.xib',
          ],
        }],
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
