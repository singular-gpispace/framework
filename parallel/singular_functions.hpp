#pragma once

#include <string>

#include <Singular/libsingular.h>
#include <Singular/links/ssiLink.h> // for ssiInfo etc.
#include <Singular/newstruct.h>

// Singular defines this in ssiLink.cc
#define SSI_VERSION 13

// these are from ssiLink.cc
char* ssiReadString(const ssiInfo *d);
void ssiWriteRing_R (ssiInfo *d, const ring r);
void ssiWriteIdeal_R (const ssiInfo *d, int typ,const ideal I, const ring R);
void ssiWriteList (si_link l, lists dd);
void ssiWriteIntmat(const ssiInfo *d,intvec * v);

// these are from newstruct.cc
BOOLEAN newstruct_deserialize(blackbox **, void **d, si_link f);
BOOLEAN newstruct_serialize(blackbox *b, void *d, si_link f);

void init_singular (std::string const&);
void load_singular_library (std::string const&);
bool register_struct (std::string const&, std::string const&);
si_link ssi_open_for_read (std::string const&);
si_link ssi_open_for_write (std::string const&);
void ssi_close_and_remove (si_link);
void ssi_write_newstruct (si_link, std::string const&, lists);
void ssi_write_newstruct (si_link, int, lists);
lists ssi_read_newstruct (si_link, std::string const&);
std::pair<int, lists> call_user_proc
  (std::string const&, std::string const&, int, lists);
void ssi_write_ring (si_link, ring, bool = false);
void ssi_write_ideal (si_link, ring, ideal);
void ssi_write_list (si_link, lists);
void ssi_write_intmat (si_link, intvec*);
