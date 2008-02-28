create or replace procedure oradumper(coll in sys.odcivarchar2list)
as
language C name "oradumper_extproc" library &oradumper_library_owner..oradumper_library
with context parameters
( CONTEXT,
  coll OCIColl,
  coll INDICATOR short
);
/

show errors
