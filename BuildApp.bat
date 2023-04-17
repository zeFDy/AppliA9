@echo off
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"

set "datestamp=%DD%/%MM%/%YYYY%" & set "timestamp=%HH%:%Min%:%Sec%" & set "fullstamp=%DD%/%MM%/%YYYY% %HH%:%Min%:%Sec%"
echo #define OUR_DATE           "%datestamp%" >  ourDateTime.h
echo #define OUR_TIME           "%timestamp%" >> ourDateTime.h
echo #define OUR_DATE_TIME_FULL "%fullstamp%" >> ourDateTime.h

echo #define OUR_DATE_TIME      "%DATE% %TIME:~0,8%" >> ourDateTime.h
make SdRamExec.o