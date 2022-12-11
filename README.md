# ORADUMPER - dump Oracle query output to a flat file

The main reason for using ORADUMPER and not SQL\*Plus or another SQL client is **speed**. There is nothing that will be faster than a PRO\*C program. This PRO\*C program is based on [Yet another ASCII dump issue on asktom.oracle.com](https://asktom.oracle.com/pls/apex/f?p=100:11:0::::P11_QUESTION_ID:459020243348).

ORADUMPER is a package containing a C program (`oradumper`) and library
(`liboradumper.la`) to dump the output of Oracle SELECT statements to a flat
file.  For more information see https://github.com/TransferWare/oradumper.

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
$ for d in dbug epc oradumper; do git clone https://github.com/TransferWare/$d; done
$ for d in dbug oradumper; do mkdir $d/build; done
$ cd dbug && ./bootstrap && cd build && ../configure && make check install; cd ~/dev
$ cd oradumper && ./bootstrap && cd build && ../configure && make install USERID=<Oracle connect string>; cd ~/dev
```

The USERID just needs to a valid Oracle connect string, like `scott/tiger@orcl` in the old days.

## Quality checks

ORADUMPER can be checked using the check library
(http://check.sourceforge.net). You may encounter problems on Cygwin (Windows)
because the check library is compiled against the Cygwin run-time DLL and not
against the Windows run-time DLL files.

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

## Usage

Run the `oradumper` program without arguments to get help.

So this command:

```
$ oradumper
```

returns this help:

```
Usage: oradumper [PARAMETER=VALUE]... [BIND VALUE]...

Mandatory parameters:
  query                           Select statement

Optional parameters:
  userid                          Oracle connect string
  fetch_size                      Array size [default = 1000]
  nls_lang                        Set NLS_LANG environment variable
  nls_date_format                 Set NLS date format
  nls_timestamp_format            Set NLS timestamp format
  nls_timestamp_tz_format         Set NLS timestamp timezone format
  nls_numeric_characters          Set NLS numeric characters
  details                         Print details about input and output values (1 = yes) [default = 0]
  record_delimiter                Record delimiter [default = \n]
  feedback                        Give feedback (0 = no feedback) [default = 1]
  column_heading                  Include column names in first line (1 = yes) [default = 1]
  fixed_column_length             Fixed column length: 1 = yes (fixed), 0 = no (variable) [default = 0]
  column_separator                The column separator [default = ,]
  enclosure_string                Put around a column when it has a variable length and contains the column separator [default = "]
  output_file                     The output file
  output_append                   Append to the output file (1 = yes)? [default = 0]
  null                            Value to print for NULL values
  zero_before_decimal_character   Print zero before a number starting with a decimal character (1 = yes) [default = 1]
  left_align_numeric_columns      Left align numeric columns when the column length is fixed (1 = yes) [default = 0]

The values for record_delimiter, column_separator and enclosure_string may contain escaped characters like \n, \012 or \x0a (linefeed).

The first argument which is not recognized as a PARAMETER=VALUE pair, is a bind value.
So abc is a bind value, but abc=xyz is a bind value as well since abc is not a known parameter.
```

## Examples

To get CSV output (standard) for the query `select * from user_tables`:

```
$ oradumper query='select * from user_tables' 1> user_tables.csv
```

or:

```
$ echo <Oracle connect string> | oradumper query='select * from user_tables' output_file=user_tables.csv
```

If you want a fixed column width file with a space as column separator:

```
$ oradumper query='select * from user_tables' output_file=user_tables.txt fixed_column_length=1 column_separator=" " userid=<Oracle connect string>
```

Please note that the userid is asked for if it is not supplied as standard input nor as a command line option.

