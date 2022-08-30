#pragma once

#include <csignal>

class scoped_sigchld_default
{
  public:
    scoped_sigchld_default()
    {
      sigaction (SIGCHLD, NULL, &old_action);
      struct sigaction new_action = old_action;
      new_action.sa_handler = SIG_DFL;
      sigaction (SIGCHLD, &new_action, NULL);
    }
    ~scoped_sigchld_default()
    {
      sigaction (SIGCHLD, &old_action, NULL);
    }
    scoped_sigchld_default (scoped_sigchld_default const&) = delete;
    scoped_sigchld_default& operator= (scoped_sigchld_default const&) = delete;
    scoped_sigchld_default (scoped_sigchld_default&&) = delete;
    scoped_sigchld_default& operator= (scoped_sigchld_default&&) = delete;
  private:
    struct sigaction old_action;
};
