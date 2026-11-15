#pragma once

#include <cstdio>
#include <cstdlib>

// YAGKP_ASSERT(condition, message): checks a condition that must always hold,
// in builds without NDEBUG (debug). If it does not hold, prints the
// condition, the message and the place, and aborts. Release and profile
// builds (-DNDEBUG) do not evaluate the condition at all
#ifdef NDEBUG
#define YAGKP_ASSERT(condition, message) ((void)0)
#else
#define YAGKP_ASSERT(condition, message)                                                           \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			std::fprintf(                                                                          \
			    stderr,                                                                            \
			    "Assertion failed in %s\n"                                                         \
			    "  at:        %s:%d\n"                                                             \
			    "  condition: %s\n"                                                                \
			    "  message:   %s\n",                                                               \
			    __func__,                                                                          \
			    __FILE__,                                                                          \
			    __LINE__,                                                                          \
			    #condition,                                                                        \
			    message                                                                            \
			);                                                                                     \
			std::abort();                                                                          \
		}                                                                                          \
	} while (false)
#endif