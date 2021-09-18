/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
#include <assert.h>
/************ Begin %include sections from the grammar ************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "qSqlparser.h"
#include "tcmdtype.h"
#include "ttoken.h"
#include "ttokendef.h"
#include "tutil.h"
#include "tvariant.h"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    ParseTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_PARAM     Code to pass %extra_argument as a subroutine parameter
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    ParseCTX_*         As ParseARG_ except for %extra_context
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYNTOKEN           Number of terminal symbols
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
**    YY_MIN_REDUCE      Minimum value for reduce actions
**    YY_MAX_REDUCE      Maximum value for reduce actions
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned short int
#define YYNOCODE 281
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE SStrToken
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  int32_t yy2;
  SCreatedTableInfo yy42;
  tSqlExpr* yy44;
  SRelationInfo* yy46;
  SCreateAcctInfo yy47;
  TAOS_FIELD yy179;
  SLimitVal yy204;
  int yy222;
  SSqlNode* yy246;
  SArray* yy247;
  SCreateDbInfo yy262;
  SCreateTableSql* yy336;
  tVariant yy378;
  int64_t yy403;
  SIntervalVal yy430;
  SWindowStateVal yy492;
  SSessionWindowVal yy507;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL SSqlInfo* pInfo;
#define ParseARG_PDECL ,SSqlInfo* pInfo
#define ParseARG_PARAM ,pInfo
#define ParseARG_FETCH SSqlInfo* pInfo=yypParser->pInfo;
#define ParseARG_STORE yypParser->pInfo=pInfo;
#define ParseCTX_SDECL
#define ParseCTX_PDECL
#define ParseCTX_PARAM
#define ParseCTX_FETCH
#define ParseCTX_STORE
#define YYFALLBACK 1
#define YYNSTATE             356
#define YYNRULE              298
#define YYNRULE_WITH_ACTION  298
#define YYNTOKEN             198
#define YY_MAX_SHIFT         355
#define YY_MIN_SHIFTREDUCE   568
#define YY_MAX_SHIFTREDUCE   865
#define YY_ERROR_ACTION      866
#define YY_ACCEPT_ACTION     867
#define YY_NO_ACTION         868
#define YY_MIN_REDUCE        869
#define YY_MAX_REDUCE        1166
/************* End control #defines *******************************************/
#define YY_NLOOKAHEAD ((int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])))

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X.
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (795)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    36,  620,  239,  620,  353,  162,  238,  266,  929,  621,
 /*    10 */   808,  621,  811,   75,   76,  353,   79,   80,  104,  929,
 /*    20 */   242,   69,   68,   67,  620,   78,  311,   83,   81,   84,
 /*    30 */    82,  243,  621,  660,  353,   74,   73,  217,  929,   72,
 /*    40 */    71,   70,   75,   76,  353,   79,   80,  237,  929,  242,
 /*    50 */    69,   68,   67,  195,   78,  311,   83,   81,   84,   82,
 /*    60 */   162,  162,  198,  927,   74,   73,  198,  896,   72,   71,
 /*    70 */    70,   75,   76, 1143,   79,   80,  196, 1143,  242,   69,
 /*    80 */    68,   67, 1089,   78,  311,   83,   81,   84,   82,   97,
 /*    90 */   627,  198,  353,   74,   73,  292,  929,   72,   71,   70,
 /*   100 */   620, 1051, 1143,   75,   77,  228,   79,   80,  621, 1029,
 /*   110 */   242,   69,   68,   67,  802,   78,  311,   83,   81,   84,
 /*   120 */    82,   72,   71,   70,  353,   74,   73,  214,  929,   72,
 /*   130 */    71,   70,  815,  191,  190,  189,  285, 1090,  107,  283,
 /*   140 */   188,  151,  150,  149,  148,   12,  569,  570,  571,  572,
 /*   150 */   573,  574,  575,  576,  577,  578,  579,  580,  581,  582,
 /*   160 */   156,   76,  223,   79,   80,  354,  222,  242,   69,   68,
 /*   170 */    67, 1048,   78,  311,   83,   81,   84,   82, 1012, 1013,
 /*   180 */    51, 1016,   74,   73,   54,  341,   72,   71,   70,   79,
 /*   190 */    80,  162,  103,  242,   69,   68,   67,  245,   78,  311,
 /*   200 */    83,   81,   84,   82,  352,  351,  154,  353,   74,   73,
 /*   210 */   215,  929,   72,   71,   70,   60,  307,  348,  347,  306,
 /*   220 */   305,  304,  346,  303,  302,  301,  345,  300,  344,  343,
 /*   230 */    61,  224,  708,  202,   37, 1026,  991,  979,  980,  981,
 /*   240 */   982,  983,  984,  985,  986,  987,  988,  989,  990,  992,
 /*   250 */   993,  201,  353,  241,  817,  909,  929,  806,  209,  809,
 /*   260 */   309,  812,  331,  330,  147,   18,   17,  208,  109,  241,
 /*   270 */   817,  316,  103,  806,  246,  809,  244,  812,  319,  318,
 /*   280 */   203,   83,   81,   84,   82,  198, 1042,  220,  221,   74,
 /*   290 */    73,  312,  918,   72,   71,   70, 1142,  240,   60,  185,
 /*   300 */   348,  347,  225,  220,  221,  346,    6,   57,  177,  345,
 /*   310 */    61,  344,  343,  176,  118,  123,  114,  122,  353,    7,
 /*   320 */   732,  908,  929,  729, 1042,  730,  353,  731,  250,   99,
 /*   330 */   929, 1023,  263,  296,   96,  620,   85,   16,   15,  138,
 /*   340 */   226,  216,  204,  621,  137,   54,  143,  146,  136,  867,
 /*   350 */   355, 1100,   85,  247,  248,  140,  999,  353,  997,  998,
 /*   360 */   900,  929,  353, 1000,   54,  899,  929, 1001,  171, 1002,
 /*   370 */  1003,  353,  818,  813,  891,  929,   54,   74,   73,  814,
 /*   380 */   235,   72,   71,   70,   54,   54,  273,  353,  818,  813,
 /*   390 */   890,  929,  232, 1015,   54,  814, 1026,  271,  270,   54,
 /*   400 */    54, 1042,   54,  748,  256,  252,  313,  249,   54,  326,
 /*   410 */   325,  233, 1137,  260,  259, 1026,  234,  264,   45,   27,
 /*   420 */  1029,  251,  627,  320,  111,  236,   14, 1026,  807, 1029,
 /*   430 */   810,  321,  322,  927,  173, 1026, 1026,  894,  733,  734,
 /*   440 */    54,  323,   22,   21,  155, 1026,  327,  328,  927,  329,
 /*   450 */  1026, 1026,  930, 1026,   13,  333,  767,  768,  110, 1026,
 /*   460 */   251,  135, 1017,  251,  932,   10,  784,  349,  960,  745,
 /*   470 */   919,  185,   98,  174,  341,  265, 1027,  185,  804,  309,
 /*   480 */     1,  175,    3,  186,  100,  101,  764,  774,  775,  752,
 /*   490 */    55, 1025,   89,  718,  288,    5,   92,  720,   50, 1014,
 /*   500 */   290,  157,   86,   39,  719,  840,  819, 1136,   55,   55,
 /*   510 */   619,  821,   86,   95, 1135,  112,  805,  145,  144,   86,
 /*   520 */    38,   38,  267,  267,  783,   38,  218,   29,  128,   28,
 /*   530 */   127,   31,  291,   30,   90,  737,   93,  738,  735,  219,
 /*   540 */   736,  199,   33,  133,   32,  132,  707,  200,  205,  197,
 /*   550 */   816,  206,   35,  207,   34,  211,  212,  213,  210,  194,
 /*   560 */  1028, 1162, 1154, 1099,  230, 1096, 1095,  231,  332,   64,
 /*   570 */   261, 1050, 1061, 1058, 1059, 1063,  161, 1043, 1082,  268,
 /*   580 */  1081,  108,  279, 1024,  889,  172, 1022,  937,  763,  293,
 /*   590 */   272,  297,  227,  274,  282,  280,  163,  294,  295,   94,
 /*   600 */    91, 1040,  286,  276,  164,  298,  299,   62,   66,  165,
 /*   610 */   192,   58,  284,  310,  166,  167,  931,  317, 1161,  168,
 /*   620 */   278,  169,  125,  170, 1160,  275, 1157,  324, 1153,   65,
 /*   630 */   130, 1152, 1149,  957,   59,   56,   63,  342,  193,  915,
 /*   640 */   139,  913,  141,  142,  911,  910,  253,  907,  906,  905,
 /*   650 */   904,  903,  902,  901,  898,  892,  885,  134,  334,  277,
 /*   660 */  1083,  335,  336,  337,  338,  339,  340,  350,  865,  254,
 /*   670 */   255,  864,  257,  258,  863,  267,  846,  845,   23,  936,
 /*   680 */   935,  119,  120,  262,  740,  102,  287,   46,  152,  269,
 /*   690 */   153,  180,  958,   19,   20,  178,  184,  179,  181,  182,
 /*   700 */     4,  183,    2,  765,   11,  158,  160,  776,  995,  959,
 /*   710 */   159,  229,  770,  105,   47,  772,  106,  281,   49,   24,
 /*   720 */  1005,   25,   26,   48,   40,   41,  113,  289,  115,   13,
 /*   730 */   116,  638,   52,   53,  117,  671,  673,  670,  669,  667,
 /*   740 */   666,  665,  662,  624,  308,    8,    9,  121,  822,  820,
 /*   750 */   314,  315,  124,   55,   87,  126,   88,  710,  709,  706,
 /*   760 */   654,  129,  131,  652,  644,  650,  646,  648,  642,  640,
 /*   770 */   676,  675,  674,  672,  668,  664,  663,  187,  622,   44,
 /*   780 */   586,  869,  868,  868,  868,  868,  868,  868,  868,  868,
 /*   790 */   868,  868,  868,   42,   43,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   269,    1,  209,    1,  200,  200,  209,  203,  204,    9,
 /*    10 */     5,    9,    7,   13,   14,  200,   16,   17,  203,  204,
 /*    20 */    20,   21,   22,   23,    1,   25,   26,   27,   28,   29,
 /*    30 */    30,  209,    9,    5,  200,   35,   36,  203,  204,   39,
 /*    40 */    40,   41,   13,   14,  200,   16,   17,  203,  204,   20,
 /*    50 */    21,   22,   23,  269,   25,   26,   27,   28,   29,   30,
 /*    60 */   200,  200,  269,  200,   35,   36,  269,  204,   39,   40,
 /*    70 */    41,   13,   14,  280,   16,   17,  269,  280,   20,   21,
 /*    80 */    22,   23,  277,   25,   26,   27,   28,   29,   30,   89,
 /*    90 */    88,  269,  200,   35,   36,  203,  204,   39,   40,   41,
 /*   100 */     1,  200,  280,   13,   14,  248,   16,   17,    9,  252,
 /*   110 */    20,   21,   22,   23,   85,   25,   26,   27,   28,   29,
 /*   120 */    30,   39,   40,   41,  200,   35,   36,  203,  204,   39,
 /*   130 */    40,   41,  127,   64,   65,   66,  275,  277,  277,  279,
 /*   140 */    71,   72,   73,   74,   75,  210,   47,   48,   49,   50,
 /*   150 */    51,   52,   53,   54,   55,   56,   57,   58,   59,   60,
 /*   160 */    61,   14,   63,   16,   17,  200,  201,   20,   21,   22,
 /*   170 */    23,  270,   25,   26,   27,   28,   29,   30,  243,  244,
 /*   180 */   245,  246,   35,   36,  200,   93,   39,   40,   41,   16,
 /*   190 */    17,  200,   84,   20,   21,   22,   23,   70,   25,   26,
 /*   200 */    27,   28,   29,   30,   67,   68,   69,  200,   35,   36,
 /*   210 */   203,  204,   39,   40,   41,  101,  102,  103,  104,  105,
 /*   220 */   106,  107,  108,  109,  110,  111,  112,  113,  114,  115,
 /*   230 */   122,  247,    5,  269,   46,  251,  226,  227,  228,  229,
 /*   240 */   230,  231,  232,  233,  234,  235,  236,  237,  238,  239,
 /*   250 */   240,   63,  200,    1,    2,  203,  204,    5,   70,    7,
 /*   260 */    86,    9,   35,   36,   76,   77,   78,   79,  277,    1,
 /*   270 */     2,   83,   84,    5,  147,    7,  149,    9,  151,  152,
 /*   280 */   269,   27,   28,   29,   30,  269,  250,   35,   36,   35,
 /*   290 */    36,   39,  208,   39,   40,   41,  280,   62,  101,  215,
 /*   300 */   103,  104,  266,   35,   36,  108,   64,   65,   66,  112,
 /*   310 */   122,  114,  115,   71,   72,   73,   74,   75,  200,   84,
 /*   320 */     2,  203,  204,    5,  250,    7,  200,    9,   70,  203,
 /*   330 */   204,  200,  144,   91,  146,    1,   84,   64,   65,   66,
 /*   340 */   266,  153,  269,    9,   71,  200,   73,   74,   75,  198,
 /*   350 */   199,  242,   84,   35,   36,   82,  226,  200,  228,  229,
 /*   360 */   203,  204,  200,  233,  200,  203,  204,  237,  256,  239,
 /*   370 */   240,  200,  120,  121,  203,  204,  200,   35,   36,  127,
 /*   380 */   249,   39,   40,   41,  200,  200,  274,  200,  120,  121,
 /*   390 */   203,  204,  247,    0,  200,  127,  251,  271,  272,  200,
 /*   400 */   200,  250,  200,   39,  145,  147,   15,  149,  200,  151,
 /*   410 */   152,  247,  269,  154,  155,  251,  248,  266,   84,   84,
 /*   420 */   252,  200,   88,  247,   89,  248,  210,  251,    5,  252,
 /*   430 */     7,  247,  247,  200,  213,  251,  251,  204,  120,  121,
 /*   440 */   200,  247,   64,   65,   66,  251,  247,  247,  200,  247,
 /*   450 */   251,  251,  204,  251,  119,  247,  128,  129,  253,  251,
 /*   460 */   200,   80,  246,  200,  208,  210,   78,  224,  225,  100,
 /*   470 */   208,  215,  267,  213,   93,   85,  213,  215,    1,   86,
 /*   480 */   211,  212,  206,  207,   85,   85,   85,   85,   85,  125,
 /*   490 */   100,  251,  100,   85,   85,  126,  100,   85,   84,  244,
 /*   500 */    85,  100,  100,  100,   85,   85,   85,  269,  100,  100,
 /*   510 */    85,  120,  100,   84,  269,  100,   39,   80,   81,  100,
 /*   520 */   100,  100,  123,  123,  136,  100,  269,  148,  148,  150,
 /*   530 */   150,  148,  118,  150,  142,    5,  140,    7,    5,  269,
 /*   540 */     7,  269,  148,  148,  150,  150,  117,  269,  269,  269,
 /*   550 */   127,  269,  148,  269,  150,  269,  269,  269,  269,  269,
 /*   560 */   252,  252,  252,  242,  242,  242,  242,  242,  242,  268,
 /*   570 */   200,  200,  200,  200,  200,  200,  200,  250,  278,  250,
 /*   580 */   278,   62,  200,  250,  200,  254,  200,  200,  127,  200,
 /*   590 */   273,   92,  273,  273,  132,  131,  264,  200,  200,  139,
 /*   600 */   141,  265,  134,  273,  263,  200,  200,  200,  138,  262,
 /*   610 */   200,  200,  137,  200,  261,  260,  200,  200,  200,  259,
 /*   620 */   130,  258,  200,  257,  200,  133,  200,  200,  200,  143,
 /*   630 */   200,  200,  200,  200,  200,  200,  200,  116,  200,  200,
 /*   640 */   200,  200,  200,  200,  200,  200,  200,  200,  200,  200,
 /*   650 */   200,  200,  200,  200,  200,  200,  200,   99,   98,  202,
 /*   660 */   202,   53,   95,   97,   57,   96,   94,   86,    5,  156,
 /*   670 */     5,    5,  156,    5,    5,  123,  103,  102,   84,  214,
 /*   680 */   214,  210,  210,  145,   85,  124,  118,   84,  205,  100,
 /*   690 */   205,  217,  223,  205,  205,  222,  216,  221,  220,  218,
 /*   700 */   206,  219,  211,   85,  100,   84,  100,   85,  241,  225,
 /*   710 */    84,    1,   85,   84,  100,   85,   84,   84,  255,  135,
 /*   720 */   241,  135,   84,  100,   84,   84,   80,  118,   89,  119,
 /*   730 */    72,    5,   90,   90,   89,    5,    9,    5,    5,    5,
 /*   740 */     5,    5,    5,   87,   15,   84,   84,   80,  120,   85,
 /*   750 */    26,   61,  150,  100,   16,  150,   16,    5,    5,   85,
 /*   760 */     5,  150,  150,    5,    5,    5,    5,    5,    5,    5,
 /*   770 */     5,    5,    5,    5,    5,    5,    5,  100,   87,   62,
 /*   780 */    62,    0,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   790 */   281,  281,  281,   21,   21,  281,  281,  281,  281,  281,
 /*   800 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   810 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   820 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   830 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   840 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   850 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   860 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   870 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   880 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   890 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   900 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   910 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   920 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   930 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   940 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   950 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   960 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   970 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   980 */   281,  281,  281,  281,  281,  281,  281,  281,  281,  281,
 /*   990 */   281,  281,  281,
};
#define YY_SHIFT_COUNT    (355)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (781)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */   188,  114,  114,  197,  197,  334,  174,  252,  268,  268,
 /*    10 */     2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
 /*    20 */     2,    2,    2,   23,   23,   23,   23,   23,   23,   23,
 /*    30 */    23,   23,   23,   23,   23,   23,    0,   99,  268,  318,
 /*    40 */   318,  318,    2,    2,    2,  108,  108,   23,   23,  328,
 /*    50 */    23,  393,   23,   23,   23,   23,  381,  174,   92,   92,
 /*    60 */    28,  795,  795,  795,  268,  268,  268,  268,  268,  268,
 /*    70 */   268,  268,  268,  268,  268,  268,  268,  268,  268,  268,
 /*    80 */   268,  268,  268,  268,  268,  268,  318,  318,  318,  227,
 /*    90 */   227,  227,  227,  227,  227,  227,   23,   23,  364,   23,
 /*   100 */    23,   23,  108,  108,   23,   23,   23,  388,   23,  388,
 /*   110 */   369,  108,   23,   23,   23,   23,   23,   23,   23,   23,
 /*   120 */    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
 /*   130 */    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
 /*   140 */    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
 /*   150 */    23,   23,   23,   23,   23,   23,   23,  461,  461,  461,
 /*   160 */   461,  519,  519,  460,  459,  468,  470,  475,  462,  464,
 /*   170 */   490,  492,  486,  499,  499,  521,  174,  174,  558,  560,
 /*   180 */   608,  567,  566,  607,  569,  572,  521,   28,  581,  581,
 /*   190 */   581,  581,  795,  795,   29,   58,   58,   90,   58,  147,
 /*   200 */   173,  242,  254,  254,  254,  254,  254,  254,  273,   69,
 /*   210 */   342,  342,  342,  342,  127,  258,  259,  335,   82,   82,
 /*   220 */     5,  423,  137,  378,  390,  399,  400,  401,  402,  403,
 /*   230 */   392,  396,  408,  409,  412,  415,  419,  414,  420,  421,
 /*   240 */   477,  235,  391,  425,  379,  380,  383,  530,  533,  394,
 /*   250 */   395,  429,  404,  437,  663,  513,  665,  666,  516,  668,
 /*   260 */   669,  573,  575,  538,  552,  568,  594,  561,  599,  603,
 /*   270 */   589,  604,  618,  621,  622,  626,  627,  606,  629,  630,
 /*   280 */   632,  710,  633,  614,  584,  623,  586,  638,  568,  640,
 /*   290 */   609,  641,  610,  646,  642,  639,  658,  726,  643,  645,
 /*   300 */   727,  730,  732,  733,  734,  735,  736,  737,  656,  729,
 /*   310 */   667,  661,  664,  628,  662,  724,  690,  738,  602,  605,
 /*   320 */   653,  653,  653,  653,  740,  611,  612,  653,  653,  653,
 /*   330 */   752,  753,  674,  653,  755,  758,  759,  760,  761,  762,
 /*   340 */   763,  764,  765,  766,  767,  768,  769,  770,  771,  677,
 /*   350 */   691,  772,  773,  717,  718,  781,
};
#define YY_REDUCE_COUNT (193)
#define YY_REDUCE_MIN   (-269)
#define YY_REDUCE_MAX   (494)
static const short yy_reduce_ofst[] = {
 /*     0 */   151,   10,   10,  130,  130,  126,  -65, -207, -203, -178,
 /*    10 */  -196, -185, -166, -156, -108,  -76,    7,   52,  118,  157,
 /*    20 */   162,  171,  187,  -16, -140, -139,  145,  164,  176,  184,
 /*    30 */   185,  194,  199,  200,  202,  208,  -99,  -35,   16, -143,
 /*    40 */   168,  177, -137,  233,  248,   36,   74, -195,   -9,  112,
 /*    50 */   131,  216,  221,  260,  263,  240,   84,  255,  256,  262,
 /*    60 */   243,  205,  269,  276, -269, -216, -193,  -36,   11,   73,
 /*    70 */   143,  238,  245,  257,  270,  272,  278,  279,  280,  282,
 /*    80 */   284,  286,  287,  288,  289,  290,  308,  309,  310,  109,
 /*    90 */   321,  322,  323,  324,  325,  326,  370,  371,  301,  372,
 /*   100 */   373,  374,  327,  329,  375,  376,  382,  300,  384,  302,
 /*   110 */   331,  333,  386,  387,  389,  397,  398,  405,  406,  407,
 /*   120 */   410,  411,  413,  416,  417,  418,  422,  424,  426,  427,
 /*   130 */   428,  430,  431,  432,  433,  434,  435,  436,  438,  439,
 /*   140 */   440,  441,  442,  443,  444,  445,  446,  447,  448,  449,
 /*   150 */   450,  451,  452,  453,  454,  455,  456,  317,  319,  320,
 /*   160 */   330,  457,  458,  336,  332,  341,  347,  353,  355,  360,
 /*   170 */   363,  366,  463,  465,  466,  467,  471,  472,  469,  473,
 /*   180 */   476,  474,  478,  481,  482,  480,  479,  484,  483,  485,
 /*   190 */   488,  489,  491,  494,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   866,  994,  933, 1004,  916,  866,  926, 1145, 1145, 1145,
 /*    10 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*    20 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*    30 */   866,  866,  866,  866,  866,  866, 1052,  886, 1145,  866,
 /*    40 */   866,  866,  866,  866,  866,  866,  866,  866,  866, 1067,
 /*    50 */   866,  926,  866,  866,  866,  866,  940,  926,  940,  940,
 /*    60 */   866, 1047,  978,  996,  866,  866,  866,  866,  866,  866,
 /*    70 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*    80 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*    90 */   866,  866,  866,  866,  866,  866,  866,  866, 1054, 1060,
 /*   100 */  1057,  866,  866,  866, 1062,  866,  866, 1086,  866, 1086,
 /*   110 */  1045,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   120 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   130 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  914,
 /*   140 */   866,  912,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   150 */   866,  866,  866,  866,  897,  866,  884,  866,  866,  866,
 /*   160 */   866,  888,  888, 1093, 1097, 1079, 1091, 1087, 1074, 1072,
 /*   170 */  1070, 1078, 1101,  938,  938,  934,  926,  926,  956,  954,
 /*   180 */   952,  944,  950,  946,  948,  942,  917,  866,  924,  924,
 /*   190 */   924,  924,  978,  996,  866, 1102, 1092,  866, 1144, 1132,
 /*   200 */  1131,  866, 1140, 1139, 1138, 1130, 1129, 1128,  866,  866,
 /*   210 */  1124, 1127, 1126, 1125,  866,  866,  866,  866, 1134, 1133,
 /*   220 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   230 */  1098, 1094,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   240 */   866, 1104,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   250 */   866, 1006,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   260 */   866,  866,  866,  866, 1044,  866,  866,  866,  866,  866,
 /*   270 */  1056, 1055,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   280 */   866,  866,  866, 1088,  866, 1080,  866,  866, 1018,  866,
 /*   290 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   300 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   310 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  866,
 /*   320 */  1163, 1158, 1159, 1156,  866,  866,  866, 1155, 1150, 1151,
 /*   330 */   866,  866,  866, 1148,  866,  866,  866,  866,  866,  866,
 /*   340 */   866,  866,  866,  866,  866,  866,  866,  866,  866,  962,
 /*   350 */   866,  895,  893,  927,  866,  866,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*         ID => nothing */
    1,  /*       BOOL => ID */
    1,  /*    TINYINT => ID */
    1,  /*   SMALLINT => ID */
    1,  /*    INTEGER => ID */
    1,  /*     BIGINT => ID */
    1,  /*      FLOAT => ID */
    1,  /*     DOUBLE => ID */
    1,  /*     STRING => ID */
    1,  /*  TIMESTAMP => ID */
    1,  /*     BINARY => ID */
    1,  /*      NCHAR => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*        NOT => nothing */
    0,  /*         EQ => nothing */
    0,  /*         NE => nothing */
    0,  /*     ISNULL => nothing */
    0,  /*    NOTNULL => nothing */
    0,  /*         IS => nothing */
    1,  /*       LIKE => ID */
    1,  /*      MATCH => ID */
    1,  /*     NMATCH => ID */
    1,  /*       GLOB => ID */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*         GT => nothing */
    0,  /*         GE => nothing */
    0,  /*         LT => nothing */
    0,  /*         LE => nothing */
    0,  /*     BITAND => nothing */
    0,  /*      BITOR => nothing */
    0,  /*     LSHIFT => nothing */
    0,  /*     RSHIFT => nothing */
    0,  /*       PLUS => nothing */
    0,  /*      MINUS => nothing */
    0,  /*     DIVIDE => nothing */
    0,  /*      TIMES => nothing */
    0,  /*       STAR => nothing */
    0,  /*      SLASH => nothing */
    0,  /*        REM => nothing */
    0,  /*     CONCAT => nothing */
    0,  /*     UMINUS => nothing */
    0,  /*      UPLUS => nothing */
    0,  /*     BITNOT => nothing */
    0,  /*       SHOW => nothing */
    0,  /*  DATABASES => nothing */
    0,  /*     TOPICS => nothing */
    0,  /*  FUNCTIONS => nothing */
    0,  /*     MNODES => nothing */
    0,  /*     DNODES => nothing */
    0,  /*   ACCOUNTS => nothing */
    0,  /*      USERS => nothing */
    0,  /*    MODULES => nothing */
    0,  /*    QUERIES => nothing */
    0,  /* CONNECTIONS => nothing */
    0,  /*    STREAMS => nothing */
    0,  /*  VARIABLES => nothing */
    0,  /*     SCORES => nothing */
    0,  /*     GRANTS => nothing */
    0,  /*     VNODES => nothing */
    0,  /*        DOT => nothing */
    0,  /*     CREATE => nothing */
    0,  /*      TABLE => nothing */
    1,  /*     STABLE => ID */
    1,  /*   DATABASE => ID */
    0,  /*     TABLES => nothing */
    0,  /*    STABLES => nothing */
    0,  /*    VGROUPS => nothing */
    0,  /*       DROP => nothing */
    0,  /*      TOPIC => nothing */
    0,  /*   FUNCTION => nothing */
    0,  /*      DNODE => nothing */
    0,  /*       USER => nothing */
    0,  /*    ACCOUNT => nothing */
    0,  /*        USE => nothing */
    0,  /*   DESCRIBE => nothing */
    1,  /*       DESC => ID */
    0,  /*      ALTER => nothing */
    0,  /*       PASS => nothing */
    0,  /*  PRIVILEGE => nothing */
    0,  /*      LOCAL => nothing */
    0,  /*    COMPACT => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         IF => nothing */
    0,  /*     EXISTS => nothing */
    0,  /*     ESCAPE => nothing */
    0,  /*         AS => nothing */
    0,  /* OUTPUTTYPE => nothing */
    0,  /*  AGGREGATE => nothing */
    0,  /*    BUFSIZE => nothing */
    0,  /*        PPS => nothing */
    0,  /*    TSERIES => nothing */
    0,  /*        DBS => nothing */
    0,  /*    STORAGE => nothing */
    0,  /*      QTIME => nothing */
    0,  /*      CONNS => nothing */
    0,  /*      STATE => nothing */
    0,  /*      COMMA => nothing */
    0,  /*       KEEP => nothing */
    0,  /*      CACHE => nothing */
    0,  /*    REPLICA => nothing */
    0,  /*     QUORUM => nothing */
    0,  /*       DAYS => nothing */
    0,  /*    MINROWS => nothing */
    0,  /*    MAXROWS => nothing */
    0,  /*     BLOCKS => nothing */
    0,  /*      CTIME => nothing */
    0,  /*        WAL => nothing */
    0,  /*      FSYNC => nothing */
    0,  /*       COMP => nothing */
    0,  /*  PRECISION => nothing */
    0,  /*     UPDATE => nothing */
    0,  /*  CACHELAST => nothing */
    0,  /* PARTITIONS => nothing */
    0,  /*   UNSIGNED => nothing */
    0,  /*       TAGS => nothing */
    0,  /*      USING => nothing */
    1,  /*       NULL => ID */
    1,  /*        NOW => ID */
    0,  /*     SELECT => nothing */
    0,  /*      UNION => nothing */
    1,  /*        ALL => ID */
    0,  /*   DISTINCT => nothing */
    0,  /*       FROM => nothing */
    0,  /*   VARIABLE => nothing */
    0,  /*   INTERVAL => nothing */
    0,  /*      EVERY => nothing */
    0,  /*    SESSION => nothing */
    0,  /* STATE_WINDOW => nothing */
    0,  /*       FILL => nothing */
    0,  /*    SLIDING => nothing */
    0,  /*      ORDER => nothing */
    0,  /*         BY => nothing */
    1,  /*        ASC => ID */
    0,  /*      GROUP => nothing */
    0,  /*     HAVING => nothing */
    0,  /*      LIMIT => nothing */
    1,  /*     OFFSET => ID */
    0,  /*     SLIMIT => nothing */
    0,  /*    SOFFSET => nothing */
    0,  /*      WHERE => nothing */
    0,  /*      RESET => nothing */
    0,  /*      QUERY => nothing */
    0,  /*     SYNCDB => nothing */
    0,  /*        ADD => nothing */
    0,  /*     COLUMN => nothing */
    0,  /*     MODIFY => nothing */
    0,  /*        TAG => nothing */
    0,  /*     CHANGE => nothing */
    0,  /*        SET => nothing */
    0,  /*       KILL => nothing */
    0,  /* CONNECTION => nothing */
    0,  /*     STREAM => nothing */
    0,  /*      COLON => nothing */
    1,  /*      ABORT => ID */
    1,  /*      AFTER => ID */
    1,  /*     ATTACH => ID */
    1,  /*     BEFORE => ID */
    1,  /*      BEGIN => ID */
    1,  /*    CASCADE => ID */
    1,  /*    CLUSTER => ID */
    1,  /*   CONFLICT => ID */
    1,  /*       COPY => ID */
    1,  /*   DEFERRED => ID */
    1,  /* DELIMITERS => ID */
    1,  /*     DETACH => ID */
    1,  /*       EACH => ID */
    1,  /*        END => ID */
    1,  /*    EXPLAIN => ID */
    1,  /*       FAIL => ID */
    1,  /*        FOR => ID */
    1,  /*     IGNORE => ID */
    1,  /*  IMMEDIATE => ID */
    1,  /*  INITIALLY => ID */
    1,  /*    INSTEAD => ID */
    1,  /*        KEY => ID */
    1,  /*         OF => ID */
    1,  /*      RAISE => ID */
    1,  /*    REPLACE => ID */
    1,  /*   RESTRICT => ID */
    1,  /*        ROW => ID */
    1,  /*  STATEMENT => ID */
    1,  /*    TRIGGER => ID */
    1,  /*       VIEW => ID */
    1,  /*    IPTOKEN => ID */
    1,  /*       SEMI => ID */
    1,  /*       NONE => ID */
    1,  /*       PREV => ID */
    1,  /*     LINEAR => ID */
    1,  /*     IMPORT => ID */
    1,  /*     TBNAME => ID */
    1,  /*       JOIN => ID */
    1,  /*     INSERT => ID */
    1,  /*       INTO => ID */
    1,  /*     VALUES => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  ParseARG_SDECL                /* A place to hold %extra_argument */
  ParseCTX_SDECL                /* A place to hold %extra_context */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
  yyStackEntry *yystackEnd;            /* Last entry in the stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  /*    0 */ "$",
  /*    1 */ "ID",
  /*    2 */ "BOOL",
  /*    3 */ "TINYINT",
  /*    4 */ "SMALLINT",
  /*    5 */ "INTEGER",
  /*    6 */ "BIGINT",
  /*    7 */ "FLOAT",
  /*    8 */ "DOUBLE",
  /*    9 */ "STRING",
  /*   10 */ "TIMESTAMP",
  /*   11 */ "BINARY",
  /*   12 */ "NCHAR",
  /*   13 */ "OR",
  /*   14 */ "AND",
  /*   15 */ "NOT",
  /*   16 */ "EQ",
  /*   17 */ "NE",
  /*   18 */ "ISNULL",
  /*   19 */ "NOTNULL",
  /*   20 */ "IS",
  /*   21 */ "LIKE",
  /*   22 */ "MATCH",
  /*   23 */ "NMATCH",
  /*   24 */ "GLOB",
  /*   25 */ "BETWEEN",
  /*   26 */ "IN",
  /*   27 */ "GT",
  /*   28 */ "GE",
  /*   29 */ "LT",
  /*   30 */ "LE",
  /*   31 */ "BITAND",
  /*   32 */ "BITOR",
  /*   33 */ "LSHIFT",
  /*   34 */ "RSHIFT",
  /*   35 */ "PLUS",
  /*   36 */ "MINUS",
  /*   37 */ "DIVIDE",
  /*   38 */ "TIMES",
  /*   39 */ "STAR",
  /*   40 */ "SLASH",
  /*   41 */ "REM",
  /*   42 */ "CONCAT",
  /*   43 */ "UMINUS",
  /*   44 */ "UPLUS",
  /*   45 */ "BITNOT",
  /*   46 */ "SHOW",
  /*   47 */ "DATABASES",
  /*   48 */ "TOPICS",
  /*   49 */ "FUNCTIONS",
  /*   50 */ "MNODES",
  /*   51 */ "DNODES",
  /*   52 */ "ACCOUNTS",
  /*   53 */ "USERS",
  /*   54 */ "MODULES",
  /*   55 */ "QUERIES",
  /*   56 */ "CONNECTIONS",
  /*   57 */ "STREAMS",
  /*   58 */ "VARIABLES",
  /*   59 */ "SCORES",
  /*   60 */ "GRANTS",
  /*   61 */ "VNODES",
  /*   62 */ "DOT",
  /*   63 */ "CREATE",
  /*   64 */ "TABLE",
  /*   65 */ "STABLE",
  /*   66 */ "DATABASE",
  /*   67 */ "TABLES",
  /*   68 */ "STABLES",
  /*   69 */ "VGROUPS",
  /*   70 */ "DROP",
  /*   71 */ "TOPIC",
  /*   72 */ "FUNCTION",
  /*   73 */ "DNODE",
  /*   74 */ "USER",
  /*   75 */ "ACCOUNT",
  /*   76 */ "USE",
  /*   77 */ "DESCRIBE",
  /*   78 */ "DESC",
  /*   79 */ "ALTER",
  /*   80 */ "PASS",
  /*   81 */ "PRIVILEGE",
  /*   82 */ "LOCAL",
  /*   83 */ "COMPACT",
  /*   84 */ "LP",
  /*   85 */ "RP",
  /*   86 */ "IF",
  /*   87 */ "EXISTS",
  /*   88 */ "ESCAPE",
  /*   89 */ "AS",
  /*   90 */ "OUTPUTTYPE",
  /*   91 */ "AGGREGATE",
  /*   92 */ "BUFSIZE",
  /*   93 */ "PPS",
  /*   94 */ "TSERIES",
  /*   95 */ "DBS",
  /*   96 */ "STORAGE",
  /*   97 */ "QTIME",
  /*   98 */ "CONNS",
  /*   99 */ "STATE",
  /*  100 */ "COMMA",
  /*  101 */ "KEEP",
  /*  102 */ "CACHE",
  /*  103 */ "REPLICA",
  /*  104 */ "QUORUM",
  /*  105 */ "DAYS",
  /*  106 */ "MINROWS",
  /*  107 */ "MAXROWS",
  /*  108 */ "BLOCKS",
  /*  109 */ "CTIME",
  /*  110 */ "WAL",
  /*  111 */ "FSYNC",
  /*  112 */ "COMP",
  /*  113 */ "PRECISION",
  /*  114 */ "UPDATE",
  /*  115 */ "CACHELAST",
  /*  116 */ "PARTITIONS",
  /*  117 */ "UNSIGNED",
  /*  118 */ "TAGS",
  /*  119 */ "USING",
  /*  120 */ "NULL",
  /*  121 */ "NOW",
  /*  122 */ "SELECT",
  /*  123 */ "UNION",
  /*  124 */ "ALL",
  /*  125 */ "DISTINCT",
  /*  126 */ "FROM",
  /*  127 */ "VARIABLE",
  /*  128 */ "INTERVAL",
  /*  129 */ "EVERY",
  /*  130 */ "SESSION",
  /*  131 */ "STATE_WINDOW",
  /*  132 */ "FILL",
  /*  133 */ "SLIDING",
  /*  134 */ "ORDER",
  /*  135 */ "BY",
  /*  136 */ "ASC",
  /*  137 */ "GROUP",
  /*  138 */ "HAVING",
  /*  139 */ "LIMIT",
  /*  140 */ "OFFSET",
  /*  141 */ "SLIMIT",
  /*  142 */ "SOFFSET",
  /*  143 */ "WHERE",
  /*  144 */ "RESET",
  /*  145 */ "QUERY",
  /*  146 */ "SYNCDB",
  /*  147 */ "ADD",
  /*  148 */ "COLUMN",
  /*  149 */ "MODIFY",
  /*  150 */ "TAG",
  /*  151 */ "CHANGE",
  /*  152 */ "SET",
  /*  153 */ "KILL",
  /*  154 */ "CONNECTION",
  /*  155 */ "STREAM",
  /*  156 */ "COLON",
  /*  157 */ "ABORT",
  /*  158 */ "AFTER",
  /*  159 */ "ATTACH",
  /*  160 */ "BEFORE",
  /*  161 */ "BEGIN",
  /*  162 */ "CASCADE",
  /*  163 */ "CLUSTER",
  /*  164 */ "CONFLICT",
  /*  165 */ "COPY",
  /*  166 */ "DEFERRED",
  /*  167 */ "DELIMITERS",
  /*  168 */ "DETACH",
  /*  169 */ "EACH",
  /*  170 */ "END",
  /*  171 */ "EXPLAIN",
  /*  172 */ "FAIL",
  /*  173 */ "FOR",
  /*  174 */ "IGNORE",
  /*  175 */ "IMMEDIATE",
  /*  176 */ "INITIALLY",
  /*  177 */ "INSTEAD",
  /*  178 */ "KEY",
  /*  179 */ "OF",
  /*  180 */ "RAISE",
  /*  181 */ "REPLACE",
  /*  182 */ "RESTRICT",
  /*  183 */ "ROW",
  /*  184 */ "STATEMENT",
  /*  185 */ "TRIGGER",
  /*  186 */ "VIEW",
  /*  187 */ "IPTOKEN",
  /*  188 */ "SEMI",
  /*  189 */ "NONE",
  /*  190 */ "PREV",
  /*  191 */ "LINEAR",
  /*  192 */ "IMPORT",
  /*  193 */ "TBNAME",
  /*  194 */ "JOIN",
  /*  195 */ "INSERT",
  /*  196 */ "INTO",
  /*  197 */ "VALUES",
  /*  198 */ "program",
  /*  199 */ "cmd",
  /*  200 */ "ids",
  /*  201 */ "dbPrefix",
  /*  202 */ "cpxName",
  /*  203 */ "ftblname",
  /*  204 */ "tblname",
  /*  205 */ "ifexists",
  /*  206 */ "alter_db_optr",
  /*  207 */ "alter_topic_optr",
  /*  208 */ "acct_optr",
  /*  209 */ "exprlist",
  /*  210 */ "ifnotexists",
  /*  211 */ "db_optr",
  /*  212 */ "topic_optr",
  /*  213 */ "typename",
  /*  214 */ "bufsize",
  /*  215 */ "pps",
  /*  216 */ "tseries",
  /*  217 */ "dbs",
  /*  218 */ "streams",
  /*  219 */ "storage",
  /*  220 */ "qtime",
  /*  221 */ "users",
  /*  222 */ "conns",
  /*  223 */ "state",
  /*  224 */ "intitemlist",
  /*  225 */ "intitem",
  /*  226 */ "keep",
  /*  227 */ "cache",
  /*  228 */ "replica",
  /*  229 */ "quorum",
  /*  230 */ "days",
  /*  231 */ "minrows",
  /*  232 */ "maxrows",
  /*  233 */ "blocks",
  /*  234 */ "ctime",
  /*  235 */ "wal",
  /*  236 */ "fsync",
  /*  237 */ "comp",
  /*  238 */ "prec",
  /*  239 */ "update",
  /*  240 */ "cachelast",
  /*  241 */ "partitions",
  /*  242 */ "signed",
  /*  243 */ "create_table_args",
  /*  244 */ "create_stable_args",
  /*  245 */ "create_table_list",
  /*  246 */ "create_from_stable",
  /*  247 */ "columnlist",
  /*  248 */ "tagitemlist",
  /*  249 */ "tagNamelist",
  /*  250 */ "select",
  /*  251 */ "column",
  /*  252 */ "tagitem",
  /*  253 */ "selcollist",
  /*  254 */ "from",
  /*  255 */ "where_opt",
  /*  256 */ "interval_option",
  /*  257 */ "sliding_opt",
  /*  258 */ "session_option",
  /*  259 */ "windowstate_option",
  /*  260 */ "fill_opt",
  /*  261 */ "groupby_opt",
  /*  262 */ "having_opt",
  /*  263 */ "orderby_opt",
  /*  264 */ "slimit_opt",
  /*  265 */ "limit_opt",
  /*  266 */ "union",
  /*  267 */ "sclp",
  /*  268 */ "distinct",
  /*  269 */ "expr",
  /*  270 */ "as",
  /*  271 */ "tablelist",
  /*  272 */ "sub",
  /*  273 */ "tmvar",
  /*  274 */ "intervalKey",
  /*  275 */ "sortlist",
  /*  276 */ "sortitem",
  /*  277 */ "item",
  /*  278 */ "sortorder",
  /*  279 */ "grouplist",
  /*  280 */ "expritem",
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "program ::= cmd",
 /*   1 */ "cmd ::= SHOW DATABASES",
 /*   2 */ "cmd ::= SHOW TOPICS",
 /*   3 */ "cmd ::= SHOW FUNCTIONS",
 /*   4 */ "cmd ::= SHOW MNODES",
 /*   5 */ "cmd ::= SHOW DNODES",
 /*   6 */ "cmd ::= SHOW ACCOUNTS",
 /*   7 */ "cmd ::= SHOW USERS",
 /*   8 */ "cmd ::= SHOW MODULES",
 /*   9 */ "cmd ::= SHOW QUERIES",
 /*  10 */ "cmd ::= SHOW CONNECTIONS",
 /*  11 */ "cmd ::= SHOW STREAMS",
 /*  12 */ "cmd ::= SHOW VARIABLES",
 /*  13 */ "cmd ::= SHOW SCORES",
 /*  14 */ "cmd ::= SHOW GRANTS",
 /*  15 */ "cmd ::= SHOW VNODES",
 /*  16 */ "cmd ::= SHOW VNODES ids",
 /*  17 */ "dbPrefix ::=",
 /*  18 */ "dbPrefix ::= ids DOT",
 /*  19 */ "cpxName ::=",
 /*  20 */ "cpxName ::= DOT ids",
 /*  21 */ "cmd ::= SHOW CREATE TABLE ftblname",
 /*  22 */ "cmd ::= SHOW CREATE STABLE ftblname",
 /*  23 */ "cmd ::= SHOW CREATE DATABASE ids",
 /*  24 */ "cmd ::= SHOW dbPrefix TABLES",
 /*  25 */ "cmd ::= SHOW dbPrefix TABLES LIKE tblname",
 /*  26 */ "cmd ::= SHOW dbPrefix STABLES",
 /*  27 */ "cmd ::= SHOW dbPrefix STABLES LIKE tblname",
 /*  28 */ "cmd ::= SHOW dbPrefix VGROUPS",
 /*  29 */ "cmd ::= SHOW dbPrefix VGROUPS ids",
 /*  30 */ "cmd ::= DROP TABLE ifexists ftblname",
 /*  31 */ "cmd ::= DROP STABLE ifexists ftblname",
 /*  32 */ "cmd ::= DROP DATABASE ifexists ids",
 /*  33 */ "cmd ::= DROP TOPIC ifexists ids",
 /*  34 */ "cmd ::= DROP FUNCTION ids",
 /*  35 */ "cmd ::= DROP DNODE ids",
 /*  36 */ "cmd ::= DROP USER ids",
 /*  37 */ "cmd ::= DROP ACCOUNT ids",
 /*  38 */ "cmd ::= USE ids",
 /*  39 */ "cmd ::= DESCRIBE ftblname",
 /*  40 */ "cmd ::= DESC ftblname",
 /*  41 */ "cmd ::= ALTER USER ids PASS ids",
 /*  42 */ "cmd ::= ALTER USER ids PRIVILEGE ids",
 /*  43 */ "cmd ::= ALTER DNODE ids ids",
 /*  44 */ "cmd ::= ALTER DNODE ids ids ids",
 /*  45 */ "cmd ::= ALTER LOCAL ids",
 /*  46 */ "cmd ::= ALTER LOCAL ids ids",
 /*  47 */ "cmd ::= ALTER DATABASE ids alter_db_optr",
 /*  48 */ "cmd ::= ALTER TOPIC ids alter_topic_optr",
 /*  49 */ "cmd ::= ALTER ACCOUNT ids acct_optr",
 /*  50 */ "cmd ::= ALTER ACCOUNT ids PASS ids acct_optr",
 /*  51 */ "cmd ::= COMPACT VNODES IN LP exprlist RP",
 /*  52 */ "ids ::= ID",
 /*  53 */ "ids ::= STRING",
 /*  54 */ "ifexists ::= IF EXISTS",
 /*  55 */ "ifexists ::=",
 /*  56 */ "ifnotexists ::= IF NOT EXISTS",
 /*  57 */ "ifnotexists ::=",
 /*  58 */ "tblname ::= ids",
 /*  59 */ "tblname ::= ESCAPE",
 /*  60 */ "ftblname ::= tblname",
 /*  61 */ "ftblname ::= ids DOT tblname",
 /*  62 */ "cmd ::= CREATE DNODE ids",
 /*  63 */ "cmd ::= CREATE ACCOUNT ids PASS ids acct_optr",
 /*  64 */ "cmd ::= CREATE DATABASE ifnotexists ids db_optr",
 /*  65 */ "cmd ::= CREATE TOPIC ifnotexists ids topic_optr",
 /*  66 */ "cmd ::= CREATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize",
 /*  67 */ "cmd ::= CREATE AGGREGATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize",
 /*  68 */ "cmd ::= CREATE USER ids PASS ids",
 /*  69 */ "bufsize ::=",
 /*  70 */ "bufsize ::= BUFSIZE INTEGER",
 /*  71 */ "pps ::=",
 /*  72 */ "pps ::= PPS INTEGER",
 /*  73 */ "tseries ::=",
 /*  74 */ "tseries ::= TSERIES INTEGER",
 /*  75 */ "dbs ::=",
 /*  76 */ "dbs ::= DBS INTEGER",
 /*  77 */ "streams ::=",
 /*  78 */ "streams ::= STREAMS INTEGER",
 /*  79 */ "storage ::=",
 /*  80 */ "storage ::= STORAGE INTEGER",
 /*  81 */ "qtime ::=",
 /*  82 */ "qtime ::= QTIME INTEGER",
 /*  83 */ "users ::=",
 /*  84 */ "users ::= USERS INTEGER",
 /*  85 */ "conns ::=",
 /*  86 */ "conns ::= CONNS INTEGER",
 /*  87 */ "state ::=",
 /*  88 */ "state ::= STATE ids",
 /*  89 */ "acct_optr ::= pps tseries storage streams qtime dbs users conns state",
 /*  90 */ "intitemlist ::= intitemlist COMMA intitem",
 /*  91 */ "intitemlist ::= intitem",
 /*  92 */ "intitem ::= INTEGER",
 /*  93 */ "keep ::= KEEP intitemlist",
 /*  94 */ "cache ::= CACHE INTEGER",
 /*  95 */ "replica ::= REPLICA INTEGER",
 /*  96 */ "quorum ::= QUORUM INTEGER",
 /*  97 */ "days ::= DAYS INTEGER",
 /*  98 */ "minrows ::= MINROWS INTEGER",
 /*  99 */ "maxrows ::= MAXROWS INTEGER",
 /* 100 */ "blocks ::= BLOCKS INTEGER",
 /* 101 */ "ctime ::= CTIME INTEGER",
 /* 102 */ "wal ::= WAL INTEGER",
 /* 103 */ "fsync ::= FSYNC INTEGER",
 /* 104 */ "comp ::= COMP INTEGER",
 /* 105 */ "prec ::= PRECISION STRING",
 /* 106 */ "update ::= UPDATE INTEGER",
 /* 107 */ "cachelast ::= CACHELAST INTEGER",
 /* 108 */ "partitions ::= PARTITIONS INTEGER",
 /* 109 */ "db_optr ::=",
 /* 110 */ "db_optr ::= db_optr cache",
 /* 111 */ "db_optr ::= db_optr replica",
 /* 112 */ "db_optr ::= db_optr quorum",
 /* 113 */ "db_optr ::= db_optr days",
 /* 114 */ "db_optr ::= db_optr minrows",
 /* 115 */ "db_optr ::= db_optr maxrows",
 /* 116 */ "db_optr ::= db_optr blocks",
 /* 117 */ "db_optr ::= db_optr ctime",
 /* 118 */ "db_optr ::= db_optr wal",
 /* 119 */ "db_optr ::= db_optr fsync",
 /* 120 */ "db_optr ::= db_optr comp",
 /* 121 */ "db_optr ::= db_optr prec",
 /* 122 */ "db_optr ::= db_optr keep",
 /* 123 */ "db_optr ::= db_optr update",
 /* 124 */ "db_optr ::= db_optr cachelast",
 /* 125 */ "topic_optr ::= db_optr",
 /* 126 */ "topic_optr ::= topic_optr partitions",
 /* 127 */ "alter_db_optr ::=",
 /* 128 */ "alter_db_optr ::= alter_db_optr replica",
 /* 129 */ "alter_db_optr ::= alter_db_optr quorum",
 /* 130 */ "alter_db_optr ::= alter_db_optr keep",
 /* 131 */ "alter_db_optr ::= alter_db_optr blocks",
 /* 132 */ "alter_db_optr ::= alter_db_optr comp",
 /* 133 */ "alter_db_optr ::= alter_db_optr update",
 /* 134 */ "alter_db_optr ::= alter_db_optr cachelast",
 /* 135 */ "alter_topic_optr ::= alter_db_optr",
 /* 136 */ "alter_topic_optr ::= alter_topic_optr partitions",
 /* 137 */ "typename ::= ids",
 /* 138 */ "typename ::= ids LP signed RP",
 /* 139 */ "typename ::= ids UNSIGNED",
 /* 140 */ "signed ::= INTEGER",
 /* 141 */ "signed ::= PLUS INTEGER",
 /* 142 */ "signed ::= MINUS INTEGER",
 /* 143 */ "cmd ::= CREATE TABLE create_table_args",
 /* 144 */ "cmd ::= CREATE TABLE create_stable_args",
 /* 145 */ "cmd ::= CREATE STABLE create_stable_args",
 /* 146 */ "cmd ::= CREATE TABLE create_table_list",
 /* 147 */ "create_table_list ::= create_from_stable",
 /* 148 */ "create_table_list ::= create_table_list create_from_stable",
 /* 149 */ "create_table_args ::= ifnotexists ftblname LP columnlist RP",
 /* 150 */ "create_stable_args ::= ifnotexists ftblname LP columnlist RP TAGS LP columnlist RP",
 /* 151 */ "create_from_stable ::= ifnotexists ftblname USING ftblname TAGS LP tagitemlist RP",
 /* 152 */ "create_from_stable ::= ifnotexists ftblname USING ftblname LP tagNamelist RP TAGS LP tagitemlist RP",
 /* 153 */ "tagNamelist ::= tagNamelist COMMA ids",
 /* 154 */ "tagNamelist ::= ids",
 /* 155 */ "create_table_args ::= ifnotexists ftblname AS select",
 /* 156 */ "columnlist ::= columnlist COMMA column",
 /* 157 */ "columnlist ::= column",
 /* 158 */ "column ::= ids typename",
 /* 159 */ "tagitemlist ::= tagitemlist COMMA tagitem",
 /* 160 */ "tagitemlist ::= tagitem",
 /* 161 */ "tagitem ::= INTEGER",
 /* 162 */ "tagitem ::= FLOAT",
 /* 163 */ "tagitem ::= STRING",
 /* 164 */ "tagitem ::= BOOL",
 /* 165 */ "tagitem ::= NULL",
 /* 166 */ "tagitem ::= NOW",
 /* 167 */ "tagitem ::= MINUS INTEGER",
 /* 168 */ "tagitem ::= MINUS FLOAT",
 /* 169 */ "tagitem ::= PLUS INTEGER",
 /* 170 */ "tagitem ::= PLUS FLOAT",
 /* 171 */ "select ::= SELECT selcollist from where_opt interval_option sliding_opt session_option windowstate_option fill_opt groupby_opt having_opt orderby_opt slimit_opt limit_opt",
 /* 172 */ "select ::= LP select RP",
 /* 173 */ "union ::= select",
 /* 174 */ "union ::= union UNION ALL select",
 /* 175 */ "cmd ::= union",
 /* 176 */ "select ::= SELECT selcollist",
 /* 177 */ "sclp ::= selcollist COMMA",
 /* 178 */ "sclp ::=",
 /* 179 */ "selcollist ::= sclp distinct expr as",
 /* 180 */ "selcollist ::= sclp STAR",
 /* 181 */ "as ::= AS ids",
 /* 182 */ "as ::= ids",
 /* 183 */ "as ::=",
 /* 184 */ "distinct ::= DISTINCT",
 /* 185 */ "distinct ::=",
 /* 186 */ "from ::= FROM tablelist",
 /* 187 */ "from ::= FROM sub",
 /* 188 */ "sub ::= LP union RP",
 /* 189 */ "sub ::= LP union RP ids",
 /* 190 */ "sub ::= sub COMMA LP union RP ids",
 /* 191 */ "tablelist ::= ftblname",
 /* 192 */ "tablelist ::= ftblname ids",
 /* 193 */ "tablelist ::= tablelist COMMA ftblname",
 /* 194 */ "tablelist ::= tablelist COMMA ftblname ids",
 /* 195 */ "tmvar ::= VARIABLE",
 /* 196 */ "interval_option ::= intervalKey LP tmvar RP",
 /* 197 */ "interval_option ::= intervalKey LP tmvar COMMA tmvar RP",
 /* 198 */ "interval_option ::=",
 /* 199 */ "intervalKey ::= INTERVAL",
 /* 200 */ "intervalKey ::= EVERY",
 /* 201 */ "session_option ::=",
 /* 202 */ "session_option ::= SESSION LP ids cpxName COMMA tmvar RP",
 /* 203 */ "windowstate_option ::=",
 /* 204 */ "windowstate_option ::= STATE_WINDOW LP ids RP",
 /* 205 */ "fill_opt ::=",
 /* 206 */ "fill_opt ::= FILL LP ID COMMA tagitemlist RP",
 /* 207 */ "fill_opt ::= FILL LP ID RP",
 /* 208 */ "sliding_opt ::= SLIDING LP tmvar RP",
 /* 209 */ "sliding_opt ::=",
 /* 210 */ "orderby_opt ::=",
 /* 211 */ "orderby_opt ::= ORDER BY sortlist",
 /* 212 */ "sortlist ::= sortlist COMMA item sortorder",
 /* 213 */ "sortlist ::= item sortorder",
 /* 214 */ "item ::= ids cpxName",
 /* 215 */ "sortorder ::= ASC",
 /* 216 */ "sortorder ::= DESC",
 /* 217 */ "sortorder ::=",
 /* 218 */ "groupby_opt ::=",
 /* 219 */ "groupby_opt ::= GROUP BY grouplist",
 /* 220 */ "grouplist ::= grouplist COMMA item",
 /* 221 */ "grouplist ::= item",
 /* 222 */ "having_opt ::=",
 /* 223 */ "having_opt ::= HAVING expr",
 /* 224 */ "limit_opt ::=",
 /* 225 */ "limit_opt ::= LIMIT signed",
 /* 226 */ "limit_opt ::= LIMIT signed OFFSET signed",
 /* 227 */ "limit_opt ::= LIMIT signed COMMA signed",
 /* 228 */ "slimit_opt ::=",
 /* 229 */ "slimit_opt ::= SLIMIT signed",
 /* 230 */ "slimit_opt ::= SLIMIT signed SOFFSET signed",
 /* 231 */ "slimit_opt ::= SLIMIT signed COMMA signed",
 /* 232 */ "where_opt ::=",
 /* 233 */ "where_opt ::= WHERE expr",
 /* 234 */ "expr ::= LP expr RP",
 /* 235 */ "expr ::= ID",
 /* 236 */ "expr ::= ID DOT ID",
 /* 237 */ "expr ::= ID DOT STAR",
 /* 238 */ "expr ::= INTEGER",
 /* 239 */ "expr ::= MINUS INTEGER",
 /* 240 */ "expr ::= PLUS INTEGER",
 /* 241 */ "expr ::= FLOAT",
 /* 242 */ "expr ::= MINUS FLOAT",
 /* 243 */ "expr ::= PLUS FLOAT",
 /* 244 */ "expr ::= STRING",
 /* 245 */ "expr ::= NOW",
 /* 246 */ "expr ::= VARIABLE",
 /* 247 */ "expr ::= PLUS VARIABLE",
 /* 248 */ "expr ::= MINUS VARIABLE",
 /* 249 */ "expr ::= BOOL",
 /* 250 */ "expr ::= NULL",
 /* 251 */ "expr ::= ID LP exprlist RP",
 /* 252 */ "expr ::= ID LP STAR RP",
 /* 253 */ "expr ::= expr IS NULL",
 /* 254 */ "expr ::= expr IS NOT NULL",
 /* 255 */ "expr ::= expr LT expr",
 /* 256 */ "expr ::= expr GT expr",
 /* 257 */ "expr ::= expr LE expr",
 /* 258 */ "expr ::= expr GE expr",
 /* 259 */ "expr ::= expr NE expr",
 /* 260 */ "expr ::= expr EQ expr",
 /* 261 */ "expr ::= expr BETWEEN expr AND expr",
 /* 262 */ "expr ::= expr AND expr",
 /* 263 */ "expr ::= expr OR expr",
 /* 264 */ "expr ::= expr PLUS expr",
 /* 265 */ "expr ::= expr MINUS expr",
 /* 266 */ "expr ::= expr STAR expr",
 /* 267 */ "expr ::= expr SLASH expr",
 /* 268 */ "expr ::= expr REM expr",
 /* 269 */ "expr ::= expr LIKE expr",
 /* 270 */ "expr ::= expr MATCH expr",
 /* 271 */ "expr ::= expr NMATCH expr",
 /* 272 */ "expr ::= expr IN LP exprlist RP",
 /* 273 */ "exprlist ::= exprlist COMMA expritem",
 /* 274 */ "exprlist ::= expritem",
 /* 275 */ "expritem ::= expr",
 /* 276 */ "expritem ::=",
 /* 277 */ "cmd ::= RESET QUERY CACHE",
 /* 278 */ "cmd ::= SYNCDB ids REPLICA",
 /* 279 */ "cmd ::= ALTER TABLE ftblname ADD COLUMN columnlist",
 /* 280 */ "cmd ::= ALTER TABLE ftblname DROP COLUMN ids",
 /* 281 */ "cmd ::= ALTER TABLE ftblname MODIFY COLUMN columnlist",
 /* 282 */ "cmd ::= ALTER TABLE ftblname ADD TAG columnlist",
 /* 283 */ "cmd ::= ALTER TABLE ftblname DROP TAG ids",
 /* 284 */ "cmd ::= ALTER TABLE ftblname CHANGE TAG ids ids",
 /* 285 */ "cmd ::= ALTER TABLE ftblname SET TAG ids EQ tagitem",
 /* 286 */ "cmd ::= ALTER TABLE ftblname MODIFY TAG columnlist",
 /* 287 */ "cmd ::= ALTER STABLE ftblname ADD COLUMN columnlist",
 /* 288 */ "cmd ::= ALTER STABLE ftblname DROP COLUMN ids",
 /* 289 */ "cmd ::= ALTER STABLE ftblname MODIFY COLUMN columnlist",
 /* 290 */ "cmd ::= ALTER STABLE ftblname ADD TAG columnlist",
 /* 291 */ "cmd ::= ALTER STABLE ftblname DROP TAG ids",
 /* 292 */ "cmd ::= ALTER STABLE ftblname CHANGE TAG ids ids",
 /* 293 */ "cmd ::= ALTER STABLE ftblname SET TAG ids EQ tagitem",
 /* 294 */ "cmd ::= ALTER STABLE ftblname MODIFY TAG columnlist",
 /* 295 */ "cmd ::= KILL CONNECTION INTEGER",
 /* 296 */ "cmd ::= KILL STREAM INTEGER COLON INTEGER",
 /* 297 */ "cmd ::= KILL QUERY INTEGER COLON INTEGER",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to ParseAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* Initialize a new parser that has already been allocated.
*/
void ParseInit(void *yypRawParser ParseCTX_PDECL){
  yyParser *yypParser = (yyParser*)yypRawParser;
  ParseCTX_STORE
#ifdef YYTRACKMAXSTACKDEPTH
  yypParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
  yypParser->yytos = NULL;
  yypParser->yystack = NULL;
  yypParser->yystksz = 0;
  if( yyGrowStack(yypParser) ){
    yypParser->yystack = &yypParser->yystk0;
    yypParser->yystksz = 1;
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  yypParser->yytos = yypParser->yystack;
  yypParser->yystack[0].stateno = 0;
  yypParser->yystack[0].major = 0;
#if YYSTACKDEPTH>0
  yypParser->yystackEnd = &yypParser->yystack[YYSTACKDEPTH-1];
#endif
}

#ifndef Parse_ENGINEALWAYSONSTACK
/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(YYMALLOCARGTYPE) ParseCTX_PDECL){
  yyParser *yypParser;
  yypParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( yypParser ){
    ParseCTX_STORE
    ParseInit(yypParser ParseCTX_PARAM);
  }
  return (void*)yypParser;
}
#endif /* Parse_ENGINEALWAYSONSTACK */


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH
  ParseCTX_FETCH
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 209: /* exprlist */
    case 253: /* selcollist */
    case 267: /* sclp */
{
tSqlExprListDestroy((yypminor->yy247));
}
      break;
    case 224: /* intitemlist */
    case 226: /* keep */
    case 247: /* columnlist */
    case 248: /* tagitemlist */
    case 249: /* tagNamelist */
    case 260: /* fill_opt */
    case 261: /* groupby_opt */
    case 263: /* orderby_opt */
    case 275: /* sortlist */
    case 279: /* grouplist */
{
taosArrayDestroy((yypminor->yy247));
}
      break;
    case 245: /* create_table_list */
{
destroyCreateTableSql((yypminor->yy336));
}
      break;
    case 250: /* select */
{
destroySqlNode((yypminor->yy246));
}
      break;
    case 254: /* from */
    case 271: /* tablelist */
    case 272: /* sub */
{
destroyRelationInfo((yypminor->yy46));
}
      break;
    case 255: /* where_opt */
    case 262: /* having_opt */
    case 269: /* expr */
    case 280: /* expritem */
{
tSqlExprDestroy((yypminor->yy44));
}
      break;
    case 266: /* union */
{
destroyAllSqlNode((yypminor->yy247));
}
      break;
    case 276: /* sortitem */
{
tVariantDestroy(&(yypminor->yy378));
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/*
** Clear all secondary memory allocations from the parser
*/
void ParseFinalize(void *p){
  yyParser *pParser = (yyParser*)p;
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
}

#ifndef Parse_ENGINEALWAYSONSTACK
/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
#ifndef YYPARSEFREENEVERNULL
  if( p==0 ) return;
#endif
  ParseFinalize(p);
  (*freeProc)(p);
}
#endif /* Parse_ENGINEALWAYSONSTACK */

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int ParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/* This array of booleans keeps track of the parser statement
** coverage.  The element yycoverage[X][Y] is set when the parser
** is in state X and has a lookahead token Y.  In a well-tested
** systems, every element of this matrix should end up being set.
*/
#if defined(YYCOVERAGE)
static unsigned char yycoverage[YYNSTATE][YYNTOKEN];
#endif

/*
** Write into out a description of every state/lookahead combination that
**
**   (1)  has not been used by the parser, and
**   (2)  is not a syntax error.
**
** Return the number of missed state/lookahead combinations.
*/
#if defined(YYCOVERAGE)
int ParseCoverage(FILE *out){
  int stateno, iLookAhead, i;
  int nMissed = 0;
  for(stateno=0; stateno<YYNSTATE; stateno++){
    i = yy_shift_ofst[stateno];
    for(iLookAhead=0; iLookAhead<YYNTOKEN; iLookAhead++){
      if( yy_lookahead[i+iLookAhead]!=iLookAhead ) continue;
      if( yycoverage[stateno][iLookAhead]==0 ) nMissed++;
      if( out ){
        fprintf(out,"State %d lookahead %s %s\n", stateno,
                yyTokenName[iLookAhead],
                yycoverage[stateno][iLookAhead] ? "ok" : "missed");
      }
    }
  }
  return nMissed;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_shift_action(
  YYCODETYPE iLookAhead,    /* The look-ahead token */
  YYACTIONTYPE stateno      /* Current state number */
){
  int i;

  if( stateno>YY_MAX_SHIFT ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
#if defined(YYCOVERAGE)
  yycoverage[stateno][iLookAhead] = 1;
#endif
  do{
    i = yy_shift_ofst[stateno];
    assert( i>=0 );
    assert( i<=YY_ACTTAB_COUNT );
    assert( i+YYNTOKEN<=(int)YY_NLOOKAHEAD );
    assert( iLookAhead!=YYNOCODE );
    assert( iLookAhead < YYNTOKEN );
    i += iLookAhead;
    assert( i<(int)YY_NLOOKAHEAD );
    if( yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      assert( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0]) );
      iFallback = yyFallback[iLookAhead];
      if( iFallback!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        assert( j<(int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])) );
        if( yy_lookahead[j]==YYWILDCARD && iLookAhead>0 ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      assert( i>=0 && i<sizeof(yy_action)/sizeof(yy_action[0]) );
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_reduce_action(
  YYACTIONTYPE stateno,     /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   ParseARG_FETCH
   ParseCTX_FETCH
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
/******** End %stack_overflow code ********************************************/
   ParseARG_STORE /* Suppress warning about unused %extra_argument var */
   ParseCTX_STORE
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState, const char *zTag){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%s%s '%s', go to state %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%s%s '%s', pending reduce %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState - YY_MIN_REDUCE);
    }
  }
}
#else
# define yyTraceShift(X,Y,Z)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  YYACTIONTYPE yyNewState,      /* The new state to shift in */
  YYCODETYPE yyMajor,           /* The major token to shift in */
  ParseTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>yypParser->yystackEnd ){
    yypParser->yytos--;
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState, "Shift");
}

/* For rule J, yyRuleInfoLhs[J] contains the symbol on the left-hand side
** of that rule */
static const YYCODETYPE yyRuleInfoLhs[] = {
   198,  /* (0) program ::= cmd */
   199,  /* (1) cmd ::= SHOW DATABASES */
   199,  /* (2) cmd ::= SHOW TOPICS */
   199,  /* (3) cmd ::= SHOW FUNCTIONS */
   199,  /* (4) cmd ::= SHOW MNODES */
   199,  /* (5) cmd ::= SHOW DNODES */
   199,  /* (6) cmd ::= SHOW ACCOUNTS */
   199,  /* (7) cmd ::= SHOW USERS */
   199,  /* (8) cmd ::= SHOW MODULES */
   199,  /* (9) cmd ::= SHOW QUERIES */
   199,  /* (10) cmd ::= SHOW CONNECTIONS */
   199,  /* (11) cmd ::= SHOW STREAMS */
   199,  /* (12) cmd ::= SHOW VARIABLES */
   199,  /* (13) cmd ::= SHOW SCORES */
   199,  /* (14) cmd ::= SHOW GRANTS */
   199,  /* (15) cmd ::= SHOW VNODES */
   199,  /* (16) cmd ::= SHOW VNODES ids */
   201,  /* (17) dbPrefix ::= */
   201,  /* (18) dbPrefix ::= ids DOT */
   202,  /* (19) cpxName ::= */
   202,  /* (20) cpxName ::= DOT ids */
   199,  /* (21) cmd ::= SHOW CREATE TABLE ftblname */
   199,  /* (22) cmd ::= SHOW CREATE STABLE ftblname */
   199,  /* (23) cmd ::= SHOW CREATE DATABASE ids */
   199,  /* (24) cmd ::= SHOW dbPrefix TABLES */
   199,  /* (25) cmd ::= SHOW dbPrefix TABLES LIKE tblname */
   199,  /* (26) cmd ::= SHOW dbPrefix STABLES */
   199,  /* (27) cmd ::= SHOW dbPrefix STABLES LIKE tblname */
   199,  /* (28) cmd ::= SHOW dbPrefix VGROUPS */
   199,  /* (29) cmd ::= SHOW dbPrefix VGROUPS ids */
   199,  /* (30) cmd ::= DROP TABLE ifexists ftblname */
   199,  /* (31) cmd ::= DROP STABLE ifexists ftblname */
   199,  /* (32) cmd ::= DROP DATABASE ifexists ids */
   199,  /* (33) cmd ::= DROP TOPIC ifexists ids */
   199,  /* (34) cmd ::= DROP FUNCTION ids */
   199,  /* (35) cmd ::= DROP DNODE ids */
   199,  /* (36) cmd ::= DROP USER ids */
   199,  /* (37) cmd ::= DROP ACCOUNT ids */
   199,  /* (38) cmd ::= USE ids */
   199,  /* (39) cmd ::= DESCRIBE ftblname */
   199,  /* (40) cmd ::= DESC ftblname */
   199,  /* (41) cmd ::= ALTER USER ids PASS ids */
   199,  /* (42) cmd ::= ALTER USER ids PRIVILEGE ids */
   199,  /* (43) cmd ::= ALTER DNODE ids ids */
   199,  /* (44) cmd ::= ALTER DNODE ids ids ids */
   199,  /* (45) cmd ::= ALTER LOCAL ids */
   199,  /* (46) cmd ::= ALTER LOCAL ids ids */
   199,  /* (47) cmd ::= ALTER DATABASE ids alter_db_optr */
   199,  /* (48) cmd ::= ALTER TOPIC ids alter_topic_optr */
   199,  /* (49) cmd ::= ALTER ACCOUNT ids acct_optr */
   199,  /* (50) cmd ::= ALTER ACCOUNT ids PASS ids acct_optr */
   199,  /* (51) cmd ::= COMPACT VNODES IN LP exprlist RP */
   200,  /* (52) ids ::= ID */
   200,  /* (53) ids ::= STRING */
   205,  /* (54) ifexists ::= IF EXISTS */
   205,  /* (55) ifexists ::= */
   210,  /* (56) ifnotexists ::= IF NOT EXISTS */
   210,  /* (57) ifnotexists ::= */
   204,  /* (58) tblname ::= ids */
   204,  /* (59) tblname ::= ESCAPE */
   203,  /* (60) ftblname ::= tblname */
   203,  /* (61) ftblname ::= ids DOT tblname */
   199,  /* (62) cmd ::= CREATE DNODE ids */
   199,  /* (63) cmd ::= CREATE ACCOUNT ids PASS ids acct_optr */
   199,  /* (64) cmd ::= CREATE DATABASE ifnotexists ids db_optr */
   199,  /* (65) cmd ::= CREATE TOPIC ifnotexists ids topic_optr */
   199,  /* (66) cmd ::= CREATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize */
   199,  /* (67) cmd ::= CREATE AGGREGATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize */
   199,  /* (68) cmd ::= CREATE USER ids PASS ids */
   214,  /* (69) bufsize ::= */
   214,  /* (70) bufsize ::= BUFSIZE INTEGER */
   215,  /* (71) pps ::= */
   215,  /* (72) pps ::= PPS INTEGER */
   216,  /* (73) tseries ::= */
   216,  /* (74) tseries ::= TSERIES INTEGER */
   217,  /* (75) dbs ::= */
   217,  /* (76) dbs ::= DBS INTEGER */
   218,  /* (77) streams ::= */
   218,  /* (78) streams ::= STREAMS INTEGER */
   219,  /* (79) storage ::= */
   219,  /* (80) storage ::= STORAGE INTEGER */
   220,  /* (81) qtime ::= */
   220,  /* (82) qtime ::= QTIME INTEGER */
   221,  /* (83) users ::= */
   221,  /* (84) users ::= USERS INTEGER */
   222,  /* (85) conns ::= */
   222,  /* (86) conns ::= CONNS INTEGER */
   223,  /* (87) state ::= */
   223,  /* (88) state ::= STATE ids */
   208,  /* (89) acct_optr ::= pps tseries storage streams qtime dbs users conns state */
   224,  /* (90) intitemlist ::= intitemlist COMMA intitem */
   224,  /* (91) intitemlist ::= intitem */
   225,  /* (92) intitem ::= INTEGER */
   226,  /* (93) keep ::= KEEP intitemlist */
   227,  /* (94) cache ::= CACHE INTEGER */
   228,  /* (95) replica ::= REPLICA INTEGER */
   229,  /* (96) quorum ::= QUORUM INTEGER */
   230,  /* (97) days ::= DAYS INTEGER */
   231,  /* (98) minrows ::= MINROWS INTEGER */
   232,  /* (99) maxrows ::= MAXROWS INTEGER */
   233,  /* (100) blocks ::= BLOCKS INTEGER */
   234,  /* (101) ctime ::= CTIME INTEGER */
   235,  /* (102) wal ::= WAL INTEGER */
   236,  /* (103) fsync ::= FSYNC INTEGER */
   237,  /* (104) comp ::= COMP INTEGER */
   238,  /* (105) prec ::= PRECISION STRING */
   239,  /* (106) update ::= UPDATE INTEGER */
   240,  /* (107) cachelast ::= CACHELAST INTEGER */
   241,  /* (108) partitions ::= PARTITIONS INTEGER */
   211,  /* (109) db_optr ::= */
   211,  /* (110) db_optr ::= db_optr cache */
   211,  /* (111) db_optr ::= db_optr replica */
   211,  /* (112) db_optr ::= db_optr quorum */
   211,  /* (113) db_optr ::= db_optr days */
   211,  /* (114) db_optr ::= db_optr minrows */
   211,  /* (115) db_optr ::= db_optr maxrows */
   211,  /* (116) db_optr ::= db_optr blocks */
   211,  /* (117) db_optr ::= db_optr ctime */
   211,  /* (118) db_optr ::= db_optr wal */
   211,  /* (119) db_optr ::= db_optr fsync */
   211,  /* (120) db_optr ::= db_optr comp */
   211,  /* (121) db_optr ::= db_optr prec */
   211,  /* (122) db_optr ::= db_optr keep */
   211,  /* (123) db_optr ::= db_optr update */
   211,  /* (124) db_optr ::= db_optr cachelast */
   212,  /* (125) topic_optr ::= db_optr */
   212,  /* (126) topic_optr ::= topic_optr partitions */
   206,  /* (127) alter_db_optr ::= */
   206,  /* (128) alter_db_optr ::= alter_db_optr replica */
   206,  /* (129) alter_db_optr ::= alter_db_optr quorum */
   206,  /* (130) alter_db_optr ::= alter_db_optr keep */
   206,  /* (131) alter_db_optr ::= alter_db_optr blocks */
   206,  /* (132) alter_db_optr ::= alter_db_optr comp */
   206,  /* (133) alter_db_optr ::= alter_db_optr update */
   206,  /* (134) alter_db_optr ::= alter_db_optr cachelast */
   207,  /* (135) alter_topic_optr ::= alter_db_optr */
   207,  /* (136) alter_topic_optr ::= alter_topic_optr partitions */
   213,  /* (137) typename ::= ids */
   213,  /* (138) typename ::= ids LP signed RP */
   213,  /* (139) typename ::= ids UNSIGNED */
   242,  /* (140) signed ::= INTEGER */
   242,  /* (141) signed ::= PLUS INTEGER */
   242,  /* (142) signed ::= MINUS INTEGER */
   199,  /* (143) cmd ::= CREATE TABLE create_table_args */
   199,  /* (144) cmd ::= CREATE TABLE create_stable_args */
   199,  /* (145) cmd ::= CREATE STABLE create_stable_args */
   199,  /* (146) cmd ::= CREATE TABLE create_table_list */
   245,  /* (147) create_table_list ::= create_from_stable */
   245,  /* (148) create_table_list ::= create_table_list create_from_stable */
   243,  /* (149) create_table_args ::= ifnotexists ftblname LP columnlist RP */
   244,  /* (150) create_stable_args ::= ifnotexists ftblname LP columnlist RP TAGS LP columnlist RP */
   246,  /* (151) create_from_stable ::= ifnotexists ftblname USING ftblname TAGS LP tagitemlist RP */
   246,  /* (152) create_from_stable ::= ifnotexists ftblname USING ftblname LP tagNamelist RP TAGS LP tagitemlist RP */
   249,  /* (153) tagNamelist ::= tagNamelist COMMA ids */
   249,  /* (154) tagNamelist ::= ids */
   243,  /* (155) create_table_args ::= ifnotexists ftblname AS select */
   247,  /* (156) columnlist ::= columnlist COMMA column */
   247,  /* (157) columnlist ::= column */
   251,  /* (158) column ::= ids typename */
   248,  /* (159) tagitemlist ::= tagitemlist COMMA tagitem */
   248,  /* (160) tagitemlist ::= tagitem */
   252,  /* (161) tagitem ::= INTEGER */
   252,  /* (162) tagitem ::= FLOAT */
   252,  /* (163) tagitem ::= STRING */
   252,  /* (164) tagitem ::= BOOL */
   252,  /* (165) tagitem ::= NULL */
   252,  /* (166) tagitem ::= NOW */
   252,  /* (167) tagitem ::= MINUS INTEGER */
   252,  /* (168) tagitem ::= MINUS FLOAT */
   252,  /* (169) tagitem ::= PLUS INTEGER */
   252,  /* (170) tagitem ::= PLUS FLOAT */
   250,  /* (171) select ::= SELECT selcollist from where_opt interval_option sliding_opt session_option windowstate_option fill_opt groupby_opt having_opt orderby_opt slimit_opt limit_opt */
   250,  /* (172) select ::= LP select RP */
   266,  /* (173) union ::= select */
   266,  /* (174) union ::= union UNION ALL select */
   199,  /* (175) cmd ::= union */
   250,  /* (176) select ::= SELECT selcollist */
   267,  /* (177) sclp ::= selcollist COMMA */
   267,  /* (178) sclp ::= */
   253,  /* (179) selcollist ::= sclp distinct expr as */
   253,  /* (180) selcollist ::= sclp STAR */
   270,  /* (181) as ::= AS ids */
   270,  /* (182) as ::= ids */
   270,  /* (183) as ::= */
   268,  /* (184) distinct ::= DISTINCT */
   268,  /* (185) distinct ::= */
   254,  /* (186) from ::= FROM tablelist */
   254,  /* (187) from ::= FROM sub */
   272,  /* (188) sub ::= LP union RP */
   272,  /* (189) sub ::= LP union RP ids */
   272,  /* (190) sub ::= sub COMMA LP union RP ids */
   271,  /* (191) tablelist ::= ftblname */
   271,  /* (192) tablelist ::= ftblname ids */
   271,  /* (193) tablelist ::= tablelist COMMA ftblname */
   271,  /* (194) tablelist ::= tablelist COMMA ftblname ids */
   273,  /* (195) tmvar ::= VARIABLE */
   256,  /* (196) interval_option ::= intervalKey LP tmvar RP */
   256,  /* (197) interval_option ::= intervalKey LP tmvar COMMA tmvar RP */
   256,  /* (198) interval_option ::= */
   274,  /* (199) intervalKey ::= INTERVAL */
   274,  /* (200) intervalKey ::= EVERY */
   258,  /* (201) session_option ::= */
   258,  /* (202) session_option ::= SESSION LP ids cpxName COMMA tmvar RP */
   259,  /* (203) windowstate_option ::= */
   259,  /* (204) windowstate_option ::= STATE_WINDOW LP ids RP */
   260,  /* (205) fill_opt ::= */
   260,  /* (206) fill_opt ::= FILL LP ID COMMA tagitemlist RP */
   260,  /* (207) fill_opt ::= FILL LP ID RP */
   257,  /* (208) sliding_opt ::= SLIDING LP tmvar RP */
   257,  /* (209) sliding_opt ::= */
   263,  /* (210) orderby_opt ::= */
   263,  /* (211) orderby_opt ::= ORDER BY sortlist */
   275,  /* (212) sortlist ::= sortlist COMMA item sortorder */
   275,  /* (213) sortlist ::= item sortorder */
   277,  /* (214) item ::= ids cpxName */
   278,  /* (215) sortorder ::= ASC */
   278,  /* (216) sortorder ::= DESC */
   278,  /* (217) sortorder ::= */
   261,  /* (218) groupby_opt ::= */
   261,  /* (219) groupby_opt ::= GROUP BY grouplist */
   279,  /* (220) grouplist ::= grouplist COMMA item */
   279,  /* (221) grouplist ::= item */
   262,  /* (222) having_opt ::= */
   262,  /* (223) having_opt ::= HAVING expr */
   265,  /* (224) limit_opt ::= */
   265,  /* (225) limit_opt ::= LIMIT signed */
   265,  /* (226) limit_opt ::= LIMIT signed OFFSET signed */
   265,  /* (227) limit_opt ::= LIMIT signed COMMA signed */
   264,  /* (228) slimit_opt ::= */
   264,  /* (229) slimit_opt ::= SLIMIT signed */
   264,  /* (230) slimit_opt ::= SLIMIT signed SOFFSET signed */
   264,  /* (231) slimit_opt ::= SLIMIT signed COMMA signed */
   255,  /* (232) where_opt ::= */
   255,  /* (233) where_opt ::= WHERE expr */
   269,  /* (234) expr ::= LP expr RP */
   269,  /* (235) expr ::= ID */
   269,  /* (236) expr ::= ID DOT ID */
   269,  /* (237) expr ::= ID DOT STAR */
   269,  /* (238) expr ::= INTEGER */
   269,  /* (239) expr ::= MINUS INTEGER */
   269,  /* (240) expr ::= PLUS INTEGER */
   269,  /* (241) expr ::= FLOAT */
   269,  /* (242) expr ::= MINUS FLOAT */
   269,  /* (243) expr ::= PLUS FLOAT */
   269,  /* (244) expr ::= STRING */
   269,  /* (245) expr ::= NOW */
   269,  /* (246) expr ::= VARIABLE */
   269,  /* (247) expr ::= PLUS VARIABLE */
   269,  /* (248) expr ::= MINUS VARIABLE */
   269,  /* (249) expr ::= BOOL */
   269,  /* (250) expr ::= NULL */
   269,  /* (251) expr ::= ID LP exprlist RP */
   269,  /* (252) expr ::= ID LP STAR RP */
   269,  /* (253) expr ::= expr IS NULL */
   269,  /* (254) expr ::= expr IS NOT NULL */
   269,  /* (255) expr ::= expr LT expr */
   269,  /* (256) expr ::= expr GT expr */
   269,  /* (257) expr ::= expr LE expr */
   269,  /* (258) expr ::= expr GE expr */
   269,  /* (259) expr ::= expr NE expr */
   269,  /* (260) expr ::= expr EQ expr */
   269,  /* (261) expr ::= expr BETWEEN expr AND expr */
   269,  /* (262) expr ::= expr AND expr */
   269,  /* (263) expr ::= expr OR expr */
   269,  /* (264) expr ::= expr PLUS expr */
   269,  /* (265) expr ::= expr MINUS expr */
   269,  /* (266) expr ::= expr STAR expr */
   269,  /* (267) expr ::= expr SLASH expr */
   269,  /* (268) expr ::= expr REM expr */
   269,  /* (269) expr ::= expr LIKE expr */
   269,  /* (270) expr ::= expr MATCH expr */
   269,  /* (271) expr ::= expr NMATCH expr */
   269,  /* (272) expr ::= expr IN LP exprlist RP */
   209,  /* (273) exprlist ::= exprlist COMMA expritem */
   209,  /* (274) exprlist ::= expritem */
   280,  /* (275) expritem ::= expr */
   280,  /* (276) expritem ::= */
   199,  /* (277) cmd ::= RESET QUERY CACHE */
   199,  /* (278) cmd ::= SYNCDB ids REPLICA */
   199,  /* (279) cmd ::= ALTER TABLE ftblname ADD COLUMN columnlist */
   199,  /* (280) cmd ::= ALTER TABLE ftblname DROP COLUMN ids */
   199,  /* (281) cmd ::= ALTER TABLE ftblname MODIFY COLUMN columnlist */
   199,  /* (282) cmd ::= ALTER TABLE ftblname ADD TAG columnlist */
   199,  /* (283) cmd ::= ALTER TABLE ftblname DROP TAG ids */
   199,  /* (284) cmd ::= ALTER TABLE ftblname CHANGE TAG ids ids */
   199,  /* (285) cmd ::= ALTER TABLE ftblname SET TAG ids EQ tagitem */
   199,  /* (286) cmd ::= ALTER TABLE ftblname MODIFY TAG columnlist */
   199,  /* (287) cmd ::= ALTER STABLE ftblname ADD COLUMN columnlist */
   199,  /* (288) cmd ::= ALTER STABLE ftblname DROP COLUMN ids */
   199,  /* (289) cmd ::= ALTER STABLE ftblname MODIFY COLUMN columnlist */
   199,  /* (290) cmd ::= ALTER STABLE ftblname ADD TAG columnlist */
   199,  /* (291) cmd ::= ALTER STABLE ftblname DROP TAG ids */
   199,  /* (292) cmd ::= ALTER STABLE ftblname CHANGE TAG ids ids */
   199,  /* (293) cmd ::= ALTER STABLE ftblname SET TAG ids EQ tagitem */
   199,  /* (294) cmd ::= ALTER STABLE ftblname MODIFY TAG columnlist */
   199,  /* (295) cmd ::= KILL CONNECTION INTEGER */
   199,  /* (296) cmd ::= KILL STREAM INTEGER COLON INTEGER */
   199,  /* (297) cmd ::= KILL QUERY INTEGER COLON INTEGER */
};

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
static const signed char yyRuleInfoNRhs[] = {
   -1,  /* (0) program ::= cmd */
   -2,  /* (1) cmd ::= SHOW DATABASES */
   -2,  /* (2) cmd ::= SHOW TOPICS */
   -2,  /* (3) cmd ::= SHOW FUNCTIONS */
   -2,  /* (4) cmd ::= SHOW MNODES */
   -2,  /* (5) cmd ::= SHOW DNODES */
   -2,  /* (6) cmd ::= SHOW ACCOUNTS */
   -2,  /* (7) cmd ::= SHOW USERS */
   -2,  /* (8) cmd ::= SHOW MODULES */
   -2,  /* (9) cmd ::= SHOW QUERIES */
   -2,  /* (10) cmd ::= SHOW CONNECTIONS */
   -2,  /* (11) cmd ::= SHOW STREAMS */
   -2,  /* (12) cmd ::= SHOW VARIABLES */
   -2,  /* (13) cmd ::= SHOW SCORES */
   -2,  /* (14) cmd ::= SHOW GRANTS */
   -2,  /* (15) cmd ::= SHOW VNODES */
   -3,  /* (16) cmd ::= SHOW VNODES ids */
    0,  /* (17) dbPrefix ::= */
   -2,  /* (18) dbPrefix ::= ids DOT */
    0,  /* (19) cpxName ::= */
   -2,  /* (20) cpxName ::= DOT ids */
   -4,  /* (21) cmd ::= SHOW CREATE TABLE ftblname */
   -4,  /* (22) cmd ::= SHOW CREATE STABLE ftblname */
   -4,  /* (23) cmd ::= SHOW CREATE DATABASE ids */
   -3,  /* (24) cmd ::= SHOW dbPrefix TABLES */
   -5,  /* (25) cmd ::= SHOW dbPrefix TABLES LIKE tblname */
   -3,  /* (26) cmd ::= SHOW dbPrefix STABLES */
   -5,  /* (27) cmd ::= SHOW dbPrefix STABLES LIKE tblname */
   -3,  /* (28) cmd ::= SHOW dbPrefix VGROUPS */
   -4,  /* (29) cmd ::= SHOW dbPrefix VGROUPS ids */
   -4,  /* (30) cmd ::= DROP TABLE ifexists ftblname */
   -4,  /* (31) cmd ::= DROP STABLE ifexists ftblname */
   -4,  /* (32) cmd ::= DROP DATABASE ifexists ids */
   -4,  /* (33) cmd ::= DROP TOPIC ifexists ids */
   -3,  /* (34) cmd ::= DROP FUNCTION ids */
   -3,  /* (35) cmd ::= DROP DNODE ids */
   -3,  /* (36) cmd ::= DROP USER ids */
   -3,  /* (37) cmd ::= DROP ACCOUNT ids */
   -2,  /* (38) cmd ::= USE ids */
   -2,  /* (39) cmd ::= DESCRIBE ftblname */
   -2,  /* (40) cmd ::= DESC ftblname */
   -5,  /* (41) cmd ::= ALTER USER ids PASS ids */
   -5,  /* (42) cmd ::= ALTER USER ids PRIVILEGE ids */
   -4,  /* (43) cmd ::= ALTER DNODE ids ids */
   -5,  /* (44) cmd ::= ALTER DNODE ids ids ids */
   -3,  /* (45) cmd ::= ALTER LOCAL ids */
   -4,  /* (46) cmd ::= ALTER LOCAL ids ids */
   -4,  /* (47) cmd ::= ALTER DATABASE ids alter_db_optr */
   -4,  /* (48) cmd ::= ALTER TOPIC ids alter_topic_optr */
   -4,  /* (49) cmd ::= ALTER ACCOUNT ids acct_optr */
   -6,  /* (50) cmd ::= ALTER ACCOUNT ids PASS ids acct_optr */
   -6,  /* (51) cmd ::= COMPACT VNODES IN LP exprlist RP */
   -1,  /* (52) ids ::= ID */
   -1,  /* (53) ids ::= STRING */
   -2,  /* (54) ifexists ::= IF EXISTS */
    0,  /* (55) ifexists ::= */
   -3,  /* (56) ifnotexists ::= IF NOT EXISTS */
    0,  /* (57) ifnotexists ::= */
   -1,  /* (58) tblname ::= ids */
   -1,  /* (59) tblname ::= ESCAPE */
   -1,  /* (60) ftblname ::= tblname */
   -3,  /* (61) ftblname ::= ids DOT tblname */
   -3,  /* (62) cmd ::= CREATE DNODE ids */
   -6,  /* (63) cmd ::= CREATE ACCOUNT ids PASS ids acct_optr */
   -5,  /* (64) cmd ::= CREATE DATABASE ifnotexists ids db_optr */
   -5,  /* (65) cmd ::= CREATE TOPIC ifnotexists ids topic_optr */
   -8,  /* (66) cmd ::= CREATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize */
   -9,  /* (67) cmd ::= CREATE AGGREGATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize */
   -5,  /* (68) cmd ::= CREATE USER ids PASS ids */
    0,  /* (69) bufsize ::= */
   -2,  /* (70) bufsize ::= BUFSIZE INTEGER */
    0,  /* (71) pps ::= */
   -2,  /* (72) pps ::= PPS INTEGER */
    0,  /* (73) tseries ::= */
   -2,  /* (74) tseries ::= TSERIES INTEGER */
    0,  /* (75) dbs ::= */
   -2,  /* (76) dbs ::= DBS INTEGER */
    0,  /* (77) streams ::= */
   -2,  /* (78) streams ::= STREAMS INTEGER */
    0,  /* (79) storage ::= */
   -2,  /* (80) storage ::= STORAGE INTEGER */
    0,  /* (81) qtime ::= */
   -2,  /* (82) qtime ::= QTIME INTEGER */
    0,  /* (83) users ::= */
   -2,  /* (84) users ::= USERS INTEGER */
    0,  /* (85) conns ::= */
   -2,  /* (86) conns ::= CONNS INTEGER */
    0,  /* (87) state ::= */
   -2,  /* (88) state ::= STATE ids */
   -9,  /* (89) acct_optr ::= pps tseries storage streams qtime dbs users conns state */
   -3,  /* (90) intitemlist ::= intitemlist COMMA intitem */
   -1,  /* (91) intitemlist ::= intitem */
   -1,  /* (92) intitem ::= INTEGER */
   -2,  /* (93) keep ::= KEEP intitemlist */
   -2,  /* (94) cache ::= CACHE INTEGER */
   -2,  /* (95) replica ::= REPLICA INTEGER */
   -2,  /* (96) quorum ::= QUORUM INTEGER */
   -2,  /* (97) days ::= DAYS INTEGER */
   -2,  /* (98) minrows ::= MINROWS INTEGER */
   -2,  /* (99) maxrows ::= MAXROWS INTEGER */
   -2,  /* (100) blocks ::= BLOCKS INTEGER */
   -2,  /* (101) ctime ::= CTIME INTEGER */
   -2,  /* (102) wal ::= WAL INTEGER */
   -2,  /* (103) fsync ::= FSYNC INTEGER */
   -2,  /* (104) comp ::= COMP INTEGER */
   -2,  /* (105) prec ::= PRECISION STRING */
   -2,  /* (106) update ::= UPDATE INTEGER */
   -2,  /* (107) cachelast ::= CACHELAST INTEGER */
   -2,  /* (108) partitions ::= PARTITIONS INTEGER */
    0,  /* (109) db_optr ::= */
   -2,  /* (110) db_optr ::= db_optr cache */
   -2,  /* (111) db_optr ::= db_optr replica */
   -2,  /* (112) db_optr ::= db_optr quorum */
   -2,  /* (113) db_optr ::= db_optr days */
   -2,  /* (114) db_optr ::= db_optr minrows */
   -2,  /* (115) db_optr ::= db_optr maxrows */
   -2,  /* (116) db_optr ::= db_optr blocks */
   -2,  /* (117) db_optr ::= db_optr ctime */
   -2,  /* (118) db_optr ::= db_optr wal */
   -2,  /* (119) db_optr ::= db_optr fsync */
   -2,  /* (120) db_optr ::= db_optr comp */
   -2,  /* (121) db_optr ::= db_optr prec */
   -2,  /* (122) db_optr ::= db_optr keep */
   -2,  /* (123) db_optr ::= db_optr update */
   -2,  /* (124) db_optr ::= db_optr cachelast */
   -1,  /* (125) topic_optr ::= db_optr */
   -2,  /* (126) topic_optr ::= topic_optr partitions */
    0,  /* (127) alter_db_optr ::= */
   -2,  /* (128) alter_db_optr ::= alter_db_optr replica */
   -2,  /* (129) alter_db_optr ::= alter_db_optr quorum */
   -2,  /* (130) alter_db_optr ::= alter_db_optr keep */
   -2,  /* (131) alter_db_optr ::= alter_db_optr blocks */
   -2,  /* (132) alter_db_optr ::= alter_db_optr comp */
   -2,  /* (133) alter_db_optr ::= alter_db_optr update */
   -2,  /* (134) alter_db_optr ::= alter_db_optr cachelast */
   -1,  /* (135) alter_topic_optr ::= alter_db_optr */
   -2,  /* (136) alter_topic_optr ::= alter_topic_optr partitions */
   -1,  /* (137) typename ::= ids */
   -4,  /* (138) typename ::= ids LP signed RP */
   -2,  /* (139) typename ::= ids UNSIGNED */
   -1,  /* (140) signed ::= INTEGER */
   -2,  /* (141) signed ::= PLUS INTEGER */
   -2,  /* (142) signed ::= MINUS INTEGER */
   -3,  /* (143) cmd ::= CREATE TABLE create_table_args */
   -3,  /* (144) cmd ::= CREATE TABLE create_stable_args */
   -3,  /* (145) cmd ::= CREATE STABLE create_stable_args */
   -3,  /* (146) cmd ::= CREATE TABLE create_table_list */
   -1,  /* (147) create_table_list ::= create_from_stable */
   -2,  /* (148) create_table_list ::= create_table_list create_from_stable */
   -5,  /* (149) create_table_args ::= ifnotexists ftblname LP columnlist RP */
   -9,  /* (150) create_stable_args ::= ifnotexists ftblname LP columnlist RP TAGS LP columnlist RP */
   -8,  /* (151) create_from_stable ::= ifnotexists ftblname USING ftblname TAGS LP tagitemlist RP */
  -11,  /* (152) create_from_stable ::= ifnotexists ftblname USING ftblname LP tagNamelist RP TAGS LP tagitemlist RP */
   -3,  /* (153) tagNamelist ::= tagNamelist COMMA ids */
   -1,  /* (154) tagNamelist ::= ids */
   -4,  /* (155) create_table_args ::= ifnotexists ftblname AS select */
   -3,  /* (156) columnlist ::= columnlist COMMA column */
   -1,  /* (157) columnlist ::= column */
   -2,  /* (158) column ::= ids typename */
   -3,  /* (159) tagitemlist ::= tagitemlist COMMA tagitem */
   -1,  /* (160) tagitemlist ::= tagitem */
   -1,  /* (161) tagitem ::= INTEGER */
   -1,  /* (162) tagitem ::= FLOAT */
   -1,  /* (163) tagitem ::= STRING */
   -1,  /* (164) tagitem ::= BOOL */
   -1,  /* (165) tagitem ::= NULL */
   -1,  /* (166) tagitem ::= NOW */
   -2,  /* (167) tagitem ::= MINUS INTEGER */
   -2,  /* (168) tagitem ::= MINUS FLOAT */
   -2,  /* (169) tagitem ::= PLUS INTEGER */
   -2,  /* (170) tagitem ::= PLUS FLOAT */
  -14,  /* (171) select ::= SELECT selcollist from where_opt interval_option sliding_opt session_option windowstate_option fill_opt groupby_opt having_opt orderby_opt slimit_opt limit_opt */
   -3,  /* (172) select ::= LP select RP */
   -1,  /* (173) union ::= select */
   -4,  /* (174) union ::= union UNION ALL select */
   -1,  /* (175) cmd ::= union */
   -2,  /* (176) select ::= SELECT selcollist */
   -2,  /* (177) sclp ::= selcollist COMMA */
    0,  /* (178) sclp ::= */
   -4,  /* (179) selcollist ::= sclp distinct expr as */
   -2,  /* (180) selcollist ::= sclp STAR */
   -2,  /* (181) as ::= AS ids */
   -1,  /* (182) as ::= ids */
    0,  /* (183) as ::= */
   -1,  /* (184) distinct ::= DISTINCT */
    0,  /* (185) distinct ::= */
   -2,  /* (186) from ::= FROM tablelist */
   -2,  /* (187) from ::= FROM sub */
   -3,  /* (188) sub ::= LP union RP */
   -4,  /* (189) sub ::= LP union RP ids */
   -6,  /* (190) sub ::= sub COMMA LP union RP ids */
   -1,  /* (191) tablelist ::= ftblname */
   -2,  /* (192) tablelist ::= ftblname ids */
   -3,  /* (193) tablelist ::= tablelist COMMA ftblname */
   -4,  /* (194) tablelist ::= tablelist COMMA ftblname ids */
   -1,  /* (195) tmvar ::= VARIABLE */
   -4,  /* (196) interval_option ::= intervalKey LP tmvar RP */
   -6,  /* (197) interval_option ::= intervalKey LP tmvar COMMA tmvar RP */
    0,  /* (198) interval_option ::= */
   -1,  /* (199) intervalKey ::= INTERVAL */
   -1,  /* (200) intervalKey ::= EVERY */
    0,  /* (201) session_option ::= */
   -7,  /* (202) session_option ::= SESSION LP ids cpxName COMMA tmvar RP */
    0,  /* (203) windowstate_option ::= */
   -4,  /* (204) windowstate_option ::= STATE_WINDOW LP ids RP */
    0,  /* (205) fill_opt ::= */
   -6,  /* (206) fill_opt ::= FILL LP ID COMMA tagitemlist RP */
   -4,  /* (207) fill_opt ::= FILL LP ID RP */
   -4,  /* (208) sliding_opt ::= SLIDING LP tmvar RP */
    0,  /* (209) sliding_opt ::= */
    0,  /* (210) orderby_opt ::= */
   -3,  /* (211) orderby_opt ::= ORDER BY sortlist */
   -4,  /* (212) sortlist ::= sortlist COMMA item sortorder */
   -2,  /* (213) sortlist ::= item sortorder */
   -2,  /* (214) item ::= ids cpxName */
   -1,  /* (215) sortorder ::= ASC */
   -1,  /* (216) sortorder ::= DESC */
    0,  /* (217) sortorder ::= */
    0,  /* (218) groupby_opt ::= */
   -3,  /* (219) groupby_opt ::= GROUP BY grouplist */
   -3,  /* (220) grouplist ::= grouplist COMMA item */
   -1,  /* (221) grouplist ::= item */
    0,  /* (222) having_opt ::= */
   -2,  /* (223) having_opt ::= HAVING expr */
    0,  /* (224) limit_opt ::= */
   -2,  /* (225) limit_opt ::= LIMIT signed */
   -4,  /* (226) limit_opt ::= LIMIT signed OFFSET signed */
   -4,  /* (227) limit_opt ::= LIMIT signed COMMA signed */
    0,  /* (228) slimit_opt ::= */
   -2,  /* (229) slimit_opt ::= SLIMIT signed */
   -4,  /* (230) slimit_opt ::= SLIMIT signed SOFFSET signed */
   -4,  /* (231) slimit_opt ::= SLIMIT signed COMMA signed */
    0,  /* (232) where_opt ::= */
   -2,  /* (233) where_opt ::= WHERE expr */
   -3,  /* (234) expr ::= LP expr RP */
   -1,  /* (235) expr ::= ID */
   -3,  /* (236) expr ::= ID DOT ID */
   -3,  /* (237) expr ::= ID DOT STAR */
   -1,  /* (238) expr ::= INTEGER */
   -2,  /* (239) expr ::= MINUS INTEGER */
   -2,  /* (240) expr ::= PLUS INTEGER */
   -1,  /* (241) expr ::= FLOAT */
   -2,  /* (242) expr ::= MINUS FLOAT */
   -2,  /* (243) expr ::= PLUS FLOAT */
   -1,  /* (244) expr ::= STRING */
   -1,  /* (245) expr ::= NOW */
   -1,  /* (246) expr ::= VARIABLE */
   -2,  /* (247) expr ::= PLUS VARIABLE */
   -2,  /* (248) expr ::= MINUS VARIABLE */
   -1,  /* (249) expr ::= BOOL */
   -1,  /* (250) expr ::= NULL */
   -4,  /* (251) expr ::= ID LP exprlist RP */
   -4,  /* (252) expr ::= ID LP STAR RP */
   -3,  /* (253) expr ::= expr IS NULL */
   -4,  /* (254) expr ::= expr IS NOT NULL */
   -3,  /* (255) expr ::= expr LT expr */
   -3,  /* (256) expr ::= expr GT expr */
   -3,  /* (257) expr ::= expr LE expr */
   -3,  /* (258) expr ::= expr GE expr */
   -3,  /* (259) expr ::= expr NE expr */
   -3,  /* (260) expr ::= expr EQ expr */
   -5,  /* (261) expr ::= expr BETWEEN expr AND expr */
   -3,  /* (262) expr ::= expr AND expr */
   -3,  /* (263) expr ::= expr OR expr */
   -3,  /* (264) expr ::= expr PLUS expr */
   -3,  /* (265) expr ::= expr MINUS expr */
   -3,  /* (266) expr ::= expr STAR expr */
   -3,  /* (267) expr ::= expr SLASH expr */
   -3,  /* (268) expr ::= expr REM expr */
   -3,  /* (269) expr ::= expr LIKE expr */
   -3,  /* (270) expr ::= expr MATCH expr */
   -3,  /* (271) expr ::= expr NMATCH expr */
   -5,  /* (272) expr ::= expr IN LP exprlist RP */
   -3,  /* (273) exprlist ::= exprlist COMMA expritem */
   -1,  /* (274) exprlist ::= expritem */
   -1,  /* (275) expritem ::= expr */
    0,  /* (276) expritem ::= */
   -3,  /* (277) cmd ::= RESET QUERY CACHE */
   -3,  /* (278) cmd ::= SYNCDB ids REPLICA */
   -6,  /* (279) cmd ::= ALTER TABLE ftblname ADD COLUMN columnlist */
   -6,  /* (280) cmd ::= ALTER TABLE ftblname DROP COLUMN ids */
   -6,  /* (281) cmd ::= ALTER TABLE ftblname MODIFY COLUMN columnlist */
   -6,  /* (282) cmd ::= ALTER TABLE ftblname ADD TAG columnlist */
   -6,  /* (283) cmd ::= ALTER TABLE ftblname DROP TAG ids */
   -7,  /* (284) cmd ::= ALTER TABLE ftblname CHANGE TAG ids ids */
   -8,  /* (285) cmd ::= ALTER TABLE ftblname SET TAG ids EQ tagitem */
   -6,  /* (286) cmd ::= ALTER TABLE ftblname MODIFY TAG columnlist */
   -6,  /* (287) cmd ::= ALTER STABLE ftblname ADD COLUMN columnlist */
   -6,  /* (288) cmd ::= ALTER STABLE ftblname DROP COLUMN ids */
   -6,  /* (289) cmd ::= ALTER STABLE ftblname MODIFY COLUMN columnlist */
   -6,  /* (290) cmd ::= ALTER STABLE ftblname ADD TAG columnlist */
   -6,  /* (291) cmd ::= ALTER STABLE ftblname DROP TAG ids */
   -7,  /* (292) cmd ::= ALTER STABLE ftblname CHANGE TAG ids ids */
   -8,  /* (293) cmd ::= ALTER STABLE ftblname SET TAG ids EQ tagitem */
   -6,  /* (294) cmd ::= ALTER STABLE ftblname MODIFY TAG columnlist */
   -3,  /* (295) cmd ::= KILL CONNECTION INTEGER */
   -5,  /* (296) cmd ::= KILL STREAM INTEGER COLON INTEGER */
   -5,  /* (297) cmd ::= KILL QUERY INTEGER COLON INTEGER */
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
**
** The yyLookahead and yyLookaheadToken parameters provide reduce actions
** access to the lookahead token (if any).  The yyLookahead will be YYNOCODE
** if the lookahead token has already been consumed.  As this procedure is
** only called from one place, optimizing compilers will in-line it, which
** means that the extra parameters have no performance impact.
*/
static YYACTIONTYPE yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno,       /* Number of the rule by which to reduce */
  int yyLookahead,             /* Lookahead token, or YYNOCODE if none */
  ParseTOKENTYPE yyLookaheadToken  /* Value of the lookahead token */
  ParseCTX_PDECL                   /* %extra_context */
){
  int yygoto;                     /* The next state */
  YYACTIONTYPE yyact;             /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH
  (void)yyLookahead;
  (void)yyLookaheadToken;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfoNRhs[yyruleno];
    if( yysize ){
      fprintf(yyTraceFILE, "%sReduce %d [%s]%s, pop back to state %d.\n",
        yyTracePrompt,
        yyruleno, yyRuleName[yyruleno],
        yyruleno<YYNRULE_WITH_ACTION ? "" : " without external action",
        yymsp[yysize].stateno);
    }else{
      fprintf(yyTraceFILE, "%sReduce %d [%s]%s.\n",
        yyTracePrompt, yyruleno, yyRuleName[yyruleno],
        yyruleno<YYNRULE_WITH_ACTION ? "" : " without external action");
    }
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfoNRhs[yyruleno]==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=yypParser->yystackEnd ){
      yyStackOverflow(yypParser);
      /* The call to yyStackOverflow() above pops the stack until it is
      ** empty, causing the main parser loop to exit.  So the return value
      ** is never used and does not matter. */
      return 0;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        /* The call to yyStackOverflow() above pops the stack until it is
        ** empty, causing the main parser loop to exit.  So the return value
        ** is never used and does not matter. */
        return 0;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* program ::= cmd */
      case 143: /* cmd ::= CREATE TABLE create_table_args */ yytestcase(yyruleno==143);
      case 144: /* cmd ::= CREATE TABLE create_stable_args */ yytestcase(yyruleno==144);
      case 145: /* cmd ::= CREATE STABLE create_stable_args */ yytestcase(yyruleno==145);
{}
        break;
      case 1: /* cmd ::= SHOW DATABASES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_DB, 0, 0);}
        break;
      case 2: /* cmd ::= SHOW TOPICS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_TP, 0, 0);}
        break;
      case 3: /* cmd ::= SHOW FUNCTIONS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_FUNCTION, 0, 0);}
        break;
      case 4: /* cmd ::= SHOW MNODES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_MNODE, 0, 0);}
        break;
      case 5: /* cmd ::= SHOW DNODES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_DNODE, 0, 0);}
        break;
      case 6: /* cmd ::= SHOW ACCOUNTS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_ACCT, 0, 0);}
        break;
      case 7: /* cmd ::= SHOW USERS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_USER, 0, 0);}
        break;
      case 8: /* cmd ::= SHOW MODULES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_MODULE, 0, 0);  }
        break;
      case 9: /* cmd ::= SHOW QUERIES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_QUERIES, 0, 0);  }
        break;
      case 10: /* cmd ::= SHOW CONNECTIONS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_CONNS, 0, 0);}
        break;
      case 11: /* cmd ::= SHOW STREAMS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_STREAMS, 0, 0);  }
        break;
      case 12: /* cmd ::= SHOW VARIABLES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_VARIABLES, 0, 0);  }
        break;
      case 13: /* cmd ::= SHOW SCORES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_SCORES, 0, 0);   }
        break;
      case 14: /* cmd ::= SHOW GRANTS */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_GRANTS, 0, 0);   }
        break;
      case 15: /* cmd ::= SHOW VNODES */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_VNODES, 0, 0); }
        break;
      case 16: /* cmd ::= SHOW VNODES ids */
{ setShowOptions(pInfo, TSDB_MGMT_TABLE_VNODES, &yymsp[0].minor.yy0, 0); }
        break;
      case 17: /* dbPrefix ::= */
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.type = 0;}
        break;
      case 18: /* dbPrefix ::= ids DOT */
{yylhsminor.yy0 = yymsp[-1].minor.yy0;  }
  yymsp[-1].minor.yy0 = yylhsminor.yy0;
        break;
      case 19: /* cpxName ::= */
{yymsp[1].minor.yy0.n = 0;  }
        break;
      case 20: /* cpxName ::= DOT ids */
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; yymsp[-1].minor.yy0.n += 1;    }
        break;
      case 21: /* cmd ::= SHOW CREATE TABLE ftblname */
{
   setDCLSqlElems(pInfo, TSDB_SQL_SHOW_CREATE_TABLE, 1, &yymsp[0].minor.yy0);
}
        break;
      case 22: /* cmd ::= SHOW CREATE STABLE ftblname */
{
   setDCLSqlElems(pInfo, TSDB_SQL_SHOW_CREATE_STABLE, 1, &yymsp[0].minor.yy0);
}
        break;
      case 23: /* cmd ::= SHOW CREATE DATABASE ids */
{
  setDCLSqlElems(pInfo, TSDB_SQL_SHOW_CREATE_DATABASE, 1, &yymsp[0].minor.yy0);
}
        break;
      case 24: /* cmd ::= SHOW dbPrefix TABLES */
{
    setShowOptions(pInfo, TSDB_MGMT_TABLE_TABLE, &yymsp[-1].minor.yy0, 0);
}
        break;
      case 25: /* cmd ::= SHOW dbPrefix TABLES LIKE tblname */
{
    setShowOptions(pInfo, TSDB_MGMT_TABLE_TABLE, &yymsp[-3].minor.yy0, &yymsp[0].minor.yy0);
}
        break;
      case 26: /* cmd ::= SHOW dbPrefix STABLES */
{
    setShowOptions(pInfo, TSDB_MGMT_TABLE_METRIC, &yymsp[-1].minor.yy0, 0);
}
        break;
      case 27: /* cmd ::= SHOW dbPrefix STABLES LIKE tblname */
{
    SStrToken token;
    tSetDbName(&token, &yymsp[-3].minor.yy0);
    setShowOptions(pInfo, TSDB_MGMT_TABLE_METRIC, &token, &yymsp[0].minor.yy0);
}
        break;
      case 28: /* cmd ::= SHOW dbPrefix VGROUPS */
{
    SStrToken token;
    tSetDbName(&token, &yymsp[-1].minor.yy0);
    setShowOptions(pInfo, TSDB_MGMT_TABLE_VGROUP, &token, 0);
}
        break;
      case 29: /* cmd ::= SHOW dbPrefix VGROUPS ids */
{
    SStrToken token;
    tSetDbName(&token, &yymsp[-2].minor.yy0);
    setShowOptions(pInfo, TSDB_MGMT_TABLE_VGROUP, &token, &yymsp[0].minor.yy0);
}
        break;
      case 30: /* cmd ::= DROP TABLE ifexists ftblname */
{
    setDropDbTableInfo(pInfo, TSDB_SQL_DROP_TABLE, &yymsp[0].minor.yy0, &yymsp[-1].minor.yy0, -1, -1);
}
        break;
      case 31: /* cmd ::= DROP STABLE ifexists ftblname */
{
    setDropDbTableInfo(pInfo, TSDB_SQL_DROP_TABLE, &yymsp[0].minor.yy0, &yymsp[-1].minor.yy0, -1, TSDB_SUPER_TABLE);
}
        break;
      case 32: /* cmd ::= DROP DATABASE ifexists ids */
{ setDropDbTableInfo(pInfo, TSDB_SQL_DROP_DB, &yymsp[0].minor.yy0, &yymsp[-1].minor.yy0, TSDB_DB_TYPE_DEFAULT, -1); }
        break;
      case 33: /* cmd ::= DROP TOPIC ifexists ids */
{ setDropDbTableInfo(pInfo, TSDB_SQL_DROP_DB, &yymsp[0].minor.yy0, &yymsp[-1].minor.yy0, TSDB_DB_TYPE_TOPIC, -1); }
        break;
      case 34: /* cmd ::= DROP FUNCTION ids */
{ setDropFuncInfo(pInfo, TSDB_SQL_DROP_FUNCTION, &yymsp[0].minor.yy0); }
        break;
      case 35: /* cmd ::= DROP DNODE ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_DROP_DNODE, 1, &yymsp[0].minor.yy0);    }
        break;
      case 36: /* cmd ::= DROP USER ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_DROP_USER, 1, &yymsp[0].minor.yy0);     }
        break;
      case 37: /* cmd ::= DROP ACCOUNT ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_DROP_ACCT, 1, &yymsp[0].minor.yy0);  }
        break;
      case 38: /* cmd ::= USE ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_USE_DB, 1, &yymsp[0].minor.yy0);}
        break;
      case 39: /* cmd ::= DESCRIBE ftblname */
      case 40: /* cmd ::= DESC ftblname */ yytestcase(yyruleno==40);
{
    setDCLSqlElems(pInfo, TSDB_SQL_DESCRIBE_TABLE, 1, &yymsp[0].minor.yy0);
}
        break;
      case 41: /* cmd ::= ALTER USER ids PASS ids */
{ setAlterUserSql(pInfo, TSDB_ALTER_USER_PASSWD, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0, NULL);    }
        break;
      case 42: /* cmd ::= ALTER USER ids PRIVILEGE ids */
{ setAlterUserSql(pInfo, TSDB_ALTER_USER_PRIVILEGES, &yymsp[-2].minor.yy0, NULL, &yymsp[0].minor.yy0);}
        break;
      case 43: /* cmd ::= ALTER DNODE ids ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_CFG_DNODE, 2, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);          }
        break;
      case 44: /* cmd ::= ALTER DNODE ids ids ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_CFG_DNODE, 3, &yymsp[-2].minor.yy0, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);      }
        break;
      case 45: /* cmd ::= ALTER LOCAL ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_CFG_LOCAL, 1, &yymsp[0].minor.yy0);              }
        break;
      case 46: /* cmd ::= ALTER LOCAL ids ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_CFG_LOCAL, 2, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);          }
        break;
      case 47: /* cmd ::= ALTER DATABASE ids alter_db_optr */
      case 48: /* cmd ::= ALTER TOPIC ids alter_topic_optr */ yytestcase(yyruleno==48);
{ SStrToken t = {0};  setCreateDbInfo(pInfo, TSDB_SQL_ALTER_DB, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy262, &t);}
        break;
      case 49: /* cmd ::= ALTER ACCOUNT ids acct_optr */
{ setCreateAcctSql(pInfo, TSDB_SQL_ALTER_ACCT, &yymsp[-1].minor.yy0, NULL, &yymsp[0].minor.yy47);}
        break;
      case 50: /* cmd ::= ALTER ACCOUNT ids PASS ids acct_optr */
{ setCreateAcctSql(pInfo, TSDB_SQL_ALTER_ACCT, &yymsp[-3].minor.yy0, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy47);}
        break;
      case 51: /* cmd ::= COMPACT VNODES IN LP exprlist RP */
{ setCompactVnodeSql(pInfo, TSDB_SQL_COMPACT_VNODE, yymsp[-1].minor.yy247);}
        break;
      case 52: /* ids ::= ID */
      case 53: /* ids ::= STRING */ yytestcase(yyruleno==53);
      case 58: /* tblname ::= ids */ yytestcase(yyruleno==58);
      case 59: /* tblname ::= ESCAPE */ yytestcase(yyruleno==59);
      case 60: /* ftblname ::= tblname */ yytestcase(yyruleno==60);
{yylhsminor.yy0 = yymsp[0].minor.yy0; }
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 54: /* ifexists ::= IF EXISTS */
{ yymsp[-1].minor.yy0.n = 1;}
        break;
      case 55: /* ifexists ::= */
      case 57: /* ifnotexists ::= */ yytestcase(yyruleno==57);
      case 185: /* distinct ::= */ yytestcase(yyruleno==185);
{ yymsp[1].minor.yy0.n = 0;}
        break;
      case 56: /* ifnotexists ::= IF NOT EXISTS */
{ yymsp[-2].minor.yy0.n = 1;}
        break;
      case 61: /* ftblname ::= ids DOT tblname */
{yylhsminor.yy0 = yymsp[-2].minor.yy0; yylhsminor.yy0.n += 1 + yymsp[0].minor.yy0.n;    }
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 62: /* cmd ::= CREATE DNODE ids */
{ setDCLSqlElems(pInfo, TSDB_SQL_CREATE_DNODE, 1, &yymsp[0].minor.yy0);}
        break;
      case 63: /* cmd ::= CREATE ACCOUNT ids PASS ids acct_optr */
{ setCreateAcctSql(pInfo, TSDB_SQL_CREATE_ACCT, &yymsp[-3].minor.yy0, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy47);}
        break;
      case 64: /* cmd ::= CREATE DATABASE ifnotexists ids db_optr */
      case 65: /* cmd ::= CREATE TOPIC ifnotexists ids topic_optr */ yytestcase(yyruleno==65);
{ setCreateDbInfo(pInfo, TSDB_SQL_CREATE_DB, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy262, &yymsp[-2].minor.yy0);}
        break;
      case 66: /* cmd ::= CREATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize */
{ setCreateFuncInfo(pInfo, TSDB_SQL_CREATE_FUNCTION, &yymsp[-5].minor.yy0, &yymsp[-3].minor.yy0, &yymsp[-1].minor.yy179, &yymsp[0].minor.yy0, 1);}
        break;
      case 67: /* cmd ::= CREATE AGGREGATE FUNCTION ids AS ids OUTPUTTYPE typename bufsize */
{ setCreateFuncInfo(pInfo, TSDB_SQL_CREATE_FUNCTION, &yymsp[-5].minor.yy0, &yymsp[-3].minor.yy0, &yymsp[-1].minor.yy179, &yymsp[0].minor.yy0, 2);}
        break;
      case 68: /* cmd ::= CREATE USER ids PASS ids */
{ setCreateUserSql(pInfo, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);}
        break;
      case 69: /* bufsize ::= */
      case 71: /* pps ::= */ yytestcase(yyruleno==71);
      case 73: /* tseries ::= */ yytestcase(yyruleno==73);
      case 75: /* dbs ::= */ yytestcase(yyruleno==75);
      case 77: /* streams ::= */ yytestcase(yyruleno==77);
      case 79: /* storage ::= */ yytestcase(yyruleno==79);
      case 81: /* qtime ::= */ yytestcase(yyruleno==81);
      case 83: /* users ::= */ yytestcase(yyruleno==83);
      case 85: /* conns ::= */ yytestcase(yyruleno==85);
      case 87: /* state ::= */ yytestcase(yyruleno==87);
{ yymsp[1].minor.yy0.n = 0;   }
        break;
      case 70: /* bufsize ::= BUFSIZE INTEGER */
      case 72: /* pps ::= PPS INTEGER */ yytestcase(yyruleno==72);
      case 74: /* tseries ::= TSERIES INTEGER */ yytestcase(yyruleno==74);
      case 76: /* dbs ::= DBS INTEGER */ yytestcase(yyruleno==76);
      case 78: /* streams ::= STREAMS INTEGER */ yytestcase(yyruleno==78);
      case 80: /* storage ::= STORAGE INTEGER */ yytestcase(yyruleno==80);
      case 82: /* qtime ::= QTIME INTEGER */ yytestcase(yyruleno==82);
      case 84: /* users ::= USERS INTEGER */ yytestcase(yyruleno==84);
      case 86: /* conns ::= CONNS INTEGER */ yytestcase(yyruleno==86);
      case 88: /* state ::= STATE ids */ yytestcase(yyruleno==88);
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;     }
        break;
      case 89: /* acct_optr ::= pps tseries storage streams qtime dbs users conns state */
{
    yylhsminor.yy47.maxUsers   = (yymsp[-2].minor.yy0.n>0)?atoi(yymsp[-2].minor.yy0.z):-1;
    yylhsminor.yy47.maxDbs     = (yymsp[-3].minor.yy0.n>0)?atoi(yymsp[-3].minor.yy0.z):-1;
    yylhsminor.yy47.maxTimeSeries = (yymsp[-7].minor.yy0.n>0)?atoi(yymsp[-7].minor.yy0.z):-1;
    yylhsminor.yy47.maxStreams = (yymsp[-5].minor.yy0.n>0)?atoi(yymsp[-5].minor.yy0.z):-1;
    yylhsminor.yy47.maxPointsPerSecond     = (yymsp[-8].minor.yy0.n>0)?atoi(yymsp[-8].minor.yy0.z):-1;
    yylhsminor.yy47.maxStorage = (yymsp[-6].minor.yy0.n>0)?strtoll(yymsp[-6].minor.yy0.z, NULL, 10):-1;
    yylhsminor.yy47.maxQueryTime   = (yymsp[-4].minor.yy0.n>0)?strtoll(yymsp[-4].minor.yy0.z, NULL, 10):-1;
    yylhsminor.yy47.maxConnections   = (yymsp[-1].minor.yy0.n>0)?atoi(yymsp[-1].minor.yy0.z):-1;
    yylhsminor.yy47.stat    = yymsp[0].minor.yy0;
}
  yymsp[-8].minor.yy47 = yylhsminor.yy47;
        break;
      case 90: /* intitemlist ::= intitemlist COMMA intitem */
      case 159: /* tagitemlist ::= tagitemlist COMMA tagitem */ yytestcase(yyruleno==159);
{ yylhsminor.yy247 = tVariantListAppend(yymsp[-2].minor.yy247, &yymsp[0].minor.yy378, -1);    }
  yymsp[-2].minor.yy247 = yylhsminor.yy247;
        break;
      case 91: /* intitemlist ::= intitem */
      case 160: /* tagitemlist ::= tagitem */ yytestcase(yyruleno==160);
{ yylhsminor.yy247 = tVariantListAppend(NULL, &yymsp[0].minor.yy378, -1); }
  yymsp[0].minor.yy247 = yylhsminor.yy247;
        break;
      case 92: /* intitem ::= INTEGER */
      case 161: /* tagitem ::= INTEGER */ yytestcase(yyruleno==161);
      case 162: /* tagitem ::= FLOAT */ yytestcase(yyruleno==162);
      case 163: /* tagitem ::= STRING */ yytestcase(yyruleno==163);
      case 164: /* tagitem ::= BOOL */ yytestcase(yyruleno==164);
{ toTSDBType(yymsp[0].minor.yy0.type); tVariantCreate(&yylhsminor.yy378, &yymsp[0].minor.yy0); }
  yymsp[0].minor.yy378 = yylhsminor.yy378;
        break;
      case 93: /* keep ::= KEEP intitemlist */
{ yymsp[-1].minor.yy247 = yymsp[0].minor.yy247; }
        break;
      case 94: /* cache ::= CACHE INTEGER */
      case 95: /* replica ::= REPLICA INTEGER */ yytestcase(yyruleno==95);
      case 96: /* quorum ::= QUORUM INTEGER */ yytestcase(yyruleno==96);
      case 97: /* days ::= DAYS INTEGER */ yytestcase(yyruleno==97);
      case 98: /* minrows ::= MINROWS INTEGER */ yytestcase(yyruleno==98);
      case 99: /* maxrows ::= MAXROWS INTEGER */ yytestcase(yyruleno==99);
      case 100: /* blocks ::= BLOCKS INTEGER */ yytestcase(yyruleno==100);
      case 101: /* ctime ::= CTIME INTEGER */ yytestcase(yyruleno==101);
      case 102: /* wal ::= WAL INTEGER */ yytestcase(yyruleno==102);
      case 103: /* fsync ::= FSYNC INTEGER */ yytestcase(yyruleno==103);
      case 104: /* comp ::= COMP INTEGER */ yytestcase(yyruleno==104);
      case 105: /* prec ::= PRECISION STRING */ yytestcase(yyruleno==105);
      case 106: /* update ::= UPDATE INTEGER */ yytestcase(yyruleno==106);
      case 107: /* cachelast ::= CACHELAST INTEGER */ yytestcase(yyruleno==107);
      case 108: /* partitions ::= PARTITIONS INTEGER */ yytestcase(yyruleno==108);
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 109: /* db_optr ::= */
{setDefaultCreateDbOption(&yymsp[1].minor.yy262); yymsp[1].minor.yy262.dbType = TSDB_DB_TYPE_DEFAULT;}
        break;
      case 110: /* db_optr ::= db_optr cache */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.cacheBlockSize = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 111: /* db_optr ::= db_optr replica */
      case 128: /* alter_db_optr ::= alter_db_optr replica */ yytestcase(yyruleno==128);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.replica = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 112: /* db_optr ::= db_optr quorum */
      case 129: /* alter_db_optr ::= alter_db_optr quorum */ yytestcase(yyruleno==129);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.quorum = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 113: /* db_optr ::= db_optr days */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.daysPerFile = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 114: /* db_optr ::= db_optr minrows */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.minRowsPerBlock = strtod(yymsp[0].minor.yy0.z, NULL); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 115: /* db_optr ::= db_optr maxrows */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.maxRowsPerBlock = strtod(yymsp[0].minor.yy0.z, NULL); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 116: /* db_optr ::= db_optr blocks */
      case 131: /* alter_db_optr ::= alter_db_optr blocks */ yytestcase(yyruleno==131);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.numOfBlocks = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 117: /* db_optr ::= db_optr ctime */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.commitTime = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 118: /* db_optr ::= db_optr wal */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.walLevel = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 119: /* db_optr ::= db_optr fsync */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.fsyncPeriod = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 120: /* db_optr ::= db_optr comp */
      case 132: /* alter_db_optr ::= alter_db_optr comp */ yytestcase(yyruleno==132);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.compressionLevel = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 121: /* db_optr ::= db_optr prec */
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.precision = yymsp[0].minor.yy0; }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 122: /* db_optr ::= db_optr keep */
      case 130: /* alter_db_optr ::= alter_db_optr keep */ yytestcase(yyruleno==130);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.keep = yymsp[0].minor.yy247; }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 123: /* db_optr ::= db_optr update */
      case 133: /* alter_db_optr ::= alter_db_optr update */ yytestcase(yyruleno==133);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.update = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 124: /* db_optr ::= db_optr cachelast */
      case 134: /* alter_db_optr ::= alter_db_optr cachelast */ yytestcase(yyruleno==134);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.cachelast = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 125: /* topic_optr ::= db_optr */
      case 135: /* alter_topic_optr ::= alter_db_optr */ yytestcase(yyruleno==135);
{ yylhsminor.yy262 = yymsp[0].minor.yy262; yylhsminor.yy262.dbType = TSDB_DB_TYPE_TOPIC; }
  yymsp[0].minor.yy262 = yylhsminor.yy262;
        break;
      case 126: /* topic_optr ::= topic_optr partitions */
      case 136: /* alter_topic_optr ::= alter_topic_optr partitions */ yytestcase(yyruleno==136);
{ yylhsminor.yy262 = yymsp[-1].minor.yy262; yylhsminor.yy262.partitions = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[-1].minor.yy262 = yylhsminor.yy262;
        break;
      case 127: /* alter_db_optr ::= */
{ setDefaultCreateDbOption(&yymsp[1].minor.yy262); yymsp[1].minor.yy262.dbType = TSDB_DB_TYPE_DEFAULT;}
        break;
      case 137: /* typename ::= ids */
{ 
  yymsp[0].minor.yy0.type = 0;
  tSetColumnType (&yylhsminor.yy179, &yymsp[0].minor.yy0);
}
  yymsp[0].minor.yy179 = yylhsminor.yy179;
        break;
      case 138: /* typename ::= ids LP signed RP */
{
  if (yymsp[-1].minor.yy403 <= 0) {
    yymsp[-3].minor.yy0.type = 0;
    tSetColumnType(&yylhsminor.yy179, &yymsp[-3].minor.yy0);
  } else {
    yymsp[-3].minor.yy0.type = -yymsp[-1].minor.yy403;  // negative value of name length
    tSetColumnType(&yylhsminor.yy179, &yymsp[-3].minor.yy0);
  }
}
  yymsp[-3].minor.yy179 = yylhsminor.yy179;
        break;
      case 139: /* typename ::= ids UNSIGNED */
{
  yymsp[-1].minor.yy0.type = 0;
  yymsp[-1].minor.yy0.n = ((yymsp[0].minor.yy0.z + yymsp[0].minor.yy0.n) - yymsp[-1].minor.yy0.z);
  tSetColumnType (&yylhsminor.yy179, &yymsp[-1].minor.yy0);
}
  yymsp[-1].minor.yy179 = yylhsminor.yy179;
        break;
      case 140: /* signed ::= INTEGER */
{ yylhsminor.yy403 = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
  yymsp[0].minor.yy403 = yylhsminor.yy403;
        break;
      case 141: /* signed ::= PLUS INTEGER */
{ yymsp[-1].minor.yy403 = strtol(yymsp[0].minor.yy0.z, NULL, 10); }
        break;
      case 142: /* signed ::= MINUS INTEGER */
{ yymsp[-1].minor.yy403 = -strtol(yymsp[0].minor.yy0.z, NULL, 10);}
        break;
      case 146: /* cmd ::= CREATE TABLE create_table_list */
{ pInfo->type = TSDB_SQL_CREATE_TABLE; pInfo->pCreateTableInfo = yymsp[0].minor.yy336;}
        break;
      case 147: /* create_table_list ::= create_from_stable */
{
  SCreateTableSql* pCreateTable = calloc(1, sizeof(SCreateTableSql));
  pCreateTable->childTableInfo = taosArrayInit(4, sizeof(SCreatedTableInfo));

  taosArrayPush(pCreateTable->childTableInfo, &yymsp[0].minor.yy42);
  pCreateTable->type = TSQL_CREATE_TABLE_FROM_STABLE;
  yylhsminor.yy336 = pCreateTable;
}
  yymsp[0].minor.yy336 = yylhsminor.yy336;
        break;
      case 148: /* create_table_list ::= create_table_list create_from_stable */
{
  taosArrayPush(yymsp[-1].minor.yy336->childTableInfo, &yymsp[0].minor.yy42);
  yylhsminor.yy336 = yymsp[-1].minor.yy336;
}
  yymsp[-1].minor.yy336 = yylhsminor.yy336;
        break;
      case 149: /* create_table_args ::= ifnotexists ftblname LP columnlist RP */
{
  yylhsminor.yy336 = tSetCreateTableInfo(yymsp[-1].minor.yy247, NULL, NULL, TSQL_CREATE_TABLE);
  setSqlInfo(pInfo, yylhsminor.yy336, NULL, TSDB_SQL_CREATE_TABLE);

  setCreatedTableName(pInfo, &yymsp[-3].minor.yy0, &yymsp[-4].minor.yy0);
}
  yymsp[-4].minor.yy336 = yylhsminor.yy336;
        break;
      case 150: /* create_stable_args ::= ifnotexists ftblname LP columnlist RP TAGS LP columnlist RP */
{
  yylhsminor.yy336 = tSetCreateTableInfo(yymsp[-5].minor.yy247, yymsp[-1].minor.yy247, NULL, TSQL_CREATE_STABLE);
  setSqlInfo(pInfo, yylhsminor.yy336, NULL, TSDB_SQL_CREATE_TABLE);

  setCreatedTableName(pInfo, &yymsp[-7].minor.yy0, &yymsp[-8].minor.yy0);
}
  yymsp[-8].minor.yy336 = yylhsminor.yy336;
        break;
      case 151: /* create_from_stable ::= ifnotexists ftblname USING ftblname TAGS LP tagitemlist RP */
{
  yylhsminor.yy42 = createNewChildTableInfo(&yymsp[-4].minor.yy0, NULL, yymsp[-1].minor.yy247, &yymsp[-6].minor.yy0, &yymsp[-7].minor.yy0);
}
  yymsp[-7].minor.yy42 = yylhsminor.yy42;
        break;
      case 152: /* create_from_stable ::= ifnotexists ftblname USING ftblname LP tagNamelist RP TAGS LP tagitemlist RP */
{
  yylhsminor.yy42 = createNewChildTableInfo(&yymsp[-7].minor.yy0, yymsp[-5].minor.yy247, yymsp[-1].minor.yy247, &yymsp[-9].minor.yy0, &yymsp[-10].minor.yy0);
}
  yymsp[-10].minor.yy42 = yylhsminor.yy42;
        break;
      case 153: /* tagNamelist ::= tagNamelist COMMA ids */
{taosArrayPush(yymsp[-2].minor.yy247, &yymsp[0].minor.yy0); yylhsminor.yy247 = yymsp[-2].minor.yy247;  }
  yymsp[-2].minor.yy247 = yylhsminor.yy247;
        break;
      case 154: /* tagNamelist ::= ids */
{yylhsminor.yy247 = taosArrayInit(4, sizeof(SStrToken)); taosArrayPush(yylhsminor.yy247, &yymsp[0].minor.yy0);}
  yymsp[0].minor.yy247 = yylhsminor.yy247;
        break;
      case 155: /* create_table_args ::= ifnotexists ftblname AS select */
{
  yylhsminor.yy336 = tSetCreateTableInfo(NULL, NULL, yymsp[0].minor.yy246, TSQL_CREATE_STREAM);
  setSqlInfo(pInfo, yylhsminor.yy336, NULL, TSDB_SQL_CREATE_TABLE);

  setCreatedTableName(pInfo, &yymsp[-2].minor.yy0, &yymsp[-3].minor.yy0);
}
  yymsp[-3].minor.yy336 = yylhsminor.yy336;
        break;
      case 156: /* columnlist ::= columnlist COMMA column */
{taosArrayPush(yymsp[-2].minor.yy247, &yymsp[0].minor.yy179); yylhsminor.yy247 = yymsp[-2].minor.yy247;  }
  yymsp[-2].minor.yy247 = yylhsminor.yy247;
        break;
      case 157: /* columnlist ::= column */
{yylhsminor.yy247 = taosArrayInit(4, sizeof(TAOS_FIELD)); taosArrayPush(yylhsminor.yy247, &yymsp[0].minor.yy179);}
  yymsp[0].minor.yy247 = yylhsminor.yy247;
        break;
      case 158: /* column ::= ids typename */
{
  tSetColumnInfo(&yylhsminor.yy179, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy179);
}
  yymsp[-1].minor.yy179 = yylhsminor.yy179;
        break;
      case 165: /* tagitem ::= NULL */
{ yymsp[0].minor.yy0.type = 0; tVariantCreate(&yylhsminor.yy378, &yymsp[0].minor.yy0); }
  yymsp[0].minor.yy378 = yylhsminor.yy378;
        break;
      case 166: /* tagitem ::= NOW */
{ yymsp[0].minor.yy0.type = TSDB_DATA_TYPE_TIMESTAMP; tVariantCreate(&yylhsminor.yy378, &yymsp[0].minor.yy0);}
  yymsp[0].minor.yy378 = yylhsminor.yy378;
        break;
      case 167: /* tagitem ::= MINUS INTEGER */
      case 168: /* tagitem ::= MINUS FLOAT */ yytestcase(yyruleno==168);
      case 169: /* tagitem ::= PLUS INTEGER */ yytestcase(yyruleno==169);
      case 170: /* tagitem ::= PLUS FLOAT */ yytestcase(yyruleno==170);
{
    yymsp[-1].minor.yy0.n += yymsp[0].minor.yy0.n;
    yymsp[-1].minor.yy0.type = yymsp[0].minor.yy0.type;
    toTSDBType(yymsp[-1].minor.yy0.type);
    tVariantCreate(&yylhsminor.yy378, &yymsp[-1].minor.yy0);
}
  yymsp[-1].minor.yy378 = yylhsminor.yy378;
        break;
      case 171: /* select ::= SELECT selcollist from where_opt interval_option sliding_opt session_option windowstate_option fill_opt groupby_opt having_opt orderby_opt slimit_opt limit_opt */
{
  yylhsminor.yy246 = tSetQuerySqlNode(&yymsp[-13].minor.yy0, yymsp[-12].minor.yy247, yymsp[-11].minor.yy46, yymsp[-10].minor.yy44, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, &yymsp[-9].minor.yy430, &yymsp[-7].minor.yy507, &yymsp[-6].minor.yy492, &yymsp[-8].minor.yy0, yymsp[-5].minor.yy247, &yymsp[0].minor.yy204, &yymsp[-1].minor.yy204, yymsp[-3].minor.yy44);
}
  yymsp[-13].minor.yy246 = yylhsminor.yy246;
        break;
      case 172: /* select ::= LP select RP */
{yymsp[-2].minor.yy246 = yymsp[-1].minor.yy246;}
        break;
      case 173: /* union ::= select */
{ yylhsminor.yy247 = setSubclause(NULL, yymsp[0].minor.yy246); }
  yymsp[0].minor.yy247 = yylhsminor.yy247;
        break;
      case 174: /* union ::= union UNION ALL select */
{ yylhsminor.yy247 = appendSelectClause(yymsp[-3].minor.yy247, yymsp[0].minor.yy246); }
  yymsp[-3].minor.yy247 = yylhsminor.yy247;
        break;
      case 175: /* cmd ::= union */
{ setSqlInfo(pInfo, yymsp[0].minor.yy247, NULL, TSDB_SQL_SELECT); }
        break;
      case 176: /* select ::= SELECT selcollist */
{
  yylhsminor.yy246 = tSetQuerySqlNode(&yymsp[-1].minor.yy0, yymsp[0].minor.yy247, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}
  yymsp[-1].minor.yy246 = yylhsminor.yy246;
        break;
      case 177: /* sclp ::= selcollist COMMA */
{yylhsminor.yy247 = yymsp[-1].minor.yy247;}
  yymsp[-1].minor.yy247 = yylhsminor.yy247;
        break;
      case 178: /* sclp ::= */
      case 210: /* orderby_opt ::= */ yytestcase(yyruleno==210);
{yymsp[1].minor.yy247 = 0;}
        break;
      case 179: /* selcollist ::= sclp distinct expr as */
{
   yylhsminor.yy247 = tSqlExprListAppend(yymsp[-3].minor.yy247, yymsp[-1].minor.yy44,  yymsp[-2].minor.yy0.n? &yymsp[-2].minor.yy0:0, yymsp[0].minor.yy0.n?&yymsp[0].minor.yy0:0);
}
  yymsp[-3].minor.yy247 = yylhsminor.yy247;
        break;
      case 180: /* selcollist ::= sclp STAR */
{
   tSqlExpr *pNode = tSqlExprCreateIdValue(NULL, TK_ALL);
   yylhsminor.yy247 = tSqlExprListAppend(yymsp[-1].minor.yy247, pNode, 0, 0);
}
  yymsp[-1].minor.yy247 = yylhsminor.yy247;
        break;
      case 181: /* as ::= AS ids */
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;    }
        break;
      case 182: /* as ::= ids */
{ yylhsminor.yy0 = yymsp[0].minor.yy0;    }
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 183: /* as ::= */
{ yymsp[1].minor.yy0.n = 0;  }
        break;
      case 184: /* distinct ::= DISTINCT */
{ yylhsminor.yy0 = yymsp[0].minor.yy0;  }
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 186: /* from ::= FROM tablelist */
      case 187: /* from ::= FROM sub */ yytestcase(yyruleno==187);
{yymsp[-1].minor.yy46 = yymsp[0].minor.yy46;}
        break;
      case 188: /* sub ::= LP union RP */
{yymsp[-2].minor.yy46 = addSubqueryElem(NULL, yymsp[-1].minor.yy247, NULL);}
        break;
      case 189: /* sub ::= LP union RP ids */
{yymsp[-3].minor.yy46 = addSubqueryElem(NULL, yymsp[-2].minor.yy247, &yymsp[0].minor.yy0);}
        break;
      case 190: /* sub ::= sub COMMA LP union RP ids */
{yylhsminor.yy46 = addSubqueryElem(yymsp[-5].minor.yy46, yymsp[-2].minor.yy247, &yymsp[0].minor.yy0);}
  yymsp[-5].minor.yy46 = yylhsminor.yy46;
        break;
      case 191: /* tablelist ::= ftblname */
{
  yylhsminor.yy46 = setTableNameList(NULL, &yymsp[0].minor.yy0, NULL);
}
  yymsp[0].minor.yy46 = yylhsminor.yy46;
        break;
      case 192: /* tablelist ::= ftblname ids */
{
  yylhsminor.yy46 = setTableNameList(NULL, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);
}
  yymsp[-1].minor.yy46 = yylhsminor.yy46;
        break;
      case 193: /* tablelist ::= tablelist COMMA ftblname */
{
  yylhsminor.yy46 = setTableNameList(yymsp[-2].minor.yy46, &yymsp[0].minor.yy0, NULL);
}
  yymsp[-2].minor.yy46 = yylhsminor.yy46;
        break;
      case 194: /* tablelist ::= tablelist COMMA ftblname ids */
{
  yylhsminor.yy46 = setTableNameList(yymsp[-3].minor.yy46, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);
}
  yymsp[-3].minor.yy46 = yylhsminor.yy46;
        break;
      case 195: /* tmvar ::= VARIABLE */
{yylhsminor.yy0 = yymsp[0].minor.yy0;}
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 196: /* interval_option ::= intervalKey LP tmvar RP */
{yylhsminor.yy430.interval = yymsp[-1].minor.yy0; yylhsminor.yy430.offset.n = 0; yylhsminor.yy430.token = yymsp[-3].minor.yy2;}
  yymsp[-3].minor.yy430 = yylhsminor.yy430;
        break;
      case 197: /* interval_option ::= intervalKey LP tmvar COMMA tmvar RP */
{yylhsminor.yy430.interval = yymsp[-3].minor.yy0; yylhsminor.yy430.offset = yymsp[-1].minor.yy0;   yylhsminor.yy430.token = yymsp[-5].minor.yy2;}
  yymsp[-5].minor.yy430 = yylhsminor.yy430;
        break;
      case 198: /* interval_option ::= */
{memset(&yymsp[1].minor.yy430, 0, sizeof(yymsp[1].minor.yy430));}
        break;
      case 199: /* intervalKey ::= INTERVAL */
{yymsp[0].minor.yy2 = TK_INTERVAL;}
        break;
      case 200: /* intervalKey ::= EVERY */
{yymsp[0].minor.yy2 = TK_EVERY;   }
        break;
      case 201: /* session_option ::= */
{yymsp[1].minor.yy507.col.n = 0; yymsp[1].minor.yy507.gap.n = 0;}
        break;
      case 202: /* session_option ::= SESSION LP ids cpxName COMMA tmvar RP */
{
   yymsp[-4].minor.yy0.n += yymsp[-3].minor.yy0.n;
   yymsp[-6].minor.yy507.col = yymsp[-4].minor.yy0;
   yymsp[-6].minor.yy507.gap = yymsp[-1].minor.yy0;
}
        break;
      case 203: /* windowstate_option ::= */
{ yymsp[1].minor.yy492.col.n = 0; yymsp[1].minor.yy492.col.z = NULL;}
        break;
      case 204: /* windowstate_option ::= STATE_WINDOW LP ids RP */
{ yymsp[-3].minor.yy492.col = yymsp[-1].minor.yy0; }
        break;
      case 205: /* fill_opt ::= */
{ yymsp[1].minor.yy247 = 0;     }
        break;
      case 206: /* fill_opt ::= FILL LP ID COMMA tagitemlist RP */
{
    tVariant A = {0};
    toTSDBType(yymsp[-3].minor.yy0.type);
    tVariantCreate(&A, &yymsp[-3].minor.yy0);

    tVariantListInsert(yymsp[-1].minor.yy247, &A, -1, 0);
    yymsp[-5].minor.yy247 = yymsp[-1].minor.yy247;
}
        break;
      case 207: /* fill_opt ::= FILL LP ID RP */
{
    toTSDBType(yymsp[-1].minor.yy0.type);
    yymsp[-3].minor.yy247 = tVariantListAppendToken(NULL, &yymsp[-1].minor.yy0, -1);
}
        break;
      case 208: /* sliding_opt ::= SLIDING LP tmvar RP */
{yymsp[-3].minor.yy0 = yymsp[-1].minor.yy0;     }
        break;
      case 209: /* sliding_opt ::= */
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.type = 0;   }
        break;
      case 211: /* orderby_opt ::= ORDER BY sortlist */
{yymsp[-2].minor.yy247 = yymsp[0].minor.yy247;}
        break;
      case 212: /* sortlist ::= sortlist COMMA item sortorder */
{
    yylhsminor.yy247 = tVariantListAppend(yymsp[-3].minor.yy247, &yymsp[-1].minor.yy378, yymsp[0].minor.yy222);
}
  yymsp[-3].minor.yy247 = yylhsminor.yy247;
        break;
      case 213: /* sortlist ::= item sortorder */
{
  yylhsminor.yy247 = tVariantListAppend(NULL, &yymsp[-1].minor.yy378, yymsp[0].minor.yy222);
}
  yymsp[-1].minor.yy247 = yylhsminor.yy247;
        break;
      case 214: /* item ::= ids cpxName */
{
  toTSDBType(yymsp[-1].minor.yy0.type);
  yymsp[-1].minor.yy0.n += yymsp[0].minor.yy0.n;

  tVariantCreate(&yylhsminor.yy378, &yymsp[-1].minor.yy0);
}
  yymsp[-1].minor.yy378 = yylhsminor.yy378;
        break;
      case 215: /* sortorder ::= ASC */
{ yymsp[0].minor.yy222 = TSDB_ORDER_ASC; }
        break;
      case 216: /* sortorder ::= DESC */
{ yymsp[0].minor.yy222 = TSDB_ORDER_DESC;}
        break;
      case 217: /* sortorder ::= */
{ yymsp[1].minor.yy222 = TSDB_ORDER_ASC; }
        break;
      case 218: /* groupby_opt ::= */
{ yymsp[1].minor.yy247 = 0;}
        break;
      case 219: /* groupby_opt ::= GROUP BY grouplist */
{ yymsp[-2].minor.yy247 = yymsp[0].minor.yy247;}
        break;
      case 220: /* grouplist ::= grouplist COMMA item */
{
  yylhsminor.yy247 = tVariantListAppend(yymsp[-2].minor.yy247, &yymsp[0].minor.yy378, -1);
}
  yymsp[-2].minor.yy247 = yylhsminor.yy247;
        break;
      case 221: /* grouplist ::= item */
{
  yylhsminor.yy247 = tVariantListAppend(NULL, &yymsp[0].minor.yy378, -1);
}
  yymsp[0].minor.yy247 = yylhsminor.yy247;
        break;
      case 222: /* having_opt ::= */
      case 232: /* where_opt ::= */ yytestcase(yyruleno==232);
      case 276: /* expritem ::= */ yytestcase(yyruleno==276);
{yymsp[1].minor.yy44 = 0;}
        break;
      case 223: /* having_opt ::= HAVING expr */
      case 233: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==233);
{yymsp[-1].minor.yy44 = yymsp[0].minor.yy44;}
        break;
      case 224: /* limit_opt ::= */
      case 228: /* slimit_opt ::= */ yytestcase(yyruleno==228);
{yymsp[1].minor.yy204.limit = -1; yymsp[1].minor.yy204.offset = 0;}
        break;
      case 225: /* limit_opt ::= LIMIT signed */
      case 229: /* slimit_opt ::= SLIMIT signed */ yytestcase(yyruleno==229);
{yymsp[-1].minor.yy204.limit = yymsp[0].minor.yy403;  yymsp[-1].minor.yy204.offset = 0;}
        break;
      case 226: /* limit_opt ::= LIMIT signed OFFSET signed */
{ yymsp[-3].minor.yy204.limit = yymsp[-2].minor.yy403;  yymsp[-3].minor.yy204.offset = yymsp[0].minor.yy403;}
        break;
      case 227: /* limit_opt ::= LIMIT signed COMMA signed */
{ yymsp[-3].minor.yy204.limit = yymsp[0].minor.yy403;  yymsp[-3].minor.yy204.offset = yymsp[-2].minor.yy403;}
        break;
      case 230: /* slimit_opt ::= SLIMIT signed SOFFSET signed */
{yymsp[-3].minor.yy204.limit = yymsp[-2].minor.yy403;  yymsp[-3].minor.yy204.offset = yymsp[0].minor.yy403;}
        break;
      case 231: /* slimit_opt ::= SLIMIT signed COMMA signed */
{yymsp[-3].minor.yy204.limit = yymsp[0].minor.yy403;  yymsp[-3].minor.yy204.offset = yymsp[-2].minor.yy403;}
        break;
      case 234: /* expr ::= LP expr RP */
{yylhsminor.yy44 = yymsp[-1].minor.yy44; yylhsminor.yy44->exprToken.z = yymsp[-2].minor.yy0.z; yylhsminor.yy44->exprToken.n = (yymsp[0].minor.yy0.z - yymsp[-2].minor.yy0.z + 1);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 235: /* expr ::= ID */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_ID);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 236: /* expr ::= ID DOT ID */
{ yymsp[-2].minor.yy0.n += (1+yymsp[0].minor.yy0.n); yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[-2].minor.yy0, TK_ID);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 237: /* expr ::= ID DOT STAR */
{ yymsp[-2].minor.yy0.n += (1+yymsp[0].minor.yy0.n); yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[-2].minor.yy0, TK_ALL);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 238: /* expr ::= INTEGER */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_INTEGER);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 239: /* expr ::= MINUS INTEGER */
      case 240: /* expr ::= PLUS INTEGER */ yytestcase(yyruleno==240);
{ yymsp[-1].minor.yy0.n += yymsp[0].minor.yy0.n; yymsp[-1].minor.yy0.type = TK_INTEGER; yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[-1].minor.yy0, TK_INTEGER);}
  yymsp[-1].minor.yy44 = yylhsminor.yy44;
        break;
      case 241: /* expr ::= FLOAT */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_FLOAT);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 242: /* expr ::= MINUS FLOAT */
      case 243: /* expr ::= PLUS FLOAT */ yytestcase(yyruleno==243);
{ yymsp[-1].minor.yy0.n += yymsp[0].minor.yy0.n; yymsp[-1].minor.yy0.type = TK_FLOAT; yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[-1].minor.yy0, TK_FLOAT);}
  yymsp[-1].minor.yy44 = yylhsminor.yy44;
        break;
      case 244: /* expr ::= STRING */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_STRING);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 245: /* expr ::= NOW */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_NOW); }
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 246: /* expr ::= VARIABLE */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_VARIABLE);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 247: /* expr ::= PLUS VARIABLE */
      case 248: /* expr ::= MINUS VARIABLE */ yytestcase(yyruleno==248);
{ yymsp[-1].minor.yy0.n += yymsp[0].minor.yy0.n; yymsp[-1].minor.yy0.type = TK_VARIABLE; yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[-1].minor.yy0, TK_VARIABLE);}
  yymsp[-1].minor.yy44 = yylhsminor.yy44;
        break;
      case 249: /* expr ::= BOOL */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_BOOL);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 250: /* expr ::= NULL */
{ yylhsminor.yy44 = tSqlExprCreateIdValue(&yymsp[0].minor.yy0, TK_NULL);}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 251: /* expr ::= ID LP exprlist RP */
{ tStrTokenAppend(pInfo->funcs, &yymsp[-3].minor.yy0); yylhsminor.yy44 = tSqlExprCreateFunction(yymsp[-1].minor.yy247, &yymsp[-3].minor.yy0, &yymsp[0].minor.yy0, yymsp[-3].minor.yy0.type); }
  yymsp[-3].minor.yy44 = yylhsminor.yy44;
        break;
      case 252: /* expr ::= ID LP STAR RP */
{ tStrTokenAppend(pInfo->funcs, &yymsp[-3].minor.yy0); yylhsminor.yy44 = tSqlExprCreateFunction(NULL, &yymsp[-3].minor.yy0, &yymsp[0].minor.yy0, yymsp[-3].minor.yy0.type); }
  yymsp[-3].minor.yy44 = yylhsminor.yy44;
        break;
      case 253: /* expr ::= expr IS NULL */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, NULL, TK_ISNULL);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 254: /* expr ::= expr IS NOT NULL */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-3].minor.yy44, NULL, TK_NOTNULL);}
  yymsp[-3].minor.yy44 = yylhsminor.yy44;
        break;
      case 255: /* expr ::= expr LT expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_LT);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 256: /* expr ::= expr GT expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_GT);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 257: /* expr ::= expr LE expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_LE);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 258: /* expr ::= expr GE expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_GE);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 259: /* expr ::= expr NE expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_NE);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 260: /* expr ::= expr EQ expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_EQ);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 261: /* expr ::= expr BETWEEN expr AND expr */
{ tSqlExpr* X2 = tSqlExprClone(yymsp[-4].minor.yy44); yylhsminor.yy44 = tSqlExprCreate(tSqlExprCreate(yymsp[-4].minor.yy44, yymsp[-2].minor.yy44, TK_GE), tSqlExprCreate(X2, yymsp[0].minor.yy44, TK_LE), TK_AND);}
  yymsp[-4].minor.yy44 = yylhsminor.yy44;
        break;
      case 262: /* expr ::= expr AND expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_AND);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 263: /* expr ::= expr OR expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_OR); }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 264: /* expr ::= expr PLUS expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_PLUS);  }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 265: /* expr ::= expr MINUS expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_MINUS); }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 266: /* expr ::= expr STAR expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_STAR);  }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 267: /* expr ::= expr SLASH expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_DIVIDE);}
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 268: /* expr ::= expr REM expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_REM);   }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 269: /* expr ::= expr LIKE expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_LIKE);  }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 270: /* expr ::= expr MATCH expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_MATCH);  }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 271: /* expr ::= expr NMATCH expr */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-2].minor.yy44, yymsp[0].minor.yy44, TK_NMATCH);  }
  yymsp[-2].minor.yy44 = yylhsminor.yy44;
        break;
      case 272: /* expr ::= expr IN LP exprlist RP */
{yylhsminor.yy44 = tSqlExprCreate(yymsp[-4].minor.yy44, (tSqlExpr*)yymsp[-1].minor.yy247, TK_IN); }
  yymsp[-4].minor.yy44 = yylhsminor.yy44;
        break;
      case 273: /* exprlist ::= exprlist COMMA expritem */
{yylhsminor.yy247 = tSqlExprListAppend(yymsp[-2].minor.yy247,yymsp[0].minor.yy44,0, 0);}
  yymsp[-2].minor.yy247 = yylhsminor.yy247;
        break;
      case 274: /* exprlist ::= expritem */
{yylhsminor.yy247 = tSqlExprListAppend(0,yymsp[0].minor.yy44,0, 0);}
  yymsp[0].minor.yy247 = yylhsminor.yy247;
        break;
      case 275: /* expritem ::= expr */
{yylhsminor.yy44 = yymsp[0].minor.yy44;}
  yymsp[0].minor.yy44 = yylhsminor.yy44;
        break;
      case 277: /* cmd ::= RESET QUERY CACHE */
{ setDCLSqlElems(pInfo, TSDB_SQL_RESET_CACHE, 0);}
        break;
      case 278: /* cmd ::= SYNCDB ids REPLICA */
{ setDCLSqlElems(pInfo, TSDB_SQL_SYNC_DB_REPLICA, 1, &yymsp[-1].minor.yy0);}
        break;
      case 279: /* cmd ::= ALTER TABLE ftblname ADD COLUMN columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_ADD_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 280: /* cmd ::= ALTER TABLE ftblname DROP COLUMN ids */
{
    toTSDBType(yymsp[0].minor.yy0.type);
    SArray* K = tVariantListAppendToken(NULL, &yymsp[0].minor.yy0, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, NULL, K, TSDB_ALTER_TABLE_DROP_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 281: /* cmd ::= ALTER TABLE ftblname MODIFY COLUMN columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_CHANGE_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 282: /* cmd ::= ALTER TABLE ftblname ADD TAG columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_ADD_TAG_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 283: /* cmd ::= ALTER TABLE ftblname DROP TAG ids */
{
    toTSDBType(yymsp[0].minor.yy0.type);
    SArray* A = tVariantListAppendToken(NULL, &yymsp[0].minor.yy0, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, NULL, A, TSDB_ALTER_TABLE_DROP_TAG_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 284: /* cmd ::= ALTER TABLE ftblname CHANGE TAG ids ids */
{

    toTSDBType(yymsp[-1].minor.yy0.type);
    SArray* A = tVariantListAppendToken(NULL, &yymsp[-1].minor.yy0, -1);

    toTSDBType(yymsp[0].minor.yy0.type);
    A = tVariantListAppendToken(A, &yymsp[0].minor.yy0, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-4].minor.yy0, NULL, A, TSDB_ALTER_TABLE_CHANGE_TAG_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 285: /* cmd ::= ALTER TABLE ftblname SET TAG ids EQ tagitem */
{

    toTSDBType(yymsp[-2].minor.yy0.type);
    SArray* A = tVariantListAppendToken(NULL, &yymsp[-2].minor.yy0, -1);
    A = tVariantListAppend(A, &yymsp[0].minor.yy378, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-5].minor.yy0, NULL, A, TSDB_ALTER_TABLE_UPDATE_TAG_VAL, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 286: /* cmd ::= ALTER TABLE ftblname MODIFY TAG columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_MODIFY_TAG_COLUMN, -1);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 287: /* cmd ::= ALTER STABLE ftblname ADD COLUMN columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_ADD_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 288: /* cmd ::= ALTER STABLE ftblname DROP COLUMN ids */
{

    toTSDBType(yymsp[0].minor.yy0.type);
    SArray* K = tVariantListAppendToken(NULL, &yymsp[0].minor.yy0, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, NULL, K, TSDB_ALTER_TABLE_DROP_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 289: /* cmd ::= ALTER STABLE ftblname MODIFY COLUMN columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_CHANGE_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 290: /* cmd ::= ALTER STABLE ftblname ADD TAG columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_ADD_TAG_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 291: /* cmd ::= ALTER STABLE ftblname DROP TAG ids */
{

    toTSDBType(yymsp[0].minor.yy0.type);
    SArray* A = tVariantListAppendToken(NULL, &yymsp[0].minor.yy0, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, NULL, A, TSDB_ALTER_TABLE_DROP_TAG_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 292: /* cmd ::= ALTER STABLE ftblname CHANGE TAG ids ids */
{

    toTSDBType(yymsp[-1].minor.yy0.type);
    SArray* A = tVariantListAppendToken(NULL, &yymsp[-1].minor.yy0, -1);

    toTSDBType(yymsp[0].minor.yy0.type);
    A = tVariantListAppendToken(A, &yymsp[0].minor.yy0, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-4].minor.yy0, NULL, A, TSDB_ALTER_TABLE_CHANGE_TAG_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 293: /* cmd ::= ALTER STABLE ftblname SET TAG ids EQ tagitem */
{

    toTSDBType(yymsp[-2].minor.yy0.type);
    SArray* A = tVariantListAppendToken(NULL, &yymsp[-2].minor.yy0, -1);
    A = tVariantListAppend(A, &yymsp[0].minor.yy378, -1);

    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-5].minor.yy0, NULL, A, TSDB_ALTER_TABLE_UPDATE_TAG_VAL, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 294: /* cmd ::= ALTER STABLE ftblname MODIFY TAG columnlist */
{
    SAlterTableInfo* pAlterTable = tSetAlterTableInfo(&yymsp[-3].minor.yy0, yymsp[0].minor.yy247, NULL, TSDB_ALTER_TABLE_MODIFY_TAG_COLUMN, TSDB_SUPER_TABLE);
    setSqlInfo(pInfo, pAlterTable, NULL, TSDB_SQL_ALTER_TABLE);
}
        break;
      case 295: /* cmd ::= KILL CONNECTION INTEGER */
{setKillSql(pInfo, TSDB_SQL_KILL_CONNECTION, &yymsp[0].minor.yy0);}
        break;
      case 296: /* cmd ::= KILL STREAM INTEGER COLON INTEGER */
{yymsp[-2].minor.yy0.n += (yymsp[-1].minor.yy0.n + yymsp[0].minor.yy0.n); setKillSql(pInfo, TSDB_SQL_KILL_STREAM, &yymsp[-2].minor.yy0);}
        break;
      case 297: /* cmd ::= KILL QUERY INTEGER COLON INTEGER */
{yymsp[-2].minor.yy0.n += (yymsp[-1].minor.yy0.n + yymsp[0].minor.yy0.n); setKillSql(pInfo, TSDB_SQL_KILL_QUERY, &yymsp[-2].minor.yy0);}
        break;
      default:
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfoLhs)/sizeof(yyRuleInfoLhs[0]) );
  yygoto = yyRuleInfoLhs[yyruleno];
  yysize = yyRuleInfoNRhs[yyruleno];
  yyact = yy_find_reduce_action(yymsp[yysize].stateno,(YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert( !(yyact>YY_MAX_SHIFT && yyact<=YY_MAX_SHIFTREDUCE) );

  /* It is not possible for a REDUCE to be followed by an error */
  assert( yyact!=YY_ERROR_ACTION );

  yymsp += yysize+1;
  yypParser->yytos = yymsp;
  yymsp->stateno = (YYACTIONTYPE)yyact;
  yymsp->major = (YYCODETYPE)yygoto;
  yyTraceShift(yypParser, yyact, "... then shift");
  return yyact;
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH
  ParseCTX_FETCH
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  ParseARG_STORE /* Suppress warning about unused %extra_argument variable */
  ParseCTX_STORE
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  ParseTOKENTYPE yyminor         /* The minor type of the error token */
){
  ParseARG_FETCH
  ParseCTX_FETCH
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/

  pInfo->valid = false;
  int32_t outputBufLen = tListLen(pInfo->msg);
  int32_t len = 0;

  if(TOKEN.z) {
    char msg[] = "syntax error near \"%s\"";
    int32_t sqlLen = strlen(&TOKEN.z[0]);

    if (sqlLen + sizeof(msg)/sizeof(msg[0]) + 1 > outputBufLen) {
        char tmpstr[128] = {0};
        memcpy(tmpstr, &TOKEN.z[0], sizeof(tmpstr)/sizeof(tmpstr[0]) - 1);
        len = sprintf(pInfo->msg, msg, tmpstr);
    } else {
        len = sprintf(pInfo->msg, msg, &TOKEN.z[0]);
    }

  } else {
    len = sprintf(pInfo->msg, "Incomplete SQL statement");
  }

  assert(len <= outputBufLen);
/************ End %syntax_error code ******************************************/
  ParseARG_STORE /* Suppress warning about unused %extra_argument variable */
  ParseCTX_STORE
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH
  ParseCTX_FETCH
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/

/*********** End %parse_accept code *******************************************/
  ParseARG_STORE /* Suppress warning about unused %extra_argument variable */
  ParseCTX_STORE
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  YYACTIONTYPE yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser = (yyParser*)yyp;  /* The parser */
  ParseCTX_FETCH
  ParseARG_STORE

  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif

  yyact = yypParser->yytos->stateno;
#ifndef NDEBUG
  if( yyTraceFILE ){
    if( yyact < YY_MIN_REDUCE ){
      fprintf(yyTraceFILE,"%sInput '%s' in state %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact);
    }else{
      fprintf(yyTraceFILE,"%sInput '%s' with pending reduce %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact-YY_MIN_REDUCE);
    }
  }
#endif

  do{
    assert( yyact==yypParser->yytos->stateno );
    yyact = yy_find_shift_action((YYCODETYPE)yymajor,yyact);
    if( yyact >= YY_MIN_REDUCE ){
      yyact = yy_reduce(yypParser,yyact-YY_MIN_REDUCE,yymajor,
                        yyminor ParseCTX_PARAM);
    }else if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,(YYCODETYPE)yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      break;
    }else if( yyact==YY_ACCEPT_ACTION ){
      yypParser->yytos--;
      yy_accept(yypParser);
      return;
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) > YY_MAX_SHIFTREDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
      if( yymajor==YYNOCODE ) break;
      yyact = yypParser->yytos->stateno;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      break;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      break;
#endif
    }
  }while( yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}

/*
** Return the fallback token corresponding to canonical token iToken, or
** 0 if iToken has no fallback.
*/
int ParseFallback(int iToken){
#ifdef YYFALLBACK
  assert( iToken<(int)(sizeof(yyFallback)/sizeof(yyFallback[0])) );
  return yyFallback[iToken];
#else
  (void)iToken;
  return 0;
#endif
}
