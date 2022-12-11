# ORADUMPER - dump Oracle query output to a flat file

This is ORADUMPER, a package containing a C program (`oradumper`) and library
(`liboradumper.la`) to dump Oracle SELECT statements to a flat file.  For more
information see https://github.com/TransferWare/oradumper.

Besides this, an Oracle external procedure can be built upon the library
when the configure option `--enable-extproc` has been specified. See
`src/sql/README` for details.

This package itself depends on the following packages:
1. [DBUG](https://github.com/TransferWare/dbug)
2. [EPC](https://github.com/TransferWare/epc)

## Installation

Clone both the DBUG and EPC repository to the same root (e.g. ~/dev) and then clone ORADUMPER.

Now just issue an INSTALL FROM SOURCE of DBUG (i.e. skip DATABASE INSTALL), see the [DBUG README](https://github.com/TransferWare/dbug#install-from-source).

You do not need to install EPC since only some EPC source files are needed to build ORADUMPER.

Installation using the GNU build tools is described in file `INSTALL`.

In order to separate build artifacts and source artifacts I usually create a
`build` directory and configure and make it from there:

The whole installation process:

```
$ mkdir ~/dev
$ cd ~/dev
$ for d in dbug epc oradumper; do git clone https://github.com/TransferWare/$d; mkdir $d/build; done
$ cd dbug && ./bootstrap && cd build && ../configure && make check install; cd ~/dev
$ cd oradumper && ./bootstrap && cd build && ../configure && make install USERID=<Oracle connect string>; cd ~/dev
```

The USERID just needs to a valid Oracle connect string, like `scott/tiger@orcl` in the old days.

## Quality checks

ORADUMPER can be checked using the check library
(http://check.sourceforge.net). You may encounter problems on Cygwin (Windows)
because the check library is compiled against the Cygwin run-time DLL and not
against the Windows run-time DLLs.

The following may be tested depending on configure options:
- full code coverage using the GNU code coverage program gcov.
- memory leaks or corruption using dmalloc (only for the oradumper
  library, but not the Oracle libraries).
- lint checking in `src/lib` using splint (http://www.splint.org).

The following configure command enables code coverage and checking memory:

```
$ configure --enable-gcov --with-dmalloc
```

Then:

```
$ make check USERID=<Oracle connect string>
```

## Documentation

Run the `oradumper` program without arguments to get help.

