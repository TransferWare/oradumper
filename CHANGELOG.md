# Changelog

Copyright (C) 2008-2022 G.J. Paulissen 


All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Types of changes:
- *Added* for new features.
- *Changed* for changes in existing functionality.
- *Deprecated* for soon-to-be removed features.
- *Removed* for now removed features.
- *Fixed* for any bug fixes.
- *Security* in case of vulnerabilities.

Please see the [ORADUMPER issue queue](https://github.com/TransferWare/oradumper/issues) for issues.

## [1.4.0] - 2022-11-07

### Fixed

* [The implementation of enclosing CSV columns is not correct.](https://github.com/TransferWare/oradumper/issues/2)

## [1.3.0] - 2022-08-11

### Fixed

* [Solve Mac build errors.](https://github.com/TransferWare/oradumper/issues/1)

## [1.2.0] - 2018-12-04

* Enabling building for gcc 4 and higher on Windows (Cygwin).

## [1.1.0] - 2014-08-11

* row count output argument added to oradumper procedure.
* Added nls_timestamp_tz_format parameter.
* testing enhanced.
* INSTALL enhanced.
* README includes hints for AIX compilation.
* src/sql/README enhanced.
* added make lint functionality.
* changed nls_language command line option into nls_lang option.
* added zero_before_decimal_character command line option.
* added left_align_numeric_columns command line option.
* numeric fields depend on database type and not the alignment.

## [1.0.1] - 2009-11-23

* check.m4 is searched for in $HOME or its subdirectories too.
* AIX specific build details added to README.

## [1.0.0] - 2008-01-01

For historical reasons, this is when I first started development for ORADUMPER.
