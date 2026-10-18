dnl Copyright (c) 2013-2015 The Bitcoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

AC_DEFUN([BITCOIN_FIND_BDB62],[
  AC_ARG_VAR([BDB_CFLAGS], [C compiler flags for BerkeleyDB, bypasses autodetection])
  AC_ARG_VAR([BDB_LIBS], [Linker flags for BerkeleyDB, bypasses autodetection])
  AC_ARG_WITH([incompatible-bdb],[AS_HELP_STRING([--with-incompatible-bdb], [allow a Berkeley DB version other than 6.2; incompatible with legacy Dorkcoin wallets])])

  if test "$use_bdb" = "no"; then
    use_bdb=no
  elif test "$BDB_CFLAGS" = ""; then
    AC_MSG_CHECKING([for Berkeley DB C++ headers])
    BDB_CPPFLAGS=
    bdbpath=X
    bdb62path=X
    bdbdirlist=
    for _vn in 6.2 62 6 5.3 5 4.8 48 4 ''; do
      for _pfx in b lib ''; do
        bdbdirlist="$bdbdirlist ${_pfx}db${_vn}"
      done
    done
    for searchpath in $bdbdirlist ''; do
      test -n "${searchpath}" && searchpath="${searchpath}/"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <${searchpath}db_cxx.h>
      ]],[[
        #if !((DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 8) || DB_VERSION_MAJOR > 4)
          #error "failed to find bdb 4.8+"
        #endif
      ]])],[
        if test "$bdbpath" = "X"; then
          bdbpath="${searchpath}"
        fi
      ],[
        continue
      ])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <${searchpath}db_cxx.h>
      ]],[[
        #if !(DB_VERSION_MAJOR == 6 && DB_VERSION_MINOR == 2)
          #error "failed to find bdb 6.2"
        #endif
      ]])],[
        bdb62path="${searchpath}"
        break
      ],[])
    done
    if test "$bdbpath" = "X"; then
      use_bdb=no
      AC_MSG_RESULT([no])
      AC_MSG_WARN([libdb_cxx headers missing])
      AC_MSG_WARN(AC_PACKAGE_NAME[ requires this library for BDB (legacy) wallet support])
      AC_MSG_WARN([Passing --without-bdb will suppress this warning])
    elif test "$bdb62path" = "X"; then
      BITCOIN_SUBDIR_TO_INCLUDE(BDB_CPPFLAGS,[${bdbpath}],db_cxx)
      if test "$with_incompatible_bdb" = "yes"; then
        AC_MSG_WARN([Found Berkeley DB other than 6.2])
        AC_MSG_WARN([BDB (legacy) wallets opened by this build will not be portable!])
        use_bdb=yes
      else
        AC_MSG_WARN([Found Berkeley DB other than 6.2])
        AC_MSG_WARN([BDB (legacy) wallets opened by this build would not be portable!])
        AC_MSG_WARN([If this is intended, pass --with-incompatible-bdb])
        AC_MSG_WARN([Passing --without-bdb will suppress this warning])
        use_bdb=no
      fi
    else
      BITCOIN_SUBDIR_TO_INCLUDE(BDB_CPPFLAGS,[${bdb62path}],db_cxx)
      bdbpath="${bdb62path}"
      use_bdb=yes
    fi
  else
    BDB_CPPFLAGS=${BDB_CFLAGS}
  fi
  AC_SUBST(BDB_CPPFLAGS)

  if test "$use_bdb" = "no"; then
    use_bdb=no
  elif test "$BDB_LIBS" = ""; then
    # TODO: Ideally this could find the library version and make sure it matches the headers being used
    for searchlib in db_cxx-6.2 db_cxx db6_cxx; do
      AC_CHECK_LIB([$searchlib],[main],[
        BDB_LIBS="-l${searchlib}"
        break
      ])
    done
    if test "$BDB_LIBS" = ""; then
        AC_MSG_WARN([libdb_cxx headers missing])
        AC_MSG_WARN(AC_PACKAGE_NAME[ requires this library for BDB (legacy) wallet support])
        AC_MSG_WARN([Passing --without-bdb will suppress this warning])
    fi
  fi
  if test "$use_bdb" != "no"; then
    bdb_save_CPPFLAGS="$CPPFLAGS"
    bdb_save_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $BDB_CPPFLAGS"
    if test "$with_incompatible_bdb" = "yes"; then
      CPPFLAGS="$CPPFLAGS -DDORK_ALLOW_INCOMPATIBLE_BDB"
      AC_MSG_WARN([Incompatible Berkeley DB explicitly allowed; old Dorkcoin wallets may not load.])
    fi
    LIBS="$BDB_LIBS $LIBS"
    AC_MSG_CHECKING([whether Berkeley DB headers and library support Dorkcoin wallets])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
      #include <db_cxx.h>
      #if !(DB_VERSION_MAJOR == 6 && DB_VERSION_MINOR == 2)
      #ifndef DORK_ALLOW_INCOMPATIBLE_BDB
      #error "Dorkcoin legacy wallets require Berkeley DB 6.2"
      #endif
      #endif
    ]], [[
      Db database(nullptr, 0);
      return database.close(0);
    ]])], [bdb_compatible=yes], [bdb_compatible=no])
    CPPFLAGS="$bdb_save_CPPFLAGS"
    LIBS="$bdb_save_LIBS"
    if test "$bdb_compatible" != "yes"; then
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([Berkeley DB 6.2 headers and matching C++ library are required for legacy Dorkcoin wallets, including when BDB_CFLAGS/BDB_LIBS are supplied.])
    fi
    AC_MSG_RESULT([yes])
    AC_DEFINE([USE_BDB], [1], [Define if BDB support should be compiled in])
    use_bdb=yes
  fi
])
