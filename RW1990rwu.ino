//A utility for read and write of RW1990.
//Some code derived from OpenLog
//Owen Duffy 20190905
//depends on enhanced OneWIre lib: https://github.com/owenduffy/OneWire

#define BEEPPIN 10
#define OWPIN 11
#define GLEDPIN 12
#define RLEDPIN 13
#define OFF 0
#define GREEN 1
#define RED 2
 
#include <OneWire.h>
#include <stdlib.h>

#if defined(ARDUINO_AVR_NANO)||(ARDUINO_AVR_PRO)||(ARDUINO_AVR_UNO)
#define CONSOLEPORT Serial
#endif
#if defined(ARDUINO_AVR_LEONARDO)||(ARDUINO_AVR_PROMICRO)
#define CONSOLEPORT Serial
#define CONSOLEPORTI if(Serial) Serial
#endif
#if defined(ARDUINO_SAMD_ZERO)
#include <avr/dtostrf.h>
#define CONSOLEPORT SerialUSB
#define CONSOLEPORTI if(SerialUSB) SerialUSB
#endif
#define BAUDRATE 115200

OneWire rw1990 (OWPIN,20);
byte id[8]; //array to store the Ibutton ID.
byte id2[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void printid(byte id[]){
  char line[80];
  byte crc;
  crc = rw1990.crc8(id, 7);
  sprintf(line,"ID: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X CRC: %02X (%s)\n",id[0],id[1],id[2],id[3],id[4],id[5],id[6],id[7],crc,crc==id[7]?"Good CRC":"Bad CRC");
  led(crc==id[7]?GREEN:RED);
  CONSOLEPORT.print(line);
}

void printcrc(byte id[]){
  char line[80];
  byte crc;
  crc = rw1990.crc8(id, 7);
  if(crc==id[7]){
    CONSOLEPORT.print(F("Good CRC: "));
  }
  else{
    CONSOLEPORT.print(F("Bad CRC: "));
  }
  sprintf(line,"id[7]=%02X,crc=%02X\n",id[7],crc);
  CONSOLEPORT.print(line);
}

#define MAX_COUNT_COMMAND_LINE_ARGS 2
struct commandArg{
  char* arg; //Points to first character in command line argument
  byte arg_length; //Length of command line argument
};
static struct commandArg cmd_arg[MAX_COUNT_COMMAND_LINE_ARGS];
char general_buffer[60]; //Needed for command shell
#define MIN(a,b) ((a)<(b))?(a):(b)

char* getCmdArg(byte index){
  memset(general_buffer, 0, sizeof(general_buffer));
  if (index < MAX_COUNT_COMMAND_LINE_ARGS)
    if ((cmd_arg[index].arg != 0) && (cmd_arg[index].arg_length > 0))
      return strncpy(general_buffer, cmd_arg[index].arg, MIN(sizeof(general_buffer), cmd_arg[index].arg_length));
  return 0;
}

//Reads a line until the \n enter character is found
byte readLine(char* buffer, byte bufferLength){
  memset(buffer, 0, bufferLength); //Clear buffer

  byte readLength = 0;
  while (readLength < bufferLength - 1) {
    while (!CONSOLEPORT.available());
    byte c = CONSOLEPORT.read();

    if (c == 0x08 || c == 0x7f) { //Backspace characters
      if (readLength < 1)
        continue;
      --readLength;
      buffer[readLength] = '\0'; //Put a terminator on the string in case we are finished

      CONSOLEPORT.print((char)0x08); //Move back one space
      CONSOLEPORT.print(F(" ")); //Put a blank there to erase the letter from the terminal
      CONSOLEPORT.print((char)0x08); //Move back again
      continue;
    }
    // Only echo back if this is enabled
    CONSOLEPORT.print((char)c);
    if (c == '\n') {
      CONSOLEPORT.println();
      buffer[readLength] = '\0';
      break;
    }
    else if (c == '\r') {
    }
    else {
      buffer[readLength] = c;
      ++readLength;
    }
  }

  //Split the command line into arguments
  splitCmdLineArgs(buffer, bufferLength);
  return readLength;
}

//Safe adding of command line arguments
void addCmdArg(char* buffer, byte bufferLength){
  byte count = countCmdArgs();
  if (count < MAX_COUNT_COMMAND_LINE_ARGS)
  {
    cmd_arg[count].arg = buffer;
    cmd_arg[count].arg_length = bufferLength;
  }
}

//Returns the number of command line arguments
byte countCmdArgs(void)
{
  byte count = 0;
  byte i = 0;
  for (; i < MAX_COUNT_COMMAND_LINE_ARGS; i++)
    if ((cmd_arg[i].arg != 0) && (cmd_arg[i].arg_length > 0))
      count++;
  return count;
}

//Split the command line arguments
byte splitCmdLineArgs(char* buffer, byte bufferLength){
  byte arg_index_start = 0;
  byte arg_index_end = 1;

  //Reset command line arguments
  memset(cmd_arg, 0, sizeof(cmd_arg));
  //Split the command line arguments
  while (arg_index_end < bufferLength)
  {
    //Search for ASCII 32 (Space)
    if ((buffer[arg_index_end] == ' ') || (arg_index_end + 1 == bufferLength))
    {
      //Fix for last character
      if (arg_index_end + 1 == bufferLength)
        arg_index_end = bufferLength;
      //Add this command line argument to the list
      addCmdArg(&(buffer[arg_index_start]), (arg_index_end - arg_index_start));
      arg_index_start = ++arg_index_end;
    }
    arg_index_end++;
  }
  //Return the number of available command line arguments
  return countCmdArgs();
}

//function to write ID bytes with RW1990 (special timing, inversion)
int writeByte_rw1990(byte data){
  int data_bit;
  for(data_bit=0; data_bit<8; data_bit++){
    rw1990.write_bit_rw1990(data);
    data=data>>1;
  }
  return 0;
}

void led(int state){
  int i;
  switch(state){
    case RED:
      digitalWrite(RLEDPIN,HIGH);
      digitalWrite(GLEDPIN,LOW);
      for(i=3;i--;){
        digitalWrite(BEEPPIN,HIGH);
        delay(100);
        digitalWrite(BEEPPIN,LOW);
        delay(100);
        }
      break;
    case GREEN:
      digitalWrite(RLEDPIN,LOW);
      digitalWrite(GLEDPIN,HIGH);
      digitalWrite(BEEPPIN,HIGH);
      delay(500);
      digitalWrite(BEEPPIN,LOW);
      delay(100);
      break;
    case OFF:
      digitalWrite(GLEDPIN,LOW);
      digitalWrite(GLEDPIN,LOW);
      break;
    }
}


int write(byte *id2){
  byte crc;
  CONSOLEPORT.print(F("Write "));
  printid(id2);
  crc = rw1990.crc8(id2, 7);
  if(crc==id2[7]){
    rw1990.reset();
    delay(20);
    rw1990.write(0x33);
    for (byte i=0; i<8; i++){
      id[i]=rw1990.read();
      }
    delay(20);
    rw1990.skip();
    delay(20);
    //prepare for ID write
    rw1990.reset();
    rw1990.write(0xD1);
    rw1990.write_bit_rw1990(1);
    rw1990.reset();
    //ID write
    rw1990.write(0xD5);
    for (byte x = 0; x<8; x++){
      writeByte_rw1990(id2[x]);
      }
    rw1990.reset();
    //end ID write
    rw1990.write(0xD1);
    rw1990.write_bit_rw1990(0);
    rw1990.reset();
    //verify
    rw1990.write(0x33);
    for (byte i=0; i<8; i++){
    id[i]=rw1990.read();
      }
    if(strncmp(id,id2,sizeof(id))){
      CONSOLEPORT.print(F("Bad read back: "));
      printid(id);
    CONSOLEPORT.println();
      }
    else{
     led(GREEN);
     CONSOLEPORT.println(F("Verified OK.\n"));
      }
    }
  else{
    printcrc(id2);
    CONSOLEPORT.println(F("Bad new CRC, write aborted.\n"));
    }
  return;
}

int search(){
  if(rw1990.reset()){
    CONSOLEPORT.println(F("searching..."));
    rw1990.reset_search();
    if (!rw1990.search (id)){
//  rw1990.reset_search();
    delay(200);
    led(RED);
    CONSOLEPORT.println(F("Failed."));
    }
  else{
//    led(GREEN);
    CONSOLEPORT.print(F("Found "));
    printid(id);
    }
  CONSOLEPORT.println();
  }
  else{
    led(RED);
    CONSOLEPORT.println(F("Failed."));
    }
return;
}

setrandom(){
  int i;
  char* pstr;
  pstr=getCmdArg(1);
  randomSeed(analogRead(0));
  id2[0]=1; //family=1
  for(i=1;i<7;i++){
    id2[i]=random()&0xff;
    pstr++;
    }
    id2[7]=rw1990.crc8(id2,7);
  CONSOLEPORT.print(F("Random set "));
  printid(id2);
  CONSOLEPORT.println();
  return;
}

int save(){
  byte crc;
  //check crc etc
  CONSOLEPORT.println(F("Save "));
  crc = rw1990.crc8(id, 7);
  if(crc==id[7]){
    memcpy(id2,id,sizeof(id2));
    led(GREEN);
    CONSOLEPORT.print(F("New ID saved "));
  }
  else{
    CONSOLEPORT.print(F("Not saved "));
    led(RED);
    }
  printid(id2);
  CONSOLEPORT.println();
  return;
}

int setid(){
  int i,j;
  char* pstr;
  pstr=getCmdArg(1);
  j=0;
  for(i=0;i<8;i++){
    id2[i]=strtoul(pstr,&pstr,16)&0xff;
    j|=id2[i];
    pstr++;
    }
  if(j){
    CONSOLEPORT.print(F("Set "));
    printid(id2);
    CONSOLEPORT.println();
    }
  else{
    led(RED);
    }
  return;
}

int show(){
  CONSOLEPORT.print(F("Show "));
  printid(id2);
  CONSOLEPORT.println();
  return;
}

void setup(){
  digitalWrite(BEEPPIN,LOW);
  pinMode(BEEPPIN,OUTPUT);
  pinMode(GLEDPIN,OUTPUT);
  pinMode(RLEDPIN,OUTPUT);
  led(GREEN);
  CONSOLEPORT.begin(BAUDRATE);
  while(!CONSOLEPORT)
    if (millis()>5000){
    CONSOLEPORT.end();
    break; //serial port does not seem to start, not plugged in?
    }
  CONSOLEPORT.println(F("\n\nRW1990rw v0.1\n\n"));
}

void loop(){
  char buffer[80];
  int i,j;
//  byte crc;
  char* commandArg;
//  char* pstr;
  
  if(readLine(buffer,sizeof(buffer))<1){
    delay(200);
    return;
    }
  led(OFF);
  commandArg=getCmdArg(0);
  //search
  if(strcmp_P(commandArg,PSTR("search"))==0){
    search();
    return;
    }
  
  //setid
  if(strcmp_P(commandArg,PSTR("setid"))==0){
    setid();
    return;
    }
  
  //set random id
  if(strcmp_P(commandArg,PSTR("random"))==0){
    setrandom();
    return;
    }
  if(strcmp_P(commandArg,PSTR("show"))==0){
    show();
    return;
    }
  
  //save last read ID to new
  if(strcmp_P(commandArg,PSTR("save"))==0){
    save();
    return;
    }
  
  //write new to iButton
  if(strcmp_P(commandArg,PSTR("write"))==0){  //check crc etc
    write(id2);
    return;
    }
    
  CONSOLEPORT.println(F("Unknown command.\n"));  
}
