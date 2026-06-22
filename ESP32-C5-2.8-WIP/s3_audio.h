#pragma once

#include "cyd_config.h"
#include <Arduino.h>

bool s3AudioBegin();
bool s3AudioReady();
int s3AudioReadLevel(uint32_t windowMs = 120);
void s3AudioTestScreen();
