AC_DEFUN([GLITE_CHECK_LB_COMMON],
[AC_MSG_CHECKING([for org.glite.lb.common])
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $GLITE_CPPFLAGS"
save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS $GLITE_LDFLAGS"
save_LIBS=$LIBS

AC_LANG_PUSH([C])

# prepare the test program, to link agains the different combinations
# of globus flavours

AC_LANG_CONFTEST(
  [AC_LANG_PROGRAM(
    [@%:@include "glite/lb/context.h"],
    [edg_wll_InitContext((edg_wll_Context*)0)]
  )]
)

LIBS="-lglite_lb_common_$GLOBUS_THR_FLAVOR $LIBS"
AC_LINK_IFELSE([],
  [AC_SUBST([GLITE_LB_COMMON_THR_LIBS], [-lglite_lb_common_$GLOBUS_THR_FLAVOR])],
  [AC_MSG_ERROR([cannot find org.glite.lb.common ($GLOBUS_THR_FLAVOR)])]
)
LIBS=$save_LIBS

LIBS="-lglite_lb_common_$GLOBUS_NOTHR_FLAVOR $LIBS"
AC_LINK_IFELSE([],
  [AC_SUBST([GLITE_LB_COMMON_NOTHR_LIBS], [-lglite_lb_common_$GLOBUS_NOTHR_FLAVOR])],
  [AC_MSG_ERROR([cannot find org.glite.lb.common (GLOBUS_NOTHR_FLAVOR)])]
)
LIBS=$save_LIBS

LDFLAGS=$save_LDFLAGS
CPPFLAGS=$save_CPPFLAGS

AC_LANG_POP([C])

AC_MSG_RESULT([yes])

])

