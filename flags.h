// flags.h

#ifndef FLAGS_H
#define FLAGS_H

#include <wx/wx.h>
#include <string>
#include <vector>
#include "cpu.h"

class FlagsDialog : public wxDialog {
public:
  	FlagsDialog(wxWindow *parent, wxWindowID id, const wxString &title,
                const wxPoint &pos=wxDefaultPosition,
                const wxSize &size=wxDefaultSize,
                long style=wxDEFAULT_DIALOG_STYLE,
                const wxString &name=wxDialogNameStr);

    void SetFlags(CPU *cpu, const std::string& flags);
    std::string GetFlags(void) const;

private:
    std::vector<CPU::Flag> m_flags;
    std::vector<wxCheckBox *> m_checkboxes;
};

#endif // FLAGS_H
