/*
 *****************************************************************************
 *
 * $B6&DL%b%8%e!<%kMQ(B $BFH<+(BI/F$B@k8@(B by TOS
 *
 *  com_if.h $B$G%$%s%/%k!<%I$5$l$k$?$a!"4pK\E*$KA4%=!<%96&DL$H$J$k!#(B
 *  $B$=$l0J30$N%=!<%9$d%X%C%@$G!"K\%U%!%$%k$r%$%s%/%k!<%I$7$J$$$3$H!#(B
 *  com_spec.c $B$GDj5A$9$k4X?t$K$D$$$F!"K\%U%!%$%k$K5-:\$9$k!#(B
 *
 *****************************************************************************
 */

#pragma once

/* $BFH<+%$%s%/%k!<%I%U%!%$%k(B ------------------------------------------------*/




/* $B4pK\(BI/f -----------------------------------------------------------------*/

/* $B8DJL%(%i!<%3!<%I(B  $B!v$3$A$i$rJQ$($?$i!"O"F0%F!<%V%k(B gErrorNameSpec[] $B$bD>$9(B */
enum {
    /* $B3+H/$9$k%W%m%0%i%`$4$H$K<+M3$K@_Dj2DG=$J%(%i!<%3!<%I(B (001-899) */
    COM_ERR_DUMMY      =   1,   // $B$3$l$O%5%s%W%k$J$N$G:o=|JQ99(BOK
};

/*
 * $B8DJL%(%i!<%3!<%I@_Dj(BI/F
 *  COM$B%b%8%e!<%kFbIt$G8F$S=P$9$@$1$N$?$a!"FC$K?($l$kI,MW$O$J$$!#(B
 *  $B;H$&B&$O>e5-$N%(%i!<%3!<%I@k8@$H(B gErrorNameSpec[]$B$rJQ99$9$k$N$_$GNI$$(B
 */
void com_registerErrorCodeSpec( void );

/*
 * $B8DJLIt=i4|2=(B  com_initializeSpec()
 * ---------------------------------------------------------------------------
 *   $B8=;~E@$G$O%(%i!<$OH/@8$7$J$$!#(B($B=hM}$r=$@5$7$?>l9g$O!"$3$NFbMF$b=$@5(B)
 * ===========================================================================
 * $B$3$N=hM}$O(B com$B%b%8%e!<%k$N0lHV:G=i$N=hM}$GF0:n$9$k$h$&$K$J$C$F$$$k$?$a!"(B
 * $B%G%P%C%0%b!<%I$J$I:G=i$KJQ99$7$?$$$b$N$K$D$$$F!"5-=R$G$-$k!#(B
 * $B5/F0%*%W%7%g%s$r8+$?$$>l9g$O!"(Bcom_getCommandLine()$B$G<hF@$9$l$PNI$$!#(B
 * ($B$3$N(BI/F$B$O(B com_sepc.h$B$G@k8@$5$l$F$$$k(B)
 *
 * $B$3$N(BI/F$B$O%m%.%s%0$d%G%P%C%05!G=$NF0:n3+;OA0$K8F$P$l$k!#(B
 * $B$3$N$?$a!"(Bcom_if.h$B$N(B COMPRINT$B!&(BCOMDEBUG$B%;%/%7%g%s$N2hLL(B/$B%m%0=PNO$rH<$$(BI/F$B$O(B
 * $B;HMQ$G$-$J$$(B($BNc$($P!"(Bcom_printf()$B!&(Bcom_debug()$B!&(Bcom_error()$BEy(B)
 * $B$3$l$i$N(BI/F$B$OB>(BI/F$B$+$i$bB??t8F$P$l$k$?$a!"(Bcom_if.h$BFb$N0J2<(BI/F$B0J30$O(B
 * $B;H$o$J$$$h$&$K$7$?$[$&$,NI$$!#(B
 *   com_getAplName()$B!&(Bcom_getVersion()$B!&(Bcom_getCommandLine()
 *   com_setLogFile()$B!&(Bcom_setTitleForm()$B!&(Bcom_setErrorCodeForm()
 *   com_setDebugPrint()
 *   com_setWatchMemInfo()
 *   com_debugMemoryErrorOn()$B!&(Bcom_debugMemoryErrorOff()
 *   com_setWatchFileInfo()
 *   com_debugFopenErrorOn()$B!&(Bcom_debugFopenErrorOff()
 *   com_debugFcloseErrorOn()$B!&(Bcom_debugFcloseErrorOff()
 *   com_noComDebugLog()
 * $B$3$l$i$N(BI/F$B$O<g$KK\(BI/F$B$OFb$G;HMQ$9$k$3$H$rA[Dj$7$?$b$N$,B?$$!#(B
 *
 * $B2hLL=PNO$,I,MW$J>l9g!"I8=`4X?t(B printf()$B$r;HMQ$9$k$3$H(B($B%m%0=PNO$OIT2D(B)$B!#(B
 * $B%W%m%0%i%`=*N;$,I,MW$J>l9g$O!"(Bexit()$B$r;HMQ$9$k$3$H!#(B
 *
 * $B$b$A$m$sFH<+=hM}$KI,MW$J=i4|2==hM}$,$"$k$J$i!"K\(BI/F$B$K5-=R$9$Y$-$G$"$k!#(B
 *
 * com_malloc()$B$N$h$&$J%a%b%j3NJ]$,I,MW$J=i4|@_Dj$r$7$?$$>l9g!"(B
 * $BK\(BI/F$BFb$G$O$=$&$7$?(BI/F$B$O;HMQIT2DG=$J$N$G!"=i4|@_DjMQ$N4X?t$rJLESMQ0U$7!"(B
 * com_initialize()$B$N8e$K8F$V$h$&$K$9$k$+!"?d>)$O$7$J$$$,!"K\(BI/F$BFb$G(B $BD>@\(B
 * malloc()$B$J$I$NI8=`4X?t$r;HMQ$9$k!#$=$N>l9g!"%G%P%C%05!G=$K$h$k%a%b%jIb$-$N(B
 * $B%A%'%C%/$O$G$-$J$/$J$k!#(B
 */
void com_initializeSpec( void );

/*
 * $B8DJLIt=*N;(B  com_finalizeSpec()
 * ---------------------------------------------------------------------------
 *   $B8=;~E@$G$O%(%i!<$OH/@8$7$J$$!#(B($B=hM}$r=$@5$7$?>l9g$O!"$3$NFbMF$b=$@5(B)
 * ===========================================================================
 * $B8DJLIt=hM}$NCf$G!"%W%m%0%i%`=*N;;~$KJRIU$1$,I,MW$J=hM}$,$"$l$P5-=R$G$-$k!#(B
 * $B$3$N(BI/F$B$O(B com_finalize()$B$NCf$G!"0lHV:G=i$K<B9T$5$l$k!#(B
 */
void com_finalizeSpec( void );


/* $BFH<+(BI/F -----------------------------------------------------------------*/

/*
 * ($B%5%s%W%k(B) $B%(%s%G%#%"%sH=Dj(B  com_isBigEndian()$B!&(Bcom_isLittleEndian()
 *   $B%A%'%C%/@.H]$r(B true/false$B$GJV$9!#(B
 * ---------------------------------------------------------------------------
 *   $B8=;~E@$G$O%(%i!<$OH/@8$7$J$$!#(B($B=hM}$r=$@5$7$?>l9g$O!"$3$NFbMF$b=$@5(B)
 * ===========================================================================
 * $B<+?H$N=hM}7O$N%(%s%G%#%"%s$r%A%'%C%/$7!"$=$N7k2L$rJV$9!#(B
 * $B%S%C%0%(%s%G%#%"%s$G$J$1$l$P!"<+F0E*$K%j%H%k%(%s%G%#%"%s$H8@$($k$@$m$&!#(B
 * $B$=$N5U$bF1MM$H$J$k!#(B
 *
 * $BK\(BI/F$B$O$"$/$^$G8DJL=hM}5-=R$N%5%s%W%k$J$N$G!"$h$jE,@Z$J%3!<%I$,$"$k$J$i$P(B
 * $B$=$N%3!<%I$K=q$-49$($?$[$&$,NI$$$@$m$&!#(B
 */
BOOL com_isBigEndian( void );
BOOL com_isLittleEndian( void );

/*
 * ($B%5%s%W%k(B) 32bit/64bit OS$BH=Dj(B  com_is32bitOS()$B!&(Bcom_is64bitOS()
 *   $B%A%'%C%/@.H]$r(B true/false$B$GJV$9!#(B
 * ---------------------------------------------------------------------------
 *   $B8=;~E@$G$O%(%i!<$OH/@8$7$J$$!#(B($B=hM}$r=$@5$7$?>l9g$O!"$3$NFbMF$b=$@5(B)
 * ===========================================================================
 * $B<+?H$N(B OS$B$,(B 32bit $B$^$?$O(B 64bit $B$+$I$&$+$r%A%'%C%/$7!"$=$N7k2L$rJV$9!#(B
 * $B:#$N$H$3$m!"$3$N(B2$B$D$OBP$G$"$j!"(Bfalse$B$G$"$l$PB>J}$G$"$k$HH=CG$G$-$k!#(B
 * (com_is32bitOS()$B$,(B false$B$rJV$7$?!a(B64bit OS$B!"$H8@$($k$H$$$&$3$H$G$"$k(B)
 *
 * $BH=Dj7?$K$O(B long$B7?$H(B int$B7?$N%5%$%:$,Ey$7$1$l$P(B 32bit$B!"0[$J$l$P(B 64bit$B!"(B
 * $B$H$$$&O@M}$r;HMQ$7$F$$$k!#(B
 *
 * $BK\(BI/F$B$O$"$/$^$G8DJL=hM}5-=R$N%5%s%W%k$J$N$G!"$h$jE,@Z$J%3!<%I$,$"$k$J$i$P(B
 * $B$=$N%3!<%I$K=q$-49$($?$[$&$,NI$$$@$m$&!#(B
 */
BOOL com_is32bitOS( void );
BOOL com_is64bitOS( void );


