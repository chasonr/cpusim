// load.cpp

#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/filedlgcustomize.h>
#include <fstream>
#include <string>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "load.h"
#include "memory.h"

// We need additional controls to specify the load address
class LoadDialogHook : public wxFileDialogCustomizeHook {
public:
  	LoadDialogHook() :
            m_from_file(nullptr),
            m_at_address(nullptr),
            m_address(nullptr),
            m_address_val(0),
            m_at_address_val(true),
            m_from_file_val(false)
    {
    }

    void AddCustomControls(wxFileDialogCustomize& customizer) override
    {
        m_from_file = customizer.AddRadioButton("From &file");
        m_from_file->SetValue(false);
        m_at_address = customizer.AddRadioButton("At &address");
        m_at_address->SetValue(true);
        m_address = customizer.AddTextCtrl("A&ddress");
        m_address->SetValue("0");
    }

    void TransferDataFromCustomControls() override
    {
        auto str = m_address->GetValue();
        m_address_val = std::strtoul(str.c_str(), NULL, 16);
        m_at_address_val = m_at_address->GetValue();
        m_from_file_val = m_from_file->GetValue();
    }

    bool AtAddress() { return m_at_address_val; }
    bool FromFile() { return m_from_file_val; }
    unsigned Address() { return m_address_val; }

private:
    wxFileDialogRadioButton *m_from_file;
    wxFileDialogRadioButton *m_at_address;
    wxFileDialogTextCtrl *m_address;
    unsigned m_address_val;
    bool m_at_address_val;
    bool m_from_file_val;
};

// Returns negative if no memory changed, else starting address
int
Load(wxWindow *parent, Memory *memory)
{
    wxFileDialog loadDlg(parent, "Load binary file");
    LoadDialogHook loadDlgHook;
    loadDlg.SetCustomizeHook(loadDlgHook);
    if (loadDlg.ShowModal() == wxID_CANCEL) {
        return -1;
    }

    unsigned start = loadDlgHook.Address();

    auto name = loadDlg.GetPath();

    unsigned addr = start;
    std::ifstream fp(name, std::ios::binary);
    try {
        fp.exceptions(std::ifstream::failbit);
        // Use address at start of file if requested
        if (fp.good() && loadDlgHook.FromFile()) {
            char ch1 = 0, ch2 = 0;
            fp.get(ch1);
            fp.get(ch2);
            start = (std::uint8_t)ch1 | ((std::uint8_t)ch2 << 8);
            addr = start;
        }

        while (fp.good() && addr < 0x10000) {
            char ch;
            fp.get(ch);
            if (!fp.good()) {
                break;
            }
            memory->Load8(addr, (std::uint8_t)ch);
            ++addr;
        }
    }
    catch (const std::ios_base::failure& fail) {
        if (!fp.eof()) {
            wxMessageBox(
                    name + std::string(": ") + std::strerror(errno), 
                    "Load failed");
        }
    }

    return (addr > start) ? start : -1;
}
