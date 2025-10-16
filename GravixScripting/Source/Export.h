#pragma once

#ifdef _WIN32
#ifdef GRAVIXSCRIPTING_EXPORTS
#define GRAVIXSCRIPTING_API __declspec(dllexport)
#else
#define GRAVIXSCRIPTING_API __declspec(dllimport)
#endif
#else
#define GRAVIXSCRIPTING_API
#endif