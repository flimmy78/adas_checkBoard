@echo  .
@echo  .  
@echo ************************************************
@echo       Convert file for download and update 
@echo ************************************************

D:\Keil_v5\ARM\ARMCC\bin\fromelf.exe --bin -o tester.bin .\objects\tester.axf
Addbin.exe tester.bin mcu_test_code.bin 

@echo   .
@echo   .
@echo *************************************************
@echo       Convert finished !        
@echo *************************************************