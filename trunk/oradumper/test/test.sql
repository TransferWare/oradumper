whenever sqlerror continue
drop table oradumper_test;

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
"rowid" ROWID,
"timestamp(3)" TIMESTAMP(3),
"timestamp(6)" TIMESTAMP(6),
"timestamp(6) with time zone" TIMESTAMP(6) WITH TIME ZONE,
"varchar2" VARCHAR2(4000)
);

insert into oradumper_test 
values
(
  null,
  'abcde',
  to_clob('abcdeabcde'),
  to_date('2000','yyyy'),
  100,
  null,
  null,
  null,
  null,
  N'abcde',
  null,
  1000,
  N'ddghghd',
  null,
  null,
  null,
  null,
  null,
  'dhgdsh'
);

commit;

set document on

>oradumper
|   >sql_connect
|   |   output: return: 0
|   <sql_connect
|   >sql_execute_immediate
|   |   input: statement: ALTER SESSION SET NLS_DATE_FORMAT = 'yyyy-mm-dd hh24:mi:ss'
|   |   output: return: 0
|   <sql_execute_immediate
|   >sql_execute_immediate
|   |   input: statement: ALTER SESSION SET NLS_TIMESTAMP_FORMAT = 'yyyy-mm-dd hh24:mi:ss'
|   |   output: return: 0
|   <sql_execute_immediate
|   >sql_execute_immediate
|   |   input: statement: ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '.,'
|   |   output: return: 0
|   <sql_execute_immediate
|   >sql_allocate_descriptors
|   |   input: max_array_size: 10
|   |   output: return: 0
|   <sql_allocate_descriptors
|   >sql_parse
|   |   input: select_statement: select * from oradumper_test
|   |   output: return: 0
|   <sql_parse
|   >sql_bind_variable_count
|   |   output: count: 0; return: 0
|   <sql_bind_variable_count
|   >sql_open_cursor
|   |   output: return: 0
|   <sql_open_cursor
|   >sql_column_count
|   |   output: count: 19; return: 0
|   <sql_column_count
|   >sql_describe_column
|   |   input: nr: 1; size: 31
|   |   output: name: blob; type: -113; length: 4000; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 1; name: blob; type: -113; length: 4000; precision: 0; scale: 0; character set: 
|   info: column_value.data[0][0]= 0x514008
|   >sql_define_column
|   |   input: nr: 1; type: 12; length: 4003; array_size: 10; data: 0x514008; ind: 0x17a28
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 2; size: 31
|   |   output: name: char; type: 1; length: 2000; precision: 0; scale: 0; character set: WE8ISO8859P1; return: 0
|   <sql_describe_column
|   info: column: 2; name: char; type: 1; length: 2000; precision: 0; scale: 0; character set: WE8ISO8859P1
|   info: column_value.data[1][0]= 0x51e008
|   >sql_define_column
|   |   input: nr: 2; type: 12; length: 2003; array_size: 10; data: 0x51e008; ind: 0x17cc8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 3; size: 31
|   |   output: name: clob; type: -112; length: 4000; precision: 0; scale: 0; character set: WE8ISO8859P1; return: 0
|   <sql_describe_column
|   info: column: 3; name: clob; type: -112; length: 4000; precision: 0; scale: 0; character set: WE8ISO8859P1
|   info: column_value.data[2][0]= 0x523008
|   >sql_define_column
|   |   input: nr: 3; type: 12; length: 4003; array_size: 10; data: 0x523008; ind: 0x17ca8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 4; size: 31
|   |   output: name: date; type: 9; length: 7; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 4; name: date; type: 9; length: 7; precision: 0; scale: 0; character set: 
|   info: column_value.data[3][0]= 0x4c9608
|   >sql_define_column
|   |   input: nr: 4; type: 12; length: 27; array_size: 10; data: 0x4c9608; ind: 0x17a48
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 5; size: 31
|   |   output: name: float; type: 2; length: 126; precision: 126; scale: -127; character set: ; return: 0
|   <sql_describe_column
|   info: column: 5; name: float; type: 2; length: 126; precision: 126; scale: -127; character set: 
|   info: column_value.data[4][0]= 0x4c9008
|   >sql_define_column
|   |   input: nr: 5; type: 12; length: 47; array_size: 10; data: 0x4c9008; ind: 0x17a68
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 6; size: 31
|   |   output: name: interval day(3) to second(0); type: 10; length: 11; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 6; name: interval day(3) to second(0); type: 10; length: 11; precision: 0; scale: 0; character set: 
|   info: column_value.data[5][0]= 0x2b4d08
|   >sql_define_column
|   |   input: nr: 6; type: 12; length: 11; array_size: 10; data: 0x2b4d08; ind: 0x17b08
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 7; size: 31
|   |   output: name: interval day(3) to second(2); type: 10; length: 11; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 7; name: interval day(3) to second(2); type: 10; length: 11; precision: 0; scale: 0; character set: 
|   info: column_value.data[6][0]= 0x2b4c08
|   >sql_define_column
|   |   input: nr: 7; type: 12; length: 11; array_size: 10; data: 0x2b4c08; ind: 0x17aa8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 8; size: 31
|   |   output: name: interval day(5) to second(1); type: 10; length: 11; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 8; name: interval day(5) to second(1); type: 10; length: 11; precision: 0; scale: 0; character set: 
|   info: column_value.data[7][0]= 0x4e2508
|   >sql_define_column
|   |   input: nr: 8; type: 12; length: 11; array_size: 10; data: 0x4e2508; ind: 0x17ae8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 9; size: 31
|   |   output: name: interval day(9) to second(6); type: 10; length: 11; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 9; name: interval day(9) to second(6); type: 10; length: 11; precision: 0; scale: 0; character set: 
|   info: column_value.data[8][0]= 0x4e2608
|   >sql_define_column
|   |   input: nr: 9; type: 12; length: 11; array_size: 10; data: 0x4e2608; ind: 0x17c68
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 10; size: 31
|   |   output: name: nchar; type: 1; length: 2000; precision: 0; scale: 0; character set: AL16UTF16; return: 0
|   <sql_describe_column
|   info: column: 10; name: nchar; type: 1; length: 2000; precision: 0; scale: 0; character set: AL16UTF16
|   info: column_value.data[9][0]= 0x52d008
|   >sql_define_column
|   |   input: nr: 10; type: 12; length: 2003; array_size: 10; data: 0x52d008; ind: 0x17a88
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 11; size: 31
|   |   output: name: nclob; type: -112; length: 4000; precision: 0; scale: 0; character set: AL16UTF16; return: 0
|   <sql_describe_column
|   info: column: 11; name: nclob; type: -112; length: 4000; precision: 0; scale: 0; character set: AL16UTF16
|   info: column_value.data[10][0]= 0x532008
|   >sql_define_column
|   |   input: nr: 11; type: 12; length: 4003; array_size: 10; data: 0x532008; ind: 0x17a08
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 12; size: 31
|   |   output: name: number; type: 2; length: 0; precision: 0; scale: -127; character set: ; return: 0
|   <sql_describe_column
|   info: column: 12; name: number; type: 2; length: 0; precision: 0; scale: -127; character set: 
|   info: column_value.data[11][0]= 0x53ce08
|   >sql_define_column
|   |   input: nr: 12; type: 12; length: 47; array_size: 10; data: 0x53ce08; ind: 0x179e8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 13; size: 31
|   |   output: name: nvarchar2; type: 12; length: 4000; precision: 0; scale: 0; character set: AL16UTF16; return: 0
|   <sql_describe_column
|   info: column: 13; name: nvarchar2; type: 12; length: 4000; precision: 0; scale: 0; character set: AL16UTF16
|   info: column_value.data[12][0]= 0x53e008
|   >sql_define_column
|   |   input: nr: 13; type: 12; length: 4003; array_size: 10; data: 0x53e008; ind: 0x179c8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 14; size: 31
|   |   output: name: raw; type: -23; length: 2000; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 14; name: raw; type: -23; length: 2000; precision: 0; scale: 0; character set: 
|   info: column_value.data[13][0]= 0x548008
|   >sql_define_column
|   |   input: nr: 14; type: 12; length: 2003; array_size: 10; data: 0x548008; ind: 0x179a8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 15; size: 31
|   |   output: name: rowid; type: -104; length: 4; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 15; name: rowid; type: -104; length: 4; precision: 0; scale: 0; character set: 
|   info: column_value.data[14][0]= 0x16188
|   >sql_define_column
|   |   input: nr: 15; type: 12; length: 7; array_size: 10; data: 0x16188; ind: 0x17348
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 16; size: 31
|   |   output: name: timestamp(3); type: 9; length: 11; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 16; name: timestamp(3); type: 9; length: 11; precision: 0; scale: 0; character set: 
|   info: column_value.data[15][0]= 0x53cc08
|   >sql_define_column
|   |   input: nr: 16; type: 12; length: 27; array_size: 10; data: 0x53cc08; ind: 0x173a8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 17; size: 31
|   |   output: name: timestamp(6); type: 9; length: 11; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 17; name: timestamp(6); type: 9; length: 11; precision: 0; scale: 0; character set: 
|   info: column_value.data[16][0]= 0x53ca08
|   >sql_define_column
|   |   input: nr: 17; type: 12; length: 27; array_size: 10; data: 0x53ca08; ind: 0x173c8
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 18; size: 31
|   |   output: name: timestamp(6) with time zone; type: 9; length: 13; precision: 0; scale: 0; character set: ; return: 0
|   <sql_describe_column
|   info: column: 18; name: timestamp(6) with time zone; type: 9; length: 13; precision: 0; scale: 0; character set: 
|   info: column_value.data[17][0]= 0x53c808
|   >sql_define_column
|   |   input: nr: 18; type: 12; length: 27; array_size: 10; data: 0x53c808; ind: 0x17388
|   <sql_define_column
|   >sql_describe_column
|   |   input: nr: 19; size: 31
|   |   output: name: varchar2; type: 12; length: 4000; precision: 0; scale: 0; character set: WE8ISO8859P1; return: 0
|   <sql_describe_column
|   info: column: 19; name: varchar2; type: 12; length: 4000; precision: 0; scale: 0; character set: WE8ISO8859P1
|   info: column_value.data[18][0]= 0x54d008
|   >sql_define_column
|   |   input: nr: 19; type: 12; length: 4003; array_size: 10; data: 0x54d008; ind: 0x173e8
|   <sql_define_column
|   >sql_fetch_rows
|   |   input: array_size: 10
|   |   output: count: 0; return: -932
|   <sql_fetch_rows
|   >sql_close_cursor
|   |   output: return: 0
|   <sql_close_cursor
|   >sql_deallocate_descriptors
|   |   output: return: 0
|   <sql_deallocate_descriptors
<oradumper

#