whenever sqlerror exit success

alter session set nls_date_format = 'yyyy-mm-dd hh24:mi:ss';
alter session set nls_timestamp_format = 'yyyy-mm-dd hh24:mi:ss.FF';

create table oradumper_test (
"blob" BLOB,
"char" CHAR(2000),
"clob" CLOB,
"date" DATE,
"float" FLOAT,
"interval day(3) to second(0)" INTERVAL DAY(3) TO SECOND(0),
"interval day(3) to second(2)" INTERVAL DAY(3) TO SECOND(2),
"interval day(5) to second(1)" INTERVAL DAY(5) TO SECOND(1),
"interval day(9) to second(6)" INTERVAL DAY(9) TO SECOND(6),
"nchar" NCHAR(1000),
"nclob" NCLOB,
"number" NUMBER,
"nvarchar2" NVARCHAR2(2000),
"raw" RAW(2000),
"timestamp(3)" TIMESTAMP(3),
"timestamp(6)" TIMESTAMP(6),
"timestamp(6) with time zone" TIMESTAMP(6) WITH TIME ZONE,
"varchar2" VARCHAR2(4000)
);

declare
  l_clob constant clob := to_clob(rpad('0123456789', 8000, '0123456789'));
  l_blob blob;
  l_start_date constant timestamp with time zone := timestamp'2004-08-08 17:02:32.212 US/Eastern';
  l_end_date constant timestamp with time zone := timestamp'2004-08-08 19:10:12.235 US/Pacific';

  procedure clob2blob(p_clob in clob, p_blob in out nocopy blob) 
  is
    -- transforming CLOB to BLOB
    l_off_rd number default 1;
    l_amt_rd number default 4096;
    l_off_wr number default 1;
    l_amt_wr number;
    l_str varchar2(4096 char);
  begin
    begin
      loop
        dbms_lob.read( p_clob, l_amt_rd, l_off_rd, l_str );

	l_amt_wr := utl_raw.length ( utl_raw.cast_to_raw( l_str) );
	dbms_lob.write( p_blob, l_amt_wr, l_off_wr, utl_raw.cast_to_raw( l_str ) );

	l_off_wr := l_off_wr + l_amt_wr;

	l_off_rd := l_off_rd + l_amt_rd;
	l_amt_rd := 4096;
      end loop;
    exception
      when no_data_found
      then
        null;
    end;
  end clob2blob;
begin
  dbms_lob.createtemporary(l_blob, true);
  clob2blob(l_clob, l_blob);
  insert into oradumper_test 
  values
  ( l_blob
  , 'abcde'
  , l_clob
  , to_date('2000','yyyy')
  , 100.12345
  , (l_end_date - l_start_date) day(3) to second(0)
  , (l_end_date - l_start_date) day(3) to second(2)
  , (l_end_date - l_start_date) day(5) to second(1)
  , (l_end_date - l_start_date) day(9) to second(6)
  , N'abcde'
  , TO_NCLOB(l_clob)
  , 1000.123
  , N'ddghghd'
  , utl_raw.cast_to_raw('This is a raw string')
  , l_start_date
  , l_end_date
  , l_start_date
  , 'dhgdsh'
  );
  commit;
end;
/
