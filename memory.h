// memory.h

#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <cstdint>

class Memory {
public:
    Memory(std::size_t size);
    virtual ~Memory(void);

    // Access memory contents, without side effects, for load and display
    virtual void Load8(std::size_t addr, std::uint8_t data);
    virtual std::uint8_t Peek8(std::size_t addr) const;

    virtual std::uint8_t Read8(std::size_t addr) const;
    virtual std::uint16_t Read16(std::size_t addr) const = 0;
    virtual std::uint32_t Read32(std::size_t addr) const = 0;
    virtual std::uint64_t Read64(std::size_t addr) const = 0;

    virtual void Write8(std::size_t addr, std::uint8_t data);
    virtual void Write16(std::size_t addr, std::uint16_t data) = 0;
    virtual void Write32(std::size_t addr, std::uint32_t data) = 0;
    virtual void Write64(std::size_t addr, std::uint64_t data) = 0;

private:
    std::vector<std::uint8_t> bytes;
    std::size_t mask;
};

class LittleEndianMemory : public Memory {
public:
    LittleEndianMemory(std::size_t size) : Memory(size) {}

    virtual std::uint16_t Read16(std::size_t addr) const override;
    virtual std::uint32_t Read32(std::size_t addr) const override;
    virtual std::uint64_t Read64(std::size_t addr) const override;

    virtual void Write16(std::size_t addr, std::uint16_t data) override;
    virtual void Write32(std::size_t addr, std::uint32_t data) override;
    virtual void Write64(std::size_t addr, std::uint64_t data) override;
};

class BigEndianMemory : public Memory {
public:
    BigEndianMemory(std::size_t size) : Memory(size) {}

    virtual std::uint16_t Read16(std::size_t addr) const override;
    virtual std::uint32_t Read32(std::size_t addr) const override;
    virtual std::uint64_t Read64(std::size_t addr) const override;

    virtual void Write16(std::size_t addr, std::uint16_t data) override;
    virtual void Write32(std::size_t addr, std::uint32_t data) override;
    virtual void Write64(std::size_t addr, std::uint64_t data) override;
};

#endif // MEMORY_H
