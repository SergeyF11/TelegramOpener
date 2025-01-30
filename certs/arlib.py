# References:
#   https://www.freebsd.org/cgi/man.cgi?query=ar&sektion=5&apropos=0&manpath=FreeBSD+10.1-RELEASE
#   http://www.opensource.apple.com/source/cctools/cctools-795/include/mach-o/ranlib.h

import os
import re
import sys
import time
import string
import struct
import shutil
import logging

GNU = 0
BSD = 1

_FORMATNAMES = {
    None: "(none)",
    GNU: "GNU",
    BSD: "BSD"
}

_BLOCKSIZE = 65535
_WHITESPACERE = re.compile(r"[\s]")

logging.basicConfig()
log = logging.getLogger("arlib")
log.setLevel(logging.INFO)

import sys
if sys.version_info >= (3, 0):
    isstring = lambda v: isinstance(v, str)
    tobytes = bytes
    tostr = str
else:
    isstring = lambda v: isinstance(v, str) or isinstance(v, unicode)
    tobytes = lambda s, e: bytes(s)
    tostr = lambda v, e: str(v)

class WrongMemberTypeException(Exception):
    pass

class InvalidArchiveException(Exception):
    pass

class ArchiveMember(object):
    format = None
    normal = True

    _header_format = struct.Struct("=16s12s6s6s8s10s2s")
    _header_fill = b" "
    _header_tail = b"`\n"

    def __init__(self, archive, path=None):
        self._name = None
        self._filename = None
        self._size = 0
        self._offset = None

        self.archive = archive
        self.sourcedir = None
        if path:
            self.init_from_file(path)
        else:
            self.init_from_archive()

    def __repr__(self):
        return "<{0}(filename={1}, sourcedir={2}, name={3}, date={4}, uid={5}, gid={6}, mode=0{7:04o}, size={8})>".format(
            self.__class__.__name__, self.filename, self.sourcedir, self.name, self.date, self.uid, self.gid, self.mode, self.size)

    @property
    def name(self):
        if isinstance(self._name, bytes):
            return tostr(self._name, self.archive.encoding)
        return self._name
    @name.setter
    def name(self, value):
        if isstring(value):
            value = tobytes(value, self.archive.encoding)
        self._name = value

    @property
    def filename(self):
        if isinstance(self._filename, bytes):
            return tostr(self._filename, self.archive.encoding)
        return self._filename
    @filename.setter
    def filename(self, value):
        if isstring(value):
            value = value.strip(string.whitespace + "\x00")
            value = tobytes(value, self.archive.encoding)
        self._filename = value

    @property
    def size(self):
        return self._size
    @size.setter
    def size(self, value):
        self._size = value

    @property
    def filesize(self):
        return self.size
    @filesize.setter
    def filesize(self, value):
        self.size = value

    @property
    def offset(self):
        return self._offset
    @offset.setter
    def offset(self, value):
        self._offset = value

    def init_from_file(self, path):
        prop = os.stat(path)

        self.set_name_from_file(os.path.basename(path))
        self.sourcedir = os.path.dirname(path)
        self.date = int(prop.st_mtime)
        self.uid = prop.st_uid
        self.gid = prop.st_gid
        self.mode = prop.st_mode
        self.filesize = prop.st_size
        self.offset = None

    def init_from_archive(self):
        header = self.archive.instream.read(self._header_format.size)
        if len(header) < self._header_format.size:
            raise EOFError()
        nameinfo, date, uid, gid, mode, size, tail = self._header_format.unpack(header)
        log.debug("Read header values: %s, %s, %s, %s, %s, %s",
            nameinfo.strip(), date.strip(), uid.strip(), gid.strip(), mode.strip(), size.strip())
        assert tail == self._header_tail

        self.set_name_from_archive(nameinfo.decode(self.archive.encoding).strip())
        self.sourcedir = None
        self.date = int(date.decode(self.archive.encoding).strip())
        self.uid = int(uid.decode(self.archive.encoding).strip())
        self.gid = int(gid.decode(self.archive.encoding).strip())
        self.mode = int(mode.decode(self.archive.encoding).strip(), 8)
        self.size = int(size.decode(self.archive.encoding).strip())
        self.offset = self.archive.instream.tell()

    @classmethod
    def derived(cls):
        classes = []
        for c in cls.__subclasses__():
            classes.append(c)
            classes += c.derived()
        return set(classes)

    def set_name_from_file(self, filename):
        raise NotImplementedError("The method set_name_from_file() must be implemented in derived classes.")

    def set_name_from_archive(self, nameinfo):
        raise NotImplementedError("The method set_name_from_archive() must be implemented in derived classes.")

    def write_header(self):
        name = self.name.encode(self.archive.encoding).ljust(16)
        date = str(self.date).encode(self.archive.encoding).ljust(12)
        uid = str(self.uid).encode(self.archive.encoding).ljust(6)
        gid = str(self.gid).encode(self.archive.encoding).ljust(6)
        mode = "{0:o}".format(self.mode).encode(self.archive.encoding).ljust(8)
        size = str(self.size).encode(self.archive.encoding).ljust(10)

        packed = self._header_format.pack(name, date, uid, gid, mode, size, self._header_tail)
        self.archive.outstream.write(packed)

    def extract(self, path):
        path = os.path.abspath(path)
        filepath = os.path.join(path, self.filename)
        if self.sourcedir is None:
            try:
                os.makedirs(path)
            except OSError:
                pass
            with open(filepath, "wb") as outfile:
                infile = self.archive.instream
                infile.seek(self.offset)
                remaining = self.filesize
                while remaining >= _BLOCKSIZE:
                    buf = infile.read(_BLOCKSIZE)
                    outfile.write(buf)
                    remaining -= _BLOCKSIZE
                if remaining > 0:
                    buf = infile.read(remaining)
                    outfile.write(buf)
            os.chmod(filepath, self.mode)
            try:
                os.chown(filepath, self.uid, self.gid)
            except:
                pass
            os.utime(filepath, (time.time(), self.date))
        else:
            externaldir = os.path.abspath(self.sourcedir)
            externalfile = os.path.join(externaldir, self.filename)
            shutil.copy2(externalfile, filepath)

    def collect(self):
        outfile = self.archive.outstream
        newoffset = outfile.tell()
        infile = None
        if self.sourcedir is None:
            infile = self.archive.instream
            infile.seek(self.offset)
        else:
            externaldir = os.path.abspath(self.sourcedir)
            externalfile = os.path.join(externaldir, self.filename)
            infile = open(externalfile, "rb")

        remaining = self.filesize
        while remaining >= _BLOCKSIZE:
            buf = infile.read(_BLOCKSIZE)
            outfile.write(buf)
            remaining -= _BLOCKSIZE
        if remaining > 0:
            buf = infile.read(remaining)
            outfile.write(buf)
        self.offset = newoffset
        self.sourcedir = None

class GNUShortMember(ArchiveMember):
    format = GNU

    _name_terminal = "/"

    def set_name_from_file(self, filename):
        if len(filename.encode(self.archive.encoding)) < 16 and not _WHITESPACERE.search(filename):
            self.filename = filename
            self.name = filename + self._name_terminal
        else:
            raise WrongMemberTypeException("Not a short GNU archive member.")

    def set_name_from_archive(self, nameinfo):
        if len(nameinfo) > 1 and nameinfo.endswith(self._name_terminal) and not nameinfo.startswith(self._name_terminal) \
                and not _WHITESPACERE.search(nameinfo):
            self.filename = nameinfo[:-len(self._name_terminal)]
            self.name = nameinfo
        else:
            raise WrongMemberTypeException("Not a short GNU archive member.")

class GNULongMember(ArchiveMember):
    format = GNU

    _name_prefix = "/"
    _name_re = re.compile(r"^/(\d+)$")

    @property
    def name(self):
        offset = self.archive.strings.string_offset(self)
        return self._name_prefix + str(offset)

    @property
    def filename(self):
        value = self.archive.strings[self]
        if isinstance(value, bytes):
            return tostr(value, self.archive.encoding)
        return value
    @filename.setter
    def filename(self, value):
        if isstring(value):
            value = value.strip(string.whitespace + "\x00")
            value = tobytes(value, self.archive.encoding)
        self.archive.strings[self] = value

    def set_name_from_file(self, filename):
        if len(filename.encode(self.archive.encoding)) >= 16 or _WHITESPACERE.search(filename):
            self.filename = filename
        else:
            raise WrongMemberTypeException("Not a long GNU archive member.")

    def set_name_from_archive(self, nameinfo):
        m = self._name_re.match(nameinfo)
        if m:
            offset = int(m.group(1))
            self.archive.strings.map(self, offset)
        else:
            raise WrongMemberTypeException("Not a long GNU archive member.")

class GNUSymbolTable(ArchiveMember):
    format = GNU
    normal = False

    _name_literal = "/"

    def __init__(self, archive, path=None):
        super(GNUSymbolTable, self).__init__(archive, path)
        self.archive.symbols = self

    def init_from_file(self, path):
        if path == True:
            self.name = self._name_literal
            self.filename = None
            self.date = int(time.time())
            self.uid = 0
            self.gid = 0
            self.mode = 0o100644
            self.offset = None
        else:
            raise WrongMemberTypeException("Not a GNU symbol table archive member.")

    # @property
    # def size(self):
    #     return self._size
    # @size.setter
    # def size(self, value):
    #     self._size = value

    def set_name_from_archive(self, nameinfo):
        if nameinfo == self._name_literal:
            self.filename = None
            self.name = nameinfo
        else:
            raise WrongMemberTypeException("Not a GNU symbol table archive member.")

class GNUStringTable(ArchiveMember):
    format = GNU
    normal = False

    _delimiter = b"/\n"
    _name_literal = "//"

    def __init__(self, archive, path=None):
        self._items = {}
        self._order = []
        super(GNUStringTable, self).__init__(archive, path)
        self.archive.strings = self

    @property
    def size(self):
        if self._size is None:
            size = 0
            delimlen = len(self._delimiter)
            for m in self._order:
                size += len(self._items[m]) + delimlen
            return size
        else:
            return self._size
    @size.setter
    def size(self, value):
        self._size = value

    def init_from_file(self, path):
        if path == True:
            self.name = self._name_literal
            self.filename = None
            self.date = int(time.time())
            self.uid = 0
            self.gid = 0
            self.mode = 0o100644
            self.offset = None
        else:
            raise WrongMemberTypeException("Not a GNU string table archive member.")

    def set_name_from_archive(self, nameinfo):
        if nameinfo == self._name_literal:
            self.name = nameinfo
            self.filename = None
        else:
            raise WrongMemberTypeException("Not a GNU string table archive member.")

    def init_from_archive(self):
        header = self.archive.instream.read(self._header_format.size)
        if len(header) < self._header_format.size:
            raise EOFError()
        nameinfo, date, uid, gid, mode, size, tail = self._header_format.unpack(header)
        log.debug("Read header values: %s, %s, %s, %s, %s, %s",
            nameinfo.strip(), date.strip(), uid.strip(), gid.strip(), mode.strip(), size.strip())
        assert tail == self._header_tail

        self.set_name_from_archive(nameinfo.decode(self.archive.encoding).strip())
        self.sourcedir = None
        try:
            self.date = int(date.decode(self.archive.encoding).strip())
        except ValueError:
            self.date = 0
        try:
            self.uid = int(uid.decode(self.archive.encoding).strip())
        except ValueError:
            self.uid = 0
        try:
            self.gid = int(gid.decode(self.archive.encoding).strip())
        except ValueError:
            self.gid = 0
        try:
            self.mode = int(mode.decode(self.archive.encoding).strip(), 8)
        except ValueError:
            self.mode = 0o100644
        self.size = int(size.decode(self.archive.encoding).strip())
        self.offset = self.archive.instream.tell()

    def map(self, member, offset):
        instream = self.archive.instream
        prevoffset = instream.tell()
        instream.seek(self.offset + offset)
        delimlen = len(self._delimiter)
        filename = instream.read(delimlen)
        while filename[-delimlen:] != self._delimiter:
            c = instream.read(1)
            if len(c) == 0:
                raise InvalidArchiveException("Unterminated string table.")
            filename += c
        filename = filename[:-2].decode(self.archive.encoding)
        self[member] = filename.strip(string.whitespace + "\x00")
        instream.seek(prevoffset)

    def __len__(self):
        return len(self._order)

    def __getitem__(self, member):
        return self._items[member]

    def __setitem__(self, member, filename):
        if isstring(filename):
            filename = filename.encode(self.archive.encoding)
        self._items[member] = filename
        if member not in self._order:
            self._order.append(member)

    def __delitem__(self, member):
        if member in self._items:
            del self._items[member]
        if member in self._order:
            self._order.remove(member)

    class Iterator(object):
        def __init__(self, order, items):
            self._order = order
            self._items = items
            self.index = -1

        def __iter__(self):
            return self

        def __next__(self):
            return self.next()

        def next(self):
            if self.index < len(self._order):
                m = self._order[self.index]
                self.index += 1
                return (m, self._items[m])
            else:
                raise StopIteration()

    def __iter__(self):
        return Iterator(self._order, self._items)

    def string_offset(self, member):
        offset = 0
        delimlen = len(self._delimiter)
        for m in self._order:
            if m == member:
                break
            offset += len(self._items[m]) + delimlen
        return offset

    def collect(self):
        outfile = self.archive.outstream
        newoffset = outfile.tell()
        for m in self._order:
            outfile.write(self._items[m] + self._delimiter)
        self.offset = newoffset

class BSDShortMember(ArchiveMember):
    format = BSD

    def set_name_from_file(self, filename):
        if len(filename.encode(self.archive.encoding)) <= 16 and not _WHITESPACERE.search(filename):
            self.name = filename
            self.filename = filename
        else:
            raise WrongMemberTypeException("Not a short BSD archive member.")

    def set_name_from_archive(self, nameinfo):
        if not nameinfo.startswith(BSDSymbolTable._name_literal) and GNUShortMember._name_terminal not in nameinfo:
            self.name = nameinfo
            self.filename = nameinfo
        else:
            raise WrongMemberTypeException("Not a short BSD archive member.")

class BSDLongMember(ArchiveMember):
    format = BSD

    _name_prefix = "#1/"

    def __init__(self, archive, path=None):
        self.namelength = 0
        super(BSDLongMember, self).__init__(archive, path)

    @property
    def filesize(self):
        return self._size - self.namelength
    @filesize.setter
    def filesize(self, value):
        self._size = value + self.namelength

    @property
    def name(self):
        return self._name_prefix + str(self.namelength)
    @name.setter
    def name(self, value):
        pass

    def set_name_from_file(self, filename):
        encoded = filename.encode(self.archive.encoding)
        if len(encoded) > 16 or _WHITESPACERE.search(filename):
            self.namelength = len(encoded)
            self.filename = filename
        else:
            raise WrongMemberTypeException("Not a long BSD archive member.")

    def set_name_from_archive(self, nameinfo):
        if nameinfo.startswith(self._name_prefix):
            self.namelength = int(nameinfo[len(self._name_prefix):])
            self.filename = self.archive.instream.read(self.namelength).decode(self.archive.encoding)
            if self.filename.startswith(BSDSymbolTable._name_literal):
                raise WrongMemberTypeException("Not a long BSD archive member.")
        else:
            raise WrongMemberTypeException("Not a long BSD archive member.")

    def write_header(self):
        super(BSDLongMember, self).write_header()
        self.archive.outstream.write(self._filename)

class BSDSymbolTable(ArchiveMember):
    UNSORTED = 0
    SORTED = 1

    format = BSD
    normal = False

    _name_literal = "__.SYMDEF"
    _sorted_suffix = "SORTED"

    def __init__(self, archive, path=None):
        super(BSDSymbolTable, self).__init__(archive, path)
        self.archive.symbols = self

    @property
    def filesize(self):
        if self.filename is None:
            return self._size
        else:
            return self._size - self.namelength
    @filesize.setter
    def filesize(self, value):
        if self.filename is None:
            self._size = value
        else:
            self._size = value + self.namelength

    @property
    def name(self):
        if self.filename is None:
            return self._name_literal
        else:
            return BSDLongMember._name_prefix + str(self.namelength)
    @name.setter
    def name(self, value):
        pass

    def init_from_file(self, path):
        if isinstance(path, int):
            self.sorted = (path == SORTED)
            if self.sorted:
                self.filename = self._name_literal + " " + self._sorted_suffix
                self.namelength = len(self.filename.encode(self.archive.encoding))
            else:
                self.filename = None
                self.namelength = 0

            self.date = int(time.time())
            self.uid = 0
            self.gid = 0
            self.mode = 0o100644
            self.offset = None
        else:
            raise WrongMemberTypeException("Not a BSD symbol table archive member.")

    def set_name_from_archive(self, nameinfo):
        if nameinfo == self._name_literal:
            self.filename = None
            self.namelength = 0
            self.sorted = False
        elif nameinfo.startswith(BSDLongMember._name_prefix):
            self.namelength = int(nameinfo[len(BSDLongMember._name_prefix):])
            self.filename = self.archive.instream.read(self.namelength).decode(self.archive.encoding)
            if not self.filename.startswith(self._name_literal):
                raise WrongMemberTypeException("Not a BSD symbol table archive member.")
            self.sorted = self.filename.endswith(self._sorted_suffix)
        else:
            raise WrongMemberTypeException("Not a BSD symbol table archive member.")

    def write_header(self):
        super(BSDSymbolTable, self).write_header()
        if self.filename is not None:
            self.archive.outstream.write(self.filename.encode(self.archive.encoding))

class Archive(object):
    _magic = b"!<arch>\n"
    _body_pad = b"\n"

    def __init__(self, format=GNU, encoding='utf-8'):
        self.reset()
        self.format = format
        self.encoding = encoding

    def __repr__(self):
        return "<Archive(format={0}, member_count={1})>".format(_FORMATNAMES[self._format], len(self.members))

    @property
    def strings(self):
        if self._strings is None and self.format == GNU:
            GNUStringTable(self, True)
        return self._strings
    @strings.setter
    def strings(self, value):
        self._strings = value

    @property
    def format(self):
        if self._format is None:
            raise InvalidArchiveException("No archive format specified or detected.")
        else:
            return self._format

    @format.setter
    def format(self, value):
        assert value == GNU or value == BSD
        self._format = value

    def reset(self):
        self._format = None
        self.symbols = None
        self._strings = None
        self.members = []
        self.instream = None
        self.outstream = None

    def load(self, filething):
        self.reset()

        log.info("Loading %r", filething)
        if isstring(filething):
            self.instream = open(filething, "rb")
        else:
            self.instream = filething
        assert hasattr(self.instream, "read") and hasattr(self.instream, "tell") and hasattr(self.instream, "seek")

        magic = self.instream.read(len(self._magic))
        if magic != self._magic:
            raise InvalidArchiveException("{0}: invalid magic: '{1}' ({2}) (expected '{3}' ({4}))".format(
                self.instream, magic, len(magic), self._magic, len(self._magic)))

        member = self.read_member()
        while member is not None:
            if member.normal:
                self.members.append(member)
            if self.instream.tell() % 2 == 1:
                padding = self.instream.read(len(self._body_pad))
                if padding != self._body_pad:
                    raise InvalidArchiveException("Source of invalid archive: {0}".format(self.instream))
            member = self.read_member()

        log.info("Loaded %r", self)

    def read_member(self):
        member = None
        for cls in ArchiveMember.derived():
            oldoffset = self.instream.tell()
            log.debug("Starting at offset %u", oldoffset)
            try:
                member = cls(self)
                self.format = member.format
                self.instream.seek(member.offset + member.filesize)
                break
            except WrongMemberTypeException as e:
                log.debug("Wrong type for member: %s", e)
                self.instream.seek(oldoffset)
            except EOFError:
                log.debug("End of file")
                return None
        if member is None:
            raise WrongMemberTypeException("Unknown member type")

        log.info("Read member %r", member)
        return member

    def save(self, filething):
        log.info("Saving %r", filething)
        if isstring(filething):
            self.outstream = open(filething, "wb")
        else:
            self.outstream = filething
        assert hasattr(self.outstream, "write") and hasattr(self.outstream, "tell") and hasattr(self.outstream, "seek")

        self.outstream.write(self._magic)
        self.write_member(self.symbols)
        if self.format == GNU:
            self.write_member(self.strings)
        for m in self.members:
            self.write_member(m)
            if self.outstream.tell() % 2 == 1:
                self.outstream.write(self._body_pad)

        log.info("Saved %r", self)

    @staticmethod
    def write_member(member):
        if member:
            member.write_header()
            member.collect()

    def add(self, filepath):
        member = None
        for cls in ArchiveMember.derived():
            try:
                if cls.format == self.format:
                    member = cls(self, filepath)
                    break
            except WrongMemberTypeException as e:
                log.debug("Wrong type for member: %s", e)
        if member is None:
            raise WrongMemberTypeException("No member type satisfied {0}".format(filepath))

        self.members.append(member)
        log.info("Added member %r", member)

    def __len__(self):
        return len(self.members)

    def __getitem__(self, filename):
        for m in self.members:
            if m.filename == filename:
                return m
        raise KeyError()

    def __iter__(self):
        return iter(self.members)

    def extract_all(self, path):
        for m in self.members:
            m.extract(path)
