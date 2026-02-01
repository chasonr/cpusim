// cpu6502.h

#ifndef CPU6502_H
#define CPU6502_H

#include <cstdint>
#include <string>
#include <vector>
#include "cpu.h"

class Memory;

class CPU6502 : public CPU {
public:
    CPU6502(Memory *mem);
    virtual ~CPU6502(void);

    virtual std::vector<std::string> GetRegisterList(void) const override;
    virtual std::vector<Flag> GetFlags(void) const override;
    virtual std::string GetRegister(const std::string& reg_name) const override;
    virtual bool SetRegister(const std::string& reg_name, const std::string& value) override;
    virtual void Step(void) override;
    virtual void Next(void) override;
    virtual void ToReturn(void) override;
    virtual std::vector<MemZone> GetMemZones(void) const override;

    virtual std::uint64_t GetPC(void) const override;

    virtual Disasm Disassemble(std::uint64_t address) const override;
    virtual Assem Assemble(std::uint64_t pc, const std::string& code) const override;

private:
    void *impl;
};

#endif
