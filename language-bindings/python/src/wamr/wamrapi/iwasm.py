# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

r"""Wrapper for wasm_export.h

Generated with:
ctypesgen ../../../../core/iwasm/include/wasm_export.h -l ../libs/libiwasm.so -o iwasm.py

Do not modify this file.
"""

__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403

_int_types = (ctypes.c_int16, ctypes.c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have ctypes.c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (ctypes.c_int64,)
for t in _int_types:
    if ctypes.sizeof(t) == ctypes.sizeof(ctypes.c_size_t):
        c_ptrdiff_t = t
del t
del _int_types



class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1 :]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1 :]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, ctypes.Union):

    _fields_ = [("raw", ctypes.POINTER(ctypes.c_char)), ("data", ctypes.c_char_p)]

    def __init__(self, obj=b""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(ctypes.POINTER(ctypes.c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, ctypes.c_char_p):
            return obj

        # Convert from POINTER(ctypes.c_char)
        elif isinstance(obj, ctypes.POINTER(ctypes.c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(ctypes.cast(obj, ctypes.POINTER(ctypes.c_char)))

        # Convert from ctypes.c_char array
        elif isinstance(obj, ctypes.c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)


# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to ctypes.c_void_p.
def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (isinstance(value, bytes) or isinstance(value, str)) else value

# End preamble

_libs = {}
_libdirs = []

# Begin loader

"""
Load libraries - appropriately for all our supported platforms
"""
# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import ctypes
import ctypes.util
import glob
import os.path
import platform
import re
import sys


def _environ_path(name):
    """Split an environment variable into a path-like list elements"""
    if name in os.environ:
        return os.environ[name].split(":")
    return []


class LibraryLoader:
    """
    A base class For loading of libraries ;-)
    Subclasses load libraries for specific platforms.
    """

    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup:
        """Looking up calling conventions for a platform"""

        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            """Return the given name according to the selected calling convention"""
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            """Return True if this given calling convention finds the given 'name'"""
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            # noinspection PyBroadException
            try:
                return self.Lookup(path)
            except Exception:  # pylint: disable=broad-except
                pass

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # check if this code is even stored in a physical file
            try:
                this_file = __file__
            except NameError:
                this_file = None

            # then we search the directory where the generated python interface is stored
            if this_file is not None:
                for fmt in self.name_formats:
                    yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, _libname):  # pylint: disable=no-self-use
        """Return all the library paths available in this platform"""
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    """Library loader for MacOS"""

    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        """
        Looking up library files for this platform (Darwin aka MacOS)
        """

        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [fmt % libname for fmt in self.name_formats]

        for directory in self.getdirs(libname):
            for name in names:
                yield os.path.join(directory, name)

    @staticmethod
    def getdirs(libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"),
                "/usr/local/lib",
                "/usr/lib",
            ]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
            dirs.extend(_environ_path("LD_RUN_PATH"))

        if hasattr(sys, "frozen") and getattr(sys, "frozen") == "macosx_app":
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "..", "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    """Library loader for POSIX-like systems (including Linux)"""

    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    name_formats = ["lib%s.so", "%s.so", "%s"]

    class _Directories(dict):
        """Deal with directories"""

        def __init__(self):
            dict.__init__(self)
            self.order = 0

        def add(self, directory):
            """Add a directory to our current set of directories"""
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            order = self.setdefault(directory, self.order)
            if order == self.order:
                self.order += 1

        def extend(self, directories):
            """Add a list of directories to our set"""
            for a_dir in directories:
                self.add(a_dir)

        def ordered(self):
            """Sort the list of directories"""
            return (i[0] for i in sorted(self.items(), key=lambda d: d[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive function to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as fileobj:
                for dirname in fileobj:
                    dirname = dirname.strip()
                    if not dirname:
                        continue

                    match = self._include.match(dirname)
                    if not match:
                        dirs.add(dirname)
                    else:
                        for dir2 in glob.glob(match.group("pattern")):
                            self._get_ld_so_conf_dirs(dir2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HP-UX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu", "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compatible
                unix_lib_dirs_list += [
                    "/lib/x86_64-linux-gnu",
                    "/usr/lib/x86_64-linux-gnu",
                ]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        # ext_re = re.compile(r"\.s[ol]$")
        for our_dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % our_dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, set())
                    cache_i.add(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, set())
                        cache_i.add(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, set())
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    """Library loader for Microsoft Windows"""

    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        """Lookup class for Windows libraries..."""

        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for path in other_dirs:
        if not os.path.isabs(path):
            path = os.path.abspath(path)
        load_library.other_dirs.append(path)


del loaderclass

# End loader

add_library_search_dirs([])

# Begin libraries
_libs["../libs/libiwasm.so"] = load_library("../libs/libiwasm.so")

# 1 libraries
# End libraries

# No modules

__uint8_t = c_ubyte# /usr/include/x86_64-linux-gnu/bits/types.h: 38

__uint32_t = c_uint# /usr/include/x86_64-linux-gnu/bits/types.h: 42

uint8_t = __uint8_t# /usr/include/x86_64-linux-gnu/bits/stdint-uintn.h: 24

uint32_t = __uint32_t# /usr/include/x86_64-linux-gnu/bits/stdint-uintn.h: 26

uintptr_t = c_ulong# /usr/include/stdint.h: 90

# wasm-micro-runtime/core/iwasm/include/lib_export.h: 22
class struct_NativeSymbol(Structure):
    pass

struct_NativeSymbol.__slots__ = [
    'symbol',
    'func_ptr',
    'signature',
    'attachment',
]
struct_NativeSymbol._fields_ = [
    ('symbol', String),
    ('func_ptr', POINTER(None)),
    ('signature', String),
    ('attachment', POINTER(None)),
]

NativeSymbol = struct_NativeSymbol# wasm-micro-runtime/core/iwasm/include/lib_export.h: 22

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 62
class struct_WASMModuleCommon(Structure):
    pass

wasm_module_t = POINTER(struct_WASMModuleCommon)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 63

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 67
class struct_WASMModuleInstanceCommon(Structure):
    pass

wasm_module_inst_t = POINTER(struct_WASMModuleInstanceCommon)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 68

WASMFunctionInstanceCommon = None# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 71

wasm_function_inst_t = POINTER(WASMFunctionInstanceCommon)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 72

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 75
class struct_wasm_section_t(Structure):
    pass

struct_wasm_section_t.__slots__ = [
    'next',
    'section_type',
    'section_body',
    'section_body_size',
]
struct_wasm_section_t._fields_ = [
    ('next', POINTER(struct_wasm_section_t)),
    ('section_type', c_int),
    ('section_body', POINTER(uint8_t)),
    ('section_body_size', uint32_t),
]

wasm_section_t = struct_wasm_section_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 83

aot_section_t = struct_wasm_section_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 83

wasm_section_list_t = POINTER(struct_wasm_section_t)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 83

aot_section_list_t = POINTER(struct_wasm_section_t)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 83

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 86
class struct_WASMExecEnv(Structure):
    pass

wasm_exec_env_t = POINTER(struct_WASMExecEnv)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 87

enum_anon_2 = c_int# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 94

Wasm_Module_Bytecode = 0# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 94

Wasm_Module_AoT = (Wasm_Module_Bytecode + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 94

Package_Type_Unknown = 0xFFFF# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 94

package_type_t = enum_anon_2# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 94

enum_anon_3 = c_int# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 108

Alloc_With_Pool = 0# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 108

Alloc_With_Allocator = (Alloc_With_Pool + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 108

Alloc_With_System_Allocator = (Alloc_With_Allocator + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 108

mem_alloc_type_t = enum_anon_3# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 108

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 112
class struct_anon_4(Structure):
    pass

struct_anon_4.__slots__ = [
    'heap_buf',
    'heap_size',
]
struct_anon_4._fields_ = [
    ('heap_buf', POINTER(None)),
    ('heap_size', uint32_t),
]

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 116
class struct_anon_5(Structure):
    pass

struct_anon_5.__slots__ = [
    'malloc_func',
    'realloc_func',
    'free_func',
    'user_data',
]
struct_anon_5._fields_ = [
    ('malloc_func', POINTER(None)),
    ('realloc_func', POINTER(None)),
    ('free_func', POINTER(None)),
    ('user_data', POINTER(None)),
]

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 124
class union_MemAllocOption(Union):
    pass

union_MemAllocOption.__slots__ = [
    'pool',
    'allocator',
]
union_MemAllocOption._fields_ = [
    ('pool', struct_anon_4),
    ('allocator', struct_anon_5),
]

MemAllocOption = union_MemAllocOption# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 124

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 132
class struct_mem_alloc_info_t(Structure):
    pass

struct_mem_alloc_info_t.__slots__ = [
    'total_size',
    'total_free_size',
    'highmark_size',
]
struct_mem_alloc_info_t._fields_ = [
    ('total_size', uint32_t),
    ('total_free_size', uint32_t),
    ('highmark_size', uint32_t),
]

mem_alloc_info_t = struct_mem_alloc_info_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 132

enum_RunningMode = c_int# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 140

Mode_Interp = 1# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 140

Mode_Fast_JIT = (Mode_Interp + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 140

Mode_LLVM_JIT = (Mode_Fast_JIT + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 140

Mode_Multi_Tier_JIT = (Mode_LLVM_JIT + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 140

RunningMode = enum_RunningMode# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 140

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 170
class struct_RuntimeInitArgs(Structure):
    pass

struct_RuntimeInitArgs.__slots__ = [
    'mem_alloc_type',
    'mem_alloc_option',
    'native_module_name',
    'native_symbols',
    'n_native_symbols',
    'max_thread_num',
    'ip_addr',
    'unused',
    'instance_port',
    'fast_jit_code_cache_size',
    'running_mode',
    'llvm_jit_opt_level',
    'llvm_jit_size_level',
]
struct_RuntimeInitArgs._fields_ = [
    ('mem_alloc_type', mem_alloc_type_t),
    ('mem_alloc_option', MemAllocOption),
    ('native_module_name', String),
    ('native_symbols', POINTER(NativeSymbol)),
    ('n_native_symbols', uint32_t),
    ('max_thread_num', uint32_t),
    ('ip_addr', c_char * int(128)),
    ('unused', c_int),
    ('instance_port', c_int),
    ('fast_jit_code_cache_size', uint32_t),
    ('running_mode', RunningMode),
    ('llvm_jit_opt_level', uint32_t),
    ('llvm_jit_size_level', uint32_t),
]

RuntimeInitArgs = struct_RuntimeInitArgs# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 170

wasm_valkind_t = uint8_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 174

enum_wasm_valkind_enum = c_int# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

WASM_I32 = 0# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

WASM_I64 = (WASM_I32 + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

WASM_F32 = (WASM_I64 + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

WASM_F64 = (WASM_F32 + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

WASM_ANYREF = 128# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

WASM_FUNCREF = (WASM_ANYREF + 1)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 175

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 190
class union_anon_6(Union):
    pass

union_anon_6.__slots__ = [
    'i32',
    'i64',
    'f32',
    'f64',
    'foreign',
]
union_anon_6._fields_ = [
    ('i32', c_int32),
    ('i64', c_int64),
    ('f32', c_float),
    ('f64', c_double),
    ('foreign', uintptr_t),
]

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 199
class struct_wasm_val_t(Structure):
    pass

struct_wasm_val_t.__slots__ = [
    'kind',
    'of',
]
struct_wasm_val_t._fields_ = [
    ('kind', wasm_valkind_t),
    ('of', union_anon_6),
]

wasm_val_t = struct_wasm_val_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 199

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 210
if _libs["../libs/libiwasm.so"].has("wasm_runtime_init", "cdecl"):
    wasm_runtime_init = _libs["../libs/libiwasm.so"].get("wasm_runtime_init", "cdecl")
    wasm_runtime_init.argtypes = []
    wasm_runtime_init.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 222
if _libs["../libs/libiwasm.so"].has("wasm_runtime_full_init", "cdecl"):
    wasm_runtime_full_init = _libs["../libs/libiwasm.so"].get("wasm_runtime_full_init", "cdecl")
    wasm_runtime_full_init.argtypes = [POINTER(RuntimeInitArgs)]
    wasm_runtime_full_init.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 232
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_is_running_mode_supported", "cdecl"):
        continue
    wasm_runtime_is_running_mode_supported = _lib.get("wasm_runtime_is_running_mode_supported", "cdecl")
    wasm_runtime_is_running_mode_supported.argtypes = [RunningMode]
    wasm_runtime_is_running_mode_supported.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 244
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_set_default_running_mode", "cdecl"):
        continue
    wasm_runtime_set_default_running_mode = _lib.get("wasm_runtime_set_default_running_mode", "cdecl")
    wasm_runtime_set_default_running_mode.argtypes = [RunningMode]
    wasm_runtime_set_default_running_mode.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 250
if _libs["../libs/libiwasm.so"].has("wasm_runtime_destroy", "cdecl"):
    wasm_runtime_destroy = _libs["../libs/libiwasm.so"].get("wasm_runtime_destroy", "cdecl")
    wasm_runtime_destroy.argtypes = []
    wasm_runtime_destroy.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 259
if _libs["../libs/libiwasm.so"].has("wasm_runtime_malloc", "cdecl"):
    wasm_runtime_malloc = _libs["../libs/libiwasm.so"].get("wasm_runtime_malloc", "cdecl")
    wasm_runtime_malloc.argtypes = [c_uint]
    wasm_runtime_malloc.restype = POINTER(c_ubyte)
    wasm_runtime_malloc.errcheck = lambda v,*a : cast(v, c_void_p)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 270
if _libs["../libs/libiwasm.so"].has("wasm_runtime_realloc", "cdecl"):
    wasm_runtime_realloc = _libs["../libs/libiwasm.so"].get("wasm_runtime_realloc", "cdecl")
    wasm_runtime_realloc.argtypes = [POINTER(None), c_uint]
    wasm_runtime_realloc.restype = POINTER(c_ubyte)
    wasm_runtime_realloc.errcheck = lambda v,*a : cast(v, c_void_p)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 277
if _libs["../libs/libiwasm.so"].has("wasm_runtime_free", "cdecl"):
    wasm_runtime_free = _libs["../libs/libiwasm.so"].get("wasm_runtime_free", "cdecl")
    wasm_runtime_free.argtypes = [POINTER(None)]
    wasm_runtime_free.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 283
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_mem_alloc_info", "cdecl"):
    wasm_runtime_get_mem_alloc_info = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_mem_alloc_info", "cdecl")
    wasm_runtime_get_mem_alloc_info.argtypes = [POINTER(mem_alloc_info_t)]
    wasm_runtime_get_mem_alloc_info.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 294
if _libs["../libs/libiwasm.so"].has("get_package_type", "cdecl"):
    get_package_type = _libs["../libs/libiwasm.so"].get("get_package_type", "cdecl")
    get_package_type.argtypes = [POINTER(uint8_t), uint32_t]
    get_package_type.restype = package_type_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 305
if _libs["../libs/libiwasm.so"].has("wasm_runtime_is_xip_file", "cdecl"):
    wasm_runtime_is_xip_file = _libs["../libs/libiwasm.so"].get("wasm_runtime_is_xip_file", "cdecl")
    wasm_runtime_is_xip_file.argtypes = [POINTER(uint8_t), uint32_t]
    wasm_runtime_is_xip_file.restype = c_bool

module_reader = CFUNCTYPE(UNCHECKED(c_bool), String, POINTER(POINTER(uint8_t)), POINTER(uint32_t))# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 310

module_destroyer = CFUNCTYPE(UNCHECKED(None), POINTER(uint8_t), uint32_t)# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 316

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 325
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_set_module_reader", "cdecl"):
        continue
    wasm_runtime_set_module_reader = _lib.get("wasm_runtime_set_module_reader", "cdecl")
    wasm_runtime_set_module_reader.argtypes = [module_reader, module_destroyer]
    wasm_runtime_set_module_reader.restype = None
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 339
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_register_module", "cdecl"):
        continue
    wasm_runtime_register_module = _lib.get("wasm_runtime_register_module", "cdecl")
    wasm_runtime_register_module.argtypes = [String, wasm_module_t, String, uint32_t]
    wasm_runtime_register_module.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 351
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_find_module_registered", "cdecl"):
        continue
    wasm_runtime_find_module_registered = _lib.get("wasm_runtime_find_module_registered", "cdecl")
    wasm_runtime_find_module_registered.argtypes = [String]
    wasm_runtime_find_module_registered.restype = wasm_module_t
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 375
if _libs["../libs/libiwasm.so"].has("wasm_runtime_load", "cdecl"):
    wasm_runtime_load = _libs["../libs/libiwasm.so"].get("wasm_runtime_load", "cdecl")
    wasm_runtime_load.argtypes = [POINTER(uint8_t), uint32_t, String, uint32_t]
    wasm_runtime_load.restype = wasm_module_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 389
if _libs["../libs/libiwasm.so"].has("wasm_runtime_load_from_sections", "cdecl"):
    wasm_runtime_load_from_sections = _libs["../libs/libiwasm.so"].get("wasm_runtime_load_from_sections", "cdecl")
    wasm_runtime_load_from_sections.argtypes = [wasm_section_list_t, c_bool, String, uint32_t]
    wasm_runtime_load_from_sections.restype = wasm_module_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 398
if _libs["../libs/libiwasm.so"].has("wasm_runtime_unload", "cdecl"):
    wasm_runtime_unload = _libs["../libs/libiwasm.so"].get("wasm_runtime_unload", "cdecl")
    wasm_runtime_unload.argtypes = [wasm_module_t]
    wasm_runtime_unload.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 408
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_get_module_hash", "cdecl"):
        continue
    wasm_runtime_get_module_hash = _lib.get("wasm_runtime_get_module_hash", "cdecl")
    wasm_runtime_get_module_hash.argtypes = [wasm_module_t]
    if sizeof(c_int) == sizeof(c_void_p):
        wasm_runtime_get_module_hash.restype = ReturnString
    else:
        wasm_runtime_get_module_hash.restype = String
        wasm_runtime_get_module_hash.errcheck = ReturnString
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 438
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_wasi_args_ex", "cdecl"):
    wasm_runtime_set_wasi_args_ex = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_wasi_args_ex", "cdecl")
    wasm_runtime_set_wasi_args_ex.argtypes = [wasm_module_t, POINTER(POINTER(c_char)), uint32_t, POINTER(POINTER(c_char)), uint32_t, POINTER(POINTER(c_char)), uint32_t, POINTER(POINTER(c_char)), c_int, c_int, c_int, c_int]
    wasm_runtime_set_wasi_args_ex.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 452
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_wasi_args", "cdecl"):
    wasm_runtime_set_wasi_args = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_wasi_args", "cdecl")
    wasm_runtime_set_wasi_args.argtypes = [wasm_module_t, POINTER(POINTER(c_char)), uint32_t, POINTER(POINTER(c_char)), uint32_t, POINTER(POINTER(c_char)), uint32_t, POINTER(POINTER(c_char)), c_int]
    wasm_runtime_set_wasi_args.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 459
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_wasi_addr_pool", "cdecl"):
    wasm_runtime_set_wasi_addr_pool = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_wasi_addr_pool", "cdecl")
    wasm_runtime_set_wasi_addr_pool.argtypes = [wasm_module_t, POINTER(POINTER(c_char)), uint32_t]
    wasm_runtime_set_wasi_addr_pool.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 463
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_wasi_ns_lookup_pool", "cdecl"):
    wasm_runtime_set_wasi_ns_lookup_pool = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_wasi_ns_lookup_pool", "cdecl")
    wasm_runtime_set_wasi_ns_lookup_pool.argtypes = [wasm_module_t, POINTER(POINTER(c_char)), uint32_t]
    wasm_runtime_set_wasi_ns_lookup_pool.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 486
if _libs["../libs/libiwasm.so"].has("wasm_runtime_instantiate", "cdecl"):
    wasm_runtime_instantiate = _libs["../libs/libiwasm.so"].get("wasm_runtime_instantiate", "cdecl")
    wasm_runtime_instantiate.argtypes = [wasm_module_t, uint32_t, uint32_t, String, uint32_t]
    wasm_runtime_instantiate.restype = wasm_module_inst_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 502
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_set_running_mode", "cdecl"):
        continue
    wasm_runtime_set_running_mode = _lib.get("wasm_runtime_set_running_mode", "cdecl")
    wasm_runtime_set_running_mode.argtypes = [wasm_module_inst_t, RunningMode]
    wasm_runtime_set_running_mode.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 516
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_get_running_mode", "cdecl"):
        continue
    wasm_runtime_get_running_mode = _lib.get("wasm_runtime_get_running_mode", "cdecl")
    wasm_runtime_get_running_mode.argtypes = [wasm_module_inst_t]
    wasm_runtime_get_running_mode.restype = RunningMode
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 524
if _libs["../libs/libiwasm.so"].has("wasm_runtime_deinstantiate", "cdecl"):
    wasm_runtime_deinstantiate = _libs["../libs/libiwasm.so"].get("wasm_runtime_deinstantiate", "cdecl")
    wasm_runtime_deinstantiate.argtypes = [wasm_module_inst_t]
    wasm_runtime_deinstantiate.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 534
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_module", "cdecl"):
    wasm_runtime_get_module = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_module", "cdecl")
    wasm_runtime_get_module.argtypes = [wasm_module_inst_t]
    wasm_runtime_get_module.restype = wasm_module_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 537
if _libs["../libs/libiwasm.so"].has("wasm_runtime_is_wasi_mode", "cdecl"):
    wasm_runtime_is_wasi_mode = _libs["../libs/libiwasm.so"].get("wasm_runtime_is_wasi_mode", "cdecl")
    wasm_runtime_is_wasi_mode.argtypes = [wasm_module_inst_t]
    wasm_runtime_is_wasi_mode.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 540
if _libs["../libs/libiwasm.so"].has("wasm_runtime_lookup_wasi_start_function", "cdecl"):
    wasm_runtime_lookup_wasi_start_function = _libs["../libs/libiwasm.so"].get("wasm_runtime_lookup_wasi_start_function", "cdecl")
    wasm_runtime_lookup_wasi_start_function.argtypes = [wasm_module_inst_t]
    wasm_runtime_lookup_wasi_start_function.restype = wasm_function_inst_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 552
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_wasi_exit_code", "cdecl"):
    wasm_runtime_get_wasi_exit_code = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_wasi_exit_code", "cdecl")
    wasm_runtime_get_wasi_exit_code.argtypes = [wasm_module_inst_t]
    wasm_runtime_get_wasi_exit_code.restype = uint32_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 564
if _libs["../libs/libiwasm.so"].has("wasm_runtime_lookup_function", "cdecl"):
    wasm_runtime_lookup_function = _libs["../libs/libiwasm.so"].get("wasm_runtime_lookup_function", "cdecl")
    wasm_runtime_lookup_function.argtypes = [wasm_module_inst_t, String, String]
    wasm_runtime_lookup_function.restype = wasm_function_inst_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 576
if _libs["../libs/libiwasm.so"].has("wasm_func_get_param_count", "cdecl"):
    wasm_func_get_param_count = _libs["../libs/libiwasm.so"].get("wasm_func_get_param_count", "cdecl")
    wasm_func_get_param_count.argtypes = [wasm_function_inst_t, wasm_module_inst_t]
    wasm_func_get_param_count.restype = uint32_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 588
if _libs["../libs/libiwasm.so"].has("wasm_func_get_result_count", "cdecl"):
    wasm_func_get_result_count = _libs["../libs/libiwasm.so"].get("wasm_func_get_result_count", "cdecl")
    wasm_func_get_result_count.argtypes = [wasm_function_inst_t, wasm_module_inst_t]
    wasm_func_get_result_count.restype = uint32_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 599
if _libs["../libs/libiwasm.so"].has("wasm_func_get_param_types", "cdecl"):
    wasm_func_get_param_types = _libs["../libs/libiwasm.so"].get("wasm_func_get_param_types", "cdecl")
    wasm_func_get_param_types.argtypes = [wasm_function_inst_t, wasm_module_inst_t, POINTER(wasm_valkind_t)]
    wasm_func_get_param_types.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 611
if _libs["../libs/libiwasm.so"].has("wasm_func_get_result_types", "cdecl"):
    wasm_func_get_result_types = _libs["../libs/libiwasm.so"].get("wasm_func_get_result_types", "cdecl")
    wasm_func_get_result_types.argtypes = [wasm_function_inst_t, wasm_module_inst_t, POINTER(wasm_valkind_t)]
    wasm_func_get_result_types.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 625
if _libs["../libs/libiwasm.so"].has("wasm_runtime_create_exec_env", "cdecl"):
    wasm_runtime_create_exec_env = _libs["../libs/libiwasm.so"].get("wasm_runtime_create_exec_env", "cdecl")
    wasm_runtime_create_exec_env.argtypes = [wasm_module_inst_t, uint32_t]
    wasm_runtime_create_exec_env.restype = wasm_exec_env_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 634
if _libs["../libs/libiwasm.so"].has("wasm_runtime_destroy_exec_env", "cdecl"):
    wasm_runtime_destroy_exec_env = _libs["../libs/libiwasm.so"].get("wasm_runtime_destroy_exec_env", "cdecl")
    wasm_runtime_destroy_exec_env.argtypes = [wasm_exec_env_t]
    wasm_runtime_destroy_exec_env.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 651
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_exec_env_singleton", "cdecl"):
    wasm_runtime_get_exec_env_singleton = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_exec_env_singleton", "cdecl")
    wasm_runtime_get_exec_env_singleton.argtypes = [wasm_module_inst_t]
    wasm_runtime_get_exec_env_singleton.restype = wasm_exec_env_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 674
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_start_debug_instance_with_port", "cdecl"):
        continue
    wasm_runtime_start_debug_instance_with_port = _lib.get("wasm_runtime_start_debug_instance_with_port", "cdecl")
    wasm_runtime_start_debug_instance_with_port.argtypes = [wasm_exec_env_t, c_int32]
    wasm_runtime_start_debug_instance_with_port.restype = uint32_t
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 680
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_start_debug_instance", "cdecl"):
        continue
    wasm_runtime_start_debug_instance = _lib.get("wasm_runtime_start_debug_instance", "cdecl")
    wasm_runtime_start_debug_instance.argtypes = [wasm_exec_env_t]
    wasm_runtime_start_debug_instance.restype = uint32_t
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 695
if _libs["../libs/libiwasm.so"].has("wasm_runtime_init_thread_env", "cdecl"):
    wasm_runtime_init_thread_env = _libs["../libs/libiwasm.so"].get("wasm_runtime_init_thread_env", "cdecl")
    wasm_runtime_init_thread_env.argtypes = []
    wasm_runtime_init_thread_env.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 701
if _libs["../libs/libiwasm.so"].has("wasm_runtime_destroy_thread_env", "cdecl"):
    wasm_runtime_destroy_thread_env = _libs["../libs/libiwasm.so"].get("wasm_runtime_destroy_thread_env", "cdecl")
    wasm_runtime_destroy_thread_env.argtypes = []
    wasm_runtime_destroy_thread_env.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 707
if _libs["../libs/libiwasm.so"].has("wasm_runtime_thread_env_inited", "cdecl"):
    wasm_runtime_thread_env_inited = _libs["../libs/libiwasm.so"].get("wasm_runtime_thread_env_inited", "cdecl")
    wasm_runtime_thread_env_inited.argtypes = []
    wasm_runtime_thread_env_inited.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 717
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_module_inst", "cdecl"):
    wasm_runtime_get_module_inst = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_module_inst", "cdecl")
    wasm_runtime_get_module_inst.argtypes = [wasm_exec_env_t]
    wasm_runtime_get_module_inst.restype = wasm_module_inst_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 731
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_module_inst", "cdecl"):
    wasm_runtime_set_module_inst = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_module_inst", "cdecl")
    wasm_runtime_set_module_inst.argtypes = [wasm_exec_env_t, wasm_module_inst_t]
    wasm_runtime_set_module_inst.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 755
if _libs["../libs/libiwasm.so"].has("wasm_runtime_call_wasm", "cdecl"):
    wasm_runtime_call_wasm = _libs["../libs/libiwasm.so"].get("wasm_runtime_call_wasm", "cdecl")
    wasm_runtime_call_wasm.argtypes = [wasm_exec_env_t, wasm_function_inst_t, uint32_t, POINTER(uint32_t)]
    wasm_runtime_call_wasm.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 776
if _libs["../libs/libiwasm.so"].has("wasm_runtime_call_wasm_a", "cdecl"):
    wasm_runtime_call_wasm_a = _libs["../libs/libiwasm.so"].get("wasm_runtime_call_wasm_a", "cdecl")
    wasm_runtime_call_wasm_a.argtypes = [wasm_exec_env_t, wasm_function_inst_t, uint32_t, POINTER(wasm_val_t), uint32_t, POINTER(wasm_val_t)]
    wasm_runtime_call_wasm_a.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 798
if _libs["../libs/libiwasm.so"].has("wasm_runtime_call_wasm_v", "cdecl"):
    _func = _libs["../libs/libiwasm.so"].get("wasm_runtime_call_wasm_v", "cdecl")
    _restype = c_bool
    _errcheck = None
    _argtypes = [wasm_exec_env_t, wasm_function_inst_t, uint32_t, POINTER(wasm_val_t), uint32_t]
    wasm_runtime_call_wasm_v = _variadic_function(_func,_restype,_argtypes,_errcheck)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 818
if _libs["../libs/libiwasm.so"].has("wasm_application_execute_main", "cdecl"):
    wasm_application_execute_main = _libs["../libs/libiwasm.so"].get("wasm_application_execute_main", "cdecl")
    wasm_application_execute_main.argtypes = [wasm_module_inst_t, c_int32, POINTER(POINTER(c_char))]
    wasm_application_execute_main.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 837
if _libs["../libs/libiwasm.so"].has("wasm_application_execute_func", "cdecl"):
    wasm_application_execute_func = _libs["../libs/libiwasm.so"].get("wasm_application_execute_func", "cdecl")
    wasm_application_execute_func.argtypes = [wasm_module_inst_t, String, c_int32, POINTER(POINTER(c_char))]
    wasm_application_execute_func.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 846
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_exception", "cdecl"):
    wasm_runtime_get_exception = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_exception", "cdecl")
    wasm_runtime_get_exception.argtypes = [wasm_module_inst_t]
    wasm_runtime_get_exception.restype = c_char_p

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 857
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_exception", "cdecl"):
    wasm_runtime_set_exception = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_exception", "cdecl")
    wasm_runtime_set_exception.argtypes = [wasm_module_inst_t, String]
    wasm_runtime_set_exception.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 866
if _libs["../libs/libiwasm.so"].has("wasm_runtime_clear_exception", "cdecl"):
    wasm_runtime_clear_exception = _libs["../libs/libiwasm.so"].get("wasm_runtime_clear_exception", "cdecl")
    wasm_runtime_clear_exception.argtypes = [wasm_module_inst_t]
    wasm_runtime_clear_exception.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 878
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_custom_data", "cdecl"):
    wasm_runtime_set_custom_data = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_custom_data", "cdecl")
    wasm_runtime_set_custom_data.argtypes = [wasm_module_inst_t, POINTER(None)]
    wasm_runtime_set_custom_data.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 887
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_custom_data", "cdecl"):
    wasm_runtime_get_custom_data = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_custom_data", "cdecl")
    wasm_runtime_get_custom_data.argtypes = [wasm_module_inst_t]
    wasm_runtime_get_custom_data.restype = POINTER(c_ubyte)
    wasm_runtime_get_custom_data.errcheck = lambda v,*a : cast(v, c_void_p)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 910
if _libs["../libs/libiwasm.so"].has("wasm_runtime_module_malloc", "cdecl"):
    wasm_runtime_module_malloc = _libs["../libs/libiwasm.so"].get("wasm_runtime_module_malloc", "cdecl")
    wasm_runtime_module_malloc.argtypes = [wasm_module_inst_t, uint32_t, POINTER(POINTER(None))]
    wasm_runtime_module_malloc.restype = uint32_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 920
if _libs["../libs/libiwasm.so"].has("wasm_runtime_module_free", "cdecl"):
    wasm_runtime_module_free = _libs["../libs/libiwasm.so"].get("wasm_runtime_module_free", "cdecl")
    wasm_runtime_module_free.argtypes = [wasm_module_inst_t, uint32_t]
    wasm_runtime_module_free.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 936
if _libs["../libs/libiwasm.so"].has("wasm_runtime_module_dup_data", "cdecl"):
    wasm_runtime_module_dup_data = _libs["../libs/libiwasm.so"].get("wasm_runtime_module_dup_data", "cdecl")
    wasm_runtime_module_dup_data.argtypes = [wasm_module_inst_t, String, uint32_t]
    wasm_runtime_module_dup_data.restype = uint32_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 951
if _libs["../libs/libiwasm.so"].has("wasm_runtime_validate_app_addr", "cdecl"):
    wasm_runtime_validate_app_addr = _libs["../libs/libiwasm.so"].get("wasm_runtime_validate_app_addr", "cdecl")
    wasm_runtime_validate_app_addr.argtypes = [wasm_module_inst_t, uint32_t, uint32_t]
    wasm_runtime_validate_app_addr.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 973
if _libs["../libs/libiwasm.so"].has("wasm_runtime_validate_app_str_addr", "cdecl"):
    wasm_runtime_validate_app_str_addr = _libs["../libs/libiwasm.so"].get("wasm_runtime_validate_app_str_addr", "cdecl")
    wasm_runtime_validate_app_str_addr.argtypes = [wasm_module_inst_t, uint32_t]
    wasm_runtime_validate_app_str_addr.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 989
if _libs["../libs/libiwasm.so"].has("wasm_runtime_validate_native_addr", "cdecl"):
    wasm_runtime_validate_native_addr = _libs["../libs/libiwasm.so"].get("wasm_runtime_validate_native_addr", "cdecl")
    wasm_runtime_validate_native_addr.argtypes = [wasm_module_inst_t, POINTER(None), uint32_t]
    wasm_runtime_validate_native_addr.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1004
if _libs["../libs/libiwasm.so"].has("wasm_runtime_addr_app_to_native", "cdecl"):
    wasm_runtime_addr_app_to_native = _libs["../libs/libiwasm.so"].get("wasm_runtime_addr_app_to_native", "cdecl")
    wasm_runtime_addr_app_to_native.argtypes = [wasm_module_inst_t, uint32_t]
    wasm_runtime_addr_app_to_native.restype = POINTER(c_ubyte)
    wasm_runtime_addr_app_to_native.errcheck = lambda v,*a : cast(v, c_void_p)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1017
if _libs["../libs/libiwasm.so"].has("wasm_runtime_addr_native_to_app", "cdecl"):
    wasm_runtime_addr_native_to_app = _libs["../libs/libiwasm.so"].get("wasm_runtime_addr_native_to_app", "cdecl")
    wasm_runtime_addr_native_to_app.argtypes = [wasm_module_inst_t, POINTER(None)]
    wasm_runtime_addr_native_to_app.restype = uint32_t

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1031
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_app_addr_range", "cdecl"):
    wasm_runtime_get_app_addr_range = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_app_addr_range", "cdecl")
    wasm_runtime_get_app_addr_range.argtypes = [wasm_module_inst_t, uint32_t, POINTER(uint32_t), POINTER(uint32_t)]
    wasm_runtime_get_app_addr_range.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1050
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_native_addr_range", "cdecl"):
    wasm_runtime_get_native_addr_range = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_native_addr_range", "cdecl")
    wasm_runtime_get_native_addr_range.argtypes = [wasm_module_inst_t, POINTER(uint8_t), POINTER(POINTER(uint8_t)), POINTER(POINTER(uint8_t))]
    wasm_runtime_get_native_addr_range.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1089
if _libs["../libs/libiwasm.so"].has("wasm_runtime_register_natives", "cdecl"):
    wasm_runtime_register_natives = _libs["../libs/libiwasm.so"].get("wasm_runtime_register_natives", "cdecl")
    wasm_runtime_register_natives.argtypes = [String, POINTER(NativeSymbol), uint32_t]
    wasm_runtime_register_natives.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1104
if _libs["../libs/libiwasm.so"].has("wasm_runtime_register_natives_raw", "cdecl"):
    wasm_runtime_register_natives_raw = _libs["../libs/libiwasm.so"].get("wasm_runtime_register_natives_raw", "cdecl")
    wasm_runtime_register_natives_raw.argtypes = [String, POINTER(NativeSymbol), uint32_t]
    wasm_runtime_register_natives_raw.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1123
if _libs["../libs/libiwasm.so"].has("wasm_runtime_unregister_natives", "cdecl"):
    wasm_runtime_unregister_natives = _libs["../libs/libiwasm.so"].get("wasm_runtime_unregister_natives", "cdecl")
    wasm_runtime_unregister_natives.argtypes = [String, POINTER(NativeSymbol)]
    wasm_runtime_unregister_natives.restype = c_bool

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1133
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_function_attachment", "cdecl"):
    wasm_runtime_get_function_attachment = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_function_attachment", "cdecl")
    wasm_runtime_get_function_attachment.argtypes = [wasm_exec_env_t]
    wasm_runtime_get_function_attachment.restype = POINTER(c_ubyte)
    wasm_runtime_get_function_attachment.errcheck = lambda v,*a : cast(v, c_void_p)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1143
if _libs["../libs/libiwasm.so"].has("wasm_runtime_set_user_data", "cdecl"):
    wasm_runtime_set_user_data = _libs["../libs/libiwasm.so"].get("wasm_runtime_set_user_data", "cdecl")
    wasm_runtime_set_user_data.argtypes = [wasm_exec_env_t, POINTER(None)]
    wasm_runtime_set_user_data.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1152
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_user_data", "cdecl"):
    wasm_runtime_get_user_data = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_user_data", "cdecl")
    wasm_runtime_get_user_data.argtypes = [wasm_exec_env_t]
    wasm_runtime_get_user_data.restype = POINTER(c_ubyte)
    wasm_runtime_get_user_data.errcheck = lambda v,*a : cast(v, c_void_p)

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1165
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_dump_mem_consumption", "cdecl"):
        continue
    wasm_runtime_dump_mem_consumption = _lib.get("wasm_runtime_dump_mem_consumption", "cdecl")
    wasm_runtime_dump_mem_consumption.argtypes = [wasm_exec_env_t]
    wasm_runtime_dump_mem_consumption.restype = None
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1173
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_dump_perf_profiling", "cdecl"):
        continue
    wasm_runtime_dump_perf_profiling = _lib.get("wasm_runtime_dump_perf_profiling", "cdecl")
    wasm_runtime_dump_perf_profiling.argtypes = [wasm_module_inst_t]
    wasm_runtime_dump_perf_profiling.restype = None
    break

wasm_thread_callback_t = CFUNCTYPE(UNCHECKED(POINTER(c_ubyte)), wasm_exec_env_t, POINTER(None))# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1176

wasm_thread_t = uintptr_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1178

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1186
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_set_max_thread_num", "cdecl"):
        continue
    wasm_runtime_set_max_thread_num = _lib.get("wasm_runtime_set_max_thread_num", "cdecl")
    wasm_runtime_set_max_thread_num.argtypes = [uint32_t]
    wasm_runtime_set_max_thread_num.restype = None
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1197
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_spawn_exec_env", "cdecl"):
        continue
    wasm_runtime_spawn_exec_env = _lib.get("wasm_runtime_spawn_exec_env", "cdecl")
    wasm_runtime_spawn_exec_env.argtypes = [wasm_exec_env_t]
    wasm_runtime_spawn_exec_env.restype = wasm_exec_env_t
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1205
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_destroy_spawned_exec_env", "cdecl"):
        continue
    wasm_runtime_destroy_spawned_exec_env = _lib.get("wasm_runtime_destroy_spawned_exec_env", "cdecl")
    wasm_runtime_destroy_spawned_exec_env.argtypes = [wasm_exec_env_t]
    wasm_runtime_destroy_spawned_exec_env.restype = None
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1218
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_spawn_thread", "cdecl"):
        continue
    wasm_runtime_spawn_thread = _lib.get("wasm_runtime_spawn_thread", "cdecl")
    wasm_runtime_spawn_thread.argtypes = [wasm_exec_env_t, POINTER(wasm_thread_t), wasm_thread_callback_t, POINTER(None)]
    wasm_runtime_spawn_thread.restype = c_int32
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1230
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_join_thread", "cdecl"):
        continue
    wasm_runtime_join_thread = _lib.get("wasm_runtime_join_thread", "cdecl")
    wasm_runtime_join_thread.argtypes = [wasm_thread_t, POINTER(POINTER(None))]
    wasm_runtime_join_thread.restype = c_int32
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1244
for _lib in _libs.values():
    if not _lib.has("wasm_externref_obj2ref", "cdecl"):
        continue
    wasm_externref_obj2ref = _lib.get("wasm_externref_obj2ref", "cdecl")
    wasm_externref_obj2ref.argtypes = [wasm_module_inst_t, POINTER(None), POINTER(uint32_t)]
    wasm_externref_obj2ref.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1257
for _lib in _libs.values():
    if not _lib.has("wasm_externref_ref2obj", "cdecl"):
        continue
    wasm_externref_ref2obj = _lib.get("wasm_externref_ref2obj", "cdecl")
    wasm_externref_ref2obj.argtypes = [uint32_t, POINTER(POINTER(None))]
    wasm_externref_ref2obj.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1269
for _lib in _libs.values():
    if not _lib.has("wasm_externref_retain", "cdecl"):
        continue
    wasm_externref_retain = _lib.get("wasm_externref_retain", "cdecl")
    wasm_externref_retain.argtypes = [uint32_t]
    wasm_externref_retain.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1277
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_dump_call_stack", "cdecl"):
        continue
    wasm_runtime_dump_call_stack = _lib.get("wasm_runtime_dump_call_stack", "cdecl")
    wasm_runtime_dump_call_stack.argtypes = [wasm_exec_env_t]
    wasm_runtime_dump_call_stack.restype = None
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1288
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_get_call_stack_buf_size", "cdecl"):
        continue
    wasm_runtime_get_call_stack_buf_size = _lib.get("wasm_runtime_get_call_stack_buf_size", "cdecl")
    wasm_runtime_get_call_stack_buf_size.argtypes = [wasm_exec_env_t]
    wasm_runtime_get_call_stack_buf_size.restype = uint32_t
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1304
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_dump_call_stack_to_buf", "cdecl"):
        continue
    wasm_runtime_dump_call_stack_to_buf = _lib.get("wasm_runtime_dump_call_stack_to_buf", "cdecl")
    wasm_runtime_dump_call_stack_to_buf.argtypes = [wasm_exec_env_t, String, uint32_t]
    wasm_runtime_dump_call_stack_to_buf.restype = uint32_t
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1317
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_get_custom_section", "cdecl"):
        continue
    wasm_runtime_get_custom_section = _lib.get("wasm_runtime_get_custom_section", "cdecl")
    wasm_runtime_get_custom_section.argtypes = [wasm_module_t, String, POINTER(uint32_t)]
    wasm_runtime_get_custom_section.restype = POINTER(uint8_t)
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1326
if _libs["../libs/libiwasm.so"].has("wasm_runtime_get_version", "cdecl"):
    wasm_runtime_get_version = _libs["../libs/libiwasm.so"].get("wasm_runtime_get_version", "cdecl")
    wasm_runtime_get_version.argtypes = [POINTER(uint32_t), POINTER(uint32_t), POINTER(uint32_t)]
    wasm_runtime_get_version.restype = None

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1333
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_is_import_func_linked", "cdecl"):
        continue
    wasm_runtime_is_import_func_linked = _lib.get("wasm_runtime_is_import_func_linked", "cdecl")
    wasm_runtime_is_import_func_linked.argtypes = [String, String]
    wasm_runtime_is_import_func_linked.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 1341
for _lib in _libs.values():
    if not _lib.has("wasm_runtime_is_import_global_linked", "cdecl"):
        continue
    wasm_runtime_is_import_global_linked = _lib.get("wasm_runtime_is_import_global_linked", "cdecl")
    wasm_runtime_is_import_global_linked.argtypes = [String, String]
    wasm_runtime_is_import_global_linked.restype = c_bool
    break

# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 31
def get_module_inst(exec_env):
    return (wasm_runtime_get_module_inst (exec_env))

WASMModuleCommon = struct_WASMModuleCommon# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 62

WASMModuleInstanceCommon = struct_WASMModuleInstanceCommon# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 67

wasm_section_t = struct_wasm_section_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 75

WASMExecEnv = struct_WASMExecEnv# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 86

MemAllocOption = union_MemAllocOption# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 124

mem_alloc_info_t = struct_mem_alloc_info_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 132

RuntimeInitArgs = struct_RuntimeInitArgs# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 170

wasm_val_t = struct_wasm_val_t# wasm-micro-runtime/core/iwasm/include/wasm_export.h: 199

# No inserted files

# No prefix-stripping

