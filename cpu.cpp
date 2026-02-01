// cpu.cpp

#include "cpu.h"
#include "memory.h"

CPU::~CPU(void)
{
    delete mem;
}

std::vector<CPU::MemZone>
CPU::GetMemZones(void) const
{
    // Default behavior is to return no zones
    std::vector<MemZone> empty;
    return empty;
}

void
CPU::SetBreakpoint(std::uint64_t addr)
{
    breakpoints.insert(addr);
}

void
CPU::ClearBreakpoint(std::uint64_t addr)
{
    breakpoints.erase(addr);
}

bool
CPU::HasBreakpoint(std::uint64_t addr, unsigned count) const
{
    for (unsigned i = 0; i < count; ++i) {
        if (breakpoints.find(addr + i) != breakpoints.end()) {
            return true;
        }
    }
    return false;
}
