/*
  @copyright Russell Standish 2000
  @author Russell Standish
  This file is a modified version of a similarly named file in EcoLab. 

  It is released as public domain.
*/
#include "ecolab.h"
#include "object.h"
#include "eco_hashmap.h"
#include "pack_stream.h"

#include "init.h"

#include <ecolab_epilogue.h>
#include <fstream>
//#include <signal.h>
extern "C" 
{
  typedef void (*__sighandler_t) (int);
  extern __sighandler_t signal (int __sig, __sighandler_t __handler);
}
#define SIG_DFL	((__sighandler_t) 0)		/* Default action.  */
#define	SIGILL		4	/* Illegal instruction (ANSI).  */
#define	SIGABRT		6	/* Abort (ANSI).  */
#define	SIGBUS		7	/* BUS error (4.2 BSD).  */
#define	SIGSEGV		11	/* Segmentation violation (ANSI).  */


using namespace std;
using namespace ecolab;
using namespace classdesc;
using namespace minsky;

#include "version.h"
NEWCMD(minsky_version,0)
{
  tclreturn r;
  r<<VERSION;
}

namespace ecolab
{
  Tk_Window mainWin=0;
}


int main(int argc, char* argv[])
{
  Tcl_Init(interp());

  /* set the TCL variables argc and argv to contain the
     arguments. */
  tclvar tcl_argc("argc"), tcl_argv("argv");
  tcl_argc=argc;
  for (int i=0; i<argc; i++) tcl_argv[i]=argv[i];

  if (Tk_Init(interp())==TCL_ERROR)
    {
      fprintf(stderr,"Error initialising Tk: %s",interp()->result);
      fprintf(stderr,"%s\n",Tcl_GetVar(interp(),"errorInfo",0)); 
      /* If Tk_Init fails, it is not necessarily a fatal error. For
         example, on unpatched macs, we get an error from the attempt
         to create a console, yet Minsky works just fine without
         one. */
      //      return 1;
    }

  // call initialisers
  vector<Fun>& init=initVec();
  for (vector<Fun>::iterator i=init.begin(); i!=init.end(); ++i)
    (*i)();


  Tcl_FindExecutable(argv[0]);
  tclcmd minskyHome;
  minskyHome << "file dirname [info nameofexecutable]\n";

  if (Tcl_EvalFile(interp(), (minskyHome.result+"/minsky.tcl").c_str())!=TCL_OK)
  {
      fprintf(stderr,"%s\n",interp()->result);
      fprintf(stderr,"%s\n",Tcl_GetVar(interp(),"errorInfo",0)); /* print out trace */
      return 1;  /* not a clean execution */
    }

  while (mainWin) /* we are running GUI mode */
    Tcl_DoOneEvent(0); /*Tk_MainLoop();*/
};

NEWCMD(GUI,0)
{
  mainWin = Tk_MainWindow(interp());
}

//this seems to be needed to get Tkinit to function correctly!!
NEWCMD(tcl_findLibrary,-1) {}

NEWCMD(exit_ecolab,0)
{
mainWin=0;
}


namespace TCLcmd 
{

  namespace trap
  {
    eco_string sigcmd[32];
    void sighand(int s) {Tcl_Eval(interp(),sigcmd[s].c_str());}
    hash_map<eco_string,int> signum;   /* signal name to number table */
    struct init_t
    {
      init_t()
      {
        signum["HUP"]=	1;	/* Hangup (POSIX).  */
        signum["INT"]=	2;	/* Interrupt (ANSI).  */
        signum["QUIT"]=	3;	/* Quit (POSIX).  */
        signum["ILL"]=	4;	/* Illegal instruction (ANSI).  */
        signum["TRAP"]=	5;	/* Trace trap (POSIX).  */
        signum["ABRT"]=	6;	/* Abort (ANSI).  */
        signum["IOT"]=	6;	/* IOT trap (4.2 BSD).  */
        signum["BUS"]=	7;	/* BUS error (4.2 BSD).  */
        signum["FPE"]=	8;	/* Floating-point exception (ANSI).  */
        signum["KILL"]=	9;	/* Kill, unblockable (POSIX).  */
        signum["USR1"]=	10;	/* User-defined signal 1 (POSIX).  */
        signum["SEGV"]=	11;	/* Segmentation violation (ANSI).  */
        signum["USR2"]=	12;	/* User-defined signal 2 (POSIX).  */
        signum["PIPE"]=	13;	/* Broken pipe (POSIX).  */
        signum["ALRM"]=	14;	/* Alarm clock (POSIX).  */
        signum["TERM"]=	15;	/* Termination (ANSI).  */
        signum["STKFLT"]=	16;	/* Stack fault.  */
        signum["CLD"]=	17;     /* Same as SIGCHLD (System V).  */
        signum["CHLD"]=   17;	/* Child status has changed (POSIX).  */
        signum["CONT"]=	18;	/* Continue (POSIX).  */
        signum["STOP"]=	19;	/* Stop, unblockable (POSIX).  */
        signum["TSTP"]=	20;	/* Keyboard stop (POSIX).  */
        signum["TTIN"]=	21;	/* Background read from tty (POSIX).  */
        signum["TTOU"]=	22;	/* Background write to tty (POSIX).  */
        signum["URG"]=	23;	/* Urgent condition on socket (4.2 BSD).  */
        signum["XCPU"]=  	24;	/* CPU limit exceeded (4.2 BSD).  */
        signum["XFSZ"]=	25;	/* File size limit exceeded (4.2 BSD).  */
        signum["VTALRM"]= 26;	/* Virtual alarm clock (4.2 BSD).  */
        signum["PROF"]=	27;	/* Profiling alarm clock (4.2 BSD).  */
        signum["WINCH"]=  28;	/* Window size change (4.3 BSD, Sun).  */
        signum["POLL"]=	29;/* Pollable event occurred (System V).  */
        signum["IO"]=	29;	/* I/O now possible (4.2 BSD).  */
        signum["PWR"]=	30;	/* Power failure restart (System V).  */
        signum["SYS"]=	31;	/* Bad system call.  */
        signum["UNUSED"]=	31;
      }
    } init;

    NEWCMD(trap,2)     /* trap argv[2] to excute argv[1] */
    {
      int signo = (isdigit(argv[1][0]))? atoi(argv[1]): 
	signum[const_cast<char*>(argv[1])];
      sigcmd[signo]=argv[2];
      signal(signo,sighand);
    }

    void aborthand(int s) {throw error("Fatal Error: Execution recovered");}
  
    NEWCMD(trapabort,-1)
    {
      void (*hand)(int);
      if (argc>1 && strcmp(argv[1],"off")==0) hand=SIG_DFL;
      else hand=aborthand;
      signal(SIGABRT,hand);
      signal(SIGSEGV,hand);
      signal(SIGBUS,hand);
      signal(SIGILL,hand);
    }
  }



#ifdef READLINE
  extern "C" char *readline(char *);
  extern "C" void add_history(char *);
#endif

  NEWCMD(cli,0)
  {
    int braces=0, brackets=0;
    bool inString=false;
    tclcmd cmd;
    cmd << "savebgerror\n";
#ifdef READLINE
    char *c;
    eco_string prompt((char*)tclvar("argv(0)")); prompt+='>';
    while ( (c=readline(const_cast<char*>(prompt.c_str())))!=NULL && strcmp(c,"exit")!=0)
#else
      char c[512];
    while (fgets(c,512,stdin)!=NULL && strcmp(c,"exit\n")!=0) 
#endif
      {
        // count up number of braces, brackets etc
        for (char* cc=c; *cc!='\0'; ++cc)
          switch(*cc)
            {
            case '{':
              if (!inString) braces++;
              break;
            case '[':
              if (!inString) brackets++;
              break;
            case '}':
              if (!inString) braces--;
              break;
            case ']':
              if (!inString) brackets--;
              break;
            case '\\':
              if (inString) cc++; //skip next character (is literal)
              break;
            case '"':
              inString = !inString;
              break;
            }
        cmd << chomp(c);
        if (!inString && braces<=0 && brackets <=0)
          { // we have a complete command, so attempt to execute it
            try
              {
                cmd.exec();
              }
            catch (std::exception& e) {fputs(e.what(),stderr);}
            catch (...) {fputs("caught unknown exception",stderr);}
            puts(cmd.result.c_str()); 
          }
#ifdef READLINE
        if (strlen(c)) add_history(c); 
        free(c);
#endif
      }
    cmd << "restorebgerror\n";
  }


}
