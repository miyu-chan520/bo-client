// src/Login/clipboard.h
#pragma once
#include <string>

// Read Android clipboard contents via `service call clipboard` (requires root)
// Returns empty string if clipboard is empty or command fails
std::string getClipboard();