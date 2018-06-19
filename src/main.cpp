#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>

SoftwareSerial mySerial(8, 7); //RX/TX

const bool DEBUG = true;

const int out_pins[4] = {A5, A4, A3, A2};
const int in_pins[4] = {6, 5, 4, 3};
const int interrupt_pin = 2;

const unsigned int DEBOUNCE_TIME = 200;
int states[4];
unsigned long debounce[4];

String message[10];

//Function prototypes
String generate_status();
String generate_error(String error);

void switch_pin(int pin, int mode);
void verify_input();
bool parse_command(String s);
void serial_flush();
void debug(const char* c);

void setup() {
    wdt_enable(WDTO_8S);

    for(int i = 0; i < 4; i++){
        states[i] = 0;
        pinMode(out_pins[i], OUTPUT);
        pinMode(in_pins[i], INPUT_PULLUP);
    }

    pinMode(interrupt_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interrupt_pin), verify_input, LOW);

    mySerial.begin(19200);
    Serial.begin(19200);

    serial_flush();

    mySerial.println(generate_status());
    debug("Alive!");
    Serial.println(generate_status());
}

void loop() {
    String buffer_string;

    if(mySerial.available()){
        buffer_string  = mySerial.readStringUntil('\n');
        Serial.print("D: Recived: ");
        Serial.println(buffer_string);
    }

    // version;sender;action;value
    if(!buffer_string.equals("")){
        if(parse_command(buffer_string)){
            debug("I'm a command !");

            if(message[0] != "0"){
                //Wrong Version
            }else if(message[1] == "server"){
                if(message[2] == "status"){
                    mySerial.flush();
                    mySerial.println(generate_status());
                }else if(message[2] == "toggle"){
                    switch_pin(message[3].toInt(), 3);
                }else if(message[2] == "on"){
                    switch_pin(message[3].toInt(), 1);
                }else if(message[2] == "off"){
                    switch_pin(message[3].toInt(), 0);
                }else{
                    //Parsing Error
                }
            }else{
                //Wrong Sender
            }
        }

        buffer_string = "";
    }

    delay(1);
    wdt_reset();
}

String generate_status(){
    String string = "0;board;status;"; //VAR

    for(int i = 0; i < 4; i++){
        string.concat(states[i] == 0 ? "off" : "on");
        string.concat(i == 3 ? "" : ",");
    }

    return string;
}

String generate_error(String error){
    String string = "0;board;error;"; //VAR

    string.concat(error);

    return string;
}

void switch_pin(int pin, int mode){
    if(millis() - debounce[pin] > DEBOUNCE_TIME){
        if(mode == 0){
            states[pin] = 0;
        }else if(mode == 1){
            states[pin] = 1;
        }else if(mode == 3){
            states[pin] = states[pin] == 1 ? 0 : 1;
        }

        digitalWrite(out_pins[pin], states[pin]);
        debounce[pin] = millis();

        mySerial.flush();
        mySerial.println(generate_status());

    }
}

void verify_input(){
    for(int i = 0; i < 4; i++){
        if(digitalRead(in_pins[i]) == LOW){
            switch_pin(i, 3);
        }
    }
}

bool parse_command(String s){
    int n = 0;

    for(int i = 0; i < 10; i++){
        message[i] = "";
    }

    for(int i = 0; i < (int)s.length(); i++){
        if(s.charAt(i) == ';'){
            n += 1;
        }else{
            message[n].concat(s.charAt(i));
        }
    }

    return n >= 3 ? true : false;
}

void serial_flush(){
    while(mySerial.available() > 0){
        char t = mySerial.read();
    }
}

void debug(const char* c){
    if(DEBUG){
        Serial.print("D: ");
        Serial.println(c);
    }
}
