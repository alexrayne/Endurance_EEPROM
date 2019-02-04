# Endurance_EEPROM
a try implement an endurance way to save log data in eeprom, going to use it for 24LC64 
i have no experience with EEPROM but this is how i attmept to make a library that should have wear leveling and safe to write logging into eeprom. 

its not a file system :)

i think a good EEPROM fs library is written by [nasa EEFS](https://github.com/nasa/eefs)?


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
[struct 1 slot n bytes][struct 2 slot n bytes][       ...      ][struct n slot n bytes]
```

the user divide the eeprom into struct pages which will have the o-ring written into it.

### what is `[struct slot]`:

its like dividing the eeprom to different areas of log. maybe you will have struct data that is written every 10 minute and another that is written every 2 days. its waste to have write them both in the same struct. so we define 2 slots one for slow update rate struct and the other for the fast update rate.
the slot that contains high rate of writes should be larger to increase the indurance of the EEPROM.

remember, this is not variable updating, all the data are log, and you can access last n structs.
defining slot size depending on:
- how many history structs you want to keep in the slot.
- how much is the size of the struct.
- how frequently you are inserting new struct to the slot.

writing struct is going to be consisting of additional bytes by the library:
- counter unsigned integer: this to identify the last struct inserted in the slot.
- CRC: checksum to prevent fail data and restore last correct written struct.
      
#### example:
- a 23 byte struct
- the user assign a slot to have full eeprom size (8K)
- \+ 1 byte for crc8
- \+ n byte for counter
with 8K/25 byte data, we can have less than 320 history structs in the slot.
so our counting integer should have bandwidth of counting to 321. 
- hence we need [9bits] for counter or [2 bytes]
- total struct size = 26 bytes, counting = 308

    
#### eeprom data example:
* what is the header of the data.\
`header: [counting byte][crc8]`

* how the eeprom should look like:\
` [[ 1 ][crc8][struct 23 byte]] [[ 2 ][crc8][struct 23 byte]] ... [[307][crc8][struct 23 byte]] `

* after adding 1 more data:\
` [[308][crc8][struct 23 byte]] [[ 2 ][crc8][struct 23 byte]] ... [[307][crc8][struct 23 byte]] `

* add 1 more data:\
` [[308][crc8][struct 23 byte]] [[ 1 ][crc8][struct 23 byte]] [[ 3 ][crc8][struct 23 byte]] ... `

#### detect last data:
the last data added is actually simple (just the biggest value), although my algorithim shouldnt check like this:\
` if (counter_of_struct[next] > counter_of_struct[current])`\
because then it will always find the [308] which is obviously not the last value.\
it should check like the following:\
` if (counter_of_stuct[next] > value_of_current_counter+1) `
