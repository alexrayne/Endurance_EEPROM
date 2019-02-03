# Endurance_EEPROM
a try implement an endurance way to save log data in eeprom, going to use it for 24LC64 

## Methodology:
http://ww1.microchip.com/downloads/en/AppNotes/doc2526.pdf

i have no experience with EEPROM but this is how i attmept to make a library that should have wear leveling and safe to write logging into eeprom. 

its not file system so just page writing data with pointer of the data.. :)

i think a good library is written by nasa EEFS? i guess .. 


`24LC64`
•  Page Write Time 5 ms, max.
•  32-Byte Page Write Buffer
•  More than 1 Million Erase/Write Cycles
•  I2C address: 0x50
