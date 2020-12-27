#include <Singular/libsingular.h>
#include <kernel/oswrapper/timer.h>

#include <share/include/interface.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <type_traits>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include <gen/pnetc/type/smoothness_task.hpp>
#include <gen/pnetc/type/jacobi_task.hpp>
#include <gen/pnetc/type/jacobi_list.hpp>
#include <we/type/value/wrap.hpp>
#include <we/type/value/unwrap.hpp>
#include <pnetc/type/jacobi_task/op.hpp>

#include "config.hpp"
#include "singular_commands.hpp"

namespace
{
  class combination_generator
  {
    public:
      combination_generator (unsigned long num_vars, unsigned long num_amb) :
        n (num_vars), k (num_amb), positions (k + 1)
      {
        std::iota (positions.begin(), positions.begin() + k, 0);
        positions[k] = n;
      }
      std::vector<unsigned long> get_combination()
      {
        return positions;
      }
      bool next();
      bool is_contained (unsigned long j);
    private:
      unsigned long n;
      unsigned long k;
      std::vector<unsigned long> positions;
  };

  bool combination_generator::next()
  {
    unsigned long j = 0;
    while ((j <= k - 1) && (positions[j] + 1 == positions[j + 1]))
    {
      positions[j] = j;
      ++j;
    }
    if (j > k - 1)
    {
      positions[k] = n;
      return false;
    }
    else
    {
      ++positions[j];
      return true;
    }
  }

  bool combination_generator::is_contained (unsigned long j)
  {
    for (unsigned long i = 0; i < k; ++i)
    {
      if (positions[i] == j)
      {
        return true;
      }
    }
    return false;
  }

  class sglogger
  {
    private:
      std::ofstream os;
      std::string _type;
      int log_level;
    public:
      sglogger (std::string const& fn_base, std::string const& type, int ll,
        bool do_not_cut_fn = false);
      ~sglogger();
      void log (std::string const& msg, int level);
  };

  sglogger::sglogger (std::string const& fn_base, std::string const& type,
    int ll, bool do_not_cut_fn)
  {
    _type = type;
    log_level = ll;
    if (log_level <= 0)
    {
      return;
    }
    std::stringstream log_fn;
    size_t dot_pos = fn_base.rfind (".");
    if (do_not_cut_fn || dot_pos == std::string::npos)
    {
      log_fn << fn_base;
    }
    else
    {
      log_fn << fn_base.substr (0, dot_pos);
    }
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname (hostname, 1023);
    log_fn << '.' << hostname << '.' << getpid() << ".log";
    os.open (log_fn.str(), std::ios_base::app);
    log ("START " + _type, 1);
  }

  sglogger::~sglogger()
  {
    log ("END " + _type, 1);
  }

  void sglogger::log (std::string const& msg, int level)
  {
    if (level > log_level)
    {
      return;
    }
    omUpdateInfo();
    std::chrono::system_clock::time_point current_time
      = std::chrono::system_clock::now();
    std::size_t ns = current_time.time_since_epoch().count();

    os << (ns / 1000000000UL) << '.' << std::setw(9) << std::setfill('0')
      << (ns % 1000000000UL) << ' ' << getTimer() << ' ' << om_Info.UsedBytes
      << ' ' << om_Info.CurrentBytesSystem << ' ' << om_Info.MaxBytesSystem
      << ' ' << msg << std::endl;
  }
}


NO_NAME_MANGLING
void singular_smoothness_trivial ( pnetc::type::smoothness_task::smoothness_task& task
                                 , int const& logging_level
                                 )
{
  //std::cout << "start trivial\n";
  init_singular (config::library().string());
  sglogger logger (task.variety_ideal, "trivial", logging_level);
  logger.log (task.ambient_ideal, 2);

  singular_load_ssi ("IX", task.variety_ideal);
  singular_load_ssi ("IW", task.ambient_ideal);
  singular_load_ssi ("g", task.exclusion_poly);

  idhdl exclusion_poly_handle = ggetid ("g");
  poly exclusion_poly = IDPOLY (exclusion_poly_handle);
  //if (exclusion_poly_file.empty())
  if (p_IsUnit (exclusion_poly, currRing))
  {
    std::string reduce_ideal_command ("ideal J = NF(IX, std(IW)); return();");
    call_singular_and_discard (reduce_ideal_command);
    //call_singular ("IX; IW; J; return();");
    idhdl reduced_handle = ggetid ("J");
    ideal reduced_ideal = IDIDEAL (reduced_handle);
    task.flag = (idElem (reduced_ideal) == 0);
    //printf ("idelem: %d\n", idElem (reduced_ideal));
  }
  else
  {
    if (singular_check_proc ("sat") == false)
    {
      singular_load_library ("elim.lib");
    }
    std::string saturation_command
      ("list sl = sat(IW,g); ideal J = NF(IX, std(sl[1])); kill sl; return();");
    call_singular_and_discard (saturation_command);
    idhdl reduced_handle = ggetid ("J");
    ideal reduced_ideal = IDIDEAL (reduced_handle);
    task.flag = (idElem (reduced_ideal) == 0);
  }

  std::string clean_command
    ("kill J; kill g; kill IX; kill IW; return();");
  call_singular_and_discard (clean_command);
  //std::cout << "end trivial\n";
}

NO_NAME_MANGLING
void singular_smoothness_delta ( pnetc::type::smoothness_task::smoothness_task& task
                               , std::string const& heureka_file
                               , int const& logging_level
                               )
{
  //std::cout << "start delta\n";
  init_singular (config::library().string());
  sglogger logger (task.variety_ideal, "delta", logging_level);
  logger.log (task.ambient_ideal, 2);


  singular_load_ssi ("IX", task.variety_ideal);
  singular_load_ssi ("IW", task.ambient_ideal);
  singular_load_ssi ("g", task.exclusion_poly);

  unsigned long num_vars = currRing->N;
  //printf ("num vars: %lu\n", num_vars);
  idhdl ambient_handle = ggetid ("IW");
  ideal ambient_ideal = IDIDEAL (ambient_handle);
  idhdl variety_handle = ggetid ("IX");
  ideal variety_ideal = IDIDEAL (variety_handle);
  idhdl exclusion_handle = ggetid ("g");
  poly g = IDPOLY (exclusion_handle);

  if ((idElem (ambient_ideal) == 0) && p_IsUnit (g, currRing)) // boundary case
  {
    bool result;
    std::string jacob_command ("ideal J = IX, jacob(IX); J = std(J); return();");
    call_singular_and_discard (jacob_command);
    idhdl delta_handle = ggetid ("J");
    ideal delta_ideal = IDIDEAL (delta_handle);
    if ((delta_ideal->ncols > 0) && p_IsUnit (delta_ideal->m[0], currRing))
    {
      logger.log ("boundary case: true", 3);
      result = true;
    }
    else
    {
      logger.log ("boundary case: false", 3);
      result = false;
    }
    std::string cleanup_command ("kill J; kill IW, IX, g; return();");
    call_singular_and_discard (cleanup_command);
    task.flag = result;
    //std::cout << "end delta\n";
    return;
  }

  unsigned long num_ambient = ambient_ideal->ncols;
  unsigned long num_variety = variety_ideal->ncols;
  if (num_ambient > num_vars)
  {
    throw std::runtime_error ("too many polys in ambient ideal");
  }
  bool delta_result (true);
  call_singular_and_discard ("matrix DIW = jacob(IW); return();");
  idhdl jacob_handle = ggetid ("DIW");
  matrix jacob_matrix = IDMATRIX (jacob_handle);
  combination_generator cg (num_vars, num_ambient);
  {
    std::stringstream log_ss;
    log_ss << "will consider " << num_vars << " over " << num_ambient
      << " combinations";
    logger.log (log_ss.str(), 3);
  }
  unsigned long combination_count = 0;
  ideal Q = idInit (1);
  if (singular_check_proc ("adjoint") == false)
  {
    singular_load_library ("linalg.lib");
  }
  if (singular_check_proc ("radicalMemberShip") == false)
  {
    singular_load_rms();
  }
   ideal variety_ideal_std = kStd (variety_ideal, currRing->qideal, testHomog, NULL);
  do
  {
    //printf ("combination:");
    //for (unsigned long blubb = 0; blubb <= num_ambient; ++blubb)
    //{
      //printf (" %lu", cg.get_combination().at (blubb));
    //}
    //printf ("\n");
    matrix sub_matrix = static_cast<matrix> (omAllocBin (sip_sideal_bin));
    sub_matrix->nrows = num_ambient;
    sub_matrix->ncols = num_ambient;
    sub_matrix->rank = num_ambient;
    sub_matrix->m = static_cast<poly*>
      (omAlloc0 (num_ambient*num_ambient*sizeof(poly)));
    for (unsigned long i = 0; i < num_ambient; ++i)
    {
      for (unsigned long j = 0; j < num_ambient; ++j)
      {
        MATELEM (sub_matrix, i + 1, j + 1)
          = MATELEM (jacob_matrix, i + 1, cg.get_combination().at (j) + 1);
      }
    }
    idhdl sub_matrix_handle = enterid ("M", 1, MATRIX_CMD, &(currRing->idroot), FALSE);
    IDMATRIX (sub_matrix_handle) = sub_matrix;
    poly q = singular_determinant (sub_matrix);
    //printf ("q: ");
    //p_Write (q, currRing);
    if (q != NULL)
    {
      poly q_red = kNF (variety_ideal_std, currRing->qideal, q);
      bool q_inside = (q_red == NULL);
      if (!q_inside)
      {
        logger.log ("q is not in IX", 4);
      }
      else
      {
        SPrintStart();
        p_Write (q, currRing);
        char* result_ptr = SPrintEnd();
        std::string result (result_ptr);
        omFree (result_ptr);
        logger.log ("q = " + result + " is in IX", 4);
      }
      p_Delete (&q_red, currRing);
      idInsertPoly_inc1 (Q, p_Copy (q, currRing));
      call_singular_and_discard ("matrix A = adjoint(M); return();");
      idhdl adjoint_matrix_handle = ggetid ("A");
      matrix adjoint_matrix = IDMATRIX (adjoint_matrix_handle);
      ideal CM = idCopy (variety_ideal);
      for (unsigned long i = 0; i < num_variety; ++i)
      {
        for (unsigned long j = 0; j < num_vars; ++j)
        {
          //printf ("i=%lu j=%lu\n",i,j);
          if (cg.is_contained (j))
          {
            //printf ("skip j\n");
            continue;
          }
          //printf ("i=%lu j=%lu\n",i,j);
          poly p = p_Mult_q (p_Copy (q, currRing),
            p_Diff (variety_ideal->m[i], j+1, currRing) , currRing);
          //printf ("first mult done\n");
          for (unsigned long k = 0; k < num_ambient; ++k)
          {
            for (unsigned long l = 0; l < num_ambient; ++l)
            {
              unsigned long kk = cg.get_combination().at (l);
              //printf ("k=%lu l=%lu kk=%lu\n",k,l,kk);
              poly p2 = p_Mult_q (p_Diff (ambient_ideal->m[l], j+1, currRing),
                p_Copy (MATELEM (adjoint_matrix, k+1, l+1), currRing),
                currRing);
              //printf ("first p2 done\n");
              p2 = p_Mult_q (p2, p_Diff (variety_ideal->m[i], kk+1, currRing),
                currRing);
              //printf ("second p2 done\n");
              p = p_Sub (p, p2, currRing);
              //printf ("sub done\n");
            }
          }
          idInsertPoly_inc1 (CM, p);
        }
      }
      idSkipZeroes (CM);
      //iiWriteMatrix (reinterpret_cast<matrix> (CM), "CM", 1, currRing, 0);
      //printf ("\n");
      idhdl CM_handle =
        enterid ("CM", 1, IDEAL_CMD, &(currRing->idroot), FALSE);
      IDIDEAL (CM_handle) = CM;
      poly check_poly =
        p_Mult_q (p_Copy(q, currRing), p_Copy(g, currRing), currRing);
      idhdl check_poly_handle =
        enterid ("cp", 1, POLY_CMD, &(currRing->idroot), FALSE);
      IDPOLY (check_poly_handle) = check_poly;
      call_singular_and_discard ("int cr = radicalMemberShip(cp, CM); return();");
      idhdl check_result_handle = ggetid ("cr");
      int cr = IDINT (check_result_handle);
      //printf ("checkresult: %d\n", cr);
      if (cr == 0)
      {
        delta_result = false;
        std::stringstream log_ss;
        log_ss << "combination " << combination_count << " has false";
        logger.log (log_ss.str(), 3);
      }
      else
      {
        std::stringstream log_ss;
        log_ss << "combination " << combination_count << " has true";
        logger.log (log_ss.str(), 4);
      }
      call_singular_and_discard ("kill cr, cp, CM, A; return();");
    }
    else
    {
      std::stringstream log_ss;
      log_ss << "combination " << combination_count << " has zero det";
      logger.log (log_ss.str(), 4);
    }
    dechain_handle (sub_matrix_handle, &(currRing->idroot));
    pDelete (&q);
    omFree (sub_matrix->m);
    omFreeBin (sub_matrix, sip_sideal_bin);
    ++combination_count;
    if (std::ifstream (heureka_file).good())
    {
      logger.log ("heureka detected, going home", 3);
      break;
    }
  }
  while (delta_result && cg.next());

  idDelete (&Q);
  idDelete (&variety_ideal_std);
  call_singular_and_discard ("kill DIW, g, IW, IX; return();");
  task.flag = delta_result;
  //std::cout << "end delta\n";
}

/*static void logg (const std::string&  fn, const std::string& message)
{
  omUpdateInfo();
  std::ofstream (fn, std::ios_base::app) << time (NULL) << ' ' << getTimer()
    << ' ' << om_Info.UsedBytes << ' ' << om_Info.CurrentBytesSystem << ' '
    << om_Info.MaxBytesSystem << ' ' << message << '\n';
}*/

static std::string get_combination_string (std::vector<unsigned long> const& v)
{
  std::stringstream ss;
  for (unsigned long e : v)
  {
    ss << ' ' << e;
  }
  return ss.str();
}

static bool check_jacobi_cover ( unsigned long first, unsigned long last
                               , ideal variety_ideal
                               , poly exclusion_poly
                               , std::vector<std::pair<int,poly>> const& minor_polys
                               )
{
  unsigned long num_variety = variety_ideal->ncols;
  unsigned long ideal_length = num_variety + (last - first);
  ideal check_ideal = idInit (ideal_length);
  for (unsigned long i = 0; i < num_variety; ++i)
  {
    check_ideal->m[i] = variety_ideal->m[i];
  }
  for (unsigned long i = 0; i < (last - first); ++i)
  {
    check_ideal->m[num_variety + i] = minor_polys.at (first + i).second;
  }
  ideal check_ideal_std = kStd (check_ideal, currRing->qideal, testHomog, NULL);
  poly tmp_reduced = kNF (check_ideal_std, currRing->qideal, exclusion_poly);
  bool res = (tmp_reduced == NULL);
  p_Delete (&tmp_reduced, currRing);
  idDelete (&check_ideal_std);
  omFree (check_ideal->m);
  omFreeBin (check_ideal, sip_sideal_bin);

  //std::cout << "tested " << first << " to " << last << " with "
  //  << std::boolalpha << res << std::endl;
  return res;
}

static std::list<int> get_small_jacobi_cover ( ideal variety_ideal
                                             , poly exclusion_poly
                                             , std::vector<std::pair<int,poly>> const& minor_polys
                                             )
{
  unsigned long num_polys = minor_polys.size();
  unsigned long left = 0;
  unsigned long right = num_polys;
  while (left + 1 < right)
  {
    unsigned long c = (left + right) / 2;
    if (check_jacobi_cover (0, c, variety_ideal, exclusion_poly, minor_polys))
    {
      right = c;
    }
    else
    {
      left = c;
    }
  }
  unsigned long right_limit = right;
  left = 0;
  while (left + 1 < right)
  {
    unsigned long c = (left + right) / 2;
    if (check_jacobi_cover (c, right_limit, variety_ideal, exclusion_poly, minor_polys))
    {
      left = c;
    }
    else
    {
      right = c;
    }
  }
  unsigned long left_limit = left;
  std::list<int> result_list;
  for (unsigned long i = left_limit; i < right_limit; ++i)
  {
    result_list.push_back (minor_polys.at (i).first);
  }
  return result_list;
}

NO_NAME_MANGLING
void singular_smoothness_jacobi ( pnetc::type::jacobi_task::jacobi_task& task
                                , std::string const& heureka_file
                                , int const& logging_level
                                )
{
  //std::cout << "start Jacobi\n";
  init_singular (config::library().string());

  sglogger logger (task.sm_task.variety_ideal, "jacobi", logging_level);

  singular_load_ssi ("IX", task.sm_task.variety_ideal);
  singular_load_ssi ("IW", task.sm_task.ambient_ideal);
  singular_load_ssi ("g", task.sm_task.exclusion_poly);
  //std::string log_filename (task.sm_task.ambient_ideal + ".log");
  //char hostname[1024];
  //hostname[1023] = '\0';
  //gethostname(hostname, 1023);
  //std::stringstream log_head;
  //log_head << "Host: " << hostname << " PID: " << getpid();
  //logg (log_filename, log_head.str());
  //logg (log_filename, "start Jacobi");

  unsigned long num_vars = currRing->N;

  idhdl ambient_handle = ggetid ("IW");
  ideal ambient_ideal = IDIDEAL (ambient_handle);
  idhdl variety_handle = ggetid ("IX");
  ideal variety_ideal = IDIDEAL (variety_handle);
  idhdl exclusion_handle = ggetid ("g");
  poly g = IDPOLY (exclusion_handle);

  matrix jacobi_iw = jacobi_matrix_of_ideal (ambient_ideal, currRing);

  unsigned long num_ambient = ambient_ideal->ncols;
  //unsigned long num_variety = variety_ideal->ncols;

  combination_generator cg (num_vars, num_ambient);
  for (int i = 0; i < task.combination; ++i)
  {
    cg.next();
  }
  std::vector<unsigned long> combination = cg.get_combination();
  logger.log (task.sm_task.ambient_ideal
    + get_combination_string (combination), 2);

  bool jacobi_result (true);

  if (singular_check_proc ("adjoint") == false)
  {
    singular_load_library ("linalg.lib");
  }
  if (singular_check_proc ("sat") == false)
  {
    singular_load_library ("elim.lib");
  }

  matrix sub_matrix = mpNew (num_ambient, num_ambient);
  for (unsigned long i = 0; i < num_ambient; ++i)
  {
    for (unsigned long j = 0; j < num_ambient; ++j)
    {
      MATELEM (sub_matrix, i + 1, j + 1)
        = MATELEM (jacobi_iw, i + 1, combination.at (j) + 1);
    }
  }
  poly q = singular_determinant (sub_matrix);
  idhdl sub_matrix_handle = enterid ("M", 1, MATRIX_CMD, &(currRing->idroot), FALSE);
  IDMATRIX (sub_matrix_handle) = sub_matrix;
  if (q != NULL)
  {
    ideal variety_ideal_std = kStd (variety_ideal, currRing->qideal, testHomog, NULL);
    poly tmp_reduced = kNF (variety_ideal_std, currRing->qideal, q);
    bool res = (tmp_reduced == NULL);
    p_Delete (&tmp_reduced, currRing);
    idDelete (&variety_ideal_std);
    if (res)
    {
      SPrintStart();
      p_Write (q, currRing);
      char* result_ptr = SPrintEnd();
      std::string result (result_ptr);
      omFree (result_ptr);
      logger.log ("NOTE q is in IX!: " + result, 4);
    }
    //std::cout << "IX has " << variety_ideal->ncols << " entries" << std::endl;

    call_singular_and_discard ("matrix A = adjoint(M); return();");
    idhdl adjoint_matrix_handle = ggetid ("A");
    matrix adjoint_matrix = IDMATRIX (adjoint_matrix_handle);
    //call_singular_and_discard ("ideal IWstd = std(IW); ideal IXred = simplify(NF(IX,IWstd),2); return();");
    call_singular_and_discard ("ideal IWstd = std(IW); ideal IXred = IX; return();");
    //logger.log (get_singular_result ("IXred; return();"));
    idhdl variety_red_handle = ggetid ("IXred");
    ideal variety_red_ideal = IDIDEAL (variety_red_handle);
    idhdl ambient_ideal_std_handle = ggetid ("IWstd");
    ideal ambient_ideal_std = IDIDEAL (ambient_ideal_std_handle);
    unsigned long num_variety_red = variety_red_ideal->ncols;
    for (unsigned long i = 0; i < num_variety_red; ++i)
    {
      //unsigned int old_length = pLength (variety_red_ideal->m[i]);
      //std::cout << "poly " << i << " has length " << old_length;
      poly tmp_reduced = kNF (ambient_ideal_std, currRing->qideal, variety_red_ideal->m[i]);
      unsigned int new_length = pLength (tmp_reduced);
      //std::cout << ", reduced one has length " << new_length;
      if (new_length == 0)
      {
        //std::cout << ", replace with 0" << std::endl;
        p_Delete (&(variety_red_ideal->m[i]), currRing);
        variety_red_ideal->m[i] = tmp_reduced;
      }
      else
      {
        //std::cout << ", keep" << std::endl;
        p_Delete (&tmp_reduced, currRing);
      }
    }
    idSkipZeroes (variety_red_ideal);
    num_variety_red = variety_red_ideal->ncols;
    //std::cout << "new IXred has " << num_variety_red << " entries" << std::endl;
    matrix jacobi_ix_param = mpNew (num_variety_red, num_vars - num_ambient);
    for (unsigned long i = 0; i < num_variety_red; ++i)
    {
      unsigned long column_counter = 0;
      for (unsigned long j = 0; j < num_vars; ++j)
      {
        if (std::find (combination.begin(), combination.end(), j) != combination.end())
        {
          continue;
        }
        poly p = p_Mult_q (p_Copy (q, currRing),
          p_Diff (variety_red_ideal->m[i], j+1, currRing) , currRing);
        for (unsigned long k = 0; k < num_ambient; ++k)
        {
          for (unsigned long l = 0; l < num_ambient; ++l)
          {
            unsigned long kk = combination.at (k);
            poly p2 = p_Mult_q (p_Diff (ambient_ideal->m[l], j+1, currRing),
              p_Copy (MATELEM (adjoint_matrix, k+1, l+1), currRing),
              currRing);
            p2 = p_Mult_q (p2, p_Diff (variety_red_ideal->m[i], kk+1, currRing),
              currRing);
            p = p_Sub (p, p2, currRing);
          }
        }
        MATELEM (jacobi_ix_param, i+1, column_counter+1) = p;
        ++column_counter;
      }
    }
    idhdl jac_ix_handle = enterid ("JM", 1, MATRIX_CMD, &(currRing->idroot), FALSE);
    IDMATRIX (jac_ix_handle) = jacobi_ix_param;
    std::stringstream jac_cmd_ss;
    //logg (log_filename, "before minor");
    std::stringstream minor_log_ss;
    minor_log_ss << "will now call minor on " << jacobi_ix_param->nrows << "x" << jacobi_ix_param->ncols << "-matrix";
    logger.log (minor_log_ss.str(), 3);
    jac_cmd_ss << "ideal J = IX, minor(JM," << task.sm_task.codimension << ",std(IX)); return();";
    //jac_cmd_ss << "ideal J = IX, minor(JM," << task.sm_task.codimension << "); return();";
    call_singular_and_discard (jac_cmd_ss.str());
    if (std::ifstream (heureka_file).good())
    {
      logger.log ("heureka detected, going home", 3);
      call_singular_and_discard ("kill J, JM, A, IWstd, IXred; return();");
      dechain_handle (sub_matrix_handle, &(currRing->idroot));
      pDelete (&q);
      omFree (sub_matrix->m);
      omFreeBin (sub_matrix, sip_sideal_bin);
      mp_Delete (&jacobi_iw, currRing);
      call_singular_and_discard ("kill g, IW, IX; return();");
      jacobi_result = true;
      return;
    }
    poly check_poly =
      p_Mult_q (p_Copy(q, currRing), p_Copy(g, currRing), currRing);
    idhdl check_poly_handle =
      enterid ("cp", 1, POLY_CMD, &(currRing->idroot), FALSE);
    IDPOLY (check_poly_handle) = check_poly;
    logger.log ("minor done, will now call sat", 3);
    //logg (log_filename, "after minor");
    call_singular_and_discard ("list SIl = sat(J, cp); ideal SI = SIl[1]; return();");
    //logg (log_filename, "after sat");
    logger.log ("sat done", 3);
    idhdl saturation_result_handle = ggetid ("SI");
    ideal saturation_result_ideal = IDIDEAL (saturation_result_handle);
    if ((saturation_result_ideal->ncols == 0)
      || !(p_IsUnit (saturation_result_ideal->m[0], currRing)))
    {
      logger.log ("variety is NOT smooth", 3);
      jacobi_result = false;
    }
    call_singular_and_discard ("kill SIl, SI, cp, J, JM, A, IWstd, IXred; return();");
  }
  dechain_handle (sub_matrix_handle, &(currRing->idroot));
  pDelete (&q);
  omFree (sub_matrix->m);
  omFreeBin (sub_matrix, sip_sideal_bin);

  mp_Delete (&jacobi_iw, currRing);
  call_singular_and_discard ("kill g, IW, IX; return();");
  task.sm_task.flag = jacobi_result;
  //std::cout << "end Jacobi\n";
  //logg (log_filename, "end Jacobi");
}

NO_NAME_MANGLING
void singular_smoothness_descent ( pnetc::type::smoothness_task::smoothness_task const& input_task
                                 , std::list<pnetc::type::smoothness_task::smoothness_task>& output_list
                                 , std::string const& heureka_file
                                 , int const& rnd_coeff_max
                                 , unsigned long const& rnd_tries
                                 , unsigned long const& max_zero_tries
                                 , int const& logging_level
                                 )
{
  //std::cout << "start descent\n";
  init_singular (config::library().string());
  sglogger logger (input_task.variety_ideal, "descent", logging_level);
  logger.log (input_task.ambient_ideal, 2);

  singular_load_ssi ("IX", input_task.variety_ideal);
  singular_load_ssi ("IW", input_task.ambient_ideal);
  singular_load_ssi ("g", input_task.exclusion_poly);

  idhdl ambient_handle = ggetid ("IW");
  ideal ambient_ideal = IDIDEAL (ambient_handle);
  idhdl variety_handle = ggetid ("IX");
  ideal variety_ideal = IDIDEAL (variety_handle);
  idhdl exclusion_handle = ggetid ("g");
  poly g = IDPOLY (exclusion_handle);
  unsigned long num_variety = variety_ideal->ncols;
  //unsigned long num_vars = currRing->N;
  if (singular_check_proc ("radicalMemberShip") == false)
  {
    singular_load_rms();
  }
  if (singular_check_proc ("slocus") == false)
  {
    singular_load_library ("sing.lib");
  }

  std::vector<std::pair<int, poly>> slocus_polys;

  ideal ambient_ideal_std = kStd (ambient_ideal, currRing->qideal, testHomog, NULL);
  {
    std::stringstream log_ss;
    log_ss << "will consider " << num_variety << " polys, seed is " << siSeed;
    logger.log (log_ss.str(), 3);
  }
  for (unsigned long i = 0; i < num_variety; ++i)
  {
    poly remainder = kNF (ambient_ideal_std, currRing->qideal, variety_ideal->m[i]);
    bool in_ambient_ideal = (remainder == NULL);
    p_Delete (&remainder, currRing);
    if (in_ambient_ideal)
    {
      {
        std::stringstream log_ss;
        log_ss << "skipping poly " << i << " which is in ambient ideal";
        logger.log (log_ss.str(), 4);
      }
      continue;
    }

    ideal U = id_Copy (ambient_ideal, currRing);
    idInsertPoly_inc1 (U, p_Copy (variety_ideal->m[i], currRing));
    idhdl ambient_candidate_handle =
      enterid ("U", 1, IDEAL_CMD, &(currRing->idroot), FALSE);
    IDIDEAL (ambient_candidate_handle) = U;

    call_singular_and_discard ("ideal sl = slocus(U); ideal V = std(sat(sl,g)[1]); return();");
    idhdl slocus_handle = ggetid ("V");
    ideal slocus_ideal = IDIDEAL (slocus_handle);

    if (p_IsUnit (slocus_ideal->m[0], currRing))
    {
      {
        std::stringstream log_ss;
        log_ss << "poly " << i << " is good choice, will terminate now";
        logger.log (log_ss.str(), 3);
      }
      std::string new_ambient_filename (input_task.ambient_ideal + ".x");
      singular_write_ssi ("U", new_ambient_filename);
      call_singular_and_discard ("kill IW, IX, g, U, V, sl; return();");
      idDelete (&ambient_ideal_std);
      for (auto sl_pair : slocus_polys)
      {
        p_Delete (&(sl_pair.second), currRing);
      }
      pnetc::type::smoothness_task::smoothness_task result
        { new_ambient_filename, input_task.variety_ideal,
          input_task.exclusion_poly, input_task.flag,
          input_task.codimension - 1, input_task.source_chart
        };
      output_list.emplace_back (result);
      //std::cout << "end descent\n";
      return;
    }
    else
    {
      idhdl sl_handle = ggetid ("sl");
      ideal sl_ideal = IDIDEAL (sl_handle);
      {
        std::stringstream log_ss;
        log_ss << "poly " << i << " is not a good choice, " << sl_ideal->ncols
          << " polys in sl_ideal";
        logger.log (log_ss.str(), 4);
      }
      for (int j = 0; j < sl_ideal->ncols; ++j)
      {
        slocus_polys.push_back ({i, p_Copy (sl_ideal->m[j], currRing)});
      }
    }

    call_singular_and_discard ("kill U, V, sl; return();");
    if (std::ifstream (heureka_file).good())
    {
      logger.log ("heureka detected, going home", 3);
      call_singular_and_discard ("kill IW, IX, g; return();");
      idDelete (&ambient_ideal_std);
      for (auto sl_pair : slocus_polys)
      {
        p_Delete (&(sl_pair.second), currRing);
      }
      return;
    }
  }
  logger.log ("no obvious good choice has been found", 3);

  unsigned long cur_tries = 0;
  unsigned long cur_zero_tries = 0;
  while ((cur_tries < rnd_tries) && (cur_zero_tries < max_zero_tries))
  {
    poly g_cand = NULL;
    for (unsigned long i = 0; i < num_variety; ++i)
    {
      number n = n_Init (draw_random_integer (rnd_coeff_max), currRing);
      if (!n_IsZero (n, currRing))
      {
        poly summand = pp_Mult_nn (variety_ideal->m[i], n, currRing);
        g_cand = p_Add_q (g_cand, summand, currRing);
      }
      n_Delete (&n, currRing);
    }
    poly remainder = kNF (ambient_ideal_std, currRing->qideal, g_cand);
    bool in_ambient_ideal = (remainder == NULL);
    p_Delete (&remainder, currRing);
    if (in_ambient_ideal)
    {
      ++cur_zero_tries;
      std::stringstream log_ss;
      log_ss << "random poly reduces to zero #" << cur_zero_tries;
      logger.log (log_ss.str(), 4);
      p_Delete (&g_cand, currRing);
      continue;
    }

    ++cur_tries;
    ideal U = id_Copy (ambient_ideal, currRing);
    idInsertPoly_inc1 (U, g_cand);
    idhdl ambient_candidate_handle = enterid ("U", 1, IDEAL_CMD, &(currRing->idroot), FALSE);
    IDIDEAL (ambient_candidate_handle) = U;
    call_singular_and_discard ("ideal sl = slocus(U); ideal V = std(sat(sl,g)[1]); return();");
    idhdl slocus_handle = ggetid ("V");
    ideal slocus_ideal = IDIDEAL (slocus_handle);
    if (p_IsUnit (slocus_ideal->m[0], currRing))
    {
      {
        std::stringstream log_ss;
        log_ss << "random poly #" << cur_tries << " is good choice, terminate now";
        //log_ss << ", poly is " << get_singular_result("U[size(U)]; return()");
        logger.log (log_ss.str(), 3);
      }
      std::string new_ambient_filename (input_task.ambient_ideal + ".x");
      singular_write_ssi ("U", new_ambient_filename);
      call_singular_and_discard ("kill IW, IX, g, U, V, sl; return();");
      idDelete (&ambient_ideal_std);
      for (auto sl_pair : slocus_polys)
      {
        p_Delete (&(sl_pair.second), currRing);
      }
      pnetc::type::smoothness_task::smoothness_task result
        { new_ambient_filename, input_task.variety_ideal,
          input_task.exclusion_poly, input_task.flag,
          input_task.codimension - 1, input_task.source_chart
        };
      output_list.emplace_back (result);
      return;
    }
    {
      std::stringstream log_ss;
      log_ss << "random poly #" << cur_tries << " is not a good choice";
      logger.log (log_ss.str(), 4);
    }
    call_singular_and_discard ("kill U, V, sl; return();");
    if (std::ifstream (heureka_file).good())
    {
      logger.log ("heureka detected, going home", 3);
      call_singular_and_discard ("kill IW, IX, g; return();");
      idDelete (&ambient_ideal_std);
      for (auto sl_pair : slocus_polys)
      {
        p_Delete (&(sl_pair.second), currRing);
      }
      return;
    }
  }

  idDelete (&ambient_ideal_std);
  logger.log ("have to lift", 3);

  ideal singular_loci_ideal = idInit (slocus_polys.size());
  for (unsigned long i = 0; i < slocus_polys.size(); ++i)
  {
    singular_loci_ideal->m[i] = slocus_polys.at (i).second;
  }
  ideal ex_poly_ideal = idInit (1);
  ex_poly_ideal->m[0] = p_Copy (g, currRing);
  ideal lift_result_ideal = idLift (singular_loci_ideal, ex_poly_ideal,
    NULL, FALSE, FALSE);
  if (is_ideal_zero (lift_result_ideal))
  {
    throw std::runtime_error ("lift failed");
  }
  matrix lift_result_matrix = id_Module2formatedMatrix (lift_result_ideal,
    IDELEMS (singular_loci_ideal), IDELEMS (ex_poly_ideal), currRing);
  //iiWriteMatrix (lift_result_matrix, "_", 2, currRing, 0);
  std::set<unsigned long> appearing_polys;
  for (unsigned long i = 0; i < slocus_polys.size(); ++i)
  {
    if (lift_result_matrix->m[i] != NULL)
    {
      appearing_polys.insert (i);
    }
  }
  {
    std::stringstream log_ss;
    log_ss << "lift gave covering with " << appearing_polys.size() << " polys";
    logger.log (log_ss.str(), 3);
  }
  call_singular_and_discard ("ideal IXs = std(IX); return();");
  for (unsigned long i : appearing_polys)
  {
    poly gj = p_Mult_q (p_Copy (g, currRing),
      p_Copy (slocus_polys.at (i).second, currRing) , currRing);
    idhdl gj_handle = enterid ("gj", 1, POLY_CMD, &(currRing->idroot), FALSE);
    IDPOLY (gj_handle) = gj;
    call_singular_and_discard ("poly r = NF(gj, IXs); return();");
    idhdl r_handle = ggetid ("r");
    poly r = IDPOLY (r_handle);
    if (r == NULL)
    {
      //std::cout << "skipping #" << i << " as new g is in IX" << std::endl;
      logger.log ("skipping #" + std::to_string(i) + " as new g is in IX", 4);
      call_singular_and_discard ("kill r, gj; return();");
      continue;
    }
    {
      std::stringstream log_ss;
      log_ss << "poly #" << i << " is coming from IX[" <<
        slocus_polys.at (i).first + 1 << "]";
      logger.log (log_ss.str(), 4);
    }
    std::stringstream new_ep_filename_sstr;
    new_ep_filename_sstr << input_task.exclusion_poly << '.' << i;
    singular_write_ssi ("gj", new_ep_filename_sstr.str());

    ideal new_ambient_ideal = id_Copy (ambient_ideal, currRing);
    idInsertPoly_inc1 (new_ambient_ideal,
      p_Copy (variety_ideal->m[slocus_polys.at (i).first], currRing));
    idhdl new_ambient_handle =
      enterid ("IZJ", 1, IDEAL_CMD, &(currRing->idroot), FALSE);
    IDIDEAL (new_ambient_handle) = new_ambient_ideal;
    std::stringstream new_ambient_filename_sstr;
    new_ambient_filename_sstr << input_task.ambient_ideal << '.' << i;
    singular_write_ssi ("IZJ", new_ambient_filename_sstr.str());
    pnetc::type::smoothness_task::smoothness_task result
      { new_ambient_filename_sstr.str(), input_task.variety_ideal,
        new_ep_filename_sstr.str(), input_task.flag,
        input_task.codimension - 1, input_task.source_chart
      };
    output_list.emplace_back (result);
    call_singular_and_discard ("kill IZJ, gj, r; return();");
  }
  {
    std::stringstream log_ss;
    log_ss << output_list.size() << " polys have been kept";
    logger.log (log_ss.str(), 3);
  }
  idDelete (reinterpret_cast<ideal*> (&lift_result_matrix));
  idDelete (&singular_loci_ideal);
  idDelete (&ex_poly_ideal);
  call_singular_and_discard ("kill IW, IX, IXs, g; return();");
  //std::cout << "end descent\n";
}

NO_NAME_MANGLING
void singular_smoothness_init ( std::string const& ideal_filename
                              , bool const& is_projective
                              , std::list<pnetc::type::smoothness_task::smoothness_task>& ideal_list
                              , std::map<int, unsigned long>& map_for_ideals
                              , std::string& heureka_file
                              , std::string& remaining_variety
                              , int const& codimension_limit
                              , int const& logging_level
                              )
{
  //std::cout << "start init\n";
  //std::cout << "Projective?" << std::boolalpha << is_projective << '\n';
  init_singular (config::library().string());
  sglogger logger (ideal_filename, "init", logging_level, is_projective);

  heureka_file = ideal_filename + ".heureka";
  if (std::ifstream (heureka_file).good())
  {
    throw std::runtime_error ("heureka file already exists");
  }
  remaining_variety = ideal_filename + ".remaining";
  if (is_projective)
  {
    singular_load_ssi ("IX", ideal_filename);
    idhdl ix_handle = ggetid ("IX");
    ideal ix_ideal = IDIDEAL (ix_handle);
    call_singular_and_discard ("ideal IXstd = std(IX); int d = dim(IXstd); def R = basering; return();");
    singular_write_ssi ("IXstd", remaining_variety);
    idhdl dim_handle = ggetid ("d");
    int affine_cone_dimension = IDINT(dim_handle);
    int affine_num_vars = currRing->N;
    if (codimension_limit >= affine_num_vars - affine_cone_dimension)
    {
      throw std::runtime_error ("codimension limit too high");
    }
    lists input_ringlist = rDecompose (currRing);
    ring input_ring = currRing;
    for (int i = 0; i < affine_num_vars; ++i)
    {
      lists new_ringlist = lCopy (input_ringlist);
      lists new_var_list = delete_from_list
        (static_cast<lists> (new_ringlist->m[1].data), i);
      new_ringlist->m[1].CleanUp();
      new_ringlist->m[1].rtyp = LIST_CMD;
      new_ringlist->m[1].data = new_var_list;
      poly one_poly = p_One (currRing);
      ideal subst_ideal = id_Subst (id_Copy (ix_ideal, currRing), i+1, one_poly, currRing);
      ideal subst_ideal_std = kStd (subst_ideal, currRing->qideal, testHomog, NULL);
      bool chart_is_empty = p_IsUnit (subst_ideal_std->m[0], currRing);
      id_Delete (&subst_ideal_std, currRing);
      p_Delete (&one_poly, currRing);
      idhdl subst_ideal_handle = enterid ("IXs", 1, IDEAL_CMD, &(currRing->idroot), FALSE);
      IDIDEAL (subst_ideal_handle) = subst_ideal;
      ring new_ring = rCompose (new_ringlist);
      idhdl new_ring_handle = enterid ("S", 1, RING_CMD, &(currPack->idroot), FALSE);
      IDRING (new_ring_handle) = new_ring;
      call_singular_and_discard
        ("setring S; ideal IXn = imap(R, IXs); ideal IWn; poly g = 1; return();");
      std::stringstream new_ideal_filename_sstr;
      new_ideal_filename_sstr << ideal_filename << '.' << input_ring->names[i];
      singular_write_ssi ("IXn", new_ideal_filename_sstr.str());
      std::stringstream new_ambient_filename_sstr;
      new_ambient_filename_sstr << ideal_filename << '.' << input_ring->names[i] << ".w";
      singular_write_ssi ("IWn", new_ambient_filename_sstr.str());
      std::stringstream new_poly_filename_sstr;
      new_poly_filename_sstr << ideal_filename << '.' << input_ring->names[i] << ".g";
      singular_write_ssi ("g", new_poly_filename_sstr.str());
      call_singular_and_discard ("kill IXn, IWn, g; setring R; kill S; kill IXs; return();");
      new_ringlist->Clean();
      if (!chart_is_empty)
      {
        pnetc::type::smoothness_task::smoothness_task task
        { new_ambient_filename_sstr.str(), new_ideal_filename_sstr.str(), new_poly_filename_sstr.str(),
          true, affine_num_vars - affine_cone_dimension, i+1 };
        ideal_list.emplace_back (task);
        map_for_ideals.emplace (i+1, 1UL);
      }
      else
      {
        logger.log ("chart for variable " + std::to_string (i+1) + " is empty", 2);
      }
    }
    input_ringlist->Clean();
    call_singular_and_discard ("kill d, IX, IXstd; return();");
    if (ideal_list.empty())
    {
      throw std::runtime_error ("no charts have been produced (empty variety?)");
    }
  }
  else
  {
    singular_load_ssi ("IX", ideal_filename);
    call_singular_and_discard ("ideal IXs = std(IX); int d = dim(IXs); ideal IW; poly g = 1; return();");
    singular_write_ssi ("IXs", remaining_variety);
    std::stringstream ambient_filename_sstr;
    ambient_filename_sstr << ideal_filename << ".w";
    singular_write_ssi ("IW", ambient_filename_sstr.str());
    std::stringstream ep_filename_sstr;
    ep_filename_sstr << ideal_filename << ".g";
    singular_write_ssi ("g", ep_filename_sstr.str());
    idhdl dim_handle = ggetid ("d");
    int dimension = IDINT(dim_handle);
    if (dimension == -1)
    {
      throw std::runtime_error ("variety is empty");
    }
    int num_vars = currRing->N;
    if (codimension_limit >= num_vars - dimension)
    {
      throw std::runtime_error ("codimension limit too high");
    }
    pnetc::type::smoothness_task::smoothness_task task
    { ambient_filename_sstr.str(), ideal_filename, ep_filename_sstr.str(),
      true, num_vars - dimension, 1 };
    ideal_list.emplace_back (task);
    map_for_ideals.emplace (1, 1UL);
    call_singular_and_discard ("kill d, IX, IXs, IW, g; return();");
  }
  //std::cout << "end init\n";
}

NO_NAME_MANGLING
void singular_smoothness_jacobisplit ( pnetc::type::smoothness_task::smoothness_task const& input_task
                                     , pnetc::type::jacobi_list::jacobi_list& output_list
                                     , unsigned long const& heuristics_options
                                     , int const& logging_level
                                     )
{
  //std::cout << "start jacobisplit\n";
  init_singular (config::library().string());
  sglogger logger (input_task.variety_ideal, "split", logging_level);
  logger.log (input_task.ambient_ideal, 2);

  singular_load_ssi ("IW", input_task.ambient_ideal);
  singular_load_ssi ("IX", input_task.variety_ideal);
  singular_load_ssi ("g", input_task.exclusion_poly);
  idhdl variety_handle = ggetid ("IX");
  ideal variety_ideal = IDIDEAL (variety_handle);
  idhdl ambient_handle = ggetid ("IW");
  ideal ambient_ideal = IDIDEAL (ambient_handle);
  idhdl exclusion_poly_handle = ggetid ("g");
  poly exclusion_poly = IDPOLY (exclusion_poly_handle);
  call_singular_and_discard ("matrix M = jacob(IW); return();");
  idhdl matrix_handle = ggetid ("M");
  matrix jacobi_iw = IDMATRIX (matrix_handle);
  unsigned long num_vars = currRing->N;
  unsigned long num_ambient = ambient_ideal->ncols;
  if (num_ambient > num_vars)
  {
    throw std::runtime_error ("too many polys in ambient ideal");
  }

  bool check_single_g = (heuristics_options & 1UL)
    && !p_IsUnit (exclusion_poly, currRing);

  combination_generator cg (num_vars, num_ambient);
  std::vector<std::pair<int, poly>> minor_polys;
  int count = 0;
  matrix sub_matrix = mpNew (num_ambient, num_ambient);
  do
  {
    for (unsigned long i = 0; i < num_ambient; ++i)
    {
      for (unsigned long j = 0; j < num_ambient; ++j)
      {
        MATELEM (sub_matrix, i + 1, j + 1) =
          MATELEM (jacobi_iw, i + 1, cg.get_combination().at (j) + 1);
      }
    }
    poly q = singular_determinant (sub_matrix);

    if (check_single_g && (q != NULL))
    {
      idhdl q_handle = enterid ("q", 1, POLY_CMD, &(currRing->idroot), FALSE);
      IDPOLY (q_handle) = q;
      call_singular_and_discard ("ideal J = IX, q; poly r = NF(g,std(J)); return();");
      idhdl r_handle = ggetid ("r");
      poly r = IDPOLY (r_handle);
      if (r == NULL)
      {
        std::stringstream log_ss;
        log_ss << "combination " << count << " (" <<
          get_combination_string (cg.get_combination()) << ") suffices";
        logger.log (log_ss.str(), 3);

        call_singular_and_discard ("kill q, J, r; return();");
        call_singular_and_discard ("kill IW, IX, g, M; return();");
        for (auto mp : minor_polys)
        {
          p_Delete (&(mp.second), currRing);
        }
        omFree (sub_matrix->m);
        omFreeBin (sub_matrix, sip_sideal_bin);
        output_list.sm_task = input_task;
        output_list.minor_list.emplace_back (count);
        return;
      }
      dechain_handle (q_handle, &(currRing->idroot));
      call_singular_and_discard ("kill J, r; return();");
    }

    if (q != NULL)
    {
      minor_polys.push_back ({count, q});
    }
    ++count;
  }
  while (cg.next());

  output_list.sm_task = input_task;

  if (heuristics_options & 2UL)
  {
    output_list.minor_list = pnet::type::value::wrap (get_small_jacobi_cover
      (variety_ideal, exclusion_poly, minor_polys));
    {
      std::stringstream log_ss;
      log_ss << count << " minor polys, " << minor_polys.size()
        << " are non-zero, " << output_list.minor_list.size()
        << " in covering";
      logger.log (log_ss.str(), 3);
    }
  }
  else
  {
    std::list<int> result_list;
    for (auto mp : minor_polys)
    {
      result_list.push_back (mp.first);
    }
    output_list.minor_list = pnet::type::value::wrap (result_list);

    std::stringstream log_ss;
    log_ss << count << " minor polys, all " << minor_polys.size()
      << " non-zero polys in covering";
    logger.log (log_ss.str(), 3);
  }

  call_singular_and_discard ("kill IW, IX, g, M; return();");
  for (auto mp : minor_polys)
  {
    p_Delete (&(mp.second), currRing);
  }
  omFree (sub_matrix->m);
  omFreeBin (sub_matrix, sip_sideal_bin);
  //std::cout << "end jacobisplit\n";
}

NO_NAME_MANGLING
void singular_smoothness_checkcover ( std::string const& ideal_filename
                                                , int const& new_var
                                                , bool& result
                                                , int const& logging_level
                                                )
{
  init_singular (config::library().string());
  sglogger logger (ideal_filename, "check", logging_level);
  {
    std::stringstream log_ss;
    log_ss << "new variable is " << new_var;
    logger.log (log_ss.str(), 2);
  }
  singular_load_ssi ("I", ideal_filename);
  std::stringstream check_cmd_ss;
  check_cmd_ss << "ideal N = var(" << new_var << "); I = std(I,N); return();";
  call_singular_and_discard (check_cmd_ss.str());
  singular_write_ssi ("I", ideal_filename);
  logger.log ("now call sat", 3);
  if (singular_check_proc ("sat") == false)
  {
    singular_load_library ("elim.lib");
  }
  call_singular_and_discard ("list SIl = sat(I, maxideal(1)); ideal SI = SIl[1]; return();");

  idhdl saturation_result_handle = ggetid ("SI");
  ideal saturation_result_ideal = IDIDEAL (saturation_result_handle);
  if (p_IsUnit (saturation_result_ideal->m[0], currRing))
  {
    logger.log ("variety has been covered, we are done", 1);
    result = true;
  }
  else
  {
    logger.log ("variety has not been covered yet", 3);
    result = false;
  }
  call_singular_and_discard ("kill SI, SIl, I, N; return();");
}
