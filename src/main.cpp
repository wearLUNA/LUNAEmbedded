#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly
  Serial.begin(9600);

  while (!Serial) {
    ;
  }
  Serial.printf("Hellow World");
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}