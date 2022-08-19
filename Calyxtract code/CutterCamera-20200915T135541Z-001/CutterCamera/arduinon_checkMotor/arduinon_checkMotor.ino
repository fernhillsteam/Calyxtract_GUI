void setup() {
  // put your setup code here, to run once:
  pinMode(7,OUTPUT);
  pinMode(6,OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(7,HIGH);
//  for(int x = 0; x < 200; x++) {
    digitalWrite(6,HIGH); 
    delay(500); 
    digitalWrite(6,LOW); 
    delay(500); 
//  }
}
