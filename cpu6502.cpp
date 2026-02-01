// cpu6502.cpp

#include <regex>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cpu6502.h"
#include "memory.h"

namespace {

enum AddrMode {
    am_invalid = -1,
    am_implied = 0,
    am_acc,
    am_immediate,
    am_abs_x,
    am_abs_y,
    am_abs,
    am_zp_x,
    am_zp_y,
    am_zp,
    am_ind_x,
    am_ind_y,
    am_ind,
    am_rel
};

struct CPU6502Impl {
    std::uint8_t reg_a;
    std::uint8_t reg_x;
    std::uint8_t reg_y;
    std::uint8_t reg_s;
    std::uint8_t reg_flags;
    std::uint16_t reg_pc;
    unsigned long emu_cycles;

    CPU *cpu;
    Memory *memory;

    CPU6502Impl(CPU *cpu, Memory *mem);
    ~CPU6502Impl(void);
    bool DoStep(void);
    bool HasBreakpoint(void);

    CPU::Disasm Disassemble(std::uint64_t address) const;
    CPU::Assem Assemble(std::uint64_t pc, const std::string& code) const;

private:
    void DoAdd(std::uint8_t byte);
    std::uint16_t GetAddress(std::uint8_t opcode);
    void Compare(std::uint8_t reg, std::uint8_t byte);
    void PushByte(std::uint8_t byte);
    std::uint8_t PopByte(void);
    void SetNZ(std::uint8_t result);
    void SetZ(std::uint8_t result);
    void SetV(bool overflow);
    void SetC(bool carry);

    void do_invalid(std::uint8_t opcode);
    void do_ORA(std::uint8_t opcode);
    void do_AND(std::uint8_t opcode);
    void do_EOR(std::uint8_t opcode);
    void do_ADC(std::uint8_t opcode);
    void do_STA(std::uint8_t opcode);
    void do_LDA(std::uint8_t opcode);
    void do_CMP(std::uint8_t opcode);
    void do_SBC(std::uint8_t opcode);
    void do_ASL(std::uint8_t opcode);
    void do_LSR(std::uint8_t opcode);
    void do_ROL(std::uint8_t opcode);
    void do_ROR(std::uint8_t opcode);
    void do_ASL_A(std::uint8_t opcode);
    void do_LSR_A(std::uint8_t opcode);
    void do_ROL_A(std::uint8_t opcode);
    void do_ROR_A(std::uint8_t opcode);
    void do_BIT(std::uint8_t opcode);
    void do_branch(std::uint8_t opcode);
    void do_BRK(std::uint8_t opcode);
    void do_CLC(std::uint8_t opcode);
    void do_CLD(std::uint8_t opcode);
    void do_CLI(std::uint8_t opcode);
    void do_CLV(std::uint8_t opcode);
    void do_CPX(std::uint8_t opcode);
    void do_CPX_imm(std::uint8_t opcode);
    void do_CPY(std::uint8_t opcode);
    void do_CPY_imm(std::uint8_t opcode);
    void do_DEC(std::uint8_t opcode);
    void do_DEX(std::uint8_t opcode);
    void do_DEY(std::uint8_t opcode);
    void do_INC(std::uint8_t opcode);
    void do_INX(std::uint8_t opcode);
    void do_INY(std::uint8_t opcode);
    void do_JMP_abs(std::uint8_t opcode);
    void do_JMP_ind(std::uint8_t opcode);
    void do_JSR(std::uint8_t opcode);
    void do_LDX(std::uint8_t opcode);
    void do_LDX_imm(std::uint8_t opcode);
    void do_LDY(std::uint8_t opcode);
    void do_LDY_imm(std::uint8_t opcode);
    void do_NOP(std::uint8_t opcode);
    void do_PHA(std::uint8_t opcode);
    void do_PHP(std::uint8_t opcode);
    void do_PLA(std::uint8_t opcode);
    void do_PLP(std::uint8_t opcode);
    void do_RTI(std::uint8_t opcode);
    void do_RTS(std::uint8_t opcode);
    void do_SEC(std::uint8_t opcode);
    void do_SED(std::uint8_t opcode);
    void do_SEI(std::uint8_t opcode);
    void do_STX(std::uint8_t opcode);
    void do_STY(std::uint8_t opcode);
    void do_TAX(std::uint8_t opcode);
    void do_TAY(std::uint8_t opcode);
    void do_TSX(std::uint8_t opcode);
    void do_TXA(std::uint8_t opcode);
    void do_TXS(std::uint8_t opcode);
    void do_TYA(std::uint8_t opcode);

    typedef void (CPU6502Impl::*opcode_handler)(std::uint8_t opcode);
    struct Instruction {
        char name[4];
        AddrMode addr_mode;
        opcode_handler handler;
    };
    static const Instruction instructions[256];
    static int FindOpcode(std::string const &instr, AddrMode mode);
};

}

CPU6502::CPU6502(Memory *mem) :
    CPU(mem),
    impl(new CPU6502Impl(this, mem))
{
    max_len = 3;
}

CPU6502::~CPU6502(void)
{
    delete reinterpret_cast<CPU6502Impl *>(impl);
    impl = nullptr;
}

std::vector<std::string>
CPU6502::GetRegisterList(void) const
{
    static const char reg_list[][8] = {
        "A", "X", "Y", "S", "FLAGS", "PC", ""
    };

    std::vector<std::string> regs;
    for (unsigned i = 0; reg_list[i][0] != '\0'; ++i) {
        regs.push_back(std::string(reg_list[i]));
    }
    return regs;
}

std::vector<CPU::Flag>
CPU6502::GetFlags(void) const
{
    static const Flag flag_list[] = {
        { "&Negative",  'N' },
        { "O&verflow",  'V' },
        { "&Break",     'B' },
        { "&Decimal",   'D' },
        { "&Interrupt", 'I' },
        { "&Zero",      'Z' },
        { "&Carry",     'C' },
        { nullptr,      0   }
    };

    std::vector<Flag> flags;
    for (unsigned i = 0; flag_list[i].name != nullptr; ++i) {
        flags.push_back(flag_list[i]);
    }
    return flags;
}

std::string
CPU6502::GetRegister(const std::string& reg_name) const
{
    unsigned value;
    int width;
    char str[10];
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);

    if (reg_name == "A") {
        width = 2;
        value = impl_->reg_a;
    } else if (reg_name == "X") {
        width = 2;
        value = impl_->reg_x;
    } else if (reg_name == "Y") {
        width = 2;
        value = impl_->reg_y;
    } else if (reg_name == "S") {
        width = 2;
        value = impl_->reg_s;
    } else if (reg_name == "FLAGS") {
        std::snprintf(str, sizeof(str), "%c%c-%c%c%c%c%c",
                (impl_->reg_flags & 0x80) ? 'N' : '-',
                (impl_->reg_flags & 0x40) ? 'V' : '-',
                (impl_->reg_flags & 0x10) ? 'B' : '-',
                (impl_->reg_flags & 0x08) ? 'D' : '-',
                (impl_->reg_flags & 0x04) ? 'I' : '-',
                (impl_->reg_flags & 0x02) ? 'Z' : '-',
                (impl_->reg_flags & 0x01) ? 'C' : '-');
        return str;
    } else if (reg_name == "PC") {
        width = 4;
        value = impl_->reg_pc;
    } else {
        return "";
    }

    std::snprintf(str, sizeof(str), "%0*X", width, value);
    return str;
}

bool
CPU6502::SetRegister(const std::string& reg_name, const std::string& value)
{
    unsigned long num;
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);

    if (reg_name == "FLAGS") {
        // Parse flags as individual letters
        static const char flags[] = "NV-BDIZC";

        num = 0x20;
        for (std::size_t i = 0; i < value.size(); ++i) {
            char ch = value.at(i);
            if (ch == '-') {
                continue;
            }
            const char *p = std::strchr(flags, std::toupper(ch));
            if (p == nullptr) {
                return false;
            }
            num |= 0x80 >> (p - flags);
        }
        impl_->reg_flags = num;
        return true;
    }

    char *ptr;

    num = std::strtoul(value.c_str(), &ptr, 16);
    while (*ptr != '\0') {
        if (!std::isspace(*ptr)) {
            return false;
        }
        ++ptr;
    }

    if (reg_name == "A") {
        impl_->reg_a = num;
    } else if (reg_name == "X") {
        impl_->reg_x = num;
    } else if (reg_name == "Y") {
        impl_->reg_y = num;
    } else if (reg_name == "S") {
        impl_->reg_s = num;
    } else if (reg_name == "PC") {
        impl_->reg_pc = num;
    } else {
        return false;
    }

    return true;
}

std::vector<CPU::MemZone>
CPU6502::GetMemZones(void) const
{
    MemZone zone;
    std::vector<MemZone> zones;

    zone.name = "Zero page";
    zone.start = 0x0000;
    zone.size = 0x0100;
    zones.push_back(zone);

    zone.name = "Stack";
    zone.start = 0x0100;
    zone.size = 0x0100;
    zones.push_back(zone);

    return zones;
}

void
CPU6502::Step(void)
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    impl_->DoStep();
}

void
CPU6502::Next(void)
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    if (impl_->DoStep() && !impl_->HasBreakpoint()) {
        ToReturn();
    }
}

void
CPU6502::ToReturn(void)
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    std::uint8_t saved_s = impl_->reg_s;
    std::uint8_t delta_s;

    do {
        Step();
        if (impl_->HasBreakpoint()) {
            break;
        }
        // reg_s < saved_s will wrap; reg_s == saved_s will give zero;
        // still works as expected if the S register wraps
        delta_s = impl_->reg_s - saved_s;
    } while (delta_s == 0 || delta_s > 3);
}

std::uint64_t
CPU6502::GetPC(void) const
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    return impl_->reg_pc;
}

CPU::Disasm
CPU6502::Disassemble(std::uint64_t address) const
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    return impl_->Disassemble(address);
}

CPU::Assem
CPU6502::Assemble(std::uint64_t pc, const std::string& code) const
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    return impl_->Assemble(pc, code);
}

unsigned long
CPU6502::GetEmuCycles(void) const
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    return impl_->emu_cycles;
}

void
CPU6502::ClearEmuCycles(void)
{
    auto impl_ = reinterpret_cast<CPU6502Impl *>(impl);
    impl_->emu_cycles = 0;
}

namespace {

// TODO: Provide an enumeration at the constructor, to choose among variant
// instruction sets
const CPU6502Impl::Instruction CPU6502Impl::instructions[256] = {
    { "BRK", am_implied,  &CPU6502Impl::do_BRK     }, /* 00 */
    { "ORA", am_ind_x,    &CPU6502Impl::do_ORA     }, /* 01 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 02 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 03 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 04 */
    { "ORA", am_zp,       &CPU6502Impl::do_ORA     }, /* 05 */
    { "ASL", am_zp,       &CPU6502Impl::do_ASL     }, /* 06 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 07 */
    { "PHP", am_implied,  &CPU6502Impl::do_PHP     }, /* 08 */
    { "ORA", am_immediate,&CPU6502Impl::do_ORA     }, /* 09 */
    { "ASL", am_acc,      &CPU6502Impl::do_ASL_A   }, /* 0A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 0B */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 0C */
    { "ORA", am_abs,      &CPU6502Impl::do_ORA     }, /* 0D */
    { "ASL", am_abs,      &CPU6502Impl::do_ASL     }, /* 0E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 0F */
    { "BPL", am_rel,      &CPU6502Impl::do_branch  }, /* 10 */
    { "ORA", am_ind_y,    &CPU6502Impl::do_ORA     }, /* 11 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 12 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 13 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 14 */
    { "ORA", am_zp_x,     &CPU6502Impl::do_ORA     }, /* 15 */
    { "ASL", am_zp_x,     &CPU6502Impl::do_ASL     }, /* 16 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 17 */
    { "CLC", am_implied,  &CPU6502Impl::do_CLC     }, /* 18 */
    { "ORA", am_abs_y,    &CPU6502Impl::do_ORA     }, /* 19 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 1A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 1B */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 1C */
    { "ORA", am_abs_x,    &CPU6502Impl::do_ORA     }, /* 1D */
    { "ASL", am_abs_x,    &CPU6502Impl::do_ASL     }, /* 1E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 1F */
    { "JSR", am_abs,      &CPU6502Impl::do_JSR     }, /* 20 */
    { "AND", am_ind_x,    &CPU6502Impl::do_AND     }, /* 21 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 22 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 23 */
    { "BIT", am_zp,       &CPU6502Impl::do_BIT     }, /* 24 */
    { "AND", am_zp,       &CPU6502Impl::do_AND     }, /* 25 */
    { "ROL", am_zp,       &CPU6502Impl::do_ROL     }, /* 26 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 27 */
    { "PLP", am_implied,  &CPU6502Impl::do_PLP     }, /* 28 */
    { "AND", am_immediate,&CPU6502Impl::do_AND     }, /* 29 */
    { "ROL", am_acc,      &CPU6502Impl::do_ROL_A   }, /* 2A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 2B */
    { "BIT", am_abs,      &CPU6502Impl::do_BIT     }, /* 2C */
    { "AND", am_abs,      &CPU6502Impl::do_AND     }, /* 2D */
    { "ROL", am_abs,      &CPU6502Impl::do_ROL     }, /* 2E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 2F */
    { "BMI", am_rel,      &CPU6502Impl::do_branch  }, /* 30 */
    { "AND", am_ind_y,    &CPU6502Impl::do_AND     }, /* 31 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 32 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 33 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 34 */
    { "AND", am_zp_x,     &CPU6502Impl::do_AND     }, /* 35 */
    { "ROL", am_zp_x,     &CPU6502Impl::do_ROL     }, /* 36 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 37 */
    { "SEC", am_implied,  &CPU6502Impl::do_SEC     }, /* 38 */
    { "AND", am_abs_y,    &CPU6502Impl::do_AND     }, /* 39 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 3A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 3B */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 3C */
    { "AND", am_abs_x,    &CPU6502Impl::do_AND     }, /* 3D */
    { "ROL", am_abs_x,    &CPU6502Impl::do_ROL     }, /* 3E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 3F */
    { "RTI", am_implied,  &CPU6502Impl::do_RTI     }, /* 40 */
    { "EOR", am_ind_x,    &CPU6502Impl::do_EOR     }, /* 41 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 42 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 43 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 44 */
    { "EOR", am_zp,       &CPU6502Impl::do_EOR     }, /* 45 */
    { "LSR", am_zp,       &CPU6502Impl::do_LSR     }, /* 46 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 47 */
    { "PHA", am_implied,  &CPU6502Impl::do_PHA     }, /* 48 */
    { "EOR", am_immediate,&CPU6502Impl::do_EOR     }, /* 49 */
    { "LSR", am_acc,      &CPU6502Impl::do_LSR_A   }, /* 4A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 4B */
    { "JMP", am_abs,      &CPU6502Impl::do_JMP_abs }, /* 4C */
    { "EOR", am_abs,      &CPU6502Impl::do_EOR     }, /* 4D */
    { "LSR", am_abs,      &CPU6502Impl::do_LSR     }, /* 4E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 4F */
    { "BVC", am_rel,      &CPU6502Impl::do_branch  }, /* 50 */
    { "EOR", am_ind_y,    &CPU6502Impl::do_EOR     }, /* 51 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 52 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 53 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 54 */
    { "EOR", am_zp_x,     &CPU6502Impl::do_EOR     }, /* 55 */
    { "LSR", am_zp_x,     &CPU6502Impl::do_LSR     }, /* 56 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 57 */
    { "CLI", am_implied,  &CPU6502Impl::do_CLI     }, /* 58 */
    { "EOR", am_abs_y,    &CPU6502Impl::do_EOR     }, /* 59 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 5A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 5B */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 5C */
    { "EOR", am_abs_x,    &CPU6502Impl::do_EOR     }, /* 5D */
    { "LSR", am_abs_x,    &CPU6502Impl::do_LSR     }, /* 5E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 5F */
    { "RTS", am_implied,  &CPU6502Impl::do_RTS     }, /* 60 */
    { "ADC", am_ind_x,    &CPU6502Impl::do_ADC     }, /* 61 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 62 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 63 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 64 */
    { "ADC", am_zp,       &CPU6502Impl::do_ADC     }, /* 65 */
    { "ROR", am_zp,       &CPU6502Impl::do_ROR     }, /* 66 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 67 */
    { "PLA", am_implied,  &CPU6502Impl::do_PLA     }, /* 68 */
    { "ADC", am_immediate,&CPU6502Impl::do_ADC     }, /* 69 */
    { "ROR", am_acc,      &CPU6502Impl::do_ROR_A   }, /* 6A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 6B */
    { "JMP", am_ind,      &CPU6502Impl::do_JMP_ind }, /* 6C */
    { "ADC", am_abs,      &CPU6502Impl::do_ADC     }, /* 6D */
    { "ROR", am_abs,      &CPU6502Impl::do_ROR     }, /* 6E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 6F */
    { "BVS", am_rel,      &CPU6502Impl::do_branch  }, /* 70 */
    { "ADC", am_ind_y,    &CPU6502Impl::do_ADC     }, /* 71 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 72 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 73 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 74 */
    { "ADC", am_zp_x,     &CPU6502Impl::do_ADC     }, /* 75 */
    { "ROR", am_zp_x,     &CPU6502Impl::do_ROR     }, /* 76 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 77 */
    { "SEI", am_implied,  &CPU6502Impl::do_SEI     }, /* 78 */
    { "ADC", am_abs_y,    &CPU6502Impl::do_ADC     }, /* 79 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 7A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 7B */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 7C */
    { "ADC", am_abs_x,    &CPU6502Impl::do_ADC     }, /* 7D */
    { "ROR", am_abs_x,    &CPU6502Impl::do_ROR     }, /* 7E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 7F */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 80 */
    { "STA", am_ind_x,    &CPU6502Impl::do_STA     }, /* 81 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 82 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 83 */
    { "STY", am_zp,       &CPU6502Impl::do_STY     }, /* 84 */
    { "STA", am_zp,       &CPU6502Impl::do_STA     }, /* 85 */
    { "STX", am_zp,       &CPU6502Impl::do_STX     }, /* 86 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 87 */
    { "DEY", am_implied,  &CPU6502Impl::do_DEY     }, /* 88 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 89 */
    { "TXA", am_implied,  &CPU6502Impl::do_TXA     }, /* 8A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 8B */
    { "STY", am_abs,      &CPU6502Impl::do_STY     }, /* 8C */
    { "STA", am_abs,      &CPU6502Impl::do_STA     }, /* 8D */
    { "STX", am_abs,      &CPU6502Impl::do_STX     }, /* 8E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 8F */
    { "BCC", am_rel,      &CPU6502Impl::do_branch  }, /* 90 */
    { "STA", am_ind_y,    &CPU6502Impl::do_STA     }, /* 91 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 92 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 93 */
    { "STY", am_zp_x,     &CPU6502Impl::do_STY     }, /* 94 */
    { "STA", am_zp_x,     &CPU6502Impl::do_STA     }, /* 95 */
    { "STX", am_zp_y,     &CPU6502Impl::do_STX     }, /* 96 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 97 */
    { "TYA", am_implied,  &CPU6502Impl::do_TYA     }, /* 98 */
    { "STA", am_abs_y,    &CPU6502Impl::do_STA     }, /* 99 */
    { "TXS", am_implied,  &CPU6502Impl::do_TXS     }, /* 9A */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 9B */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 9C */
    { "STA", am_abs_x,    &CPU6502Impl::do_STA     }, /* 9D */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 9E */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* 9F */
    { "LDY", am_immediate,&CPU6502Impl::do_LDY_imm }, /* A0 */
    { "LDA", am_ind_x,    &CPU6502Impl::do_LDA     }, /* A1 */
    { "LDX", am_immediate,&CPU6502Impl::do_LDX_imm }, /* A2 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* A3 */
    { "LDY", am_zp,       &CPU6502Impl::do_LDY     }, /* A4 */
    { "LDA", am_zp,       &CPU6502Impl::do_LDA     }, /* A5 */
    { "LDX", am_zp,       &CPU6502Impl::do_LDX     }, /* A6 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* A7 */
    { "TAY", am_implied,  &CPU6502Impl::do_TAY     }, /* A8 */
    { "LDA", am_immediate,&CPU6502Impl::do_LDA     }, /* A9 */
    { "TAX", am_implied,  &CPU6502Impl::do_TAX     }, /* AA */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* AB */
    { "LDY", am_abs,      &CPU6502Impl::do_LDY     }, /* AC */
    { "LDA", am_abs,      &CPU6502Impl::do_LDA     }, /* AD */
    { "LDX", am_abs,      &CPU6502Impl::do_LDX     }, /* AE */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* AF */
    { "BCS", am_rel,      &CPU6502Impl::do_branch  }, /* B0 */
    { "LDA", am_ind_y,    &CPU6502Impl::do_LDA     }, /* B1 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* B2 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* B3 */
    { "LDY", am_zp_x,     &CPU6502Impl::do_LDY     }, /* B4 */
    { "LDA", am_zp_x,     &CPU6502Impl::do_LDA     }, /* B5 */
    { "LDX", am_zp_y,     &CPU6502Impl::do_LDX     }, /* B6 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* B7 */
    { "CLV", am_implied,  &CPU6502Impl::do_CLV     }, /* B8 */
    { "LDA", am_abs_y,    &CPU6502Impl::do_LDA     }, /* B9 */
    { "TSX", am_implied,  &CPU6502Impl::do_TSX     }, /* BA */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* BB */
    { "LDY", am_abs_x,    &CPU6502Impl::do_LDY     }, /* BC */
    { "LDA", am_abs_x,    &CPU6502Impl::do_LDA     }, /* BD */
    { "LDX", am_abs_y,    &CPU6502Impl::do_LDX     }, /* BE */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* BF */
    { "CPY", am_immediate,&CPU6502Impl::do_CPY_imm }, /* C0 */
    { "CMP", am_ind_x,    &CPU6502Impl::do_CMP     }, /* C1 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* C2 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* C3 */
    { "CPY", am_zp,       &CPU6502Impl::do_CPY     }, /* C4 */
    { "CMP", am_zp,       &CPU6502Impl::do_CMP     }, /* C5 */
    { "DEC", am_zp,       &CPU6502Impl::do_DEC     }, /* C6 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* C7 */
    { "INY", am_implied,  &CPU6502Impl::do_INY     }, /* C8 */
    { "CMP", am_immediate,&CPU6502Impl::do_CMP     }, /* C9 */
    { "DEX", am_implied,  &CPU6502Impl::do_DEX     }, /* CA */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* CB */
    { "CPY", am_abs,      &CPU6502Impl::do_CPY     }, /* CC */
    { "CMP", am_abs,      &CPU6502Impl::do_CMP     }, /* CD */
    { "DEC", am_abs,      &CPU6502Impl::do_DEC     }, /* CE */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* CF */
    { "BNE", am_rel,      &CPU6502Impl::do_branch  }, /* D0 */
    { "CMP", am_ind_y,    &CPU6502Impl::do_CMP     }, /* D1 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* D2 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* D3 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* D4 */
    { "CMP", am_zp_x,     &CPU6502Impl::do_CMP     }, /* D5 */
    { "DEC", am_zp_x,     &CPU6502Impl::do_DEC     }, /* D6 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* D7 */
    { "CLD", am_implied,  &CPU6502Impl::do_CLD     }, /* D8 */
    { "CMP", am_abs_y,    &CPU6502Impl::do_CMP     }, /* D9 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* DA */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* DB */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* DC */
    { "CMP", am_abs_x,    &CPU6502Impl::do_CMP     }, /* DD */
    { "DEC", am_abs_x,    &CPU6502Impl::do_DEC     }, /* DE */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* DF */
    { "CPX", am_immediate,&CPU6502Impl::do_CPX_imm }, /* E0 */
    { "SBC", am_ind_x,    &CPU6502Impl::do_SBC     }, /* E1 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* E2 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* E3 */
    { "CPX", am_zp,       &CPU6502Impl::do_CPX     }, /* E4 */
    { "SBC", am_zp,       &CPU6502Impl::do_SBC     }, /* E5 */
    { "INC", am_zp,       &CPU6502Impl::do_INC     }, /* E6 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* E7 */
    { "INX", am_implied,  &CPU6502Impl::do_INX     }, /* E8 */
    { "SBC", am_immediate,&CPU6502Impl::do_SBC     }, /* E9 */
    { "NOP", am_implied,  &CPU6502Impl::do_NOP     }, /* EA */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* EB */
    { "CPX", am_abs,      &CPU6502Impl::do_CPX     }, /* EC */
    { "SBC", am_abs,      &CPU6502Impl::do_SBC     }, /* ED */
    { "INC", am_abs,      &CPU6502Impl::do_INC     }, /* EE */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* EF */
    { "BEQ", am_rel,      &CPU6502Impl::do_branch  }, /* F0 */
    { "SBC", am_ind_y,    &CPU6502Impl::do_SBC     }, /* F1 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* F2 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* F3 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* F4 */
    { "SBC", am_zp_x,     &CPU6502Impl::do_SBC     }, /* F5 */
    { "INC", am_zp_x,     &CPU6502Impl::do_INC     }, /* F6 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* F7 */
    { "SED", am_implied,  &CPU6502Impl::do_SED     }, /* F8 */
    { "SBC", am_abs_y,    &CPU6502Impl::do_SBC     }, /* F9 */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* FA */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* FB */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }, /* FC */
    { "SBC", am_abs_x,    &CPU6502Impl::do_SBC     }, /* FD */
    { "INC", am_abs_x,    &CPU6502Impl::do_INC     }, /* FE */
    { "",    am_invalid,  &CPU6502Impl::do_invalid }  /* FF */
};

CPU6502Impl::CPU6502Impl(CPU *cpu_, Memory *mem) :
    reg_a(0),
    reg_x(0),
    reg_y(0),
    reg_s(0),
    reg_flags(0x20),
    reg_pc(0),
    emu_cycles(0),
    cpu(cpu_),
    memory(mem)
{
}

CPU6502Impl::~CPU6502Impl(void)
{
}

CPU::Disasm
CPU6502Impl::Disassemble(std::uint64_t address) const
{
    auto opcode = memory->Peek8(address+0);
    std::string mnemonic = instructions[opcode].name;
    char buf[20];
    unsigned count = 1;

    switch (instructions[opcode].addr_mode) {
    case am_invalid:
    default:
        std::snprintf(buf, sizeof(buf), "??? $%02X", opcode);
        return CPU::Disasm(buf, 1);

    case am_implied:
        buf[0] = '\0';
        count = 1;
        break;

    case am_acc:
        std::snprintf(buf, sizeof(buf), " A");
        count = 1;
        break;

    case am_immediate:
        std::snprintf(buf, sizeof(buf), " #$%02X", memory->Peek8(address+1));
        count = 2;
        break;

    case am_abs_x:
        std::snprintf(buf, sizeof(buf), " $%02X%02X,X",
                memory->Peek8(address+2),
                memory->Peek8(address+1));
        count = 3;
        break;

    case am_abs_y:
        std::snprintf(buf, sizeof(buf), " $%02X%02X,Y",
                memory->Peek8(address+2),
                memory->Peek8(address+1));
        count = 3;
        break;

    case am_abs:
        std::snprintf(buf, sizeof(buf), " $%02X%02X",
                memory->Peek8(address+2),
                memory->Peek8(address+1));
        count = 3;
        break;

    case am_zp_x:
        std::snprintf(buf, sizeof(buf), " $%02X,X", memory->Peek8(address+1));
        count = 2;
        break;

    case am_zp_y:
        std::snprintf(buf, sizeof(buf), " $%02X,Y", memory->Peek8(address+1));
        count = 2;
        break;

    case am_zp:
        std::snprintf(buf, sizeof(buf), " $%02X", memory->Peek8(address+1));
        count = 2;
        break;

    case am_ind_x:
        std::snprintf(buf, sizeof(buf), " ($%02X,X)",
                memory->Peek8(address+1));
        count = 2;
        break;

    case am_ind_y:
        std::snprintf(buf, sizeof(buf), " ($%02X),Y",
                memory->Peek8(address+1));
        count = 2;
        break;

    case am_ind:
        std::snprintf(buf, sizeof(buf), " ($%02X%02X)",
                memory->Peek8(address+2),
                memory->Peek8(address+1));
        count = 3;
        break;

    case am_rel:
        {
            std::uint16_t offset = memory->Peek8(address+1);
            offset = (offset ^ 0x80) - 0x80;
            std::snprintf(buf, sizeof(buf), " $%04X",
                    static_cast<std::uint16_t>(address + 2 + offset));
        }
        count = 2;
        break;
    }

    return CPU::Disasm(mnemonic + buf, count);
}

// Step through one instruction. Return true if the instruction was JSR, and
// Next() should continue until a return
bool
CPU6502Impl::DoStep(void)
{
    std::uint8_t opcode = memory->Read8(reg_pc++);
    auto handler = instructions[opcode].handler;
    (this->*handler)(opcode);

    // Indicate to Next() whether to continue to a return
    return opcode == 0x20; // JSR
}

bool
CPU6502Impl::HasBreakpoint(void)
{
    auto pc = cpu->GetPC();
    auto opcode = memory->Peek8(pc);
    unsigned count = 1;
    switch (instructions[opcode].addr_mode) {
    case am_invalid:
    case am_implied:
    case am_acc:
    default:
        count = 1;
        break;

    case am_immediate:
    case am_zp_x:
    case am_zp_y:
    case am_zp:
    case am_ind_x:
    case am_ind_y:
    case am_rel:
        count = 2;
        break;

    case am_abs_x:
    case am_abs_y:
    case am_abs:
    case am_ind:
        count = 3;
        break;
    }

    return cpu->HasBreakpoint(pc, count);
}

void
CPU6502Impl::do_invalid(std::uint8_t opcode)
{
    // Undocumented opcodes come here
    --reg_pc;
    char err[40];
    std::snprintf(err, sizeof(err), "Undocumented opcode %02X", opcode);
    throw CPUExcept(err);
}

void
CPU6502Impl::do_ORA(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    reg_a |= byte;
    SetNZ(reg_a);
    emu_cycles += 2;
}

void
CPU6502Impl::do_AND(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    reg_a &= byte;
    SetNZ(reg_a);
    emu_cycles += 2;
}

void
CPU6502Impl::do_EOR(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    reg_a ^= byte;
    SetNZ(reg_a);
    emu_cycles += 2;
}

void
CPU6502Impl::do_ADC(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    emu_cycles += 2;
    DoAdd(byte);
}

void
CPU6502Impl::do_STA(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    memory->Write8(addr, reg_a);
    emu_cycles += 2;
}

void
CPU6502Impl::do_LDA(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    reg_a = byte;
    SetNZ(reg_a);
    emu_cycles += 2;
}

void
CPU6502Impl::do_CMP(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    Compare(reg_a, byte);
    emu_cycles += 2;
}

void
CPU6502Impl::do_SBC(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    if (reg_flags & 0x08) {
        byte = 0x99 - byte;
    } else {
        byte = byte ^ 0xFF;
    }
    DoAdd(byte);
    emu_cycles += 2;
}

// Common to ADC and SBC
void
CPU6502Impl::DoAdd(std::uint8_t byte)
{
    int result;
    if (reg_flags & 0x08) {
        // Decimal mode
        int r1 = (reg_a & 0x0F) + (byte & 0xF0) + (reg_flags & 0x01);
        if (r1 > 0x09) {
            r1 += 0x06;
        }
        int r2 = (reg_a & 0xF0) + (byte & 0xF0);
        if (r2 > 0x90) {
            r2 += 0x60;
        }
        result = r1 + r2;
    } else {
        // Binary mode
        result = reg_a + byte + (reg_flags & 0x01);
        int r7 = (reg_a & 0x7F) + (byte & 0x7F) + (reg_flags & 0x01);
        int overflow = ((r7 << 1) ^ result) & 0x100;
        SetV(overflow);
    }
    reg_a = static_cast<std::uint8_t>(result);
    SetNZ(result);
    SetC(result > 0xFF);
}

void
CPU6502Impl::do_ASL(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    std::uint8_t result = byte << 1;
    memory->Write8(addr, result);
    SetNZ(result);
    SetC((byte & 0x80) != 0);
    emu_cycles += 4;
}

void
CPU6502Impl::do_LSR(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    std::uint8_t result = byte >> 1;
    memory->Write8(addr, result);
    SetNZ(result);
    SetC((byte & 0x01) != 0);
    emu_cycles += 4;
}

void
CPU6502Impl::do_ROL(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    std::uint8_t result = (byte << 1) | (reg_flags & 0x01);
    memory->Write8(addr, result);
    SetNZ(result);
    SetC((byte & 0x80) != 0);
    emu_cycles += 4;
}

void
CPU6502Impl::do_ROR(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    std::uint8_t result = (byte >> 1) | ((reg_flags & 0x01) << 7);
    memory->Write8(addr, result);
    SetNZ(result);
    SetC((byte & 0x01) != 0);
    emu_cycles += 4;
}

void
CPU6502Impl::do_ASL_A(std::uint8_t /*opcode*/)
{
    std::uint8_t byte = reg_a;
    std::uint8_t result = byte << 1;
    reg_a = result;
    SetNZ(result);
    SetC((byte & 0x80) != 0);
    emu_cycles += 2;
}

void
CPU6502Impl::do_LSR_A(std::uint8_t /*opcode*/)
{
    std::uint8_t byte = reg_a;
    std::uint8_t result = byte >> 1;
    reg_a = result;
    SetNZ(result);
    SetC((byte & 0x01) != 0);
    emu_cycles += 2;
}

void
CPU6502Impl::do_ROL_A(std::uint8_t /*opcode*/)
{
    std::uint8_t byte = reg_a;
    std::uint8_t result = (byte << 1) | (reg_flags & 0x01);
    reg_a = result;
    SetNZ(result);
    SetC((byte & 0x80) != 0);
    emu_cycles += 2;
}

void
CPU6502Impl::do_ROR_A(std::uint8_t /*opcode*/)
{
    auto byte = reg_a;
    std::uint8_t result = (byte >> 1) | ((reg_flags & 0x01) << 7);
    reg_a = result;
    SetNZ(result);
    SetC((byte & 0x01) != 0);
    emu_cycles += 2;
}

void
CPU6502Impl::do_BIT(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    SetZ(byte & reg_a);
    reg_flags = (reg_flags & 0x3F) | (byte & 0xC0);
    emu_cycles += 2;
}

void
CPU6502Impl::do_branch(std::uint8_t opcode)
{
    static const std::uint8_t flag_bits[] = {
        0x80, // BPL, BMI
        0x40, // BVC, BVS
        0x01, // BCC, BCS
        0x02  // BNE, BEQ
    };

    auto offset = memory->Read8(reg_pc++);
    std::uint16_t address = reg_pc + (offset ^ 0x80) - 0x80;
    bool which = (opcode & 0x20) != 0;
    bool flag = (reg_flags & flag_bits[opcode >> 6]) != 0;
    emu_cycles += 2;
    if (flag == which) {
        emu_cycles += 1;
        if ((address >> 8) != (reg_pc >> 8)) {
            emu_cycles += 1;
        }
        reg_pc = address;
    }
}

void
CPU6502Impl::do_BRK(std::uint8_t /*opcode*/)
{
    auto byte1 = memory->Read8(0xFFFE);
    auto byte2 = memory->Read8(0xFFFF);

    reg_pc++; // skip byte after BRK
    PushByte(reg_pc >> 8);
    PushByte(reg_pc & 0xFF);
    PushByte(reg_flags);
    reg_flags |= 0x14;
    reg_pc = byte2 * 0x100 + byte1;
    emu_cycles += 7;
}

void
CPU6502Impl::do_CLC(std::uint8_t /*opcode*/)
{
    reg_flags &= 0xFE;
    emu_cycles += 2;
}

void
CPU6502Impl::do_CLD(std::uint8_t /*opcode*/)
{
    reg_flags &= 0xF7;
    emu_cycles += 2;
}

void
CPU6502Impl::do_CLI(std::uint8_t /*opcode*/)
{
    reg_flags &= 0xFB;
    emu_cycles += 2;
}

void
CPU6502Impl::do_CLV(std::uint8_t /*opcode*/)
{
    reg_flags &= 0xBF;
    emu_cycles += 2;
}

void
CPU6502Impl::do_CPX(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    Compare(reg_x, byte);
    emu_cycles += 2;
}

void
CPU6502Impl::do_CPX_imm(std::uint8_t /*opcode*/)
{
    auto byte = memory->Read8(reg_pc++);
    Compare(reg_x, byte);
    emu_cycles += 2;
}

void
CPU6502Impl::do_CPY(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    Compare(reg_y, byte);
    emu_cycles += 2;
}

void
CPU6502Impl::do_CPY_imm(std::uint8_t /*opcode*/)
{
    int byte = memory->Read8(reg_pc++);
    Compare(reg_y, byte);
    emu_cycles += 2;
}

void
CPU6502Impl::do_DEC(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    std::uint8_t result = byte - 1;
    memory->Write8(addr, result);
    SetNZ(result);
    emu_cycles += 4;
}

void
CPU6502Impl::do_DEX(std::uint8_t /*opcode*/)
{
    SetNZ(--reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_DEY(std::uint8_t /*opcode*/)
{
    SetNZ(--reg_y);
    emu_cycles += 2;
}

void
CPU6502Impl::do_INC(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    std::uint8_t result = byte + 1;
    memory->Write8(addr, result);
    SetNZ(result);
    emu_cycles += 4;
}

void
CPU6502Impl::do_INX(std::uint8_t /*opcode*/)
{
    SetNZ(++reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_INY(std::uint8_t /*opcode*/)
{
    SetNZ(++reg_y);
    emu_cycles += 2;
}

void
CPU6502Impl::do_JMP_abs(std::uint8_t /*opcode*/)
{
    auto byte1 = memory->Read8(reg_pc++);
    auto byte2 = memory->Read8(reg_pc++);
    std::uint16_t addr = byte2 * 0x100 + byte1;
    reg_pc = addr;
    emu_cycles += 3;
}

void
CPU6502Impl::do_JMP_ind(std::uint8_t /*opcode*/)
{
    auto byte1 = memory->Read8(reg_pc++);
    auto byte2 = memory->Read8(reg_pc++);
    std::uint16_t addr = byte2 * 0x100 + byte1;
    auto byte3 = memory->Read8(addr);

    // The original 6502 has a bug in this instruction, where a vector at an
    // address ending in FF will wrap around to the start of the 256 byte page
    // instead of advancing to the next page. This bug is emulated here.
    // TODO: Bypass this if emulating a 65C02.

    ++byte1;
    addr = byte2 * 0x100 + byte1;
    auto byte4 = memory->Read8(addr);
    reg_pc = byte4 * 0x100 + byte3;
    emu_cycles += 5;
}

void
CPU6502Impl::do_JSR(std::uint8_t /*opcode*/)
{
    auto byte1 = memory->Read8(reg_pc++);

    // reg_pc is not incremented, because JSR pushes the PC address of the
    // last byte of the instruction, not the first byte of the next
    // instruction.

    auto byte2 = memory->Read8(reg_pc);
    std::uint16_t addr = byte2 * 0x100 + byte1;
    PushByte(reg_pc >> 8);
    PushByte(reg_pc & 0xFF); 
    reg_pc = addr;
    emu_cycles += 6;
}

void
CPU6502Impl::do_LDX(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    reg_x = byte;
    SetNZ(reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_LDX_imm(std::uint8_t /*opcode*/)
{
    auto byte = memory->Read8(reg_pc++);
    reg_x = byte;
    SetNZ(reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_LDY(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    auto byte = memory->Read8(addr);
    reg_y = byte;
    SetNZ(reg_y);
    emu_cycles += 2;
}

void
CPU6502Impl::do_LDY_imm(std::uint8_t /*opcode*/)
{
    auto byte = memory->Read8(reg_pc++);
    reg_y = byte;
    SetNZ(reg_y);
    emu_cycles += 2;
}

void
CPU6502Impl::do_NOP(std::uint8_t /*opcode*/)
{
    emu_cycles += 2;
}

void
CPU6502Impl::do_PHA(std::uint8_t /*opcode*/)
{
    PushByte(reg_a);
    emu_cycles += 3;
}

void
CPU6502Impl::do_PHP(std::uint8_t /*opcode*/)
{
    PushByte(reg_flags | 0x20);
    emu_cycles += 3;
}

void
CPU6502Impl::do_PLA(std::uint8_t /*opcode*/)
{
    auto byte = PopByte();
    reg_a = byte;
    SetNZ(reg_a);
    emu_cycles += 4;
}

void
CPU6502Impl::do_PLP(std::uint8_t /*opcode*/)
{
    auto byte = PopByte();
    reg_flags = byte | 0x20;
    emu_cycles += 4;
}

void
CPU6502Impl::do_RTI(std::uint8_t /*opcode*/)
{
    auto byte1 = PopByte();
    reg_flags = byte1 | 0x20;
    auto byte2 = PopByte();
    auto byte3 = PopByte();
    reg_pc = byte3 * 0x100 + byte2;
    emu_cycles += 6;
}

void
CPU6502Impl::do_RTS(std::uint8_t /*opcode*/)
{
    auto byte1 = PopByte();
    auto byte2 = PopByte();
    reg_pc = byte2 * 0x100 + byte1 + 1;
    emu_cycles += 6;
}

void
CPU6502Impl::do_SEC(std::uint8_t /*opcode*/)
{
    reg_flags |= 0x01;
    emu_cycles += 2;
}

void
CPU6502Impl::do_SED(std::uint8_t /*opcode*/)
{
    reg_flags |= 0x08;
    emu_cycles += 2;
}

void
CPU6502Impl::do_SEI(std::uint8_t /*opcode*/)
{
    reg_flags |= 0x04;
    emu_cycles += 2;
}

void
CPU6502Impl::do_STX(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    memory->Write8(addr, reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_STY(std::uint8_t opcode)
{
    auto addr = GetAddress(opcode);
    memory->Write8(addr, reg_y);
    emu_cycles += 2;
}

void
CPU6502Impl::do_TAX(std::uint8_t /*opcode*/)
{
    reg_x = reg_a;
    SetNZ(reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_TAY(std::uint8_t /*opcode*/)
{
    reg_y = reg_a;
    SetNZ(reg_y);
    emu_cycles += 2;
}

void
CPU6502Impl::do_TSX(std::uint8_t /*opcode*/)
{
    reg_x = reg_s;
    SetNZ(reg_x);
    emu_cycles += 2;
}

void
CPU6502Impl::do_TXA(std::uint8_t /*opcode*/)
{
    reg_a = reg_x;
    SetNZ(reg_a);
    emu_cycles += 2;
}

void
CPU6502Impl::do_TXS(std::uint8_t /*opcode*/)
{
    reg_s = reg_x;
    emu_cycles += 2;
}

void
CPU6502Impl::do_TYA(std::uint8_t /*opcode*/)
{
    reg_a = reg_y;
    SetNZ(reg_a);
    emu_cycles += 2;
}

// Return the address of the operand
std::uint16_t
CPU6502Impl::GetAddress(std::uint8_t opcode)
{
    auto mode = instructions[opcode].addr_mode;
    switch (mode) {
    case am_ind_x:
        // ABC (zp, X)
        {
            int byte1 = memory->Read8(reg_pc++);
            if (byte1 < 0) {
                return -1;
            }
            std::uint8_t zp_addr = byte1 + reg_x;
            //zp_addr++, not zp_addr + 1, because read from 0xFF wraps to 0x00
            int byte2 = memory->Read8(zp_addr++);
            if (byte2 < 0) {
                return -1;
            }
            int byte3 = memory->Read8(zp_addr);
            if (byte3 < 0) {
                return -1;
            }
            emu_cycles += 4;
            return byte3 * 0x100 + byte2;
        }

    case am_immediate:
        // ABC #imm
        return reg_pc++;

    case am_ind_y:
        // ABC (zp), Y
        {
            int byte1 = memory->Read8(reg_pc++);
            if (byte1 < 0) {
                return -1;
            }
            std::uint8_t zp_addr = byte1;
            //zp_addr++, not zp_addr + 1, because read from 0xFF wraps to 0x00
            int byte2 = memory->Read8(zp_addr++);
            if (byte2 < 0) {
                return -1;
            }
            int byte3 = memory->Read8(zp_addr);
            if (byte3 < 0) {
                return -1;
            }
            std::uint16_t addr1 = byte3 * 0x100 + byte2;
            std::uint16_t addr2 = addr1 + reg_y;
            emu_cycles += 3;
            if ((addr2 >> 8) != (addr1 >> 8)) {
                emu_cycles += 1;
            }
            return addr2;
        }

    case am_zp: // ABC zp
    case am_zp_x: // ABC zp, X
    case am_zp_y: // ABC zp, Y
        {
            int byte1 = memory->Read8(reg_pc++);
            if (byte1 < 0) {
                return -1;
            }
            emu_cycles += 2;
            if (mode == am_zp_x) {
                byte1 += reg_x;
            } else if (mode == am_zp_y) {
                byte1 += reg_y;
            }
            // Overflow past zero page wraps to zero
            return byte1 & 0xFF;
        }

    case am_abs: // ABC abs
    case am_abs_x: // ABC abs, X
    case am_abs_y: // ABC abs, Y
        {
            int byte1 = memory->Read8(reg_pc++);
            if (byte1 < 0) {
                return -1;
            }
            int byte2 = memory->Read8(reg_pc++);
            if (byte2 < 0) {
                return -1;
            }
            std::uint16_t addr1 = byte2 * 0x100 + byte1;
            std::uint16_t addr2 = addr1;
            if (mode == am_abs_x) {
                addr2 += reg_x;
            } else if (mode == am_abs_y) {
                addr2 += reg_y;
            }
            emu_cycles += 2;
            if ((addr2 >> 8) != (addr1 >> 8)) {
                emu_cycles += 1;
            }
            return addr2;
        }

    default: // shouldn't happen
    //am_invalid = -1,
    //am_implied = 0,
    //am_acc,
    //am_ind,
    //am_rel
        return -1;
    }
}

void
CPU6502Impl::Compare(std::uint8_t reg, std::uint8_t byte)
{
    int result = reg + (byte ^ 0xFF) + 1;
    SetNZ(result);
    SetC(result > 0xFF);
}

void
CPU6502Impl::PushByte(std::uint8_t byte)
{
    std::uint16_t address = 0x0100 + --reg_s;
    memory->Write8(address, byte);
}

std::uint8_t
CPU6502Impl::PopByte(void)
{
    std::uint16_t address = 0x0100 + reg_s++;
    return memory->Read8(address);
}

void
CPU6502Impl::SetNZ(std::uint8_t result)
{
    reg_flags = (reg_flags & 0x7F) | (result & 0x80);
    SetZ(result);
}

void
CPU6502Impl::SetZ(std::uint8_t result)
{
    if (result == 0) {
        reg_flags |= 0x02;
    } else {
        reg_flags &= 0xFD;
    }
}

void
CPU6502Impl::SetV(bool overflow)
{
    if (overflow) {
        reg_flags |= 0x40;
    } else {
        reg_flags &= 0xBF;
    }
}

void
CPU6502Impl::SetC(bool carry)
{
    if (carry) {
        reg_flags |= 0x01;
    } else {
        reg_flags &= 0xFE;
    }
}

//////////////////////////////////////////////////////////////////////////////
//                           Begin assembler code                           //
//////////////////////////////////////////////////////////////////////////////

static AddrMode parseOperand(std::string const &operand, std::string &address);
static AddrMode checkAddress(AddrMode mode, const std::string& address);

CPU::Assem
CPU6502Impl::Assemble(std::uint64_t pc, const std::string& code) const
{
    // Return this if assembly fails
    static const CPU::Assem error(false);

    // Isolate the instruction mnemonic
    std::size_t start = 0;
    while (start < code.size() && std::isspace(code.at(start))) {
        ++start;
    }
    std::size_t len = 0;
    while (start+len < code.size() && !std::isspace(code.at(start+len))) {
        ++len;
    }
    if (len == 0) {
        return error;
    }
    auto instr = code.substr(start, len);
    for (std::size_t i = 0; i < instr.size(); ++i) {
        instr[i] = std::toupper(instr[i]);
    }
    auto operand = code.substr(start+len);

    // Parse the operand
    std::string address;
    auto mode = parseOperand(operand, address);
    // Handle cases where an instruction does not support both zero page and
    // absolute
    AddrMode alt_mode = am_invalid;
    auto addr_num = std::strtoul(address.c_str(), NULL, 16);
    switch (mode) {
    case am_abs_x:
        if (address.size() <= 2) {
            mode = am_zp_x;
            alt_mode = am_abs_x;
        } else if (addr_num < 0x100) {
            alt_mode = am_zp_x;
        }
        break;

    case am_abs_y:
        if (address.size() <= 2) {
            mode = am_zp_y;
            alt_mode = am_abs_y;
        } else if (addr_num < 0x100) {
            alt_mode = am_zp_y;
        }
        break;

    case am_abs:
        if (address.size() <= 2) {
            mode = am_zp;
            alt_mode = am_abs;
        } else if (addr_num < 0x100) {
            alt_mode = am_zp;
        }
        break;

    default:
        break;
    }

    // Find the instruction in the table
    int opcode = FindOpcode(instr, mode);
    if (opcode < 0 && alt_mode != am_invalid) {
        opcode = FindOpcode(instr, alt_mode);
        if (opcode >= 0) {
            mode = alt_mode;
        } else if (mode == am_abs || mode == am_zp) {
            // Relative branches
            opcode = FindOpcode(instr, am_rel);
            if (opcode >= 0) {
                mode = am_rel;
            }
        }
    }

    if (opcode < 0) {
        return error; // Instruction not recognized
    }

    // Build the return record
    CPU::Assem assem(true);
    assem.bytes.push_back(opcode);

    switch (mode) {
    case am_invalid:
    default:
        return error;

    // No bytes follow
    case am_implied:
    case am_acc:
        break;

    // One byte follows
    case am_immediate:
    case am_zp_x:
    case am_zp_y:
    case am_zp:
    case am_ind_x:
    case am_ind_y:
        assem.bytes.push_back(addr_num);
        break;

    // Two bytes follow
    case am_abs_x:
    case am_abs_y:
    case am_abs:
    case am_ind:
        assem.bytes.push_back(addr_num & 0xFF);
        assem.bytes.push_back(addr_num >> 8);
        break;

    // Relative branch
    case am_rel:
        {
            int offset = addr_num - (pc + 2);
            if (offset < -128 || offset > +127) {
                return error;
            }
            assem.bytes.push_back(offset & 0xFF);
        }
        break;
    }

    return assem;
}

// Scan the instructions table for the instruction and mode
int
CPU6502Impl::FindOpcode(std::string const &instr, AddrMode mode)
{
    for (int i = 0; i < 256; ++i) {
        if (instr == instructions[i].name && mode == instructions[i].addr_mode) {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////

static AddrMode
parseOperand(std::string const &operand, std::string &address)
{
    // Delete whitespace and convert to uppercase
    std::vector<char> buf(operand.size()+1);
    std::size_t j = 0;
    for (std::size_t i = 0; i < operand.size(); ++i) {
        char ch = operand.at(i);
        if (!std::isspace(ch)) {
            buf[j++] = std::toupper(ch);
        }
    }
    buf[j] = '\0';
    std::string op2(&buf[0]);

    // Check for some simple cases
    if (op2 == "") {
        address = "";
        return checkAddress(am_implied, address);
    }
    if (op2 == "A") {
        address = "";
        return checkAddress(am_acc, address);
    }

    std::smatch match;

    // Immediate mode
    std::regex imm_regex(R"-(#\$?([0-9A-F]+))-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, imm_regex)) {
        address = match[1];
        return checkAddress(am_immediate, address);
    }

    // Absolute-X and zero-page-X mode
    std::regex abs_x_regex(R"-(^\$?([0-9A-F]+),X)-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, abs_x_regex)) {
        address = match[1];
        return checkAddress(am_abs_x, address);
    }

    // Absolute-Y and zero-page-Y mode
    std::regex abs_y_regex(R"-(^\$?([0-9A-F]+),Y)-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, abs_y_regex)) {
        address = match[1];
        return checkAddress(am_abs_y, address);
    }

    // Absolute and zero-page mode
    std::regex abs_regex(R"-(^\$?([0-9A-F]+))-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, abs_regex)) {
        address = match[1];
        return checkAddress(am_abs, address);
    }

    // Indirect-X mode
    std::regex ind_x_regex(R"-(^\(\$?([0-9A-F]+),X\)$)-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, ind_x_regex)) {
        address = match[1];
        return checkAddress(am_ind_x, address);
    }

    // Indirect-Y mode
    std::regex ind_y_regex(R"-(^\(\$?([0-9A-F]+)\),Y$)-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, ind_y_regex)) {
        address = match[1];
        return checkAddress(am_ind_y, address);
    }

    // Indirect mode
    std::regex ind_regex(R"-(^\(\$?([0-9A-F]+)\)$)-",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    if (std::regex_match(op2, match, ind_regex)) {
        address = match[1];
        return checkAddress(am_ind, address);
    }

    address = "";
    return am_invalid;
}

// Check address and return am_invalid if out of range
static AddrMode
checkAddress(AddrMode mode, const std::string& address)
{
    switch (mode) {
    case am_abs_x:
    case am_abs_y:
    case am_abs:
    case am_ind:
    case am_rel:
        {
            auto a = std::strtoul(address.c_str(), NULL, 16);
            return a > 0xFFFF ? am_invalid : mode;
        }

    case am_immediate:
    case am_zp_x:
    case am_zp_y:
    case am_zp:
    case am_ind_x:
    case am_ind_y:
        {
            auto a = std::strtoul(address.c_str(), NULL, 16);
            return a > 0xFF ? am_invalid : mode;
        }

    default:
        return address.empty() ? mode : am_invalid;
    }
}

}
