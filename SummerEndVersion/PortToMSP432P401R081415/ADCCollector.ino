void setupADCCollector()
{
  Serial.begin(9600);
}

void loopADCCollector()
{
  NTimer++;
  //Serial.println("ADC COLLECTED");
  delayMicroseconds(1000000 / ADC_SAMPLE_FREQUENCY);
}
