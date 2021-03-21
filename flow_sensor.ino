
void flow() 
{
  flow_frequency++;
  currentTime = millis();
  if (currentTime >= (cloopTime + 1000)) {
    cloopTime = currentTime;
    liters += flow_frequency / 7.5;
    flow_frequency = 0;
  }
}
