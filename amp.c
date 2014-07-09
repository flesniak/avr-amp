#include <avr/io.h>
#include <avr/sleep.h>
#include <stdbool.h>
#include <stdint.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define BLINKDELAY 28

//INPUT-Makros
#define INPUT_DISC       (PINC >> 0 & 0x01)
#define INPUT_CD         (PINC >> 1 & 0x01)
#define INPUT_TUN        (PINC >> 2 & 0x01)
#define INPUT_AUX        (PINC >> 3 & 0x01)
#define INPUT_CPAB       (PINC >> 4 & 0x01)
#define INPUT_CPBA       (PINC >> 5 & 0x01)
#define INPUT_TAPEA      (PIND >> 0 & 0x01)
#define INPUT_TAPEB      (PIND >> 1 & 0x01)
//OUTPUT-Makros
#define OUTPUT_DISC(x)   (PORTD = x ? PORTD | 0x01 << 2 : PORTD & ~(0x01 << 2) ) //0
#define OUTPUT_CD(x)     (PORTD = x ? PORTD | 0x01 << 3 : PORTD & ~(0x01 << 3) ) //1
#define OUTPUT_TUN(x)    (PORTD = x ? PORTD | 0x01 << 4 : PORTD & ~(0x01 << 4) ) //2
#define OUTPUT_AUX(x)    (PORTD = x ? PORTD | 0x01 << 5 : PORTD & ~(0x01 << 5) ) //3
#define OUTPUT_CPAB      (PORTD = PORTD ^ (0x01 << 6)) //4
#define OUTPUT_CPBA      (PORTD = PORTD ^ (0x01 << 7)) //5
#define OUTPUT_TAPEA(x)  (PORTB = x ? PORTB | 0x01 << 0 : PORTB & ~(0x01 << 0) ) //6
#define OUTPUT_TAPEB(x)  (PORTB = x ? PORTB | 0x01 << 1 : PORTB & ~(0x01 << 1) ) //7
#define OUTPUT_MASTER(x) (PORTB = x ? PORTB | 0x01 << 2 : PORTB & ~(0x01 << 2) )
#define OUTPUT_LED(x)    (PORTB = x ? PORTB | 0x01 << 3 : PORTB & ~(0x01 << 3) )

static char lastoutput = 0;
static uint8_t savetime = 0;
static bool save = 0;

//Kopierfunktion
void setcopy(char pressed)
{
if(pressed) {
  if(PORTD >> 6 & 0x01) //copy-ab ggf abschalten
    OUTPUT_CPAB;
  OUTPUT_CPBA; //copy umschalten
  }
else {
  if(PORTD >> 7 & 0x01) //copy-ba ggf abschalten
    OUTPUT_CPBA;
  OUTPUT_CPAB; //copy umschalten
  }
}

void toggle(char output, bool value)
{
switch(output)
  {
  case 0 : OUTPUT_DISC(value);
           break;
  case 1 : OUTPUT_CD(value);
           break;
  case 2 : OUTPUT_TUN(value);
           break;
  case 3 : OUTPUT_AUX(value);
           break;
  case 4 : setcopy(0);
           break;
  case 5 : setcopy(1);
           break;
  case 6 : OUTPUT_TAPEA(value);
           break;
  case 7 : OUTPUT_TAPEB(value);
           break;
  }
}

void setoutput(char output)
{
if(output==4 | output==5) //copy-funktion gesondert behandeln
  toggle(output,1);
else
  if(output != lastoutput) //nur ändern wenn anderer als aktuell aktiviert
    {
    OUTPUT_MASTER(1);
    toggle(lastoutput,0);
    toggle(output,1);
    OUTPUT_MASTER(0);
    lastoutput=output;
    save=true;
    savetime=0;
    }
}

ISR(TIMER1_COMPA_vect) //timer interupt
{
if(!INPUT_DISC)
  setoutput(0);
if(!INPUT_CD)
  setoutput(1);
if(!INPUT_TUN)
  setoutput(2);
if(!INPUT_AUX)
  setoutput(3);
if(!INPUT_CPAB)
  setoutput(4);
if(!INPUT_CPBA)
  setoutput(5);
if(!INPUT_TAPEA)
  setoutput(6);
if(!INPUT_TAPEB)
  setoutput(7);
//speicherfunktion
if(save)
  {
  if(savetime > 200) //200 = ~10 sekunden
    {
    //EEPROM save
    eeprom_write_byte(0,lastoutput);
    save=false;
    }
  else
    savetime++;
  }
//warten bis taste losgelassen ist, v.a. nütlich für copy-toggle
while(!(INPUT_DISC & INPUT_CD & INPUT_TUN & INPUT_AUX & INPUT_CPAB & INPUT_CPBA & INPUT_TAPEA & INPUT_TAPEB));
//timer von vorne anlaufen lassen
TCNT1 = 0;
}

void blink()
{
char channel;
for(channel=2;channel<=9;channel++)
  {
  if(channel>7)
    {
    PORTB |= (0x01 << (channel-8));
    _delay_ms(BLINKDELAY);
    PORTB &= !(0x01 << (channel-8));
    }
  else
    {
    PORTD |= (0x01 << channel);
    _delay_ms(BLINKDELAY);
    PORTD &= !(0x01 << channel);
    }
  }
for(channel=8;channel>=0;channel--)
  {
  if(channel>7)
    {
    PORTB |= (0x01 << (channel-8));
    _delay_ms(BLINKDELAY);
    PORTB &= !(0x01 << (channel-8));
    }
  else
    {
    PORTD |= (0x01 << channel);
    _delay_ms(BLINKDELAY);
    PORTD &= !(0x01 << channel);
    }
  }
_delay_ms(BLINKDELAY);
}

int main()
{
char resetoutput;
//Init IO's
DDRD = 0xFC;
DDRC &= 0x60;
DDRB |= 0x0F;

OUTPUT_MASTER(1);

//Init LED ;) - abgeschalten, sonst programmierproblem
//OUTPUT_LED(1);

//welcome ;)
//blink();

//Init Timer
TCCR1A = 0;
TCCR1B |= 5;
TIMSK |= 0x01 << OCIE1A;
OCR1A = 240;

//Set interrupts
sei();

//Restore last channel
lastoutput=8;
resetoutput=eeprom_read_byte(0);
if(resetoutput > 7 | resetoutput < 0)
  resetoutput=0;
setoutput(resetoutput);

//eigentlich default
set_sleep_mode(SLEEP_MODE_IDLE);

//Main Loop
for(;;)
 sleep_mode();

return(0);
}
