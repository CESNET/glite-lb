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

dnl Usage:
dnl AC_GLITE_LB
dnl - GLITE_LB_THR_CLIENT_LIBS
dnl - GLITE_LB_THR_CLIENTPP_LIBS
dnl - GLITE_LB_THR_COMMON_LIBS
dnl - GLITE_LB_NOTHR_CLIENT_LIBS
dnl - GLITE_LB_NOTHR_CLIENTPP_LIBS
dnl - GLITE_LB_NOTHR_COMMON_LIBS
dnl - GLITE_STATIC_LB_NOTHR_CLIENT_LIBS
dnl - GLITE_STATIC_LB_NOTHR_COMMON_LIBS

AC_DEFUN([AC_GLITE_LB],
[
    AC_REQUIRE([AC_GLITE])
    ac_glite_lb_prefix=$GLITE_LOCATION

    AC_SEC_GSOAP_PLUGIN

    case $GLITE_LDFLAGS in
	*lib64* ) 
		ac_glite_lb_libdir=lib64
		;;

	*lib32* ) 
		ac_glite_lb_libdir=lib32
		;;

	* )
		ac_glite_lb_libdir=lib
		;;
    esac


    if test -n "$ac_glite_lb_prefix" ; then
	dnl
	dnl 
	dnl
	ac_glite_lb_lib="-L$ac_glite_lb_prefix/$ac_glite_lb_libdir"
	GLITE_LB_THR_CLIENT_LIBS="$ac_glite_lb_lib -lglite_lb_client_$GLOBUS_THR_FLAVOR"
	GLITE_LB_THR_CLIENTPP_LIBS="$ac_glite_lb_lib -lglite_lb_clientpp_$GLOBUS_THR_FLAVOR"
	GLITE_LB_THR_COMMON_LIBS="$ac_glite_lb_lib -lglite_lb_common_$GLOBUS_THR_FLAVOR $SEC_GSOAP_PLUGIN_GSS_THR_LIBS"
	GLITE_LB_NOTHR_CLIENT_LIBS="$ac_glite_lb_lib -lglite_lb_client_$GLOBUS_NOTHR_FLAVOR"
	GLITE_LB_NOTHR_CLIENTPP_LIBS="$ac_glite_lb_lib -lglite_lb_clientpp_$GLOBUS_NOTHR_FLAVOR"
        GLITE_LB_NOTHR_COMMON_LIBS="$ac_glite_lb_lib -lglite_lb_common_$GLOBUS_NOTHR_FLAVOR $SEC_GSOAP_PLUGIN_GSS_NOTHR_LIBS"
	GLITE_STATIC_LB_NOTHR_CLIENT_LIBS="$ac_glite_lb_prefix/$ac_glite_lb_libdir/libglite_lb_client_$GLOBUS_NOTHR_FLAVOR.a"
	GLITE_STATIC_LB_NOTHR_COMMON_LIBS="$ac_glite_lb_prefix/$ac_glite_lb_libdir/libglite_lb_common_$GLOBUS_NOTHR_FLAVOR.a $SEC_GSOAP_PLUGIN_GSS_STATIC_NOTHR_LIBS"
	ifelse([$2], , :, [$2])
    else
	GLITE_LB_THR_CLIENT_LIBS=""
	GLITE_LB_THR_CLIENTPP_LIBS=""
	GLITE_LB_THR_COMMON_LIBS=""
	GLITE_LB_NOTHR_CLIENT_LIBS=""
	GLITE_LB_NOTHR_CLIENTPP_LIBS=""
	GLITE_LB_NOTHR_COMMON_LIBS=""
	GLITE_STATIC_LB_NOTHR_CLIENT_LIBS=""
        GLITE_STATIC_LB_NOTHR_COMMON_LIBS=""
	ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_LB_THR_CLIENT_LIBS)
    AC_SUBST(GLITE_LB_THR_CLIENTPP_LIBS)
    AC_SUBST(GLITE_LB_THR_COMMON_LIBS)
    AC_SUBST(GLITE_LB_NOTHR_CLIENT_LIBS)
    AC_SUBST(GLITE_LB_NOTHR_CLIENTPP_LIBS)
    AC_SUBST(GLITE_LB_NOTHR_COMMON_LIBS)
    AC_SUBST(GLITE_STATIC_LB_NOTHR_CLIENT_LIBS)
    AC_SUBST(GLITE_STATIC_LB_NOTHR_COMMON_LIBS)
])

AC_DEFUN([GLITE_CHECK_LB_CLIENT],
[AC_MSG_CHECKING([for org.glite.lb.client])
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $GLITE_CPPFLAGS"
save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS $GLITE_LDFLAGS -L$GLOBUS_LOCATION/lib"
save_LIBS=$LIBS

AC_LANG_PUSH([C])

# prepare the test program, to link against the different combinations
# of globus flavours

AC_LANG_CONFTEST(
  [AC_LANG_PROGRAM(
    [@%:@include "glite/lb/consumer.h"],
    [edg_wll_QueryEvents(
      (edg_wll_Context)0,
      (const edg_wll_QueryRec*)0,
      (const edg_wll_QueryRec*)0,
      (edg_wll_Event**)0
    );]
  )]
)

LIBS="-lglite_lb_client_$GLOBUS_THR_FLAVOR $LIBS"
AC_LINK_IFELSE([],
  [AC_SUBST([GLITE_LB_CLIENT_THR_LIBS], [-lglite_lb_client_$GLOBUS_THR_FLAVOR])],
  [AC_MSG_ERROR([cannot find org.glite.lb.client ($GLOBUS_THR_FLAVOR)])]
)
LIBS=$save_LIBS

LIBS="-lglite_lb_client_$GLOBUS_NOTHR_FLAVOR $LIBS"
AC_LINK_IFELSE([],
  [AC_SUBST([GLITE_LB_CLIENT_NOTHR_LIBS], [-lglite_lb_client_$GLOBUS_NOTHR_FLAVOR])],
  [AC_MSG_ERROR([cannot find org.glite.lb.client ($GLOBUS_NOTHR_FLAVOR)])]
)
LIBS=$save_LIBS

AC_LANG_POP([C])

AC_LANG_PUSH([C++])

# prepare the test program, to link against the different combinations
# of globus flavours

AC_LANG_CONFTEST(
  [AC_LANG_PROGRAM(
    [@%:@include "glite/lb/Job.h"],
    [glite::lb::Job job;]
  )]
)

save_LIBS=$LIBS
LIBS="-lglite_lb_clientpp_$GLOBUS_THR_FLAVOR $LIBS"
AC_LINK_IFELSE([],
  [AC_SUBST([GLITE_LB_CLIENTPP_THR_LIBS], [-lglite_lb_clientpp_$GLOBUS_THR_FLAVOR])],
  [AC_MSG_ERROR([cannot find org.glite.lb.client (C++ - $GLOBUS_THR_FLAVOR)])]
)
LIBS=$save_LIBS

LIBS="-lglite_lb_clientpp_$GLOBUS_NOTHR_FLAVOR $LIBS"
AC_LINK_IFELSE([],
  [AC_SUBST([GLITE_LB_CLIENTPP_NOTHR_LIBS], [-lglite_lb_clientpp_$GLOBUS_NOTHR_FLAVOR])],
  [AC_MSG_ERROR([cannot find org.glite.lb.client (C++ - $GLOBUS_NOTHR_FLAVOR)])]
)
LIBS=$save_LIBS

AC_LANG_POP([C++])

LDFLAGS="$save_LDFLAGS"
CPPFLAGS=$save_CPPFLAGS

AC_MSG_RESULT([yes])

])
