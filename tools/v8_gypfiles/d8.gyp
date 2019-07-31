# Copyright 2018 the V8 project authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'V8_ROOT': '../../deps/v8',
    'v8_code': 1,
    # Enable support for Intel VTune. Supported on ia32/x64 only
    'v8_enable_vtunejit%': 0,
    'v8_enable_i18n_support%': 1,
  },
  'includes': ['toolchain.gypi', 'features.gypi'],
  'targets': [
    {
      'target_name': 'd8',
      'type': 'executable',
      'dependencies': [
        'v8.gyp:v8_base',
        'v8.gyp:v8_init',
        'v8.gyp:v8_libplatform',
        'v8.gyp:v8_snapshot',
        'v8.gyp:v8_libbase',
      ],
      # Generated source files need this explicitly:
      'include_dirs+': [
        '..',
        '<(DEPTH)',
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/generate-bytecode-output-root',
        '<(SHARED_INTERMEDIATE_DIR)/torque-output-root',
      ],
      'sources': [
        '<(V8_ROOT)/src/async-hooks-wrapper.cc',
        '<(V8_ROOT)/src/async-hooks-wrapper.h',
        '<(V8_ROOT)/src/d8-console.cc',
        '<(V8_ROOT)/src/d8-console.h',
        '<(V8_ROOT)/src/d8-js.cc',
        '<(V8_ROOT)/src/d8-platforms.cc',
        '<(V8_ROOT)/src/d8-platforms.h',
        '<(V8_ROOT)/src/d8.cc',
        '<(V8_ROOT)/src/d8.h',
      ],
      'conditions': [
        [ 'want_separate_host_toolset==1', {
          'toolsets': [ 'target', ],
          'dependencies': [
            'd8_js2c#host',
          ],
        }],
        ['(OS=="linux" or OS=="mac" or OS=="freebsd" or OS=="netbsd" \
           or OS=="openbsd" or OS=="solaris" or OS=="android" \
           or OS=="qnx" or OS=="aix" or OS=="zos")', {
             'sources': [ '<(V8_ROOT)/src/d8-posix.cc', ]
           }],
        [ 'OS=="win"', {
          'sources': [ '../src/d8-windows.cc', ]
        }],
        [ 'component!="shared_library"', {
          'conditions': [
            [ 'v8_postmortem_support==1', {
              'xcode_settings': {
                'OTHER_LDFLAGS': [
                   '-Wl,-force_load,<(PRODUCT_DIR)/libv8_base.a'
                ],
              },
            }],
          ],
        }],
        ['v8_enable_vtunejit==1', {
          'dependencies': [
            'v8vtune.gyp:v8_vtune',
          ],
        }],
        ['v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icui18n',
            '<(icu_gyp_path):icuuc',
          ],
        }],
        ['OS=="win" and v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icudata',
          ],
        }],
      ],
    },
  ],
}
