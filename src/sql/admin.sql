/*
    The oradumper dumps Oracle queries.
    Copyright (C) 2008  G.J. Paulissen, Transfer Solutions b.v., Leerdam, Netherlands

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
PROMPT
PROMPT  $HeadURL$
REMARK 
REMARK  $Date$
REMARK
REMARK  $Author$
REMARK
REMARK  $Revision$
REMARK
REMARK  Description:    Create the library oradumper_library 
REMARK                  in schema &oradumper_library_owner.
REMARK
REMARK  Notes:		If &oradumper_library_owner is not the current user 
REMARK			it must be a user with the CREATE ANY LIBRARY 
REMARK 			privilege (for instance a DBA).
REMARK

column oradumper_library_owner new_value oradumper_library_owner
column oradumper_library new_value oradumper_library

set termout off

select  user as oradumper_library_owner
,       '/usr/local/lib/liboradumper.so' as oradumper_library
from    dual
/

set termout on

accept oradumper_library_owner -
prompt "Owner of the oradumper library ? [&&oradumper_library_owner] " -
default "&&oradumper_library_owner"

accept oradumper_library -
prompt "File name of the oradumper shared library ? [&&oradumper_library] " -
default "&&oradumper_library"

create or replace library &&oradumper_library_owner..oradumper_library as '&&oradumper_library'
/

column oradumper_library_owner clear
column oradumper_library clear

undefine oradumper_library_owner oradumper_library
