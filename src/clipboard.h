// src/Login/clipboard.h
#pragma once
#include <string>

// Baca clipboard Android via `service call clipboard` (butuh root)
// Return "" kalau clipboard kosong / error
std::string getClipboard();