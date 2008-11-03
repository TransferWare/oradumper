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
REMARK  Description:    Test the procedure oradumper.
REMARK
REMARK  Notes:		See the oradumper.sql for creating the procedure.
REMARK

variable row_count number

PROMPT Dump all user objects to file &&path/user_objects1.lis
execute oradumper(sys.odcivarchar2list('query=select * from user_objects', 'output_file=&&path/user_objects1.lis'), :row_count)

print

PROMPT A query is mandatory
execute oradumper(sys.odcivarchar2list('dbug_options="d,g,t,o=&&path/dbug.log"'), :row_count)

print

PROMPT An invalid query raises an exception
execute oradumper(sys.odcivarchar2list('query=xxx', 'dbug_options="d,g,t,o=&&path/dbug.log"'), :row_count)

print

PROMPT Empty arguments are skipped
PROMPT Dump all user objects to file &&path/user_objects2.lis
execute oradumper(sys.odcivarchar2list(NULL, 'query=select * from user_objects', 'output_file=&&path/user_objects2.lis'), :row_count)

print

