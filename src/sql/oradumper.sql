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
REMARK  Description:    Create the procedure oradumper.
REMARK
REMARK  Notes:		See the admin.sql for creating the library oradumper_library.
REMARK

create or replace 
procedure oradumper(coll in sys.odcivarchar2list, row_count out pls_integer)
as
language C name "oradumper_extproc" library oradumper_library
with context parameters
( CONTEXT,
  coll OCIColl,
  coll INDICATOR short,
  row_count unsigned int
);
/

show errors
