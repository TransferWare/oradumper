REMARK Must be run by a DBA

create or replace library oradumper_library as '&oradumper_library'
/

grant execute on oradumper_library to &grantee
/
