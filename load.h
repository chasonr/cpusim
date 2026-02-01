// load.h

#ifndef LOAD_H
#define LOAD_H

class Memory;
class wxWindow;

// Returns negative if no memory changed, else starting address
extern int Load(wxWindow *parent, Memory *memory);

#endif
