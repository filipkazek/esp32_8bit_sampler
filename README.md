
# ESP32 and LabView based groovebox



## Hardware
ESP-32 based sampler/sequencer. 
 - It has on-board mic for recoring samples. Due to memory limitations it can store four 1 sec samples (8bit, 22.05kHz).
 - 8 step sequencer for each sample. Every step can be modulated -/+ 12 semitones.
 - UI - Keypad for patterns and sound modulation, encoder - global BPM change, - button for switching between samples.
 - Sound is outputed using DAC (3.5mm jack)
 - Communication via serial port with PC
<p align="center">
  <img width="750" height="500" src="https://github.com/user-attachments/assets/20074ee6-2ab3-418c-97e7-b799aa22f85b">
</p>

## PC SimpleSynth
As an addition a simple synthesizer was made in NI LabView environment. It communicates with hardware via serial port. You can send generated sample directly into track which is being played on the hardware, replacing previous sample.
 - It is capable of generating basic sound shapes (sin, square, triangle, saw)
 - Options like ADSR envelope, bitcrushing, detune
 
 <img width="1170" height="875" alt="Zrzut ekranu 2026-03-21 193831" src="https://github.com/user-attachments/assets/1c65a8b7-cf07-4f61-9f4e-4e9819e4d725" />
