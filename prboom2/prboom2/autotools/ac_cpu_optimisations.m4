dnl AC_CPU_OPTIMISATIONS
dnl Tries to find compiler optimisation flags for the target system
AC_DEFUN(AC_CPU_OPTIMISATIONS,[
AC_REQUIRE([AC_CANONICAL_SYSTEM])
AC_ARG_ENABLE(cpu-opt,[  --disable-cpu-opt        turns off cpu specific optimisations],[
CPU_CFLAGS=""
],[
case "$target" in
        # marginal gains from aligning code
i386-*) CPU_CFLAGS="-m386" ;;
i486-*) CPU_CFLAGS="-m486" ;;
        # nothing special for pentium  
        # CMOV op on ppro/II/686 can help us
i686-*) CPU_CFLAGS="-mcpu=i686 -march=i686" ;;
esac
])
])
