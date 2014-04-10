%{
/* --------------------------------------------------------------------- *
 * Manager:  Symbol table manager and parser                             *
 *                                                                       *
 * This file contains functions for managing a set of lookup tables and  *
 * several parsers.  The tables maintained are: (1) a global symbol tab- *
 * le for the parser containing mathematical constants and functions,    *
 * (2) a parameter table used by the spectral element solver, and (3) an *
 * option table used maintaining integer-valued options.  The interface  *
 * routines are as follows:                                              *
 *                                                                       *
 *                                                                       *
 * Internal Symbol Table                                                 *
 * ---------------------                                                 * 
 * Symbol *install(char *name, int type, ...)                            *
 * Symbol *lookup (char *name)                                           *
 *                                                                       *
 * Parameter Symbol Table                                                *
 * ----------------------                                                *
 * int     iparam     (char *name)                                       *
 * int     iparam_set (char *name, int value)                            *
 *                                                                       *
 * double  dparam     (char *name)                                       *
 * double  dparam_set (char *name, double value)                         *
 *                                                                       *
 * Options Table                                                         *
 * -------------                                                         *
 * int     option     (char *name)                                       *
 * int     option_set (char *name, int status)                           *
 *                                                                       *
 *                                                                       *
 * Vector/Scalar Parser                                                  *
 * --------------------                                                  *
 * The parsers provide two types of function-string parsing based on the *
 * type of access involved: a vector parser for forcing functions and    *
 * boundary conditions and a scalar parser for miscellaneous applica-    *   
 * tions.  The interfaces for these routines are:                        *
 *                                                                       *
 * void    vector_def (char *vlist, char *function)                      *
 * void    vector_set (int   vsize, v1, v2, ..., f(v))                   *
 *                                                                       *
 * double  scalar     (char *function)                                   *
 * --------------------------------------------------------------------- */
 
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>

#include "tree.h"
#include "zbesj.h"


#define SIZE   512       /* Maximum number of function string characters */

typedef double (*PFD)(); /* Pointer to a function returning double */

typedef struct Symbol {  /* Symbol table entry */
        char *name   ;   /* Symbol name        */
        short type   ;   /* VAR, BLTIN, UNDEF, DPARAM, IPARAM, OPTION */
	short status ;   /* (see status definitions) */
        union {
		int    num ;
                double val ;
		PFD    ptr ;
	      } u;
      } Symbol;


/*-----------------------------------------------------------------------*
 *     Functions reqired to set womersley profile on multiple inlets     *
 *-----------------------------------------------------------------------*/
  extern double doWomprofA(double x,double y,double z); /* womersley profile */
  extern double doWomprofB(double x,double y,double z); /* womersley profile */
  extern double doWomprofC(double x,double y,double z); /* womersley profile */
  extern double doWomprofD(double x,double y,double z); /* womersley profile */

  extern double doUserdefAu(double x, double y, double z);
  extern double doUserdefAv(double x, double y, double z);
  extern double doUserdefAw(double x, double y, double z);
  extern double doUserdefBu(double x, double y, double z);
  extern double doUserdefBv(double x, double y, double z);
  extern double doUserdefBw(double x, double y, double z);
  extern double doUserdefC(double x, double y, double z);
  extern double doUserdefD(double x, double y, double z);


/* --------------------------------------------------------------------- *
 *                 Function declarations and prototypes                  *
 * --------------------------------------------------------------------- */

/* Internal Prototypes */

static Symbol  *install(char*, int, ...),    /* Table management (LOCAL) */
               *lookup (char*);
static  double 
  Sqrt(double),                         /* Operators (mathematical) */
  Rand(double), Integer(double), 
  Mod(double), 
  Log(double), Log10(double), 
  Exp(double), 
  Radius(double,double),                /* ... binary operators ... */
  Jn(double,double),
  Yn(double,double),
  Angle(double,double),
  Step(double,double),                  /* Step function */
  Step2(double,double),                  /* Step function */
  Pow(double,double), 
  Shock(double,double,double),
  ReJn(double, double, double),
  ImJn(double, double, double),
  Jacobi(double,double,double,double),
  Bump(double), 
  Single(double, double),               /* NUWC new single tile: electrodes  */
  Womsin(double,double,double,double,double),
  Womcos(double,double,double,double,double);




static
double WomprofA(double,double,double),
       WomprofB(double,double,double),
       WomprofC(double,double,double),
       WomprofD(double,double,double);

static double 
      UserdefAu(double,double,double),
      UserdefAv(double,double,double),
      UserdefAw(double,double,double),
      UserdefBu(double,double,double),
      UserdefBv(double,double,double),
      UserdefBw(double,double,double),
      UserdefC(double,double,double),
      UserdefD(double,double,double);

static double 
      Wannier_stream(double, double, double),
      Wannier_cross(double, double, double);


extern void show_symbol (Symbol *s);         /* Print symbol's value     */

/* --------------------------------------------------------------------- *
 *                                                                       *
 *          P R O G R A M    D E F A U L T   P A R A M E T E R S         *
 *                                                                       *
 * The following are the default values for options and parameters used  *
 * in the Helmholtz and Navier-Stokes solvers.                           *
 *                                                                       *
 * --------------------------------------------------------------------- */

static struct {
  char   *name;
  int     oval;
} O_default[] = {                 /* Options */
	 "binary",      1,
	 "direct",      1,
	 "core",        1,
	 "ReCalcPrecon",1,
	 "NPVIZ",       2,
	  0, 0
};

static struct {                /* Parameters (integer) */
  char   *name;
  int     pval;
} I_default[] = {
         "P_HYBRID_STATUS",       1,
         "P_MAX_NMB_OF_STEPS",    0,
         "P_TIME_PERIODIC_DUMPS", 0,
         "P_TIME_DUMPS_NMB",      6,
         "P_TIME_INTERP_ORD",     5,
         "P_N_BUNCHS",            1,
         "P_REVERSE_DBG",         0,
         "P_COLOR_ID",            1,
         "P_RK_scheme_id",        7,
         "P_RK_IOsteps",          0,
         "HISSTEP",               0,
	 "CFLSTEP",               0,
         "DIM",                   2,
	 "NSTEPS",                1,
	 "NRCSTEP",               0, /* needed for steering of computation. resets RC B.C. values */
	 "IOSTEP",                0,
         "ELEMENTS",              0,
	 "NORDER",                5,
	 "MODES",                 0,
	 "EQTYPE",                0,
	 "INTYPE",                2,
         "VTKSTEPS",              0,
         "RMTHST",                0, /* 1 if remote host is spesified 0 if not */
	 "NPODORDER",		  0, /* number of fields for accelerator */
         "NPPODORDER",            0, /* number of fields for accelerator */	
         "NPORT",              5001, /* default port number */
	 "LQUAD",                 0, /* quadrature points in 'a' direction  */
         "MQUAD",                 0, /* quadrature points in 'b' direction  */
#if DIM == 3
	 "NQUAD",                 0, /* quadrature points in 'c' direction  */
#endif
	 "IDpatch",		  0, /* handle, index of domain currently being processed*/
	  0,                      0
};

static struct {                  /* Parameters (double) */
  char   *name;
  double  pval;
} D_default[] = {
         "P_COOR_TOL",      1.E-14,
         "P_TIME_STEP_TOL", 1.E-6,
         "P_BUNCH_TIME",    0.,
         "P_MAX_TIME",      0.,
         "P_RK_Kfixed",     0.,
         "P_DUMP_TIME",     0.,
         "DT",              0.001,    /* Time step (also below)    */
	 "DELT",            0.,
	 "STARTIME",        0.,
	 "XSCALE",          1.,
	 "YSCALE",          1.,
	 "TOL",             1.e-8,    /* Last-resort tolerance       */
	 "TOLCG",           1.e-8,    /* Conjugate Gradient Solver   */
	 "TOLCGP",          1.e-8,    /* Pressure Conjugate Gradient */
	 "TOLABS",          1.e-8,    /* Default PCG tolerance       */
	 "TOLREL",          1.e-6,    /* Default for ?               */
	 "IOTIME",          0.,
	 "FLOWRATE",        0.,
	 "PGRADX",          0.,
	 "FFZ",             0.,       /* Applied force for N-S       */
	 "FFY",             0.,
	 "FFX",             0.,
	 "LAMBDA",          1.e30,    /* Helmholtz Constant          */
	 "KINVIS",          1.,       /* 1/Re for N-S                */
	 "THETA",           0.0,      /* theta scheme variable       */
	 "BNDTIMEFCE",      1.0,      /* time dependent boundary fce */
	 "LZ",              1.0,      /* default Z direction length  */
	 "Re_Uinf",         1.0,      /* default velocity in Re      */
	 "Re_Len",          1.0,      /* default length in Re        */
	 "PSfactor",        1.0,      /* default scaling width of prism */
         "DMAXANGLESUR",    0.95,     /* maximum angle for Recon surfaces */ 
	  0,                0.
};


static struct {                  /* Constants */
  char    *name;
  double   cval;
} consts[] = {
        "PI",     3.14159265358979323846,   /* Pi */
	"E",      2.71828182845904523536,   /* Natural logarithm */
	"GAMMA",  0.57721566490153286060,   /* Euler */
	"DEG",   57.29577951308232087680,   /* deg/radian */
	"PHI",    1.61803398874989484820,   /* golden ratio */
	 0,       0
};

static struct {                /* Built-ins */
  char    *name;               /* Function name */
  short    args;               /* # of arguments */
  PFD      func;               /* Pointer to the function */
} builtins[] = {
         "sin",   1,  sin,
	 "cos",   1,  cos,
	 "cosh",  1,  cosh,
	 "sinh",  1,  sinh,
         "tanh",  1,  tanh,
	 "atan",  1,  atan,
	 "abs",   1,  fabs,
	 "int",   1,  Integer,     /* .... Argument Checking .... */
	 "log",   1,  Log,         
	 "log10", 1,  Log10,       
	 "exp",   1,  Exp,         
	 "sqrt",  1,  Sqrt,        
	 "rand",  1,  Rand,        /* random number (input the magnitude) */
	 "mod",   1,  Mod,   /* remainder */
	 "bump",  1,  Bump, 
	 "single", 2, Single,
	 "jn",    2,  Jn,      /* Bessel function J */
	 "yn",    2,  Yn,      /* Bessel function Y */
	 "rad",   2,  Radius,  /* rad = sqrt(x^2 + y^2) */
	 "ang",   2,  Angle,   /* ang = atan2(x,y)      */
	 "step",  2,  Step,    /* step(x,a) = 0  (if x < a) else 1 */
	 "step2", 2,  Step2,   /* step(x,a) = 0 (if x <= a) else 1 */
	 "rejn",  3,  ReJn,    /* Real part of complex Bessel function Jn(z) */
	 "imjn",  3,  ImJn,    /* Imag part of complex Bessel function Jn(z) */
         "shock", 3,  Shock,   /* shock(x,a,b) = a (if x < 0), (a+b)/2 (if x==0) or b (if x > 0) */
         "jacobi", 4,  Jacobi,
	 "womsin", 5,  Womsin,     /* Womersley solution due to sin component of u_avg  */
	 "womcos", 5,  Womcos,     /* Womersley solution due to cos component of u_avg  */
         "womprofA", 3, WomprofA,    /* womersley profile  */
         "womprofB", 3, WomprofB,    /* womersley profile  */
         "womprofC", 3, WomprofC,    /* womersley profile  */
         "womprofD", 3, WomprofD,    /* womersley profile  */
         "userdefAu", 3, UserdefAu,
         "userdefAv", 3, UserdefAv,
         "userdefAw", 3, UserdefAw,
         "userdefBu", 3, UserdefBu,
         "userdefBv", 3, UserdefBv,
         "userdefBw", 3, UserdefBw,
         "userdefC", 3, UserdefC,
         "userdefD", 3, UserdefD,
         "wannierS", 3, Wannier_stream,
         "wannierC", 3, Wannier_cross,
	 0,       0
};

/* External variables */

Tree*    Symbols  = 0;     /* Symbol table     */
Tree*    Options  = 0;     /* Option table     */
Tree*    Params   = 0;     /* Parameters table */
jmp_buf  begin;            

static char     func_string[SIZE], 
                *cur_string;
static double   stack_value;

extern int errno;

%}
%union {                /* stack type */
	double  val;    /* actual value */
	Symbol *sym;    /* symbol table pointer */
}
%token	<val>	NUMBER
%token	<sym>	VAR BLTIN_UNARY BLTIN_BINARY BLTIN_TRINARY BLTIN_QUATERNARY BLTIN_QUINTIC UNDEF DPARAM IPARAM OPTION
%type	<val>	expr asgn
%right	'='
%left	'+' '-'        	/* left associative, same precedence */
%left	'*' '/'        	/* left associative, higher precedence */
%left	UNARYMINUS
%right	'^'		/* exponentiation */
%%
list:  /* nothing */
	| list '\n'
	| list asgn  '\n'
	| list expr  '\n'     	{ stack_value = $2; }
	| list error '\n'	{ yyerrok; }	
	;
asgn:	  VAR '=' expr { $$=$1->u.val=$3; $1->type = VAR; }
	;
expr:	  NUMBER  { $$ = $1; }
	| VAR     { if ($1->type == UNDEF)
		      execerrnr("undefined variable",$1->name);
		    $$ = $1->u.val; }
        | IPARAM  { $$ = (double) $1->u.num; }
        | DPARAM  { $$ = $1->u.val; }
	| asgn
	| BLTIN_UNARY  '(' expr ')'	
            { $$ = (*($1->u.ptr))($3); }
	| BLTIN_BINARY '(' expr ',' expr ')'	
            { $$ = (*($1->u.ptr))($3,$5); }
        | BLTIN_TRINARY '(' expr ',' expr ',' expr ')'
            { $$ = (*($1->u.ptr))($3,$5,$7); }  
        | BLTIN_QUATERNARY '(' expr ',' expr ',' expr ',' expr ')'
            { $$ = (*($1->u.ptr))($3,$5,$7,$9); }  
        | BLTIN_QUINTIC '(' expr ',' expr ',' expr ',' expr ',' expr ')'
            { $$ = (*($1->u.ptr))($3,$5,$7,$9,$11); }  
	| expr '+' expr { $$ = $1 + $3; }
	| expr '-' expr { $$ = $1 - $3; }
	| expr '*' expr { $$ = $1 * $3; }
	| expr '/' expr {
	  if ($3 == 0.0){
#if ZERONULLDIV
	    $$ = 0.0;
#else
	    execerror("division by zero","");
	    $$ = $1 / $3; 
#endif
	  }
	  else{
	    $$ = $1 / $3; 
	  }
	}
	| expr '^' expr	{ $$ = Pow($1,$3); }
	| '(' expr ')'	{ $$ = $2; }
	| '-' expr %prec UNARYMINUS { $$ = -$2; }
	;
%%
	/* end of grammer */

/* --------------------------------------------------------------------- *
 *                                                                       *
 *                              P A R S E R                              *
 *                                                                       *
 * --------------------------------------------------------------------- */

yylex()
{
	int c;

	while((c = *cur_string++) == ' ' || c == '\t');

	if(c == EOF)
		return 0;
	if(c == '.' || isdigit(c)) {                      /* number */
	        char *p;
	        yylval.val = strtod(--cur_string, &p);
		cur_string = p;
		return NUMBER;
	}
	if(isalpha(c)) {                                  /* symbol */
		Symbol *s;
		char sbuf[100], *p = sbuf;

		do
		  *p++ = c;
		while
		  ((c = *cur_string++) != EOF && (isalnum(c) || c == '_')); 

		cur_string--;
		*p = '\0';
		if(!(s=lookup(sbuf))) 
		  s = install(sbuf, UNDEF, 0.);
		yylval.sym = s;
		return (s->type == UNDEF) ? VAR : s->type;
	}

	return c;
}

warning(char *s, char *t)    /* print warning message */
{
  fprintf(stderr,"parser: %s",s);
  if (t)
    fprintf(stderr," %s\n",t);
  else
    fprintf(stderr," in function string %s\n",func_string);
}

yyerror(char *s)      /* called for yacc syntax error */
{
  warning (s, (char *) 0);
}

execerror(char *s, char *t)    /* recover from run-time error */
{
  warning (s,t);
  longjmp (begin,0);
}

execerrnr(char *s, char *t)   /* run-time error, no recovery */
{
  warning(s,t);
  fprintf(stderr,"exiting to system...\n");
  exit(-1);
}

fpecatch()	 /* catch floating point exceptions */
{
  fputs ("speclib: floating point exception\n"
	 "exiting to system...\n", stderr);
  exit  (-1);
}

/* --------------------------------------------------------------------- *
 * Vector/Scalar parser                                                  *
 *                                                                       *
 * The scalar and vector parsers are the interfaces to the arithmetic    *
 * routines.  The scalar parser evaluates a single expression using      *
 * variables that have been defined as PARAM's or VAR's.                 *
 *                                                                       *
 * The vector parser is just a faster way to call the scalar parser.     *
 * Using the vector parser involves two steps: a call to vector_def() to *
 * declare the names of the vectors and the function, and a call to      *
 * vector_set() to evaluate it.                                          *
 *                                                                       *
 * Example:    vector_def ("x y z", "sin(x)*cos(y)*exp(z)");             *
 *             vector_set (100, x, y, z, u);                             *
 *                                                                       *
 * In this example, "x y z" is the space-separated list of vector names  *
 * referenced in the function string "sin(x)...".  The number 100 is the *
 * length of the vectors to be processed.  The function is evaluted as:  *
 *                                                                       *
 *             u[i] = sin(x[i])*cos(y[i])*exp(z[i])                      *
 *                                                                       *
 * --------------------------------------------------------------------- */

#define  VMAX     10    /* maximum number of vectors in a single call */
#define  VLEN   SIZE    /* maximum vector name string length          */

double scalar (char *function)
{
  if (strlen(function) > SIZE-1)
    execerrnr ("Too many characters in function:\n", function);
  
  sprintf (cur_string = func_string, "%s\n", function);
  yyparse ();
  
  return stack_value;
}

double scalar_set (char *name, double val)
{
  Node   *np;
  Symbol *sp;

  if (np = tree_search (Symbols->root, name)) {
    if ((sp = (Symbol*) np->other)->type == VAR)
      sp->u.val = val;
    else
      warning (name, "has a type other than VAR.  Not set.");
  } else
    install (name, VAR, val);

  return val;
}

static int     nvec;
static Symbol *vs[VMAX];
static double *vv[VMAX];

#ifndef VELINTERP
void vector_def (char *vlist, char *function)
{
  Symbol  *s;
  char    *name, buf[VLEN];

  if (strlen(vlist) > SIZE)
    execerrnr("name string is too long:\n", vlist);
  else
    strcpy(buf, vlist);

  /* install the vector names in the symbol table */

  name = strtok(buf, " ");
  nvec = 0;
  while (name && nvec < VMAX) {
    if (!(s=lookup(name))) 
      s = install (name, VAR, 0.);
    vs[nvec++] = s;
    name  = strtok((char*) NULL, " ");
  }

  if (strlen(function) > SIZE-1)
    execerrnr("too many characters in function:\n", function);

  sprintf (func_string, "%s\n", function);

  return;
}

void vector_set (int n, ...)
{
  va_list  ap;
  double   *fv;
  register int i;

  /* initialize the vectors */

  va_start(ap, n);
  for (i = 0; i < nvec; i++) vv[i] = va_arg(ap, double*);
  fv = va_arg(ap, double*);
  va_end(ap);

  /* evaluate the function */

  while (n--) {
    for (i = 0; i < nvec; i++) vs[i]->u.val = *(vv[i]++);    
    cur_string = func_string; 
    yyparse();
    *(fv++)    = stack_value;
  }

  return;
}
#endif
#undef VMAX
#undef VLEN

/* --------------------------------------------------------------------- *
 * Parameters and Options                                                *
 *                                                                       *
 * The following functions simply set and lookup values from the tables  *
 * of variables.   If a symbol isn't found, they silently return zero.   *
 * --------------------------------------------------------------------- */

int iparam (char *name)
{
  Node   *np;
  Symbol *sp;
  int    num = 0;

  if ((np = tree_search (Params->root, name)) &&
      (sp = (Symbol*) np->other)->type == IPARAM)
    num = sp->u.num;

  return num;
}

int iparam_set (char *name, int num)
{
  Node   *np;
  Symbol *sp;

  if (np = tree_search (Params->root, name)) {
    if ((sp = (Symbol*) np->other)->type == IPARAM)
      sp->u.num = num;
    else
      warning (name, "has a type other than IPARAM.  Not set.");
  } else
    install (name, IPARAM, num);

  return num;
}

double dparam (char *name)
{
  Node   *np;
  Symbol *sp;
  double val = 0.;

  if ((np = tree_search (Params->root, name)) &&
      (sp = (Symbol*) np->other)->type == DPARAM)
    val = sp->u.val;

  return val;
}

double dparam_set (char *name, double val)
{
  Node   *np;
  Symbol *sp;

  if (np = tree_search (Params->root, name)) {
    if ((sp = (Symbol*) np->other)->type == DPARAM)
      sp->u.val = val;
    else
      warning (name, "has a type other than DPARAM.  Not set.");
  } else
    install (name, DPARAM, val);

  return val;
}

int option (char *name)
{
  Node *np;
  int   status = 0;
  
  if (np = tree_search (Options->root, name))
    status = ((Symbol*) np->other)->u.num;

  return status;
}

int option_set (char *name, int status)
{
  Node *np;

  if (np = tree_search (Options->root, name))
    ((Symbol*) np->other)->u.num = status;
  else
    install (name, OPTION, status);
  
  return status;
}

/* --------------------------------------------------------------------- *
 * manager_init() -- Initialize the parser                               *
 *                                                                       *
 * The following function must be called before any other parser func-   *
 * tions to install the symbol tables and builtin functions.             *
 * --------------------------------------------------------------------- */

void manager_init (void)
{
  register int i;

  /* initialize the trees */

  Symbols = create_tree (show_symbol, free);
  Options = create_tree (show_symbol, free);
  Params  = create_tree (show_symbol, free);

  /* initialize the signal manager */

  setjmp(begin);
  signal(SIGFPE, (void(*)()) fpecatch);

  /* options and parameters */

  for(i = 0; O_default[i].name; i++)
     install(O_default[i].name,OPTION,O_default[i].oval);
  for(i = 0; I_default[i].name; i++) 
     install(I_default[i].name,IPARAM,I_default[i].pval);
  for(i = 0; D_default[i].name; i++)
     install(D_default[i].name,DPARAM,D_default[i].pval);

  /* constants and built-ins */

  for(i = 0; consts[i].name; i++)
    install (consts[i].name,VAR,consts[i].cval);
  for(i = 0; builtins[i].name; i++) {
    switch  (builtins[i].args) {
    case 1:
      install (builtins[i].name, BLTIN_UNARY, builtins[i].func);
      break;
    case 2:
      install (builtins[i].name, BLTIN_BINARY, builtins[i].func);
      break;
    case 3:
      install (builtins[i].name, BLTIN_TRINARY, builtins[i].func);
      break;
    case 4:
      install (builtins[i].name, BLTIN_QUATERNARY, builtins[i].func);
      break;
    case 5:
      install (builtins[i].name, BLTIN_QUINTIC, builtins[i].func);
      break;
    default:
      execerrnr ("too many arguments for builtin:", builtins[i].name);
      break;
    }
  }
  
  return;
}

/* Print parameter, option, and symbol tables */

void show_symbols(void) { puts ("\nSymbol table:"); tree_walk (Symbols); }
void show_options(void) { puts ("\nOptions:")     ; tree_walk (Options); }
void show_params (void) { puts ("\nParameters:")  ; tree_walk (Params);  }

/* Print a Symbol */

void show_symbol (Symbol *s)
{
  printf ("%-15s -- ", s->name);
  switch (s->type) {
  case OPTION:
  case IPARAM:
    printf ("%d\n", s->u.num);
    break;
  case DPARAM:
  case VAR:
    printf ("%g\n", s->u.val);
    break;
  default:
    puts   ("unprintable");
    break;
  }
  return;
}

/* ..........  Symbol Table Functions  .......... */

static Symbol *lookup (char *key)
{
  Node *np;

  if (np = tree_search (Symbols->root, key))
    return (Symbol*) np->other;

  if (np = tree_search (Params ->root, key))
    return (Symbol*) np->other;

  if (np = tree_search (Options->root, key))
    return (Symbol*) np->other;

  return (Symbol*) NULL;     /* not found */
}                  

/* 
 * install "key" in a symbol table 
 */

static Symbol *install (char *key, int type, ...)     
{
  Node   *np;
  Symbol *sp;
  Tree   *tp;
  va_list ap;

  va_start (ap, type);
  
  /* Get a node for this key and create a new symbol */

  np       = create_node (key);
  sp       = (Symbol *) malloc(sizeof(Symbol));
  sp->name = np->name;

  switch (sp->type = type) {
  case OPTION:
    tp        = Options;
    sp->u.num = va_arg(ap, int);
    break;
  case IPARAM:
    tp        = Params;
    sp->u.num = va_arg(ap, int);
    break;
  case DPARAM:
    tp        = Params;
    sp->u.val = va_arg(ap, double);
    break;
  case VAR: 
  case UNDEF:  
    tp        = Symbols;
    sp->u.val = va_arg(ap, double);
    break;
  case BLTIN_UNARY:
  case BLTIN_BINARY:
  case BLTIN_TRINARY:
  case BLTIN_QUATERNARY:
  case BLTIN_QUINTIC:
    tp        = Symbols;
    sp->u.ptr = va_arg(ap, PFD);
    break;
  default:
    tp        = Symbols;
    sp->u.val = va_arg(ap, double);
    sp->type  = UNDEF;
    break;
  }

  va_end (ap);

  np->other = (void *) sp;     /* Save the symbol */
  tree_insert (tp, np);        /* Insert the node */

  return sp;
}

/*
 *  Math Functions
 *  --------------  */

static double errcheck (double d, char *s)
{
  if (errno == EDOM) {
    errno = 0                              ;
    execerror(s, "argument out of domain") ;
  }
  else if (errno == ERANGE) {
    errno = 0                           ;
    execerror(s, "result out of range") ;
  }
  return d;
}

static double Log (double x)
{
  return errcheck(log(x), "log") ;
}

static double Log10 (double x) 
{
  return errcheck(log10(x), "log10") ;
}

static double Exp (double x)
{
  if(x<-28.)
    return 0.;
  
  return errcheck(exp(x), "exp") ;
}

static double Sqrt (double x)
{
  return errcheck(sqrt(x), "sqrt") ;
}

static double Pow (double x, double y)
{
  const
  double yn = floor(y + .5);
  double px = 1.;

  if (yn >= 0 && yn == y) {     /* Do it inline if y is an integer power */
      register int n = yn;
      while (n--) 
         px *= x;
  } else  
      px = errcheck (pow(x,y), "exponentiation");

  return px;
}

static double Integer (double x)
{
  return (double) (long) x;
} 

static double Mod (double x)
{
  double tmp;
  return (double) modf(x,&tmp);
} 

static double Rand (double x)
{
  return x * drand();
}


static double Bump (double x)
{
  if(x >= 0. && x < .125)
    return -1;
  if(x >= 0.125 && x < .25)
    return 0.;
  if(x >= 0.25 && x < .375)
    return 1.;
  if(x >= 0.375 && x <= .5)
    return 0.;

  return -9999.;
}



static double Radius (double x, double y)
{
  if (x != 0. || y != 0.)
    return sqrt (x*x + y*y);
  else
    return 0.;
}

static double Jn (double i, double x)
{
    return jn((int)i, x);
}

static double Yn (double i, double x)
{
    return yn((int)i, x);
}


static double ReJn (double n, double x,  double y)
{
  double rej, imj;
  int nz,ierr;
  
  zbesj(&x,&y,n,1,1,&rej,&imj,&nz,&ierr);
  return rej;
}

static double ImJn (double n, double x, double y)
{
  double rej, imj;
  int nz,ierr;
  
  zbesj(&x,&y,n,1,1,&rej,&imj,&nz,&ierr);
  return imj;
}

/* Calcualte the Womersley solution at r for a pipe of radius R and
   wave number wnum.  The solution is assumed to be set so that the
   spatail mean fo the flow satisfies u_avg(r) = A cos (wnum t) + B
   sin(wnum t) 
*/

static double Womersley(double A,double B,double r,double R,double mu, 
			double wnum,double t){
  
  double x,y;

  if(r > R) fprintf(stderr,"Error in manager.y: Womersley - r > R\n");

  if(wnum == 0) /* return poseuille flow  with mean of 1.*/
    return 2*(1-r*r/R/R);
  else{
    int    ierr,nz;
    double cr,ci,J0r,J0i,rej,imj,re,im,fac;
    double isqrt2 = 1.0/sqrt(2.0);
    static double R_str, wnum_str,mu_str;
    static double Jr,Ji,alpha,j0r,j0i, isqrt;
    

    /* for case of repeated calls to with same parameters look to store 
       parameters independent of r. */
    if((R != R_str)||(wnum != wnum_str)||(mu != mu_str)){
      double retmp[2],imtmp[2];
      alpha = R*sqrt(wnum/mu);

      re  = -alpha*isqrt2;
      im  =  alpha*isqrt2;
      zbesj(&re,&im,0,1,2,retmp,imtmp,&nz,&ierr);
      j0r = retmp[0]; j0i = imtmp[0];
      rej = retmp[1]; imj = imtmp[1];

      fac = 1/(j0r*j0r+j0i*j0i);
      Jr  = 1 + 2*fac/alpha*((rej*j0r+imj*j0i)*isqrt2 - (imj*j0r - rej*j0i)*isqrt2);
      Ji  = 2*fac/alpha*((rej*j0r+imj*j0i)*isqrt2 + (imj*j0r - rej*j0i)*isqrt2);

      R_str = R; wnum_str = wnum; mu_str = mu;
    }

    /* setup cr, ci from pre-stored value of Jr & Ji */
    fac = 1/(Jr*Jr + Ji*Ji);
    cr  =  (A*Jr - B*Ji)*fac;
    ci  = -(A*Ji + B*Jr)*fac;
    
    /* setup J0r, J0i */
    re  = -alpha*isqrt2*r/R;
    im  =  alpha*isqrt2*r/R;
    zbesj(&re,&im,0,1,1,&rej,&imj,&nz,&ierr);
    fac = 1/(j0r*j0r+j0i*j0i);
    J0r = 1-fac*(rej*j0r+imj*j0i);
    J0i = -fac*(imj*j0r-rej*j0i); 

    /* return solution */
    return (cr*J0r - ci*J0i)*cos(wnum*t) - (ci*J0r + cr*J0i)*sin(wnum*t);
  }
}

static double Womsin(double r, double R, double mu, double wnum, double t){
  if(wnum == 0) /* no sin term for zeroth mode */
    return 0;
  else
    return Womersley(0,1,r,R,mu,wnum,t);
}

static double Womcos(double r, double R, double mu, double wnum, double t){
  return Womersley(1,0,r,R,mu, wnum,t);
}

#ifndef M_PI
#define M_PI  consts[0].cval
#endif

static double Angle (double x, double y)
{
  double theta = 0.;

  if ((x != 0.)||(y != 0.))
    theta =  atan2 (y,x);

  return theta;
}


/* Heaviside step function H(x-a) =1 if x >= a else =0 */
static double Step (double x, double a)
{
  double H = 1.0;
  if (x < a)
    H = 0.0;

  return H;
}

/* Heaviside step function H(x-a) =1 if x > a else =0 */ 
static double Step2 (double x, double a)
{
  double H = 1.0;
  if (x <= a)
    H = 0.0;

  return H;
} 

static double Shock(double x, double a, double b)
{
  if(x==0)
    return 0.5*(a+b);
  if(x>0)
    return b;
  if(x<0)
    return a;
  return 0;
}


/* -----------------------------------------------------------------
   jacobi() - jacobi polynomials 
   
   Get a vector 'poly' of values of the n_th order Jacobi polynomial
   P^(alpha,beta)_n(z) alpha > -1, beta > -1 at the z
   ----------------------------------------------------------------- */

static double Jacobi(double z, double n, double alpha, double beta){

  register int i,k;
  double  one = 1.0;
  double   a1,a2,a3,a4;
  double   two = 2.0, apb = alpha + beta;
  double   poly, polyn1,polyn2;
  
  polyn2 = one;
  polyn1 = 0.5*(alpha - beta + (alpha + beta + 2)*z);
  
  for(k = 2; k <= n; ++k){
    a1 =  two*k*(k + apb)*(two*k + apb - two);
    a2 = (two*k + apb - one)*(alpha*alpha - beta*beta);
    a3 = (two*k + apb - two)*(two*k + apb - one)*(two*k + apb);
    a4 =  two*(k + alpha - one)*(k + beta - one)*(two*k + apb);
    
    a2 /= a1;
    a3 /= a1;
    a4 /= a1;
    
    poly   = (a2 + a3*z)*polyn1 - a4*polyn2;
    polyn2 = polyn1;
    polyn1 = poly  ;
  }

  return poly;
}


#if 1
static double Single(double x, double y)
{
#if 1
  double gamma = 64.0*64.0;
  double tmp;

  if (y>=3.0 && y<=4.0)
    {
      tmp = (y-3.)*(y-4.)*(y-3.)*(y-4.);
      if (x>=1.0 && x<=2.)
	return gamma*(x-1.0)*(x-2.)*(x-1.0)*(x-2.);
      if(x>=3.0 && x<=4.0)
	return gamma*(x-3.0)*(x-4.0)*(x-3.0)*(x-4.0);
      if(x>=5. && x<=6.) 
	return gamma*(x-5.0)*(x-6.)*(x-5.0)*(x-6.);
    }
#else
  double gamma = 64.0*64.0;
  double xa,xb,ya,yb;

  if(x>1. && x<2. && y>1. && y<2.){
    xa = 1.;    xb = 2.;
    ya = 1.;    yb = 2.;
  }
  else if(x>3. && x<4. && y>3. && y<4.){
    xa = 3.;    xb = 4.;
    ya = 3.;    yb = 4.;
  }
  else if(x>5. && x<6. && y>5. && y<6.){
    xa = 5.;    xb = 6.;
    ya = 5.;    yb = 6.;
  }
  else 
    return 0.;

  return gamma*(x-xa)*(x-xa)*(x-xb)*(x-xb)*(y-ya)*(y-ya)*(y-yb)*(y-yb);
#endif
}
#endif

/*-----------------------------------------------------*/
/* Artery tree, velocity BC at inlets - womersley      */
/*-----------------------------------------------------*/
// here all functions return 0.0
// these functions are not important for VIZ. purposes but only for computation
// we keep them in order to be able to preocess the input *rea file



static double WomprofA(double x, double y, double z)
{
    return 0;//doWomprofA(x,y,z);
}
static double WomprofB(double x, double y, double z)
{
    return 0;//doWomprofB(x,y,z);
}

static double WomprofC(double x, double y, double z)
{
    return 0;//doWomprofC(x,y,z);
}

static double WomprofD(double x, double y, double z)
{
    return 0;//doWomprofD(x,y,z);
}

static double UserdefAu(double x, double y, double z){
    return 0;//doUserdefAu(x,y,z);
}

static double UserdefAv(double x, double y, double z){
    return 0;//doUserdefAv(x,y,z);
}

static double UserdefAw(double x, double y, double z){
    return 0;//doUserdefAw(x,y,z);
}


static double UserdefBu(double x, double y, double z){
    return 0;//doUserdefBu(x,y,z);
}

static double UserdefBv(double x, double y, double z){
    return 0;//doUserdefBv(x,y,z);
}

static double UserdefBw(double x, double y, double z){
  return doUserdefBw(x,y,z);
}


static double UserdefC(double x, double y, double z){
  return doUserdefC(x,y,z);
}

static double UserdefD(double x, double y, double z){
  return doUserdefD(x,y,z);
}

static double Wannier_stream(double x, double y, double z){
    return 0;//doWannier_stream(x,y,z);
}

static double Wannier_cross(double x, double y, double z){
    return 0;//doWannier_cross(x,y,z);
}


  
#undef M_PI




