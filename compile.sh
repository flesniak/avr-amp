avr-gcc -DF_CPU=8000000 -Os -mmcu=atmega8 -o amp amp.c && avr-objcopy -O ihex amp amp.hex && avrdude -p m8 -c avrusb500 -eU flash:w:amp.hex
