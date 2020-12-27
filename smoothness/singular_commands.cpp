#include "singular_commands.hpp"

  void init_singular (std::string const& library_path)
  {
    if (currPack == NULL)
    {
      mp_set_memory_functions (omMallocFunc, omReallocSizeFunc, omFreeSizeFunc);
      siInit (const_cast<char*> (library_path.c_str()));
      currentVoice = feInitStdin (NULL);
      errorreported = 0;
      myynest = 1;
    }
  }

  void call_singular (std::string const& command)
  {
    int err = iiAllStart
      (NULL, const_cast<char*> (command.c_str()), BT_proc, 0);
    if (err)
    {
      errorreported = 0;
      throw std::runtime_error ("Singular returned an error ...");
    }
  }

  void call_singular_and_discard (std::string const& command)
  {
    SPrintStart();
    call_singular (command);
    char* result_ptr = SPrintEnd();
    omFree (result_ptr);
  }

  std::string get_singular_result (std::string const& command)
  {
    SPrintStart();
    call_singular (command);
    char* result_ptr = SPrintEnd();
    std::string result (result_ptr);
    omFree (result_ptr);
    if (result.size() > 0)
    {
      result.pop_back();
    }
    return result;
  }

  void dechain_handle (idhdl h, idhdl* ih)
  {
    idhdl hh;
    if (IDID (h) != NULL)
    {
      omFree (static_cast<ADDRESS> (const_cast<char*> (IDID (h))));
    }
    IDID (h) = NULL;
    IDDATA(h) = NULL;
    if (h == (*ih))
    {
      // h is at the beginning of the list
      *ih = IDNEXT(h) /* ==*ih */;
    }
    else if (ih != NULL)
    {
      // h is somethere in the list:
      hh = *ih;
      while (true)
      {
        if (hh == NULL)
        {
          //PrintS(">>?<< not found for kill\n");
          return;
        }
        idhdl hhh = IDNEXT (hh);
        if (hhh == h)
        {
          IDNEXT (hh) = IDNEXT (hhh);
          break;
        }
        hh = hhh;
      }
    }
    omFreeBin (static_cast<ADDRESS> (h), idrec_bin);
  }

  poly singular_determinant (matrix m)
  {
    // This is basically duplicated from jjDET in Singular/iparith.cc
    poly p = mp_Det (m, currRing);
    return p;
  }

  bool singular_check_proc (std::string const& proc_name)
  {
    idhdl h (IDROOT);
    while (h != NULL)
    {
      if (proc_name == h->id)
      {
        return true;
      }
      h = IDNEXT (h);
    }
    return false;
  }

  void singular_load_library (std::string const& library_name)
  {
    std::stringstream load_command;
    load_command << "LIB \"" << library_name << "\"; return();";
    call_singular_and_discard (load_command.str());
  }

  void singular_load_ssi (std::string const& symbol_name, std::string const& file_name)
  {
    std::stringstream load_command;
    load_command << "link l = \"ssi:r " << file_name << "\"; def "
      << symbol_name << " = read(l); close(l); kill l; return();";
    call_singular_and_discard (load_command.str());
  }

  void singular_write_ssi (std::string const& symbol_name, std::string const& file_name)
  {
    std::stringstream write_command;
    write_command << "link l = \"ssi:w " << file_name << "\"; write(l,"
      << symbol_name << "); close(l); kill l; return();";
    call_singular_and_discard (write_command.str());
  }

  void singular_load_rms()
  {
    std::string rms_proc_string ("proc radicalMemberShip (poly f,ideal i)\n"
    "\"""USAGE:  radicalMemberShip (f,i); f poly, i ideal\nRETURN:  int, 1 if "
    "f is in the radical of i, 0 else\nEXAMPLE:     example radicalMemberShip;"
    "   shows an example\"\n{\n  def BASERING=basering;\n  execute(\"ring"
    " RADRING=(\"+charstr(basering)+\"),(@T,\"+varstr(basering)+\"),(dp(1),\""
    "+ordstr(basering)+\");\");\n  ideal I=ideal(imap(BASERING,i))+ideal(1-@T"
    "*imap(BASERING,f));\n  if (reduce(1,std(I))==0)\n  {\n    return(1);\n  }"
    "\n  else\n  {\n    return(0);\n  }\n}\nreturn();");
    call_singular_and_discard (rms_proc_string);
  }

  // as in libpolys/polys/simpleideals.cc, but do not introduce more zeroes
  BOOLEAN idInsertPoly_inc1 (ideal h1, poly h2)
  {
    if (h2==NULL) return FALSE;
    assume (h1 != NULL);

    int j = IDELEMS(h1) - 1;

    while ((j >= 0) && (h1->m[j] == NULL)) j--;
    j++;
    if (j==IDELEMS(h1))
    {
      pEnlargeSet(&(h1->m),IDELEMS(h1),1);
      IDELEMS(h1)+=1;
    }
    h1->m[j]=h2;
    return TRUE;
  }

  bool is_ideal_zero (ideal id)
  {
    if (id == NULL)
    {
      return true;
    }
    for (int i = 0; i < IDELEMS (id); ++i)
    {
      if (id->m[i] != NULL)
      {
        return false;
      }
    }
    return true;
  }

  // as in lDelete; does not modify argument
  lists delete_from_list (lists l, int index)
  {
    lists temp_list = lCopy (l);
    int last_index = l->nr;
    int last_used_index = lSize(l);
    if ((index < 0) || (index > last_index))
    {
      return temp_list;
    }
    int new_list_size = last_used_index;
    if (index > last_used_index)
    {
      ++new_list_size;
    }
    lists new_list = static_cast<lists> (omAllocBin (slists_bin));
    new_list->Init (new_list_size);
    int i, j;
    for (i = 0, j = 0; i <= last_used_index; ++i, ++j)
    {
      if (i == index)
      {
        temp_list->m[i].CleanUp();
        --j;
      }
      else
      {
        new_list->m[j] = temp_list->m[i];
        memset (&temp_list->m[i], 0, sizeof (temp_list->m[i]));
      }
    }
    omFreeSize (static_cast<ADDRESS> (temp_list->m),
      ((temp_list->nr+1) * sizeof (sleftv)));
    omFreeBin (static_cast<ADDRESS> (temp_list), slists_bin);
    return new_list;
  }

  // this comes from mpJAcobi in Singular/ipshell.cc
  matrix jacobi_matrix_of_ideal (ideal id, ring R)
  {
    int i, j;
    matrix result = mpNew (IDELEMS (id), rVar (R));
    for (i = 1; i <= IDELEMS (id); i++)
    {
      for (j = 1; j <= rVar (R); j++)
      {
        MATELEM (result, i, j) = p_Diff (id->m[i-1], j, R);
      }
    }
    return result;
  }

  // as in jjRANDOM, gives int in [-max_abs, ..., max_abs]
  int draw_random_integer (const int max_abs)
  {
    return siRand() % (2*max_abs + 1) - max_abs;
  }
