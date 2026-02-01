// cpu.h

#ifndef CPU_H
#define CPU_H

#include <stdexcept>
#include <string>
#include <set>
#include <vector>
#include <cstdint>

class Memory;

class CPUExcept : public std::runtime_error {
public:
    CPUExcept(const std::string& msg) : std::runtime_error(msg) {}
    CPUExcept(const char *msg) : std::runtime_error(msg) {}
};

class CPU {
public:
    CPU(Memory *memory) : max_len(3), mem(memory) {}
    virtual ~CPU(void);

    virtual std::vector<std::string> GetRegisterList(void) const = 0;
    virtual std::string GetRegister(const std::string& reg_name) const = 0;
    virtual bool SetRegister(const std::string& reg_name, const std::string& value) = 0;
    virtual void Step(void) = 0;
    virtual void Next(void) = 0;
    virtual void ToReturn(void) = 0;

    virtual std::uint64_t GetPC(void) const = 0;

    struct MemZone {
        const char *name;
        std::uint64_t start;
        std::uint64_t size;
    };
    virtual std::vector<MemZone> GetMemZones(void) const;

    struct Flag {
        const char *name;
        char letter;
    };
    virtual std::vector<Flag> GetFlags(void) const = 0;

    struct Disasm {
        std::string disasm;
        unsigned num_bytes;
        Disasm(const std::string& disasm_, unsigned num_bytes_) :
            disasm(disasm_), num_bytes(num_bytes_) {}
    };
    virtual Disasm Disassemble(std::uint64_t address) const = 0;

    struct Assem {
        bool valid;
        std::vector<std::uint8_t> bytes;
        Assem(bool valid_) : valid(valid_) {}
        Assem(bool valid_, const std::vector<std::uint8_t>& bytes_) :
            valid(valid_), bytes(bytes_) {}
    };
    virtual Assem Assemble(std::uint64_t pc, const std::string& code) const = 0;

    virtual unsigned long GetEmuCycles(void) const = 0;
    virtual void ClearEmuCycles(void) = 0;

    void SetBreakpoint(std::uint64_t addr);
    void ClearBreakpoint(std::uint64_t addr);
    bool HasBreakpoint(std::uint64_t addr, unsigned count) const;

    Memory *GetMemory(void) const { return mem; }
    unsigned GetMaxLen(void) const { return max_len; }

protected:
    unsigned max_len;

private:
    Memory *mem;
    std::set<std::uint64_t> breakpoints;
};

#endif
