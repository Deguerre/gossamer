// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#include "Utils.hh"
#include "GossamerException.hh"
#include <bitset>
#include <signal.h>
#include <chrono>

#pragma intrinsic(__cpuid)
#pragma intrinsic(__cpuidex)

namespace Gossamer {
    
    std::string defaultTmpDir()
    {
        const char* fallBack = "."; // should not happen really, but lets give reasonable default
        const char* val = getenv( "temp" );
        return val ? val : fallBack;
    }

}


namespace Gossamer { namespace Windows {

    const char* sigCodeString(int pSig )
    {
        switch (pSig)
        {
        case SIGINT:   return "interrupt";
        case SIGILL:   return "illegal instruction - invalid function image";
        case SIGFPE:   return "floating point exception";
        case SIGSEGV:  return "segment violation";
        case SIGTERM:  return "Software termination signal from kill";
        case SIGBREAK: return "Ctrl-Break sequence";
        case SIGABRT:  return "abnormal termination triggered by abort call";
        default: break;
        }

        return "unknown signal code";
    }

    void printBacktrace()
    {
         unsigned int   i;
         void         * stack[ 100 ];
         unsigned short frames;
         SYMBOL_INFO  * symbol;
         HANDLE         process;

         process = GetCurrentProcess();

         SymInitialize( process, NULL, TRUE );

         frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
         symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
         symbol->MaxNameLen   = 255;
         symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

         for( i = 0; i < frames; i++ )
         {
            SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );

            printf( "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address );
         }

         free( symbol );
    }

    void printBacktraceAndExit(int pSig)
    {
        std::cerr << "Caught signal " << pSig << ": " << sigCodeString(pSig) << "\n";
        printBacktrace();
        exit(1);
    }

    void installSignalHandlers()
    {
        int sigs[] = {SIGILL, SIGFPE, SIGSEGV};
        for (unsigned i = 0; i < sizeof(sigs) / sizeof(int); ++i)
        {
            const int sig = sigs[i];
            if ( SIG_ERR == signal(sig, &printBacktraceAndExit) )
            {
                std::cerr << "Error setting signal handler for " << sig << "\n";
                exit(1);
            } 
        }
    }

    enum {
        kCpuCapPopcnt = 0,
        kCpuCapLast
    };

    std::bitset<kCpuCapLast> sCpuCaps;

    uint32_t sLogicalProcessorCount;

    void probeCpu()
    {
        sLogicalProcessorCount = 1;
        sCpuCaps[kCpuCapPopcnt] = false;

        int cpuInfo[4];
        __cpuid(cpuInfo, 0);

        int cpuInfoExt[4];
        __cpuid(cpuInfoExt, 0x80000000);

        if (cpuInfo[0] >= 1)
        {
            int cpuInfo1[4];
            __cpuid(cpuInfo1, 1);
            sLogicalProcessorCount = (cpuInfo1[1] >> 16) & 0xFF;
            sCpuCaps[kCpuCapPopcnt] = cpuInfo1[2] & (1 << 23);
        }
    }

    void setupPopcnt()
    {
        if (!sCpuCaps[kCpuCapPopcnt]) {
            BOOST_THROW_EXCEPTION(
                Gossamer::error()
                << Gossamer::general_error_info("popcnt instruction not detected on this platform"));
        }
    }

} }


void
MachineAutoSetup::setupMachineSpecific()
{
    Gossamer::Windows::installSignalHandlers();
    Gossamer::Windows::probeCpu();
    Gossamer::Windows::setupPopcnt();
}


uint32_t
Gossamer::logicalProcessorCount()
{
    return Gossamer::Windows::sLogicalProcessorCount;
}


int gettimeofday(struct timeval* tv, void* tz)
{
    if (tv) {
        FILETIME               filetime; /* 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 00:00 UTC */
        ULARGE_INTEGER         x;
        ULONGLONG              usec;
        static const ULONGLONG epoch_offset_us = 11644473600000000ULL; /* microseconds betweeen Jan 1,1601 and Jan 1,1970 */

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
        GetSystemTimePreciseAsFileTime(&filetime);
#else
        GetSystemTimeAsFileTime(&filetime);
#endif
        x.LowPart = filetime.dwLowDateTime;
        x.HighPart = filetime.dwHighDateTime;
        usec = x.QuadPart / 10 - epoch_offset_us;
        tv->tv_sec = (time_t)(usec / 1000000ULL);
        tv->tv_usec = (long)(usec % 1000000ULL);
    }
    return 0;
}

