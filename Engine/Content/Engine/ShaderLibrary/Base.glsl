#ifndef BASE_INCLUDED
#define BASE_INCLUDED

#define BeginPass(name) #if defined(name)
#define BeginPassVertex(name) #if defined(name) && defined(VERTEX)
#define BeginPassFragment(name) #if defined(name) && defined(FRAGMENT)
#define EndPass #endif

#define BeginVertex(name) #if defined(VERTEX)
#define EndVertex(name) #endif

#define BeginFragment(name) #if defined(FRAGMENT)
#define EndFragment(name) #endif

#endif