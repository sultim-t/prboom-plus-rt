dnl AC_CPU_OPTIMISATIONS
dnl Tries to find compiler optimisation flags for the target system
AC_DEFUN([AC_CPU_OPTIMISATIONS],[
AC_REQUIRE([AC_CANONICAL_TARGET])
AC_ARG_ENABLE(cpu-opt,AC_HELP_STRING([--disable-cpu-opt],[turns off cpu specific optimisations]),[
],[
AC_MSG_CHECKING(whether compiler supports -march=native)
OLD_CFLAGS="$CFLAGS"
CFLAGS="$OLD_CFLAGS -march=native"
AC_TRY_COMPILE(,[
  void f() {};
],[
  AC_MSG_RESULT(yes)
],[
  AC_MSG_RESULT(no)
  CFLAGS="$OLD_CFLAGS"

  case "$target" in
        # marginal gains from aligning code
  i386-*) ;;
  i486-*) CPU_CFLAGS="-mtune=i486" ;;
        # nothing special for pentium  
        # CMOV op on ppro/II/686 can help us
  i686-*) CPU_CFLAGS="-march=i686" ;;
  esac
  AC_C_COMPILE_FLAGS($CPU_CFLAGS) 
])
])
])
