# Endurance_EEPROM
a try implement an endurance way to save log data in eeprom, going to use it for 24LC64 
i have no experience with EEPROM but this is how i attmept to make a library that should have wear leveling and safe to write logging into eeprom. 

its not file system so just page writing data with pointer of the data.. :)

i think a good library is written by [nasa EEFS](https://github.com/nasa/eefs)?
but i think its not for what im planning for...

## Links to read:

1- [AVR101: High Endurance EEPROM Storage](http://ww1.microchip.com/downloads/en/AppNotes/doc2526.pdf)\
2- [EEPROM Reliability and Wear Leveling](http://www.mosaic-industries.com/embedded-systems/sbc-single-board-computers/freescale-hcs12-9s12-c-language/instrument-control/eeprom-lifetime-reliability-wear-leveling)\
3- [stack-exchange wear leveling](https://electronics.stackexchange.com/questions/60342/wear-leveling-on-a-microcontrollers-eeprom)\




### [24LC64](http://ww1.microchip.com/downloads/en/DeviceDoc/21189T.pdf) specs:
*  64Kbit = 8Kbyte = 250 Write Page
*  Page Write Time 5 ms, max.
*  32-Byte Page Write Buffer
*  More than 1 Million Erase/Write Cycles
*  I2C address: 0x50


## methodology:
```
[                      EEPROM Available Space is 8KB                                  ]
[Page 1 32 byte][Page 2 32 byte][                                     ][Page n 32 byte]
[struct 1 log page n bytes][struct 2 log page n bytes][    ][struct n log page n bytes]
```

the user divide the eeprom into struct pages which will have the o-ring written into it.

### what is ```[struct log page]```:

user struct of data should be written to this page. its like dividing the eeprom to different areas of log. maybe you will have data that is written every 10 minute and data that is written every 2 days. its waste to have them written both in the struct. so you define 2 log pages one for slow update rate and the second for the fast update rate.
the fast update should have larger bytes as you want to have better endurance.
writing struct is going to be consisting of:
- counter unsigned integer: what is the last struct written.
- CRC: checksum to prevent fail data and restore last correct written log.
      
#### example:
- a 23 byte struct
- the user assign it to have full eeprom size (8K)
- \+ 1 byte for crc8
with 8K/24 byte data, we can have 333 data before overriding old data...
so our counting integer should have bandwidth of counting to 334. 
- hence we need [9bits] for counter [2 bytes]
- final struct size = 26 bytes.



    
#### eeprom data example:
* what is the header of the data.\
`header: [counting byte][crc8]`

* how the eeprom should look like:\
` [[ 1 ][crc8][struct 23 byte]] [[ 2 ][crc8][struct 23 byte]] ... [[333][crc8][struct 23 byte]] `

* after adding 1 more data:\
` [[334][crc8][struct 23 byte]] [[ 2 ][crc8][struct 23 byte]] ... [[333][crc8][struct 23 byte]] `

* add 1 more data:\
` [[334][crc8][struct 23 byte]] [[ 1 ][crc8][struct 23 byte]] [[ 3 ][crc8][struct 23 byte]] ... `

#### detect last data:
the last data added is actually simple (just the biggest value), although my algorithim shouldnt check like this:\
` if (counter_of_struct[next] > counter_of_struct[current])`\
because then it will always find the [334] which is obviously not the last value.\
it should check like the following:\
` if (counter_of_stuct[next] > value_of_current_counter+1) `
