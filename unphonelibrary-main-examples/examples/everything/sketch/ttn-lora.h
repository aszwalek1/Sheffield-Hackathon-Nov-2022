// lora.h
// the LoRa board and TTN LoRaWAN
// (a tiny API to mediate between MCCI LMIC/LMIC-node/TTN and the unPhone
// class)

#ifndef LORA_H
#define LORA_H

void lora_setup();                       // initialise lora/ttn
void lora_loop();                        // service pending lora transactions
void lora_send(const char *, va_list);   // send a ttn message (vsprintf style)
void lora_shutdown();                    // shut down LMIC

#endif
