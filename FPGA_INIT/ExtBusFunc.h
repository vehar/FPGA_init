#pragma once

void SetDataValue(DWORD val, DWORD addr);
DWORD GetDataValue(DWORD addr);
void SetCs1Value(DWORD val, DWORD addr);
DWORD GetCs1Value(DWORD addr);
DWORD GetRegVal(int i);
void ExtBusCs1Init(void);