// memory.cpp

#include "memory.h"

Memory::Memory(std::size_t size) : bytes(size)
{
    // Set a bit mask to wrap addresses
    std::size_t p2;
    for (p2 = 1; p2 < size && p2 != 0; p2 <<= 1) {}
    mask = p2 - 1;
}

Memory::~Memory(void)
{
    // Empty destructor so subclasses can inherit
}

std::uint8_t
Memory::Peek8(std::size_t addr) const
{
    addr &= mask;
    if (addr < bytes.size()) {
        return bytes[addr];
    } else {
        return 0xFF;
    }
}

std::uint8_t
Memory::Read8(std::size_t addr) const
{
    addr &= mask;
    if (addr < bytes.size()) {
        return bytes[addr];
    } else {
        return 0xFF;
    }
}

void
Memory::Write8(std::size_t addr, std::uint8_t data)
{
    addr &= mask;
    if (addr < bytes.size()) {
        bytes[addr] = data;
    }
}

void
Memory::Load8(std::size_t addr, std::uint8_t data)
{
    addr &= mask;
    if (addr < bytes.size()) {
        bytes[addr] = data;
    }
}

//////////////////////////////////////////////////////////////////////////////

std::uint16_t
BigEndianMemory::Read16(std::size_t addr) const
{
    return (static_cast<std::uint16_t>(Read8(addr+0)) << 8)
         | (static_cast<std::uint16_t>(Read8(addr+1)) << 0);
}

std::uint32_t
BigEndianMemory::Read32(std::size_t addr) const
{
    return (static_cast<std::uint32_t>(Read8(addr+0)) << 24)
         | (static_cast<std::uint32_t>(Read8(addr+1)) << 16)
         | (static_cast<std::uint32_t>(Read8(addr+2)) <<  8)
         | (static_cast<std::uint32_t>(Read8(addr+3)) <<  0);
}

std::uint64_t
BigEndianMemory::Read64(std::size_t addr) const
{
    return (static_cast<std::uint64_t>(Read8(addr+0)) << 56)
         | (static_cast<std::uint64_t>(Read8(addr+1)) << 48)
         | (static_cast<std::uint64_t>(Read8(addr+2)) << 40)
         | (static_cast<std::uint64_t>(Read8(addr+3)) << 32)
         | (static_cast<std::uint64_t>(Read8(addr+4)) << 24)
         | (static_cast<std::uint64_t>(Read8(addr+5)) << 16)
         | (static_cast<std::uint64_t>(Read8(addr+6)) <<  8)
         | (static_cast<std::uint64_t>(Read8(addr+7)) <<  0);
}

void
BigEndianMemory::Write16(std::size_t addr, std::uint16_t data)
{
    Write8(addr+0, static_cast<std::uint8_t>(data >> 8));
    Write8(addr+1, static_cast<std::uint8_t>(data >> 0));
}

void
BigEndianMemory::Write32(std::size_t addr, std::uint32_t data)
{
    Write8(addr+0, static_cast<std::uint8_t>(data >> 24));
    Write8(addr+1, static_cast<std::uint8_t>(data >> 16));
    Write8(addr+2, static_cast<std::uint8_t>(data >>  8));
    Write8(addr+3, static_cast<std::uint8_t>(data >>  0));
}

void
BigEndianMemory::Write64(std::size_t addr, std::uint64_t data)
{
    Write8(addr+0, static_cast<std::uint8_t>(data >> 56));
    Write8(addr+1, static_cast<std::uint8_t>(data >> 48));
    Write8(addr+2, static_cast<std::uint8_t>(data >> 40));
    Write8(addr+3, static_cast<std::uint8_t>(data >> 32));
    Write8(addr+4, static_cast<std::uint8_t>(data >> 24));
    Write8(addr+5, static_cast<std::uint8_t>(data >> 16));
    Write8(addr+6, static_cast<std::uint8_t>(data >>  8));
    Write8(addr+7, static_cast<std::uint8_t>(data >>  0));
}

//////////////////////////////////////////////////////////////////////////////

std::uint16_t
LittleEndianMemory::Read16(std::size_t addr) const
{
    return (static_cast<std::uint16_t>(Read8(addr+0)) << 0)
         | (static_cast<std::uint16_t>(Read8(addr+1)) << 8);
}

std::uint32_t
LittleEndianMemory::Read32(std::size_t addr) const
{
    return (static_cast<std::uint32_t>(Read8(addr+0)) <<  0)
         | (static_cast<std::uint32_t>(Read8(addr+1)) <<  8)
         | (static_cast<std::uint32_t>(Read8(addr+2)) << 16)
         | (static_cast<std::uint32_t>(Read8(addr+3)) << 24);
}

std::uint64_t
LittleEndianMemory::Read64(std::size_t addr) const
{
    return (static_cast<std::uint64_t>(Read8(addr+0)) <<  0)
         | (static_cast<std::uint64_t>(Read8(addr+1)) <<  8)
         | (static_cast<std::uint64_t>(Read8(addr+2)) << 16)
         | (static_cast<std::uint64_t>(Read8(addr+3)) << 24)
         | (static_cast<std::uint64_t>(Read8(addr+4)) << 32)
         | (static_cast<std::uint64_t>(Read8(addr+5)) << 40)
         | (static_cast<std::uint64_t>(Read8(addr+6)) << 48)
         | (static_cast<std::uint64_t>(Read8(addr+7)) << 56);
}

void
LittleEndianMemory::Write16(std::size_t addr, std::uint16_t data)
{
    Write8(addr+0, static_cast<std::uint8_t>(data >> 0));
    Write8(addr+1, static_cast<std::uint8_t>(data >> 8));
}

void
LittleEndianMemory::Write32(std::size_t addr, std::uint32_t data)
{
    Write8(addr+0, static_cast<std::uint8_t>(data >>  0));
    Write8(addr+1, static_cast<std::uint8_t>(data >>  8));
    Write8(addr+2, static_cast<std::uint8_t>(data >> 16));
    Write8(addr+3, static_cast<std::uint8_t>(data >> 24));
}

void
LittleEndianMemory::Write64(std::size_t addr, std::uint64_t data)
{
    Write8(addr+0, static_cast<std::uint8_t>(data >>  0));
    Write8(addr+1, static_cast<std::uint8_t>(data >>  8));
    Write8(addr+2, static_cast<std::uint8_t>(data >> 16));
    Write8(addr+3, static_cast<std::uint8_t>(data >> 24));
    Write8(addr+4, static_cast<std::uint8_t>(data >> 32));
    Write8(addr+5, static_cast<std::uint8_t>(data >> 40));
    Write8(addr+6, static_cast<std::uint8_t>(data >> 48));
    Write8(addr+7, static_cast<std::uint8_t>(data >> 56));
}
