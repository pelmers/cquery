#!/usr/bin/env python
# encoding: utf-8

try:
    from urllib2 import urlopen # Python 2
except ImportError:
    from urllib.request import urlopen # Python 3

import os.path
import string
import subprocess
import sys

VERSION = '0.0.1'
APPNAME = 'cquery'

top = '.'
out = 'build'


# Example URLs
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu16.04.tar.xz
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu14.04.tar.xz
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-x86_64-apple-darwin.tar.xz

if sys.platform == 'darwin':
  CLANG_TARBALL_NAME = 'clang+llvm-$version-x86_64-apple-darwin'
elif sys.platform.startswith('freebsd'):
  CLANG_TARBALL_NAME = 'clang+llvm-$version-amd64-unknown-freebsd10'
# It is either 'linux2' or 'linux3' before Python 3.3
elif sys.platform.startswith('linux'):
  # These executable depend on libtinfo.so.5
  CLANG_TARBALL_NAME = 'clang+llvm-$version-linux-x86_64-ubuntu14.04'
else:
  # TODO: windows support (it's an exe!)
  sys.stderr.write('ERROR: Unknown platform {0}\n'.format(sys.platform))
  sys.exit(1)

from waflib.Tools.compiler_cxx import cxx_compiler
cxx_compiler['linux'] = ['clang++', 'g++']

def options(opt):
  opt.load('compiler_cxx')
  grp = opt.add_option_group('Configuration options related to use of clang from the system (not recommended)')
  grp.add_option('--use-system-clang', dest='use_system_clang', default=False, action='store_true',
                 help='enable use of clang from the system')
  grp.add_option('--bundled-clang', dest='bundled_clang', default='4.0.0', choices=('4.0.0', '5.0.0'),
                 help='bundled clang version')
  grp.add_option('--llvm-config', dest='llvm_config', default='llvm-config',
                 help='specify path to llvm-config for automatic configuration [default: %default]')
  grp.add_option('--clang-prefix', dest='clang_prefix', default='',
                 help='enable fallback configuration method by specifying a clang installation prefix (e.g. /opt/llvm)')
  grp.add_option('--cxxflags', dest='cxxflags', default=None,
                 help='space separated list of cxxflags for compilation. This replaces the default cxxflags')

def download_and_extract(destdir, url):
  dest = destdir + '.tar.xz'
  # Download and save the compressed tarball as |compressed_file_name|.
  if not os.path.isfile(dest):
    print('Downloading tarball')
    print('   destination: {0}'.format(dest))
    print('   source:      {0}'.format(url))
    # TODO: verify checksum
    response = urlopen(url)
    with open(dest, 'wb') as f:
      f.write(response.read())
  else:
    print('Found tarball at {0}'.format(dest))

  # Extract the tarball.
  if not os.path.isdir(destdir):
    print('Extracting')
    # TODO: make portable.
    subprocess.call(['tar', '-x', '-C', out, '-f', dest])
  else:
    print('Found extracted at {0}'.format(destdir))

def configure(conf):
  conf.load('compiler_cxx')
  conf.check(header_name='stdio.h', features='cxx cxxprogram', mandatory=True)
  conf.load('clang_compilation_database', tooldir='.')

  conf.env['use_system_clang'] = conf.options.use_system_clang

  if conf.options.cxxflags is None:
    conf.env['cxxflags'] = ['-g', '-O3', '-std=c++11', '-Wall', '-Wno-sign-compare', '-Werror']
  else:
    conf.env['cxxflags'] = conf.options.cxxflags.split(' ')


  if conf.options.llvm_config:
    conf.find_program(conf.options.llvm_config, msg='checking for llvm-config', var='LLVM_CONFIG', mandatory=False)
    # Ask llvm-config for cflags and ldflags
    conf.check_cfg(msg='Checking for clang flags',
                   path=conf.env.LLVM_CONFIG,
                   package='',
                   uselib_store='clang',
                   args='--cppflags --ldflags')
    # llvm-config does not provide the actual library we want so we check for it
    # using the provided info so far.
    conf.check_cxx(lib='clang', uselib_store='clang', use='clang')

  elif conf.options.use_system_clang: # Fallback method using a prefix path
      conf.start_msg('Checking for clang prefix')
      if not conf.options.clang_prefix:
        raise conf.errors.ConfigurationError('not found (--clang-prefix must be specified when llvm-config is not found)')

      prefix = conf.root.find_node(conf.options.clang_prefix)
      if not prefix:
        raise conf.errors.ConfigurationError('clang prefix not found: "%s"'%conf.options.clang_prefix)

      conf.end_msg(prefix)

      includes = [ n.abspath() for n in [ prefix.find_node('include') ] if n ]
      libpath  = [ n.abspath() for n in [ prefix.find_node(l) for l in ('lib', 'lib64')] if n ]
      conf.check_cxx(msg='Checking for library clang', lib='clang', uselib_store='clang', includes=includes, libpath=libpath)

  else:
    global CLANG_TARBALL_NAME

    # TODO Remove these after dropping clang 4 (after we figure out how to index Chrome)
    if conf.options.bundled_clang[0] == '4':
      if sys.platform == 'darwin':
        CLANG_TARBALL_NAME = 'clang+llvm-$version-x86_64-apple-darwin'
      elif sys.platform.startswith('linux'):
        # These executable depend on libtinfo.so.5
        CLANG_TARBALL_NAME = 'clang+llvm-$version-x86_64-linux-gnu-ubuntu-14.04'
      else:
        # TODO: windows support (it's an exe!)
        sys.stderr.write('ERROR: Unknown platform {0}\n'.format(sys.platform))
        sys.exit(1)

    CLANG_TARBALL_NAME = string.Template(CLANG_TARBALL_NAME).substitute(version=conf.options.bundled_clang)
    # Directory clang has been extracted to.
    CLANG_DIRECTORY = '{0}/{1}'.format(out, CLANG_TARBALL_NAME)
    # URL of the tarball to download.
    CLANG_TARBALL_URL = 'http://releases.llvm.org/{0}/{1}.tar.xz'.format(conf.options.bundled_clang, CLANG_TARBALL_NAME)

    print('Checking for clang')
    download_and_extract(CLANG_DIRECTORY, CLANG_TARBALL_URL)
    clang_node = conf.path.find_dir(CLANG_DIRECTORY)

    conf.check_cxx(uselib_store='clang',
                   includes=clang_node.find_dir('include').abspath(),
                   libpath=clang_node.find_dir('lib').abspath(),
                   lib='clang')

  conf.msg('Clang includes', conf.env.INCLUDES_clang)
  conf.msg('Clang library dir', conf.env.LIBPATH_clang)

  """
  # Download and save the compressed tarball as |compressed_file_name|.
  if not os.path.isfile(CLANG_TARBALL_LOCAL_PATH):
    print('Downloading clang tarball')
    print('   destination: {0}'.format(CLANG_TARBALL_LOCAL_PATH))
    print('   source:      {0}'.format(CLANG_TARBALL_URL))
    # TODO: verify checksum
    response = urlopen(CLANG_TARBALL_URL)
    with open(CLANG_TARBALL_LOCAL_PATH, 'wb') as f:
      f.write(response.read())
  else:
    print('Found clang tarball at {0}'.format(CLANG_TARBALL_LOCAL_PATH))

  # Extract the tarball.
  if not os.path.isdir(CLANG_DIRECTORY):
    print('Extracting clang')
    # TODO: make portable.
    call(['tar', 'xf', CLANG_TARBALL_LOCAL_PATH, '-C', out])
  else:
    print('Found extracted clang at {0}'.format(CLANG_DIRECTORY))
  """

def build(bld):
  # todo: configure vars

  cc_files = bld.path.ant_glob(['src/*.cc'])

  lib = []
  if sys.platform.startswith('linux'):
    lib.append('rt')
    lib.append('pthread')
    lib.append('dl')
  elif sys.platform == 'darwin':
    lib.append('pthread')

  bld.program(
      source=cc_files,
      use='clang',
      cxxflags=bld.env['cxxflags'],
      includes=[
        'third_party/',
        'third_party/doctest/',
        'third_party/loguru/',
        'third_party/rapidjson/include/',
        'third_party/sparsehash/src/',
        'third_party/sparsepp/'],
      defines=['LOGURU_WITH_STREAMS=1'],
      lib=lib,
      rpath=[] if bld.env['use_system_clang'] else bld.env['LIBPATH_clang'],
      target='cquery')

  #bld.shlib(source='a.cpp', target='mylib', vnum='9.8.7')
  #bld.shlib(source='a.cpp', target='mylib2', vnum='9.8.7', cnum='9.8')
  #bld.shlib(source='a.cpp', target='mylib3')
  #bld.program(source=cc_files, target='app', use='mylib')
  #bld.stlib(target='foo', source='b.cpp')

  # just a test to check if the .c is compiled as c++ when no c compiler is found
  #bld.program(features='cxx cxxprogram', source='main.c', target='app2')

  #if bld.cmd != 'clean':
  #  from waflib import Logs
  #  bld.logger = Logs.make_logger('test.log', 'build') # just to get a clean output
  #  bld.check(header_name='sadlib.h', features='cxx cxxprogram', mandatory=False)
  #  bld.logger = None
