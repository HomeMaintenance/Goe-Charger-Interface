#pragma once

#ifdef SERVER_LIB
#define SERVER_EXPORT __declspec(dllexport)
#else
#define SERVER_EXPORT __declspec(dllimport)
#endif
